#include "days_gone_900p.h"

/* ===== UE4 GameUserSettings hooking ===== */

/* Hook for GameUserSettings tick/update to re-apply settings periodically */
static void (*g_original_GUS_Tick)(void* this_ptr) = NULL;

/* Counter to avoid applying every single frame */
static int g_tick_counter = 0;

void GUS_Tick_Hook(void* this_ptr) {
    /* Call original */
    if (g_original_GUS_Tick) {
        g_original_GUS_Tick(this_ptr);
    }
    
    /* Apply settings every ~60 frames (once per second at 60fps) */
    g_tick_counter++;
    if (g_tick_counter >= 60) {
        g_tick_counter = 0;
        if (g_settings_applied) {
            /* Re-apply in case game reset them */
            days_gone_900p_apply_settings();
        }
    }
}

/* Find and hook GameUserSettings::Tick */
int hook_gamesettings_tick(void) {
    if (!g_game_base || !g_game_size) return -1;
    
    /* Look for GameUserSettings vtable - search for the string */
    const char* vtable_marker = "GameUserSettings";
    const size_t marker_len = 16;
    
    void* marker = memmem_custom((const void*)g_game_base, g_game_size, vtable_marker, marker_len);
    if (!marker) {
        _klog("GameUserSettings vtable marker not found\n");
        return -1;
    }
    
    /* The vtable should be nearby - scan for function pointers */
    uint8_t* scan = (uint8_t*)marker;
    uint8_t* limit = scan + 0x1000;  /* Search 4KB forward */
    
    while (scan < limit) {
        uint64_t* ptr = (uint64_t*)scan;
        /* Check if this looks like a vtable entry (points to code) */
        if (*ptr > g_game_base && *ptr < g_game_base + g_game_size) {
            /* Check if it's a function prologue */
            if (is_function_prologue((const uint8_t*)*ptr)) {
                _klog("Potential GUS vtable entry at %p -> %p\n", scan, (void*)*ptr);
                /* This might be the Tick function - hook it */
                if (hook_install((void*)*ptr, GUS_Tick_Hook, (void**)&g_original_GUS_Tick) == 0) {
                    _klog("Hooked GameUserSettings function\n");
                    return 0;
                }
            }
        }
        scan += 8;
    }
    
    _klog("Could not find suitable GUS Tick function to hook\n");
    return -1;
}

/* Alternative: Hook a known game tick function */
int hook_game_tick(void) {
    /* Search for common game loop patterns */
    /* This is a fallback if GUS hook fails */
    return -1;
}