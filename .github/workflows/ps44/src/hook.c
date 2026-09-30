#include "days_gone_900p.h"

/* ===== Inline hook (detour) implementation ===== */

#define HOOK_JMP_SIZE 14  /* FF 25 00 00 00 00 + 8-byte address = 14 bytes */

typedef struct {
    void* target;
    void* hook;
    uint8_t original[HOOK_JMP_SIZE];
    void* trampoline;
    bool installed;
} hook_t;

static hook_t g_hooks[16];
static int g_hook_count = 0;

/* Allocate executable memory for trampoline */
static void* allocate_trampoline(size_t size) {
    int uid = sceKernelAllocMemBlock("trampoline", SCE_KERNEL_MEMBLOCK_TYPE_USER_RWX, 
                                      (size + 0xFFF) & ~0xFFF, NULL);
    if (uid < 0) return NULL;
    
    void* addr = NULL;
    int ret = sceKernelMapMemBlock(uid, &addr, SCE_KERNEL_PROT_READ | SCE_KERNEL_PROT_WRITE | SCE_KERNEL_PROT_EXEC);
    if (ret < 0) {
        sceKernelFreeMemBlock(uid);
        return NULL;
    }
    return addr;
}

/* Build trampoline: original bytes + JMP to target+HOOK_JMP_SIZE */
static void* build_trampoline(void* target, const uint8_t* original, size_t orig_len) {
    void* tramp = allocate_trampoline(orig_len + HOOK_JMP_SIZE);
    if (!tramp) return NULL;
    
    /* Copy original bytes */
    __builtin_memcpy(tramp, original, orig_len);
    
    /* Add JMP to target + orig_len */
    uint8_t* jmp = (uint8_t*)tramp + orig_len;
    jmp[0] = 0xFF;
    jmp[1] = 0x25;
    jmp[2] = 0x00;
    jmp[3] = 0x00;
    jmp[4] = 0x00;
    jmp[5] = 0x00;
    *(uint64_t*)(jmp + 6) = (uint64_t)target + orig_len;
    
    /* Flush cache */
    sceKernelDcacheWritebackInvalidateRange(tramp, orig_len + HOOK_JMP_SIZE);
    
    return tramp;
}

/* Install inline hook */
int hook_install(void* target, void* hook, void** original_out) {
    if (g_hook_count >= 16) return -1;
    
    /* Check if already hooked */
    for (int i = 0; i < g_hook_count; i++) {
        if (g_hooks[i].target == target) return -1;
    }
    
    hook_t* h = &g_hooks[g_hook_count++];
    h->target = target;
    h->hook = hook;
    
    /* Save original bytes */
    __builtin_memcpy(h->original, target, HOOK_JMP_SIZE);
    h->original_len = HOOK_JMP_SIZE;
    
    /* Build trampoline for calling original */
    h->trampoline = build_trampoline(target, h->original, HOOK_JMP_SIZE);
    if (!h->trampoline) {
        _klog("Failed to allocate trampoline\n");
        return -1;
    }
    
    /* Write JMP to hook */
    uint8_t* t = (uint8_t*)target;
    t[0] = 0xFF;
    t[1] = 0x25;
    t[2] = 0x00;
    t[3] = 0x00;
    t[4] = 0x00;
    t[5] = 0x00;
    *(uint64_t*)(t + 6) = (uint64_t)hook;
    
    /* Flush instruction cache */
    sceKernelDcacheWritebackInvalidateRange(target, HOOK_JMP_SIZE);
    
    h->installed = true;
    
    if (original_out) *original_out = h->trampoline;
    
    _klog("Hook installed: target=%p hook=%p trampoline=%p\n", target, hook, h->trampoline);
    return 0;
}

/* Remove hook */
void hook_remove(void* target) {
    for (int i = 0; i < g_hook_count; i++) {
        if (g_hooks[i].target == target && g_hooks[i].installed) {
            /* Restore original bytes */
            __builtin_memcpy(target, g_hooks[i].original, HOOK_JMP_SIZE);
            sceKernelDcacheWritebackInvalidateRange(target, HOOK_JMP_SIZE);
            
            /* Free trampoline (would need to track UID) */
            g_hooks[i].installed = false;
            
            _klog("Hook removed: target=%p\n", target);
            break;
        }
    }
}