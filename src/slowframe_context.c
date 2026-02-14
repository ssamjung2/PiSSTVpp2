/**
 * @file slowframe_context.c
 * @brief Implementation of application context and state management
 *
 * Provides initialization, cleanup, and state management for the complete
 * application. Orchestrates subsystem initialization and ensures proper
 * resource lifecycle.
 *
 * @author SlowFrame Contributors
 * @version 2.1.0
 * @date February 2026
 */

#include "slowframe_context.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ===========================================================================
// MODULE STATE STRUCTURES
// ===========================================================================

/**
 * @brief Image processing module state
 *
 * Currently a placeholder; in Task 1.4, this will be expanded to hold:
 * - VipsImage pointer
 * - Dimension and aspect ratio information
 * - Intermediate image buffers
 * - Processing statistics
 */
struct SlowframeImageState {
    int initialized;  /**< Dummy field for now; will expand in Task 1.4 */
};

/**
 * @brief SSTV encoding module state
 *
 * Currently a placeholder; in Task 1.4, this will expand to hold:
 * - SSTV mode registry
 * - VIS code lookup table
 * - Audio sample buffer
 * - Encoding statistics
 */
struct SlowframeSSTVState {
    int initialized;  /**< Dummy field for now; will expand in Task 1.4 */
};

/**
 * @brief Audio encoder module state
 *
 * Currently a placeholder; in Task 1.4, this will expand to hold:
 * - Format-specific encoder context
 * - Output file handle
 * - Sample rate and bit depth configuration
 * - Encoding progress tracking
 */
struct SlowframeAudioState {
    int initialized;  /**< Dummy field for now; will expand in Task 1.4 */
};

// ===========================================================================
// PUBLIC FUNCTION IMPLEMENTATIONS
// ===========================================================================

/**
 * @brief Initialize application context from configuration
 */
int slowframe_context_init(SlowframeContext *ctx, SlowframeConfig *config) {
    if (!ctx || !config) {
        error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL, 
                "Invalid context or config pointer");
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }

    // Initialize context structure
    memset(ctx, 0, sizeof(SlowframeContext));

    // =====================================================================
    // COPY CONFIGURATION
    // =====================================================================
    memcpy(&ctx->config, config, sizeof(SlowframeConfig));
    ctx->config_initialized = 1;

    // =====================================================================
    // INITIALIZE LIBVIPS
    // =====================================================================
    if (VIPS_INIT("slowframe")) {
        error_log(SLOWFRAME_ERR_SSTV_INIT, 
                "Failed to initialize libvips: %s", vips_error_buffer());
        return SLOWFRAME_ERR_SSTV_INIT;
    }
    ctx->vips_initialized = 1;

    // =====================================================================
    // INITIALIZE IMAGE PROCESSING MODULE
    // =====================================================================
    ctx->image_state = (SlowframeImageState *)malloc(sizeof(SlowframeImageState));
    if (!ctx->image_state) {
        error_log(SLOWFRAME_ERR_MEMORY_ALLOC, 
                "Failed to allocate image module state");
        slowframe_context_cleanup(ctx);
        return SLOWFRAME_ERR_MEMORY_ALLOC;
    }
    memset(ctx->image_state, 0, sizeof(SlowframeImageState));
    ctx->image_state->initialized = 1;
    ctx->image_initialized = 1;

    // =====================================================================
    // INITIALIZE SSTV ENCODING MODULE
    // =====================================================================
    ctx->sstv_state = (SlowframeSSTVState *)malloc(sizeof(SlowframeSSTVState));
    if (!ctx->sstv_state) {
        error_log(SLOWFRAME_ERR_MEMORY_ALLOC,
                "Failed to allocate SSTV module state");
        slowframe_context_cleanup(ctx);
        return SLOWFRAME_ERR_MEMORY_ALLOC;
    }
    memset(ctx->sstv_state, 0, sizeof(SlowframeSSTVState));
    ctx->sstv_state->initialized = 1;
    ctx->sstv_initialized = 1;

    // =====================================================================
    // INITIALIZE AUDIO ENCODER MODULE
    // =====================================================================
    ctx->audio_state = (SlowframeAudioState *)malloc(sizeof(SlowframeAudioState));
    if (!ctx->audio_state) {
        error_log(SLOWFRAME_ERR_MEMORY_ALLOC,
                "Failed to allocate audio encoder state");
        slowframe_context_cleanup(ctx);
        return SLOWFRAME_ERR_MEMORY_ALLOC;
    }
    memset(ctx->audio_state, 0, sizeof(SlowframeAudioState));
    ctx->audio_state->initialized = 1;
    ctx->audio_initialized = 1;

    return SLOWFRAME_OK;
}

/**
 * @brief Clean up application context and release all resources
 */
void slowframe_context_cleanup(SlowframeContext *ctx) {
    if (!ctx) {
        return;  // Safe to call with NULL
    }

    // =====================================================================
    // CLEANUP AUDIO ENCODER (first, in reverse of init order)
    // =====================================================================
    if (ctx->audio_initialized && ctx->audio_state) {
        free(ctx->audio_state);
        ctx->audio_state = NULL;
        ctx->audio_initialized = 0;
    }

    // =====================================================================
    // CLEANUP SSTV ENCODING MODULE
    // =====================================================================
    if (ctx->sstv_initialized && ctx->sstv_state) {
        free(ctx->sstv_state);
        ctx->sstv_state = NULL;
        ctx->sstv_initialized = 0;
    }

    // =====================================================================
    // CLEANUP IMAGE PROCESSING MODULE
    // =====================================================================
    if (ctx->image_initialized && ctx->image_state) {
        free(ctx->image_state);
        ctx->image_state = NULL;
        ctx->image_initialized = 0;
    }

    // =====================================================================
    // SHUTDOWN LIBVIPS
    // =====================================================================
    if (ctx->vips_initialized) {
        vips_shutdown();
        ctx->vips_initialized = 0;
    }

    // =====================================================================
    // CLEAR CONFIGURATION
    // =====================================================================
    ctx->config_initialized = 0;
    memset(&ctx->config, 0, sizeof(SlowframeConfig));

    // Zero out the entire context for safety
    memset(ctx, 0, sizeof(SlowframeContext));
}

/**
 * @brief Check if all required subsystems are initialized
 */
int slowframe_context_is_valid(const SlowframeContext *ctx) {
    if (!ctx) {
        return 0;
    }

    // All subsystems must be initialized
    return ctx->config_initialized &&
           ctx->image_initialized &&
           ctx->sstv_initialized &&
           ctx->audio_initialized &&
           ctx->vips_initialized &&
           ctx->image_state &&
           ctx->sstv_state &&
           ctx->audio_state;
}

/**
 * @brief Get configuration from context (const access)
 */
const SlowframeConfig* slowframe_context_get_config(const SlowframeContext *ctx) {
    if (!ctx) {
        return NULL;
    }
    return &ctx->config;
}

/**
 * @brief Print context state to output for debugging
 */
void slowframe_context_print_state(const SlowframeContext *ctx) {
    if (!ctx) {
        return;
    }

    printf("=== Application Context State ===\n");
    printf("Configuration initialized: %s\n", 
           ctx->config_initialized ? "Yes" : "No");
    printf("libvips initialized:       %s\n",
           ctx->vips_initialized ? "Yes" : "No");
    printf("Image module initialized:  %s\n",
           ctx->image_initialized ? "Yes" : "No");
    printf("SSTV module initialized:   %s\n",
           ctx->sstv_initialized ? "Yes" : "No");
    printf("Audio module initialized:  %s\n",
           ctx->audio_initialized ? "Yes" : "No");
    printf("Context valid:             %s\n",
           slowframe_context_is_valid(ctx) ? "Yes" : "No");
    printf("==================================\n");

    if (ctx->config_initialized) {
        // Print configuration details
        printf("\nConfiguration:\n");
        printf("  Input:    %s\n", ctx->config.input_file);
        printf("  Output:   %s\n", ctx->config.output_file);
        printf("  Protocol: %s\n", ctx->config.protocol);
        printf("  Format:   %s\n", ctx->config.format);
        printf("  Sample rate: %d Hz\n", ctx->config.sample_rate);
    }
}

/**
 * @brief Get opaque pointer to image module state
 */
SlowframeImageState* slowframe_context_get_image_state(SlowframeContext *ctx) {
    if (!ctx) {
        return NULL;
    }
    return ctx->image_state;
}

/**
 * @brief Get opaque pointer to SSTV module state
 */
SlowframeSSTVState* slowframe_context_get_sstv_state(SlowframeContext *ctx) {
    if (!ctx) {
        return NULL;
    }
    return ctx->sstv_state;
}

/**
 * @brief Get opaque pointer to audio encoder state
 */
SlowframeAudioState* slowframe_context_get_audio_state(SlowframeContext *ctx) {
    if (!ctx) {
        return NULL;
    }
    return ctx->audio_state;
}
