/*
 * image_aspect.h
 *
 * Image aspect ratio correction for PiSSTVpp2
 * 
 * Provides three algorithms for adapting images to target dimensions:
 * - CENTER: Crop from center, discards edges
 * - PAD: Add black padding while preserving aspect
 * - STRETCH: Non-uniform resize (may distort)
 */

#ifndef IMAGE_ASPECT_H
#define IMAGE_ASPECT_H

#include <vips/vips.h>
#include <string.h>
#include "../../util/error.h"
#include "image_processor.h"

/* Aspect ratio correction modes */
typedef enum {
    ASPECT_CENTER,     /**< Center-crop to exact target dimensions */
    ASPECT_PAD,        /**< Add black padding to reach target, preserve original aspect */
    ASPECT_STRETCH     /**< Direct resize to target (may distort) */
} AspectMode;

/**
 * image_aspect_correct
 * 
 * Resize and/or adjust image to match target dimensions and aspect ratio.
 * 
 * @param image Input VipsImage (preserved if no correction needed)
 * @param target_width Desired output width in pixels
 * @param target_height Desired output height in pixels
 * @param mode Aspect correction algorithm to use
 * @param out_corrected Output: transformed VipsImage (may differ from input)
 * @param verbose If non-zero, print diagnostic messages
 * @param timestamp_logging If non-zero, add timestamps to messages
 * 
 * @return PISSTVPP2_OK on success, or error code:
 *   - PISSTVPP2_ERR_ARG_INVALID: Invalid arguments
 *   - PISSTVPP2_ERR_IMAGE_DIMENSIONS_INVALID: Invalid target dimensions
 *   - PISSTVPP2_ERR_ARG_ASPECT_MODE_INVALID: Unknown aspect mode
 *   - PISSTVPP2_ERR_IMAGE_PROCESS: Transformation failed
 * 
 * Algorithms:
 * - ASPECT_CENTER:
 *   1. Calculate centered crop box matching target aspect ratio
 *   2. Crop image to that box
 *   3. Resize cropped region to exact target dimensions
 *   → Maximizes detail, may lose edge content
 *   
 * - ASPECT_PAD:
 *   1. Keep original image at original size/aspect
 *   2. Center image in larger canvas
 *   3. Add black padding to reach target dimensions
 *   → No content loss, but black bars appear
 *   
 * - ASPECT_STRETCH:
 *   1. Directly resize to exact target dimensions
 *   2. No aspect ratio preservation
 *   → May distort if aspect ratios differ
 * 
 * Memory: Returns new VipsImage on success. If mode required transformation,
 * caller must g_object_unref(out_corrected) after use.
 * If no transformation needed, out_corrected == image (same object).
 */
int image_aspect_correct(VipsImage *image, int target_width, int target_height,
                        AspectMode mode, VipsImage **out_corrected,
                        int verbose, int timestamp_logging);

/**
 * image_aspect_get_name
 * 
 * Get human-readable name for aspect mode.
 * 
 * @param mode Aspect mode
 * @return String like "CENTER", "PAD", or "STRETCH"
 */
const char *image_aspect_get_name(AspectMode mode);

/**
 * image_aspect_parse
 * 
 * Parse aspect mode from string.
 * 
 * @param mode_str String like "center", "pad", "stretch" (case-insensitive)
 * @param out_mode Output: parsed AspectMode
 * 
 * @return PISSTVPP2_OK on success, PISSTVPP2_ERR_ARG_ASPECT_MODE_INVALID on unknown string
 */
int image_aspect_parse(const char *mode_str, AspectMode *out_mode);

#endif /* IMAGE_ASPECT_H */
