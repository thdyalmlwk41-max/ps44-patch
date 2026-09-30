#ifndef DAYS_GONE_900P_H
#define DAYS_GONE_900P_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ===== Orbis Kernel Types ===== */
typedef int64_t ssize_t;
typedef uint64_t size_t;
typedef int32_t pid_t;

/* ===== SCE Kernel Error Codes ===== */
#define SCE_OK                    0
#define SCE_KERNEL_ERROR_EINVAL   0x80010016
#define SCE_KERNEL_ERROR_ESRCH    0x80010003
#define SCE_KERNEL_ERROR_EFAULT   0x80010014
#define SCE_KERNEL_ERROR_ENOMEM   0x80010012
#define SCE_KERNEL_ERROR_EBUSY    0x80010011

/* ===== Memory Protection ===== */
#define SCE_KERNEL_PROT_READ      0x1
#define SCE_KERNEL_PROT_WRITE     0x2
#define SCE_KERNEL_PROT_EXEC      0x4

/* ===== Memory Block Types ===== */
#define SCE_KERNEL_MEMBLOCK_TYPE_USER_RW        0x0C20D060
#define SCE_KERNEL_MEMBLOCK_TYPE_USER_RX        0x0C20D050
#define SCE_KERNEL_MEMBLOCK_TYPE_USER_RWX       0x0C20D0F0
#define SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RWX     0x0C40D0F0

/* ===== SCE Module Info ===== */
typedef struct SceKernelModuleInfo {
    uint32_t size;
    char name[256];
    uint32_t attribute;
    uint32_t version;
    void* entry;
    void* gp;
    uint32_t tls_id;
    uint32_t tls_filesz;
    uint32_t tls_memsz;
    uint32_t tls_align;
    uint8_t  reserved[128];
} SceKernelModuleInfo;

/* ===== Process VM Map Entry ===== */
typedef struct SceKernelProcessVmMapEntry {
    uint64_t start;
    uint64_t end;
    uint32_t prot;
    uint32_t flags;
    char name[256];
    uint64_t file_offset;
    uint64_t file_size;
    uint8_t  reserved[64];
} SceKernelProcessVmMapEntry;

/* ===== Function pointer types for UE4 GameUserSettings ===== */
typedef void (*SetResolutionScaleNormalized_t)(void* gus, float value);
typedef void (*SetResolutionScaleValue_t)(void* gus, float value);
typedef void (*SetScreenResolution_t)(void* gus, int32_t width, int32_t height);
typedef void (*SetShadowQuality_t)(void* gus, int32_t quality);
typedef void (*SetViewDistanceQuality_t)(void* gus, int32_t quality);
typedef void (*SetPostProcessingQuality_t)(void* gus, int32_t quality);
typedef void (*ApplyResolutionSettings_t)(void* gus);
typedef void* (*GetGameUserSettings_t)(void);

/* ===== Global resolved function pointers ===== */
extern SetResolutionScaleNormalized_t g_SetResolutionScaleNormalized;
extern SetResolutionScaleValue_t g_SetResolutionScaleValue;
extern SetScreenResolution_t g_SetScreenResolution;
extern SetShadowQuality_t g_SetShadowQuality;
extern SetViewDistanceQuality_t g_SetViewDistanceQuality;
extern SetPostProcessingQuality_t g_SetPostProcessingQuality;
extern ApplyResolutionSettings_t g_ApplyResolutionSettings;
extern GetGameUserSettings_t g_GetGameUserSettings;

/* ===== Game base address (resolved at runtime) ===== */
extern uint64_t g_game_base;
extern uint64_t g_game_size;

/* ===== Public API ===== */
int days_gone_900p_init(void);
void days_gone_900p_apply_settings(void);
void days_gone_900p_reapply_settings(void);

/* ===== Debug logging (uses kernel printf) ===== */
#define klog(fmt, ...)  do { \
    extern void _klog(const char*, ...); \
    _klog("[days_gone_900p] " fmt, ##__VA_ARGS__); \
} while(0)

void _klog(const char* fmt, ...);

#endif