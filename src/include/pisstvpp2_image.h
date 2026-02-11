/*
 * pisstvpp2_image.h
 * 
 * Image processing module for PiSSTVpp2
 * Handles loading, resizing, aspect ratio correction, and debug export
 * 
 * Architecture:
 * - image_load_from_file(): Load image from disk using libvips
 * - image_get_dimensions(): Query current image dimensions
 * - image_convert_aspect(): Resize/crop/pad image to target dimensions
 * - image_get_pixel_rgb(): Access individual pixels
 * - image_save_to_file(): Export intermediate images for debugging
 * - image_free(): Release all resources
 */

#ifndef PISSTVPP2_IMAGE_H
#define PISSTVPP2_IMAGE_H

#include <stdint.h>
#include <vips/vips.h>
#include "error.h"

/* Image buffer structure - stores decoded RGB pixel data */
typedef struct {
    uint8_t *data;      /**< Contiguous RGB pixel data (3 bytes per pixel) */
    int width;          /**< Width in pixels */
    int height;         /**< Height in pixels */
    int rowstride;      /**< Bytes per row (may include padding) */
} ImageBuffer;

/* Aspect ratio correction modes */
typedef enum {
    ASPECT_CENTER,     /**< Center-crop to exact target dimensions */
    ASPECT_PAD,        /**< Add black padding to reach target, preserve original aspect ratio */
    ASPECT_STRETCH     /**< Direct resize to target dimensions (may distort) */
} AspectMode;

/* ============================================================================
   IMAGE LIFECYCLE MANAGEMENT
   ============================================================================ */

/**
 * image_load_from_file
 * 
 * Load an image from disk using libvips and buffer pixel data.
 * 
 * @param filename Path to image file (PNG, JPEG, GIF, BMP, TIFF, WebP, etc.)
 * @param verbose If non-zero, print debug messages during loading
 * @param timestamp_logging If non-zero (and verbose is non-zero), add timestamps to output
 * @param debug_output_dir If non-NULL, save debug images to this directory
 * 
 * @return Error code: PISSTVPP2_OK on success, or one of:
 *   - PISSTVPP2_ERR_FILE_NOT_FOUND: File does not exist
 *   - PISSTVPP2_ERR_IMAGE_LOAD: libvips failed to load the image
 *   - PISSTVPP2_ERR_IMAGE_FORMAT_UNSUPPORTED: Format not recognized by libvips
 *   - PISSTVPP2_ERR_MEMORY_ALLOC: Failed to allocate buffer for pixel data
 * 
 * Effects:
 * - Loads image from disk using libvips auto-detect format
 * - Converts to RGB color space (3-band, 8-bit per channel)
 * - Allocates and fills internal pixel buffer
 * - On failure, cleans up allocated resources
 * - On success with debug_output_dir, saves loaded image to disk
 */
int image_load_from_file(const char *filename, int verbose, int timestamp_logging, const char *debug_output_dir);

/**
 * image_get_dimensions
 * 
 * Query the current image dimensions.
 * 
 * @param width Output: pointer to receive width in pixels
 * @param height Output: pointer to receive height in pixels
 * 
 * @return PISSTVPP2_OK if image is loaded, PISSTVPP2_ERR_IMAGE_LOAD if no image
 */
int image_get_dimensions(int *width, int *height);

/**
 * image_get_pixel_rgb
 * 
 * Read RGB pixel values from loaded image at (x, y).
 * 
 * @param x X coordinate (0..width-1)
 * @param y Y coordinate (0..height-1)
 * @param r Output: red channel (0-255)
 * @param g Output: green channel (0-255)
 * @param b Output: blue channel (0-255)
 * 
 * Effects:
 * - If coordinates out of bounds, returns (0,0,0)
 * - Performs direct buffer access (O(1) lookup)
 */
void image_get_pixel_rgb(int x, int y, uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * image_get_original_extension
 * 
 * Get the file extension of the originally loaded image.
 * 
 * @return Pointer to extension string (including the dot, e.g. ".png"), 
 *         or empty string "" if no extension or no image loaded
 */
const char *image_get_original_extension(void);

/**
 * image_free
 * 
 * Release all allocated image resources.
 * 
 * Effects:
 * - Frees pixel buffer
 * - Unreferences VipsImage objects
 * - Resets internal state
 * - Safe to call multiple times
 */
void image_free(void);

/* ============================================================================
   IMAGE TRANSFORMATION
   ============================================================================ */

/**
 * image_correct_aspect_and_resize
 * 
 * Resize and/or adjust image to match target dimensions and aspect ratio.
 * 
 * @param target_width Desired output width in pixels
 * @param target_height Desired output height in pixels
 * @param mode Which correction algorithm to use (CROP, PAD, or STRETCH)
 * @param verbose If non-zero, print diagnostic messages
 * @param timestamp_logging If non-zero (and verbose is non-zero), add timestamps to output
 * @param debug_output_dir If non-NULL, save intermediate/final images to this directory
 * 
 * @return Error code: PISSTVPP2_OK on success, or one of:
 *   - PISSTVPP2_ERR_IMAGE_LOAD: No image loaded
 *   - PISSTVPP2_ERR_IMAGE_PROCESS: libvips transformation failed
 *   - PISSTVPP2_ERR_IMAGE_ASPECT_CORRECTION: Aspect correction failed
 *   - PISSTVPP2_ERR_MEMORY_ALLOC: Failed to allocate for transformed image
 * 
 * Algorithm:
 * 
 * ASPECT_CENTER (center-crop to exact target):
 * 1. Calculate centered crop box matching target aspect ratio and dimensions
 * 2. Extract center region from source image
 * 3. Result is exact target dimensions
 * Preserves detail in center but discards edges
 * Best for images that are already close to target size
 * 
 * ASPECT_PAD (add padding to reach target):
 * 1. Add black padding (vertical, horizontal, or both) around original image
 * 2. Center the original image within the padded area
 * 3. Maintain original image's aspect ratio
 * Preserves all original content visible
 * Best for images smaller than target resolution
 * 
 * ASPECT_STRETCH (direct resize, may distort):
 * 1. Directly resize image to target dimensions
 * 2. If source is smaller: upscale and stretch to target aspect ratio
 * 3. If source is larger: downscale and compress to target aspect ratio
 * Image may be distorted to fit target aspect ratio
 * Useful when any scaling is acceptable
 * 
 * Effects:
 * - Modifies internal image state
 * - Rebuffers pixels after transformation
 * - Old image state is released
 * - On failure with partial state, attempts cleanup
 * - If debug_output_dir provided, saves corrected image before buffering
 * 
 * Error handling:
 * - Uses error_log for detailed error reporting
 * - Cleans up intermediate images on failure
 * - Returns error code for all failure cases
 */
int image_correct_aspect_and_resize(int target_width, int target_height, AspectMode mode, int verbose, int timestamp_logging, const char *debug_output_dir);

/* ============================================================================
   DEBUG AND INSPECTION
   ============================================================================ */

/**
 * image_save_to_file
 * 
 * Export current image to PNG file for inspection/debugging.
 * 
 * @param output_path Full path to output file (e.g., "/tmp/debug_001.png")
 * @param verbose If non-zero, print confirmation message
 * 
 * @return Error code: PISSTVPP2_OK on success, or:
 *   - PISSTVPP2_ERR_FILE_WRITE: Failed to write output file
 *   - PISSTVPP2_ERR_IMAGE_PROCESS: libvips save operation failed
 * 
 * Use cases:
 * - Save loaded image before transformation for comparison
 * - Save cropped intermediate for verification
 * - Save padded intermediate for verification
 * - Save final image before SSTV encoding
 * 
 * Effects:
 * - Writes PNG file to specified path
 * - Creates file with full precision (no quality loss)
 * - Overwrites existing file
 * - Logs errors on failure
 */
int image_save_to_file(const char *output_path, int verbose);

/**
 * image_print_diagnostics
 * 
 * Print diagnostic information about current image state.
 * 
 * Effects:
 * - Prints to stdout
 * - Shows dimensions, aspect ratio, color space
 * - Useful for debugging and understanding processing steps
 */
void image_print_diagnostics(void);

/* ============================================================================
   HELPER UTILITIES
   ============================================================================ */

/**
 * aspect_mode_to_string
 * 
 * Convert AspectMode enum to human-readable string.
 * 
 * @param mode The AspectMode value
 * @return Pointer to static string ("CENTER", "PAD", "STRETCH", or "UNKNOWN")
 */
const char* aspect_mode_to_string(AspectMode mode);

/**
 * image_calculate_crop_box
 * 
 * Calculate centered crop box to match target aspect ratio.
 * 
 * @param src_width Current image width
 * @param src_height Current image height
 * @param target_aspect Target width/height ratio
 * @param out_left Output: left edge of crop box
 * @param out_top Output: top edge of crop box
 * @param out_width Output: width of crop box
 * @param out_height Output: height of crop box
 * 
 * Effects:
 * - Calculates largest centered rectangle with target aspect ratio
 * - If source is too narrow, crop top/bottom
 * - If source is too wide, crop left/right
 * - Crop box is always centered
 */
void image_calculate_crop_box(int src_width, int src_height, double target_aspect, 
                              int *out_left, int *out_top, int *out_width, int *out_height);

#endif /* PISSTVPP2_IMAGE_H */
