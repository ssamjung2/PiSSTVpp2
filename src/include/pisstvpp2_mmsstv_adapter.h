/*
 * pisstvpp2_mmsstv_adapter.h
 *
 * MMSSTV Library Integration Adapter for PiSSTVpp2
 *
 * This module provides the integration layer between PiSSTVpp2 and the
 * mmsstv-portable library. It handles:
 * - Mode translation between PiSSTVpp2 and MMSSTV formats
 * - Pixel access delegation to image module
 * - Audio buffer management and delegation
 * - Configuration parameter mapping
 *
 * Architecture:
 * - Implements callbacks required by MMSSTV library
 * - Provides unified interface for both legacy and MMSSTV modes
 * - Handles conditional compilation when MMSSTV is unavailable
 *
 * Build Configuration:
 * - Define HAVE_MMSSTV_SUPPORT to enable MMSSTV integration
 * - Without this define, only legacy 7 modes are available
 *
 * Author: Integration stub for mmsstv-portable
 * Date: January 29, 2026
 * Status: Development - Ready for mmsstv-portable completion
 */

#ifndef PISSTVPP2_MMSSTV_ADAPTER_H
#define PISSTVPP2_MMSSTV_ADAPTER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef HAVE_MMSSTV_SUPPORT
#include "mmsstv_stub.h"  /* Replace with actual mmsstv.h when available */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
   CONFIGURATION
   ============================================================================ */

/**
 * Maximum number of SSTV modes supported.
 * - Legacy: 7 modes (Martin 1/2, Scottie 1/2/DX, Robot 36/72)
 * - With MMSSTV: 100+ modes
 */
#ifdef HAVE_MMSSTV_SUPPORT
#define MMSSTV_ADAPTER_MAX_MODES    128
#else
#define MMSSTV_ADAPTER_MAX_MODES    7
#endif

/* ============================================================================
   MODE MAPPING STRUCTURE
   ============================================================================ */

/**
 * Maps PiSSTVpp2 protocol strings to mode identifiers.
 * Supports both legacy and MMSSTV modes.
 */
typedef struct {
    const char *protocol_str;        /**< Protocol string (e.g., "m1", "pd120") */
    uint8_t vis_code;                /**< VIS code identifier */
    const char *mode_name;           /**< Human-readable name */
    bool is_legacy;                  /**< True if handled by legacy code */
#ifdef HAVE_MMSSTV_SUPPORT
    mmsstv_mode_t mmsstv_mode;       /**< MMSSTV mode identifier */
#endif
} mmsstv_mode_map_t;

/* ============================================================================
   ADAPTER CONTEXT
   ============================================================================ */

/**
 * Opaque adapter context.
 * Manages state for MMSSTV encoding with PiSSTVpp2.
 */
typedef struct mmsstv_adapter_ctx mmsstv_adapter_ctx_t;

/* ============================================================================
   INITIALIZATION AND CLEANUP
   ============================================================================ */

/**
 * Initialize MMSSTV adapter.
 * Must be called before any MMSSTV operations.
 *
 * @param sample_rate Audio sample rate in Hz
 * @param verbose Enable verbose logging
 * @return 0 on success, negative on error
 */
int mmsstv_adapter_init(uint32_t sample_rate, bool verbose);

/**
 * Cleanup MMSSTV adapter and free resources.
 */
void mmsstv_adapter_cleanup(void);

/**
 * Check if MMSSTV support is available.
 *
 * @return True if compiled with MMSSTV support, false for legacy-only
 */
bool mmsstv_adapter_is_available(void);

/**
 * Get MMSSTV adapter version.
 *
 * @return Version string
 */
const char* mmsstv_adapter_version(void);

/* ============================================================================
   MODE DISCOVERY AND VALIDATION
   ============================================================================ */

/**
 * List all available SSTV modes.
 *
 * @param modes Array to receive mode map structures
 * @param max_modes Maximum number of modes to return
 * @return Number of modes returned, or -1 on error
 */
int mmsstv_adapter_list_modes(mmsstv_mode_map_t *modes, int max_modes);

/**
 * Find mode by protocol string.
 *
 * @param protocol Protocol string (e.g., "m1", "s2", "pd120")
 * @param mode_map Pointer to receive mode mapping information
 * @return 0 if found, -1 if not found
 */
int mmsstv_adapter_find_mode(const char *protocol, mmsstv_mode_map_t *mode_map);

/**
 * Validate if protocol string is supported.
 *
 * @param protocol Protocol string to check
 * @return True if supported, false otherwise
 */
bool mmsstv_adapter_is_mode_supported(const char *protocol);

/**
 * Get mode parameters (resolution, timing, etc.).
 *
 * @param protocol Protocol string
 * @param width Pointer to receive image width
 * @param height Pointer to receive image height
 * @param duration Pointer to receive encoding duration in seconds
 * @return 0 on success, -1 on error
 */
int mmsstv_adapter_get_mode_info(
    const char *protocol,
    uint16_t *width,
    uint16_t *height,
    double *duration
);

/* ============================================================================
   ENCODING INTERFACE
   ============================================================================ */

/**
 * Create encoder context for specified mode.
 *
 * @param protocol Protocol string (e.g., "m1", "pd120")
 * @param sample_rate Audio sample rate in Hz
 * @return Encoder context, or NULL on error
 */
mmsstv_adapter_ctx_t* mmsstv_adapter_create(
    const char *protocol,
    uint32_t sample_rate
);

/**
 * Destroy encoder context.
 *
 * @param ctx Encoder context
 */
void mmsstv_adapter_destroy(mmsstv_adapter_ctx_t *ctx);

/**
 * Configure adapter to use legacy SSTV module.
 * Only called for modes handled by pisstvpp2_sstv.c (m1, m2, s1, s2, sdx, r36, r72).
 *
 * @param ctx Encoder context
 * @return 0 on success, -1 on error
 */
int mmsstv_adapter_use_legacy(mmsstv_adapter_ctx_t *ctx);

/**
 * Configure adapter to use MMSSTV library.
 * Called for all modes when HAVE_MMSSTV_SUPPORT is defined.
 *
 * @param ctx Encoder context
 * @return 0 on success, -1 on error
 */
int mmsstv_adapter_use_mmsstv(mmsstv_adapter_ctx_t *ctx);

/**
 * Set VIS header generation.
 *
 * @param ctx Encoder context
 * @param enable True to include VIS header
 * @return 0 on success, -1 on error
 */
int mmsstv_adapter_set_vis_header(mmsstv_adapter_ctx_t *ctx, bool enable);

/**
 * Encode complete SSTV frame.
 * Routes to appropriate backend (legacy or MMSSTV).
 *
 * @param ctx Encoder context
 * @param audio_buffer Audio buffer to receive samples
 * @param max_samples Maximum buffer capacity
 * @param samples_written Pointer to receive number of samples written
 * @return 0 on success, negative on error
 */
int mmsstv_adapter_encode_frame(
    mmsstv_adapter_ctx_t *ctx,
    uint16_t *audio_buffer,
    uint32_t max_samples,
    uint32_t *samples_written
);

/**
 * Get estimated sample count for mode.
 * Useful for buffer pre-allocation.
 *
 * @param protocol Protocol string
 * @param sample_rate Audio sample rate in Hz
 * @return Estimated sample count, or 0 on error
 */
uint32_t mmsstv_adapter_estimate_samples(
    const char *protocol,
    uint32_t sample_rate
);

/* ============================================================================
   CALLBACK REGISTRATION (for MMSSTV backend)
   ============================================================================ */

/**
 * Internal pixel callback for MMSSTV library.
 * Delegates to PiSSTVpp2 image module.
 * DO NOT CALL DIRECTLY - Used internally by adapter.
 */
int mmsstv_adapter_pixel_callback(
    int x, int y,
    uint8_t *r, uint8_t *g, uint8_t *b,
    void *user_data
);

/**
 * Internal audio callback for MMSSTV library.
 * Writes to PiSSTVpp2 audio buffer.
 * DO NOT CALL DIRECTLY - Used internally by adapter.
 */
int mmsstv_adapter_audio_callback(
    const uint16_t *samples,
    uint32_t count,
    void *user_data
);

/* ============================================================================
   UTILITY FUNCTIONS
   ============================================================================ */

/**
 * Print mode capabilities and details.
 *
 * @param protocol Protocol string
 */
void mmsstv_adapter_print_mode_details(const char *protocol);

/**
 * Print list of all supported modes.
 *
 * @param include_legacy Include legacy modes in output
 * @param include_mmsstv Include MMSSTV modes in output
 */
void mmsstv_adapter_print_mode_list(bool include_legacy, bool include_mmsstv);

/**
 * Get last adapter error message.
 *
 * @return Error string, or NULL if no error
 */
const char* mmsstv_adapter_get_error(void);

#ifdef __cplusplus
}
#endif

#endif /* PISSTVPP2_MMSSTV_ADAPTER_H */
