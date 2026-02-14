/*
 * image_text_overlay.h
 *
 * Text overlay and color bars for PiSSTVpp2
 * 
 * Adds station identification, grid squares, timestamps, and other
 * text overlays to SSTV images for FCC Part 97 Section 97.113(a)(4) compliance.
 * 
 * Supports:
 * - Flexible text placement (top, bottom, left, right, custom)
 * - Multiple text lines
 * - Color bars with text labels
 * - Opacity/transparency controls
 */

#ifndef IMAGE_TEXT_OVERLAY_H
#define IMAGE_TEXT_OVERLAY_H

#include <vips/vips.h>
#include <stdint.h>
#include "../../util/error.h"

/* Text placement options */
typedef enum {
    TEXT_PLACEMENT_TOP,
    TEXT_PLACEMENT_BOTTOM,
    TEXT_PLACEMENT_LEFT,
    TEXT_PLACEMENT_RIGHT,
    TEXT_PLACEMENT_CUSTOM
} TextPlacement;

/* Text overlay configuration */
typedef struct {
    const char *text;           /**< Text content to display */
    const char *font_family;    /**< Font name (e.g. "Arial", "Courier") */
    int font_size;              /**< Font size in points */
    
    /* Colors (RGB, 0-255) */
    uint8_t text_r, text_g, text_b;         /**< Text color */
    uint8_t bg_r, bg_g, bg_b;               /**< Background color */
    uint8_t bar_r, bar_g, bar_b;            /**< Bar (border) color */
    
    /* Placement */
    TextPlacement placement;
    int custom_x, custom_y;     /**< Custom coordinates (if placement=CUSTOM) */
    int bar_height;             /**< Height of background bar in pixels */
    int padding;                /**< Padding around text in pixels */
    float opacity;              /**< Opacity: 0.0 (transparent) to 1.0 (opaque) */
} TextOverlayConfig;

/**
 * image_text_overlay_create_config
 * 
 * Create default text overlay configuration.
 * 
 * @return Initialized TextOverlayConfig with sensible defaults
 */
TextOverlayConfig image_text_overlay_create_config(void);

/**
 * image_text_overlay_apply
 * 
 * Apply text overlay to image.
 * 
 * @param image Input VipsImage
 * @param config Text overlay configuration
 * @param out_overlaid Output: image with text overlay
 * @param verbose If non-zero, print progress
 * 
 * @return PISSTVPP2_OK on success, or error code:
 *   - PISSTVPP2_ERR_IMAGE_TEXT_OVERLAY: Text rendering failed
 *   - PISSTVPP2_ERR_MEMORY_ALLOC: Failed to allocate
 * 
 * Caller responsibility: g_object_unref(out_overlaid) after use
 */
int image_text_overlay_apply(VipsImage *image, const TextOverlayConfig *config,
                            VipsImage **out_overlaid, int verbose);

/**
 * image_text_overlay_add_color_bar
 * 
 * Add colored bar with optional text to image.
 * Better for simple use cases than full text overlay.
 * 
 * @param image Input VipsImage
 * @param placement Where to place the bar (top/bottom/left/right)
 * @param bar_height Height of bar in pixels
 * @param bar_r, bar_g, bar_b Bar color (RGB)
 * @param text Optional text to overlay on bar (NULL for no text)
 * @param text_r, text_g, text_b Text color (ignored if text=NULL)
 * @param out_barred Output: image with bar
 * @param verbose If non-zero, print progress
 * 
 * @return PISSTVPP2_OK on success
 * Caller responsibility: g_object_unref(out_barred) after use
 */
int image_text_overlay_add_color_bar(VipsImage *image, int placement, int bar_height,
                                    uint8_t bar_r, uint8_t bar_g, uint8_t bar_b,
                                    const char *text, uint8_t text_r, uint8_t text_g, uint8_t text_b,
                                    VipsImage **out_barred, int verbose);

/**
 * image_text_overlay_add_station_id
 * 
 * Add standard station identification overlay for FCC compliance.
 * Convenience function combining call sign and grid square.
 * 
 * Example: K0ABC/EN96 in bottom bar with white text on black background
 * 
 * @param image Input VipsImage
 * @param callsign Station call sign (e.g. "K0ABC")
 * @param grid_square Optional grid square (e.g. "EN96", NULL to omit)
 * @param out_labeled Output: image with station ID
 * @param verbose If non-zero, print progress
 * 
 * @return PISSTVPP2_OK on success
 * Caller responsibility: g_object_unref(out_labeled) after use
 */
int image_text_overlay_add_station_id(VipsImage *image, const char *callsign,
                                     const char *grid_square, VipsImage **out_labeled,
                                     int verbose);

#endif /* IMAGE_TEXT_OVERLAY_H */
