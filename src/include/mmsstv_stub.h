/*
 * mmsstv_stub.h
 *
 * Stub interface for mmsstv-portable library integration
 *
 * This header defines the expected interface for the mmsstv-portable library
 * (encoder-only extraction from MMSSTV) currently under development.
 *
 * Based on: MMSSTV to Encoder-Only Library Porting Plan (Jan 28, 2026)
 * Matches: sstv_encoder.h API design from mmsstv-portable
 *
 * Once the mmsstv-portable library is complete, this stub will be replaced
 * with the actual sstv_encoder.h from that project.
 *
 * Key Design Principles:
 * - Encoder-only (no decoder/RX functionality)
 * - Streaming API (generate samples incrementally)
 * - Float sample output (-1.0 to +1.0 range)
 * - Simple image structure (direct pixel buffer)
 * - 43 SSTV modes from MMSSTV
 *
 * Author: Stub interface matching mmsstv-portable design
 * Date: January 29, 2026
 * Status: STUB - Replace with actual sstv_encoder.h when library is complete
 */

#ifndef MMSSTV_STUB_H
#define MMSSTV_STUB_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
   VERSION INFORMATION
   ============================================================================ */

#define SSTV_ENCODER_VERSION "1.0.0"

/* ============================================================================
   SSTV MODE ENUMERATION (43 Modes from MMSSTV)
   ============================================================================ */

/**
 * SSTV mode enumeration - all 43 modes from MMSSTV.
 * Matches the mode ordering from mmsstv-portable porting plan.
 */
typedef enum {
    /* Core modes (most common) */
    SSTV_ROBOT36 = 0,
    SSTV_ROBOT72,
    SSTV_SCOTTIE1,
    SSTV_SCOTTIE2,
    SSTV_SCOTTIEX,       /**< Scottie DX */
    SSTV_MARTIN1,
    SSTV_MARTIN2,
    
    /* PD modes (high resolution) */
    SSTV_PD50,
    SSTV_PD90,
    SSTV_PD120,
    SSTV_PD160,
    SSTV_PD180,
    SSTV_PD240,
    SSTV_PD290,
    
    /* Extended Robot modes */
    SSTV_ROBOT24,
    SSTV_ROBOT12,
    SSTV_ROBOT8BW,
    
    /* AVT modes */
    SSTV_AVT90,
    SSTV_AVT94,
    SSTV_AVT24,
    
    /* SC2 Wraase modes */
    SSTV_SC2_60,
    SSTV_SC2_120,
    SSTV_SC2_180,
    
    /* Pasokon modes */
    SSTV_PASOKON3,
    SSTV_PASOKON5,
    SSTV_PASOKON7,
    
    /* Martin R/P/L modes */
    SSTV_MARTINR,
    SSTV_MARTINP,
    SSTV_MARTINL,
    
    /* Narrow modes */
    SSTV_MN73,
    SSTV_MN110,
    SSTV_MN140,
    SSTV_MC110,
    SSTV_MC140,
    SSTV_MC180,
    
    /* Additional modes */
    SSTV_SCOTTIE3,
    SSTV_SCOTTIE4,
    SSTV_MARTIN3,
    SSTV_MARTIN4,
    
    /* FAX modes */
    SSTV_FAX480,
    SSTV_FAX240,
    
    /* Mode count */
    SSTV_MODE_COUNT
} sstv_mode_t;

/* ============================================================================
   PIXEL FORMAT AND IMAGE STRUCTURE
   ============================================================================ */

/**
 * Pixel format for image data.
 */
typedef enum {
    SSTV_RGB24,        /**< 24-bit RGB (R, G, B bytes) */
    SSTV_GRAY8         /**< 8-bit grayscale */
} sstv_pixel_format_t;

/**
 * Image structure for encoder input.
 * Points to a pixel buffer that must remain valid during encoding.
 */
typedef struct {
    uint8_t *pixels;              /**< Image pixel data (not owned by encoder) */
    uint32_t width;               /**< Image width in pixels */
    uint32_t height;              /**< Image height in pixels */
    uint32_t stride;              /**< Bytes per row (usually width * bpp) */
    sstv_pixel_format_t format;   /**< Pixel format */
} sstv_image_t;

/* ============================================================================
   MODE INFORMATION STRUCTURE
   ============================================================================ */

/**
 * Mode information structure.
 * Provides details about a specific SSTV mode.
 */
typedef struct {
    sstv_mode_t mode;             /**< Mode identifier */
    const char *name;             /**< Human-readable mode name */
    uint32_t width;               /**< Image width in pixels */
    uint32_t height;              /**< Image height in pixels */
    uint8_t vis_code;             /**< VIS code byte */
    double duration_sec;          /**< Total transmission time in seconds */
    int is_color;                 /**< 1=color, 0=grayscale */
} sstv_mode_info_t;

/* ============================================================================
   ENCODER CONTEXT (Opaque Handle)
   ============================================================================ */

/**
 * Opaque encoder context.
 * Maintains state during SSTV encoding process.
 */
typedef struct sstv_encoder_s sstv_encoder_t;

/* ============================================================================
   CORE ENCODER API
   ============================================================================ */

/**
 * Create an encoder for the specified mode.
 *
 * @param mode SSTV mode to encode
 * @param sample_rate Audio sample rate (typically 8000, 11025, 22050, 48000)
 * @return Encoder handle or NULL on error
 */
sstv_encoder_t* sstv_encoder_create(sstv_mode_t mode, double sample_rate);

/**
 * Free encoder resources.
 *
 * @param encoder Encoder handle
 */
void sstv_encoder_free(sstv_encoder_t *encoder);

/**
 * Set the source image to encode.
 * Image must remain valid until encoding is complete.
 *
 * @param encoder Encoder handle
 * @param image Source image
 * @return 0 on success, -1 on error
 */
int sstv_encoder_set_image(sstv_encoder_t *encoder, const sstv_image_t *image);

/**
 * Enable/disable VIS code transmission.
 * VIS code is transmitted at the start to identify the mode.
 *
 * @param encoder Encoder handle
 * @param enable 1 to enable, 0 to disable
 */
void sstv_encoder_set_vis_enabled(sstv_encoder_t *encoder, int enable);

/**
 * Generate audio samples.
 * Call repeatedly until sstv_encoder_is_complete() returns 1.
 *
 * Output samples are in float format, range -1.0 to +1.0.
 *
 * @param encoder Encoder handle
 * @param samples Output buffer for float samples
 * @param max_samples Maximum samples to generate
 * @return Number of samples generated (0 means complete)
 */
size_t sstv_encoder_generate(
    sstv_encoder_t *encoder,
    float *samples,
    size_t max_samples
);

/**
 * Check if encoding is complete.
 *
 * @param encoder Encoder handle
 * @return 1 if complete, 0 if more samples to generate
 */
int sstv_encoder_is_complete(sstv_encoder_t *encoder);

/**
 * Get encoding progress (0.0 to 1.0).
 *
 * @param encoder Encoder handle
 * @return Progress fraction
 */
float sstv_encoder_get_progress(sstv_encoder_t *encoder);

/**
 * Reset encoder to start.
 * Allows re-encoding same or different image without recreating encoder.
 *
 * @param encoder Encoder handle
 */
void sstv_encoder_reset(sstv_encoder_t *encoder);

/**
 * Get total number of samples that will be generated.
 * Useful for pre-allocating buffers.
 *
 * @param encoder Encoder handle
 * @return Total sample count
 */
size_t sstv_encoder_get_total_samples(sstv_encoder_t *encoder);

/* ============================================================================
   MODE INFORMATION API
   ============================================================================ */

/**
 * Get information about a specific mode.
 *
 * @param mode SSTV mode
 * @return Mode info or NULL if invalid mode
 */
const sstv_mode_info_t* sstv_get_mode_info(sstv_mode_t mode);

/**
 * Get list of all available modes.
 *
 * @param count Output: number of modes
 * @return Array of mode info structures
 */
const sstv_mode_info_t* sstv_get_all_modes(size_t *count);

/**
 * Find mode by name (case-insensitive).
 *
 * @param name Mode name (e.g., "scottie 1", "Martin2")
 * @return Mode enum or -1 if not found
 */
sstv_mode_t sstv_find_mode_by_name(const char *name);

/**
 * Calculate required image dimensions for a mode.
 *
 * @param mode SSTV mode
 * @param width Pointer to receive image width
 * @param height Pointer to receive image height
 * @return 0 on success, -1 if mode is invalid
 */
int sstv_get_mode_dimensions(
    sstv_mode_t mode,
    uint32_t *width,
    uint32_t *height
);

/* ============================================================================
   UTILITY API
   ============================================================================ */

/**
 * Get library version string.
 *
 * @return Version string (e.g., "1.0.0")
 */
const char* sstv_encoder_version(void);

/**
 * Helper: Create image structure from RGB buffer.
 * Note: Does NOT copy data - caller must keep buffer valid.
 *
 * @param rgb_data RGB pixel data (R,G,B bytes)
 * @param width Image width
 * @param height Image height
 * @return Image structure
 */
sstv_image_t sstv_image_from_rgb(
    uint8_t *rgb_data,
    uint32_t width,
    uint32_t height
);

/**
 * Helper: Create image structure from grayscale buffer.
 *
 * @param gray_data Grayscale pixel data (1 byte per pixel)
 * @param width Image width
 * @param height Image height
 * @return Image structure
 */
sstv_image_t sstv_image_from_gray(
    uint8_t *gray_data,
    uint32_t width,
    uint32_t height
);

#ifdef __cplusplus
}
#endif

#endif /* MMSSTV_STUB_H */
