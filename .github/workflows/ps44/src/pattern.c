#include "days_gone_900p.h"

/* ===== Pattern scanning for UE4 function resolution ===== */

/* Known string patterns from eboot.bin analysis */
static const struct {
    const char* func_name;
    const char* string_ref;
    size_t str_len;
} g_target_strings[] = {
    {"SetResolutionScaleNormalized", "SetResolutionScaleNormalized", 28},
    {"SetResolutionScaleValue", "SetResolutionScaleValue", 23},
    {"SetScreenResolution", "SetScreenResolution", 19},
    {"SetShadowQuality", "SetShadowQuality", 16},
    {"SetViewDistanceQuality", "SetViewDistanceQuality", 22},
    {"SetPostProcessingQuality", "SetPostProcessingQuality", 24},
    {"ApplyResolutionSettings", "ApplyResolutionSettings", 21},
    {"GetGameUserSettings", "GetGameUserSettings", 19},
    {NULL, NULL, 0}
};

/* String table location (from eboot.bin analysis - will be adjusted at runtime) */
static const char* g_string_table_pattern = "SetResolutionScaleNormalized";
static const size_t g_string_table_pattern_len = 28;

/* Find string in memory region */
static void* memmem_custom(const void* haystack, size_t haystack_len, 
                           const void* needle, size_t needle_len) {
    if (needle_len == 0) return (void*)haystack;
    if (haystack_len < needle_len) return NULL;
    
    const uint8_t* h = (const uint8_t*)haystack;
    const uint8_t* n = (const uint8_t*)needle;
    const uint8_t* end = h + haystack_len - needle_len;
    
    while (h <= end) {
        if (h[0] == n[0] && __builtin_memcmp(h, n, needle_len) == 0) {
            return (void*)h;
        }
        h++;
    }
    return NULL;
}

/* Check if address has function prologue (push rbp; mov rbp, rsp) */
static bool is_function_prologue(const uint8_t* addr) {
    /* push rbp (0x55) ; mov rbp, rsp (0x48 0x89 0xe5) */
    return addr[0] == 0x55 && addr[1] == 0x48 && addr[2] == 0x89 && addr[3] == 0xe5;
}

/* Scan for RIP-relative reference to a target address */
static uint64_t find_rip_relative_ref(const uint8_t* code_start, size_t code_len, uint64_t target_addr) {
    const uint8_t* end = code_start + code_len - 7;
    const uint8_t* p = code_start;
    
    while (p <= end) {
        /* LEA reg, [rip + disp32] : 48 8d XX 05 disp32  or  4c 8d XX 05 disp32 */
        if ((p[0] == 0x48 || p[0] == 0x4c) && p[1] == 0x8d && p[2] == 0x05) {
            int32_t disp32 = *(const int32_t*)(p + 3);
            uint64_t instr_addr = (uint64_t)p;
            uint64_t target = instr_addr + 7 + disp32;
            if (target == target_addr) {
                return instr_addr;
            }
        }
        /* MOV reg, [rip + disp32] : 48 8b XX 05 disp32 */
        else if (p[0] == 0x48 && p[1] == 0x8b && p[2] == 0x05) {
            int32_t disp32 = *(const int32_t*)(p + 3);
            uint64_t instr_addr = (uint64_t)p;
            uint64_t target = instr_addr + 7 + disp32;
            if (target == target_addr) {
                return instr_addr;
            }
        }
        p++;
    }
    return 0;
}

/* Find function start by scanning backwards for prologue */
static uint64_t find_function_start(const uint8_t* region_start, uint64_t ref_addr) {
    const uint8_t* p = (const uint8_t*)ref_addr;
    const uint8_t* limit = region_start;
    int scanned = 0;
    
    while (p > limit && scanned < 1024) {
        if (is_function_prologue(p)) {
            return (uint64_t)p;
        }
        p--;
        scanned++;
    }
    return 0;
}

/* Resolve all UE4 graphics functions */
int resolve_ue4_functions(void) {
    if (g_game_base == 0 || g_game_size == 0) {
        return -1;
    }
    
    /* The string table is typically in the .rodata section after the code */
    /* Based on eboot.bin analysis: strings around offset 0x4a80000 from base */
    /* But we'll search the entire game region */
    
    const uint8_t* search_start = (const uint8_t*)g_game_base;
    size_t search_len = g_game_size;
    
    _klog("Scanning game memory for UE4 strings...\n");
    
    /* First, find the string table by looking for our marker string */
    void* marker = memmem_custom(search_start, search_len, 
                                  g_string_table_pattern, g_string_table_pattern_len);
    if (!marker) {
        _klog("Marker string not found in memory!\n");
        return -1;
    }
    
    uint64_t string_table_base = (uint64_t)marker;
    /* String table spans roughly 64KB */
    const uint8_t* string_table = (const uint8_t*)string_table_base;
    size_t string_table_len = 0x10000;  /* 64KB */
    
    _klog("Found string table at 0x%lx\n", string_table_base);
    
    int resolved = 0;
    
    for (int i = 0; g_target_strings[i].func_name; i++) {
        const char* str = g_target_strings[i].string_ref;
        size_t str_len = g_target_strings[i].str_len;
        
        /* Find this specific string in the string table */
        void* str_addr = memmem_custom(string_table, string_table_len, str, str_len);
        if (!str_addr) {
            _klog("String not found: %s\n", str);
            continue;
        }
        
        uint64_t str_vaddr = (uint64_t)str_addr;
        _klog("Found '%s' at 0x%lx\n", str, str_vaddr);
        
        /* Find RIP-relative reference to this string in code section */
        uint64_t ref = find_rip_relative_ref(search_start, search_len, str_vaddr);
        if (!ref) {
            _klog("  No code reference to string\n");
            continue;
        }
        
        _klog("  Reference at 0x%lx\n", ref);
        
        /* Find function start */
        uint64_t func_addr = find_function_start(search_start, ref);
        if (!func_addr) {
            _klog("  No function prologue found\n");
            continue;
        }
        
        _klog("  Function at 0x%lx\n", func_addr);
        
        /* Assign to function pointer */
        if (strcmp(str, "SetResolutionScaleNormalized") == 0) {
            g_SetResolutionScaleNormalized = (SetResolutionScaleNormalized_t)func_addr;
            resolved++;
        } else if (strcmp(str, "SetResolutionScaleValue") == 0) {
            g_SetResolutionScaleValue = (SetResolutionScaleValue_t)func_addr;
            resolved++;
        } else if (strcmp(str, "SetScreenResolution") == 0) {
            g_SetScreenResolution = (SetScreenResolution_t)func_addr;
            resolved++;
        } else if (strcmp(str, "SetShadowQuality") == 0) {
            g_SetShadowQuality = (SetShadowQuality_t)func_addr;
            resolved++;
        } else if (strcmp(str, "SetViewDistanceQuality") == 0) {
            g_SetViewDistanceQuality = (SetViewDistanceQuality_t)func_addr;
            resolved++;
        } else if (strcmp(str, "SetPostProcessingQuality") == 0) {
            g_SetPostProcessingQuality = (SetPostProcessingQuality_t)func_addr;
            resolved++;
        } else if (strcmp(str, "ApplyResolutionSettings") == 0) {
            g_ApplyResolutionSettings = (ApplyResolutionSettings_t)func_addr;
            resolved++;
        } else if (strcmp(str, "GetGameUserSettings") == 0) {
            g_GetGameUserSettings = (GetGameUserSettings_t)func_addr;
            resolved++;
        }
    }
    
    _klog("Resolved %d UE4 functions\n", resolved);
    return resolved > 0 ? 0 : -1;
}