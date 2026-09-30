#include "days_gone_900p.h"

/* ===== External SCE Kernel Functions (resolved at runtime) ===== */
extern int sceKernelGetModuleInfo(int modid, SceKernelModuleInfo* info);
extern int sceKernelGetProcessVmMap(pid_t pid, SceKernelProcessVmMapEntry* entries, int max_entries, int* count);
extern int sceKernelAllocMemBlock(const char* name, int type, size_t size, void* opt);
extern int sceKernelFreeMemBlock(int uid);
extern int sceKernelMapMemBlock(int uid, void** addr, int prot);
extern int sceKernelMemcpyUserToUser(void* dst, const void* src, size_t len);
extern void sceKernelDcacheWritebackInvalidateRange(void* addr, size_t len);
extern int sceKernelLdlsym(int modid, const char* name, void** addr);
extern int sceKernelGetProcessId(void);

/* ===== Kernel printf for logging ===== */
extern int _printf(const char* fmt, ...);

void _klog(const char* fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    int len = __builtin_vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    _printf("%s", buf);
}

/* ===== Global state ===== */
uint64_t g_game_base = 0;
uint64_t g_game_size = 0;

SetResolutionScaleNormalized_t g_SetResolutionScaleNormalized = NULL;
SetResolutionScaleValue_t g_SetResolutionScaleValue = NULL;
SetScreenResolution_t g_SetScreenResolution = NULL;
SetShadowQuality_t g_SetShadowQuality = NULL;
SetViewDistanceQuality_t g_SetViewDistanceQuality = NULL;
SetPostProcessingQuality_t g_SetPostProcessingQuality = NULL;
ApplyResolutionSettings_t g_ApplyResolutionSettings = NULL;
GetGameUserSettings_t g_GetGameUserSettings = NULL;

static bool g_initialized = false;
static bool g_settings_applied = false;

/* Forward declarations */
int resolve_ue4_functions(void);
int find_game_memory_region(void);

/* ===== Module entry point ===== */
int _start(void) {
    _klog("Module loaded, initializing...\n");
    
    /* Step 1: Find game's memory region */
    int ret = find_game_memory_region();
    if (ret < 0) {
        _klog("Failed to find game memory region: %d\n", ret);
        return ret;
    }
    _klog("Game memory: base=0x%lx size=0x%lx\n", g_game_base, g_game_size);
    
    /* Step 2: Resolve UE4 functions via pattern scanning */
    ret = resolve_ue4_functions();
    if (ret < 0) {
        _klog("Failed to resolve UE4 functions: %d\n", ret);
        return ret;
    }
    _klog("UE4 functions resolved\n");
    
    g_initialized = true;
    
    /* Step 3: Apply settings (will re-apply on each call if needed) */
    days_gone_900p_apply_settings();
    
    return SCE_OK;
}

void _fini(void) {
    _klog("Module unloaded\n");
}

/* ===== Find Days Gone memory region via process VM map ===== */
int find_game_memory_region(void) {
    pid_t pid = sceKernelGetProcessId();
    if (pid < 0) return pid;
    
    SceKernelProcessVmMapEntry entries[256];
    int count = 0;
    int ret = sceKernelGetProcessVmMap(pid, entries, 256, &count);
    if (ret < 0) return ret;
    
    _klog("Process VM map entries: %d\n", count);
    
    /* Look for the main executable region (largest RX region) */
    uint64_t best_base = 0;
    uint64_t best_size = 0;
    
    for (int i = 0; i < count; i++) {
        uint64_t start = entries[i].start;
        uint64_t end = entries[i].end;
        uint32_t prot = entries[i].prot;
        uint64_t size = end - start;
        
        /* Executable + readable (code section) */
        if ((prot & (SCE_KERNEL_PROT_READ | SCE_KERNEL_PROT_EXEC)) == 
            (SCE_KERNEL_PROT_READ | SCE_KERNEL_PROT_EXEC)) {
            _klog("  RX region: 0x%lx-0x%lx (%lx bytes) %s\n", start, end, size, entries[i].name);
            
            /* Prefer larger regions that look like main executable */
            if (size > best_size && size > 0x100000) {  /* > 1MB */
                best_base = start;
                best_size = size;
            }
        }
    }
    
    if (best_base == 0) {
        _klog("No suitable game memory region found\n");
        return SCE_KERNEL_ERROR_ESRCH;
    }
    
    g_game_base = best_base;
    g_game_size = best_size;
    return SCE_OK;
}

/* ===== Apply graphics settings ===== */
void days_gone_900p_apply_settings(void) {
    if (!g_initialized) return;
    
    /* Get GameUserSettings instance */
    void* gus = NULL;
    if (g_GetGameUserSettings) {
        gus = g_GetGameUserSettings();
        _klog("GameUserSettings: %p\n", gus);
    } else {
        _klog("GetGameUserSettings not resolved!\n");
        return;
    }
    
    if (!gus) {
        _klog("GameUserSettings is NULL!\n");
        return;
    }
    
    /* Target: 900p from 1080p = 75% screen percentage */
    const float screen_pct = 0.75f;
    const int32_t quality_medium = 1;  /* 0=Low, 1=Medium, 2=High, 3=Epic */
    
    _klog("Applying 900p + quality reductions...\n");
    
    if (g_SetResolutionScaleNormalized) {
        g_SetResolutionScaleNormalized(gus, screen_pct);
        _klog("  SetResolutionScaleNormalized(%.2f)\n", screen_pct);
    } else if (g_SetResolutionScaleValue) {
        g_SetResolutionScaleValue(gus, 75.0f);
        _klog("  SetResolutionScaleValue(75.0)\n");
    } else if (g_SetScreenResolution) {
        g_SetScreenResolution(gus, 1600, 900);
        _klog("  SetScreenResolution(1600, 900)\n");
    }
    
    if (g_SetShadowQuality) {
        g_SetShadowQuality(gus, quality_medium);
        _klog("  SetShadowQuality(%d)\n", quality_medium);
    }
    
    if (g_SetViewDistanceQuality) {
        g_SetViewDistanceQuality(gus, quality_medium);
        _klog("  SetViewDistanceQuality(%d)\n", quality_medium);
    }
    
    if (g_SetPostProcessingQuality) {
        g_SetPostProcessingQuality(gus, quality_medium);
        _klog("  SetPostProcessingQuality(%d)\n", quality_medium);
    }
    
    if (g_ApplyResolutionSettings) {
        g_ApplyResolutionSettings(gus);
        _klog("  ApplyResolutionSettings()\n");
    }
    
    g_settings_applied = true;
    _klog("Settings applied\n");
}

void days_gone_900p_reapply_settings(void) {
    g_settings_applied = false;
    days_gone_900p_apply_settings();
}

/* ===== Simple va_list for _klog ===== */
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap) __builtin_va_end(ap)