/**
 * @file mmsstv_mock.c
 * @brief Mock MMSSTV library for testing Phase 4 integration
 * 
 * This is a minimal implementation that provides 3 test modes:
 * - PD 120 (high-res color)
 * - Martin 3 (mid-res color)  
 * - Scottie 3 (alternative timing)
 * 
 * Build:
 *   gcc -shared -fPIC -o libmmsstv.dylib mmsstv_mock.c
 * 
 * Test:
 *   MMSSTV_LIB_PATH=./libmmsstv.dylib ./bin/slowframe -L
 * 
 * @date February 15, 2026
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* ========================================================================
 * Mock Mode Definitions
 * ======================================================================== */

typedef struct {
    const char *code;
    const char *name;
    int vis_code;
    int width;
    int height;
    int duration_ms;
    const char *color_space;
    uint32_t capabilities;
    void *mode_data;
} mmsstv_mode_t;

/* Mock modes for testing */
static mmsstv_mode_t g_mock_modes[] = {
    {
        .code = "pd120",
        .name = "PD 120",
        .vis_code = 0x5F,
        .width = 640,
        .height = 496,
        .duration_ms = 126000,  /* 126 seconds */
        .color_space = "YUV",
        .capabilities = 0x01,   /* Color */
        .mode_data = (void*)0   /* Mode enum 0 */
    },
    {
        .code = "m3",
        .name = "Martin 3",
        .vis_code = 0x68,
        .width = 320,
        .height = 256,
        .duration_ms = 57000,   /* 57 seconds */
        .color_space = "RGB",
        .capabilities = 0x01,   /* Color */
        .mode_data = (void*)1   /* Mode enum 1 */
    },
    {
        .code = "s3",
        .name = "Scottie 3",
        .vis_code = 0x34,
        .width = 320,
        .height = 256,
        .duration_ms = 36000,   /* 36 seconds */
        .color_space = "RGB",
        .capabilities = 0x01,   /* Color */
        .mode_data = (void*)2   /* Mode enum 2 */
    }
};

#define MOCK_MODE_COUNT (sizeof(g_mock_modes) / sizeof(g_mock_modes[0]))

/* ========================================================================
 * MMSSTV Library API Implementation
 * ======================================================================== */

/**
 * Get library version
 */
const char* mmsstv_get_version(void) {
    return "Mock-MMSSTV-1.0.0-test";
}

/**
 * Get API version (for compatibility checking)
 */
void mmsstv_get_api_version(int *major, int *minor, int *patch) {
    if (major) *major = 1;
    if (minor) *minor = 0;
    if (patch) *patch = 0;
}

/**
 * Get number of available modes
 */
int mmsstv_get_mode_count(void) {
    return MOCK_MODE_COUNT;
}

/**
 * Get mode by index
 */
const mmsstv_mode_t* mmsstv_get_mode(int index) {
    if (index < 0 || index >= (int)MOCK_MODE_COUNT) {
        return NULL;
    }
    return &g_mock_modes[index];
}

/**
 * Find mode by code (case-insensitive)
 */
const mmsstv_mode_t* mmsstv_find_mode(const char *code) {
    if (!code) {
        return NULL;
    }
    
    for (size_t i = 0; i < MOCK_MODE_COUNT; i++) {
        if (strcasecmp(g_mock_modes[i].code, code) == 0) {
            return &g_mock_modes[i];
        }
    }
    
    return NULL;
}

/**
 * Get error message for error code
 */
const char* mmsstv_get_error(int error_code) {
    switch (error_code) {
        case 0:  return "Success";
        case -1: return "Invalid argument";
        case -2: return "Out of memory";
        case -3: return "Invalid mode";
        default: return "Unknown error";
    }
}

/* ========================================================================
 * Stub Functions (Not Implemented in Mock)
 * ======================================================================== */

/**
 * Create encoder (stub - not implemented in mock)
 */
void* mmsstv_encoder_create(const mmsstv_mode_t *mode, const void *image) {
    (void)mode;
    (void)image;
    return NULL;  /* Not implemented in mock */
}

/**
 * Encode samples (stub - not implemented in mock)
 */
int mmsstv_encode(void *encoder, float *samples, int max_samples, int *samples_written) {
    (void)encoder;
    (void)samples;
    (void)max_samples;
    (void)samples_written;
    return -1;  /* Not implemented in mock */
}

/**
 * Destroy encoder (stub - not implemented in mock)
 */
void mmsstv_encoder_destroy(void *encoder) {
    (void)encoder;
    /* Not implemented in mock */
}
