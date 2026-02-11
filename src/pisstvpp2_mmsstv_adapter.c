/*
 * pisstvpp2_mmsstv_adapter.c
 *
 * MMSSTV Library Integration Adapter Implementation
 *
 * This module implements the integration between PiSSTVpp2 and mmsstv-portable.
 * It provides a unified interface that can use either:
 * - Legacy SSTV module (pisstvpp2_sstv.c) for 7 basic modes
 * - MMSSTV library for 100+ extended modes
 *
 * The adapter automatically routes encoding requests to the appropriate backend
 * based on mode selection and build configuration.
 *
 * Author: Integration adapter for mmsstv-portable
 * Date: January 29, 2026
 * Status: STUB IMPLEMENTATION - Replace with actual logic when mmsstv-portable is ready
 */

#include "pisstvpp2_mmsstv_adapter.h"
#include "pisstvpp2_image.h"
#include "pisstvpp2_sstv.h"
#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
   MODE MAPPING TABLE
   ============================================================================ */

/**
 * Master mode table mapping protocol strings to mode information.
 * Supports both legacy and MMSSTV modes.
 */
static const mmsstv_mode_map_t g_mode_table[] = {
    /* Legacy modes (always available) */
    { "m1",     44, "Martin 1",          true  },
    { "m2",     40, "Martin 2",          true  },
    { "s1",     60, "Scottie 1",         true  },
    { "s2",     56, "Scottie 2",         true  },
    { "sdx",    76, "Scottie DX",        true  },
    { "r36",    8,  "Robot 36 Color",    true  },
    { "r72",    12, "Robot 72 Color",    true  },

#ifdef HAVE_MMSSTV_SUPPORT
    /* Extended MMSSTV modes (only with library) */
    { "m3",     104, "Martin 3",         false, MMSSTV_MODE_MARTIN_M3 },
    { "m4",     108, "Martin 4",         false, MMSSTV_MODE_MARTIN_M4 },
    { "s3",     52,  "Scottie 3",        false, MMSSTV_MODE_SCOTTIE_S3 },
    { "s4",     48,  "Scottie 4",        false, MMSSTV_MODE_SCOTTIE_S4 },
    { "r12",    4,   "Robot 12 Color",   false, MMSSTV_MODE_ROBOT_12 },
    { "r24",    12,  "Robot 24 Color",   false, MMSSTV_MODE_ROBOT_24 },
    
    /* PD modes */
    { "pd50",   93,  "PD 50",            false, MMSSTV_MODE_PD_50 },
    { "pd90",   99,  "PD 90",            false, MMSSTV_MODE_PD_90 },
    { "pd120",  95,  "PD 120",           false, MMSSTV_MODE_PD_120 },
    { "pd160",  98,  "PD 160",           false, MMSSTV_MODE_PD_160 },
    { "pd180",  96,  "PD 180",           false, MMSSTV_MODE_PD_180 },
    { "pd240",  97,  "PD 240",           false, MMSSTV_MODE_PD_240 },
    { "pd290",  94,  "PD 290",           false, MMSSTV_MODE_PD_290 },
    
    /* Wraase modes */
    { "sc2_60",  56, "Wraase SC2-60",    false, MMSSTV_MODE_WRAASE_SC2_60 },
    { "sc2_120", 55, "Wraase SC2-120",   false, MMSSTV_MODE_WRAASE_SC2_120 },
    
    /* AVT modes */
    { "avt24",  68,  "AVT 24",           false, MMSSTV_MODE_AVT_24 },
    { "avt90",  69,  "AVT 90",           false, MMSSTV_MODE_AVT_90 },
    { "avt94",  70,  "AVT 94",           false, MMSSTV_MODE_AVT_94 },
    
    /* Pasokon modes */
    { "p3",     113, "Pasokon P3",       false, MMSSTV_MODE_PASOKON_P3 },
    { "p5",     114, "Pasokon P5",       false, MMSSTV_MODE_PASOKON_P5 },
    { "p7",     115, "Pasokon P7",       false, MMSSTV_MODE_PASOKON_P7 },
    
    /* FAX mode */
    { "fax480", 0,   "FAX 480",          false, MMSSTV_MODE_FAX_480 },
    
    /* ML modes */
    { "ml180",  120, "ML 180",           false, MMSSTV_MODE_ML_180 },
    { "ml240",  121, "ML 240",           false, MMSSTV_MODE_ML_240 },
    { "ml320",  122, "ML 320",           false, MMSSTV_MODE_ML_320 },
#endif
};

static const int g_mode_table_size = sizeof(g_mode_table) / sizeof(g_mode_table[0]);

/* ============================================================================
   ADAPTER CONTEXT STRUCTURE
   ============================================================================ */

struct mmsstv_adapter_ctx {
    const char *protocol;                /**< Protocol string */
    uint32_t sample_rate;                /**< Audio sample rate */
    bool use_legacy;                     /**< True to use legacy backend */
    bool vis_header_enabled;             /**< VIS header enable flag */
    
#ifdef HAVE_MMSSTV_SUPPORT
    mmsstv_encoder_ctx_t *mmsstv_ctx;    /**< MMSSTV encoder context */
    mmsstv_mode_t mmsstv_mode;           /**< MMSSTV mode identifier */
#endif
    
    /* Audio buffer state */
    uint16_t *audio_buffer;              /**< Audio output buffer */
    uint32_t max_samples;                /**< Buffer capacity */
    uint32_t samples_written;            /**< Samples written so far */
};

/* ============================================================================
   MODULE STATE
   ============================================================================ */

static bool g_adapter_initialized = false;
static bool g_verbose = false;
static char g_error_msg[256] = {0};

/* ============================================================================
   ERROR HANDLING
   ============================================================================ */

static void set_error(const char *msg) {
    snprintf(g_error_msg, sizeof(g_error_msg), "%s", msg);
}

const char* mmsstv_adapter_get_error(void) {
    return g_error_msg[0] ? g_error_msg : NULL;
}

/* ============================================================================
   INITIALIZATION
   ============================================================================ */

int mmsstv_adapter_init(uint32_t sample_rate, bool verbose) {
    if (g_adapter_initialized) {
        set_error("Adapter already initialized");
        return PISSTVPP2_ERR_MMSSTV_INIT;
    }
    
    g_verbose = verbose;
    
    if (g_verbose) {
        LOG_INFO("Initializing MMSSTV adapter (sample rate: %u Hz)", sample_rate);
    }
    
#ifdef HAVE_MMSSTV_SUPPORT
    if (mmsstv_init() != 0) {
        set_error("Failed to initialize MMSSTV library");
        return PISSTVPP2_ERR_MMSSTV_INIT;
    }
    
    if (g_verbose) {
        LOG_INFO("MMSSTV library initialized: %s", mmsstv_version());
    }
#else
    if (g_verbose) {
        LOG_INFO("MMSSTV support not compiled - using legacy modes only");
    }
#endif
    
    g_adapter_initialized = true;
    return PISSTVPP2_OK;
}

void mmsstv_adapter_cleanup(void) {
    if (!g_adapter_initialized) {
        return;
    }
    
#ifdef HAVE_MMSSTV_SUPPORT
    mmsstv_cleanup();
#endif
    
    g_adapter_initialized = false;
    
    if (g_verbose) {
        LOG_INFO("MMSSTV adapter cleanup complete");
    }
}

bool mmsstv_adapter_is_available(void) {
#ifdef HAVE_MMSSTV_SUPPORT
    return true;
#else
    return false;
#endif
}

const char* mmsstv_adapter_version(void) {
    return "1.0.0-stub";
}

/* ============================================================================
   MODE DISCOVERY
   ============================================================================ */

int mmsstv_adapter_list_modes(mmsstv_mode_map_t *modes, int max_modes) {
    int count = 0;
    
    for (int i = 0; i < g_mode_table_size && count < max_modes; i++) {
#ifndef HAVE_MMSSTV_SUPPORT
        /* Skip non-legacy modes if MMSSTV not available */
        if (!g_mode_table[i].is_legacy) {
            continue;
        }
#endif
        modes[count++] = g_mode_table[i];
    }
    
    return count;
}

int mmsstv_adapter_find_mode(const char *protocol, mmsstv_mode_map_t *mode_map) {
    for (int i = 0; i < g_mode_table_size; i++) {
        if (strcmp(g_mode_table[i].protocol_str, protocol) == 0) {
#ifndef HAVE_MMSSTV_SUPPORT
            /* Reject non-legacy modes if MMSSTV not available */
            if (!g_mode_table[i].is_legacy) {
                set_error("Mode requires MMSSTV support (not compiled)");
                return -1;
            }
#endif
            *mode_map = g_mode_table[i];
            return 0;
        }
    }
    
    set_error("Unknown protocol");
    return -1;
}

bool mmsstv_adapter_is_mode_supported(const char *protocol) {
    mmsstv_mode_map_t mode_map;
    return (mmsstv_adapter_find_mode(protocol, &mode_map) == 0);
}

int mmsstv_adapter_get_mode_info(
    const char *protocol,
    uint16_t *width,
    uint16_t *height,
    double *duration
) {
    mmsstv_mode_map_t mode_map;
    
    if (mmsstv_adapter_find_mode(protocol, &mode_map) != 0) {
        return -1;
    }
    
#ifdef HAVE_MMSSTV_SUPPORT
    if (!mode_map.is_legacy) {
        mmsstv_mode_params_t params;
        if (mmsstv_get_mode_params(mode_map.mmsstv_mode, &params) == 0) {
            *width = params.width;
            *height = params.height;
            *duration = params.frame_time / 1000000.0;
            return 0;
        }
        return -1;
    }
#endif
    
    /* Legacy mode information (hardcoded for now) */
    /* TODO: Get this from sstv module once refactored */
    if (strcmp(protocol, "m1") == 0 || strcmp(protocol, "m2") == 0) {
        *width = 320;
        *height = 256;
        *duration = (strcmp(protocol, "m1") == 0) ? 114.0 : 58.0;
    } else if (strcmp(protocol, "s1") == 0 || strcmp(protocol, "s2") == 0 || 
               strcmp(protocol, "sdx") == 0) {
        *width = 320;
        *height = 256;
        *duration = (strcmp(protocol, "s1") == 0) ? 110.0 : 71.0;
    } else if (strcmp(protocol, "r36") == 0 || strcmp(protocol, "r72") == 0) {
        *width = 320;
        *height = 240;
        *duration = (strcmp(protocol, "r36") == 0) ? 36.0 : 72.0;
    } else {
        return -1;
    }
    
    return 0;
}

/* ============================================================================
   ENCODER CREATION
   ============================================================================ */

mmsstv_adapter_ctx_t* mmsstv_adapter_create(
    const char *protocol,
    uint32_t sample_rate
) {
    if (!g_adapter_initialized) {
        set_error("Adapter not initialized");
        return NULL;
    }
    
    mmsstv_mode_map_t mode_map;
    if (mmsstv_adapter_find_mode(protocol, &mode_map) != 0) {
        return NULL;
    }
    
    mmsstv_adapter_ctx_t *ctx = calloc(1, sizeof(mmsstv_adapter_ctx_t));
    if (!ctx) {
        set_error("Memory allocation failed");
        return NULL;
    }
    
    ctx->protocol = mode_map.protocol_str;
    ctx->sample_rate = sample_rate;
    ctx->use_legacy = mode_map.is_legacy;
    ctx->vis_header_enabled = true;
    
#ifdef HAVE_MMSSTV_SUPPORT
    if (!ctx->use_legacy) {
        ctx->mmsstv_mode = mode_map.mmsstv_mode;
        ctx->mmsstv_ctx = mmsstv_encoder_create(ctx->mmsstv_mode, sample_rate);
        if (!ctx->mmsstv_ctx) {
            set_error("Failed to create MMSSTV encoder");
            free(ctx);
            return NULL;
        }
    }
#endif
    
    if (g_verbose) {
        LOG_INFO("Created adapter context for %s (%s backend)",
                 mode_map.mode_name,
                 ctx->use_legacy ? "legacy" : "MMSSTV");
    }
    
    return ctx;
}

void mmsstv_adapter_destroy(mmsstv_adapter_ctx_t *ctx) {
    if (!ctx) {
        return;
    }
    
#ifdef HAVE_MMSSTV_SUPPORT
    if (ctx->mmsstv_ctx) {
        mmsstv_encoder_destroy(ctx->mmsstv_ctx);
    }
#endif
    
    free(ctx);
}

/* ============================================================================
   ENCODING CONFIGURATION
   ============================================================================ */

int mmsstv_adapter_set_vis_header(mmsstv_adapter_ctx_t *ctx, bool enable) {
    if (!ctx) {
        set_error("Invalid context");
        return -1;
    }
    
    ctx->vis_header_enabled = enable;
    
#ifdef HAVE_MMSSTV_SUPPORT
    if (!ctx->use_legacy && ctx->mmsstv_ctx) {
        return mmsstv_encoder_set_vis_header(ctx->mmsstv_ctx, enable);
    }
#endif
    
    return 0;
}

/* ============================================================================
   PIXEL CALLBACK (delegates to image module)
   ============================================================================ */

int mmsstv_adapter_pixel_callback(
    int x, int y,
    uint8_t *r, uint8_t *g, uint8_t *b,
    void *user_data
) {
    /* Delegate to image module's pixel access function */
    return image_get_pixel_rgb(x, y, r, g, b);
}

/* ============================================================================
   AUDIO CALLBACK (writes to buffer)
   ============================================================================ */

int mmsstv_adapter_audio_callback(
    const uint16_t *samples,
    uint32_t count,
    void *user_data
) {
    mmsstv_adapter_ctx_t *ctx = (mmsstv_adapter_ctx_t*)user_data;
    
    if (!ctx || !ctx->audio_buffer) {
        return -1;
    }
    
    if (ctx->samples_written + count > ctx->max_samples) {
        set_error("Audio buffer overflow");
        return -1;
    }
    
    memcpy(&ctx->audio_buffer[ctx->samples_written], samples, count * sizeof(uint16_t));
    ctx->samples_written += count;
    
    return (int)count;
}

/* ============================================================================
   FRAME ENCODING
   ============================================================================ */

int mmsstv_adapter_encode_frame(
    mmsstv_adapter_ctx_t *ctx,
    uint16_t *audio_buffer,
    uint32_t max_samples,
    uint32_t *samples_written
) {
    if (!ctx || !audio_buffer || !samples_written) {
        set_error("Invalid parameters");
        return -1;
    }
    
    ctx->audio_buffer = audio_buffer;
    ctx->max_samples = max_samples;
    ctx->samples_written = 0;
    
    if (ctx->use_legacy) {
        /* Delegate to legacy SSTV module */
        /* NOTE: This is a stub - actual integration requires refactoring
         * the legacy module to return samples instead of using globals */
        if (g_verbose) {
            LOG_INFO("Using legacy SSTV encoder for %s", ctx->protocol);
        }
        
        /* TODO: Call sstv_encode_frame() when module is refactored */
        set_error("Legacy encoding not yet integrated");
        return -1;
    }
    
#ifdef HAVE_MMSSTV_SUPPORT
    /* Use MMSSTV library */
    if (!ctx->mmsstv_ctx) {
        set_error("MMSSTV encoder not initialized");
        return -1;
    }
    
    /* Set callbacks */
    if (mmsstv_encoder_set_pixel_callback(ctx->mmsstv_ctx, 
                                          mmsstv_adapter_pixel_callback, 
                                          NULL) != 0) {
        set_error("Failed to set pixel callback");
        return -1;
    }
    
    if (mmsstv_encoder_set_audio_callback(ctx->mmsstv_ctx,
                                          mmsstv_adapter_audio_callback,
                                          ctx) != 0) {
        set_error("Failed to set audio callback");
        return -1;
    }
    
    /* Encode frame */
    if (g_verbose) {
        LOG_INFO("Encoding frame using MMSSTV library");
    }
    
    int result = mmsstv_encoder_encode_frame(ctx->mmsstv_ctx);
    if (result != 0) {
        set_error(mmsstv_get_error());
        return result;
    }
    
    *samples_written = ctx->samples_written;
    
    if (g_verbose) {
        LOG_INFO("Encoding complete: %u samples generated", *samples_written);
    }
    
    return 0;
#else
    set_error("MMSSTV support not compiled");
    return -1;
#endif
}

uint32_t mmsstv_adapter_estimate_samples(
    const char *protocol,
    uint32_t sample_rate
) {
    uint16_t width, height;
    double duration;
    
    if (mmsstv_adapter_get_mode_info(protocol, &width, &height, &duration) != 0) {
        return 0;
    }
    
    /* Estimate: duration * sample_rate * 1.1 (10% safety margin) */
    return (uint32_t)(duration * sample_rate * 1.1);
}

/* ============================================================================
   UTILITY FUNCTIONS
   ============================================================================ */

void mmsstv_adapter_print_mode_details(const char *protocol) {
    mmsstv_mode_map_t mode_map;
    
    if (mmsstv_adapter_find_mode(protocol, &mode_map) != 0) {
        printf("Unknown mode: %s\n", protocol);
        return;
    }
    
    uint16_t width, height;
    double duration;
    
    if (mmsstv_adapter_get_mode_info(protocol, &width, &height, &duration) == 0) {
        printf("  Mode name:     %s\n", mode_map.mode_name);
        printf("  Protocol:      %s\n", mode_map.protocol_str);
        printf("  VIS code:      %u\n", mode_map.vis_code);
        printf("  Resolution:    %ux%u\n", width, height);
        printf("  TX Time:       %.1f seconds\n", duration);
        printf("  Backend:       %s\n", mode_map.is_legacy ? "Legacy" : "MMSSTV");
    }
}

void mmsstv_adapter_print_mode_list(bool include_legacy, bool include_mmsstv) {
    printf("Available SSTV modes:\n\n");
    
    for (int i = 0; i < g_mode_table_size; i++) {
        bool is_legacy = g_mode_table[i].is_legacy;
        
        if ((is_legacy && !include_legacy) || (!is_legacy && !include_mmsstv)) {
            continue;
        }
        
#ifndef HAVE_MMSSTV_SUPPORT
        if (!is_legacy) {
            continue;
        }
#endif
        
        printf("  %-10s - %s (VIS %u) %s\n",
               g_mode_table[i].protocol_str,
               g_mode_table[i].mode_name,
               g_mode_table[i].vis_code,
               is_legacy ? "[Legacy]" : "[MMSSTV]");
    }
    
    printf("\n");
}
