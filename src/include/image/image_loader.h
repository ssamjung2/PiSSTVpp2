/*
 * image_loader.h
 *
 * Image file loading and format detection for PiSSTVpp2
 * 
 * Handles loading images from disk using libvips with support for:
 * - PNG, JPEG, GIF, BMP, TIFF, WebP, PPM, and other libvips-supported formats
 * - Automatic format detection and validation
 * - RGB colorspace conversion
 * - File existence checking
 * - Detailed error reporting with context
 * - Memory management with bounds checking
 * 
 * Memory Footprint:
 * - Image buffer size: width * height * 3 bytes (RGB 8-bit per channel)
 * - Examples: 320x240 = 230 KB, 640x480 = 920 KB, 1600x1200 = 5.8 MB
 * - Tested up to 19200x10800 (311 MB) on systems with sufficient RAM
 * 
 * Format Support:
 * PNG, JPEG, GIF, BMP, TIFF, WebP, PPM/PGM, IFF, FITS, OpenEXR, ICO, CUR, SVG, PDF
 * (Complete list: whatever libvips can load on the current platform)
 */

#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include <stdint.h>
#include <vips/vips.h>

/* Image buffer structure - stores decoded RGB pixel data */
typedef struct {
    uint8_t *data;      /**< Contiguous RGB pixel data (3 bytes per pixel) */
    int width;          /**< Width in pixels */
    int height;         /**< Height in pixels */
    int rowstride;      /**< Bytes per row (may include padding) */
} ImageBuffer;

/* ============================================================================
   PRIMARY FUNCTIONS: Image Loading and Buffering
   ============================================================================ */

/**
 * image_loader_load_image
 * 
 * Load an image file from disk using libvips with automatic format detection.
 * Converts all images to sRGB format for consistent downstream processing.
 * 
 * Supports: PNG, JPEG, GIF, BMP, TIFF, WebP, PPM, and more
 * 
 * @param filename Path to image file (may be absolute or relative)
 * @param verbose If non-zero, print progress and diagnostic messages
 * @param timestamp_logging If non-zero (and verbose), add timestamps to output
 * @param debug_output_dir If non-NULL, save loaded image to this directory
 * 
 * @return Error code:
 *   - PISSTVPP2_OK: Successfully loaded
 *   - PISSTVPP2_ERR_ARG_FILENAME_INVALID: filename is NULL
 *   - PISSTVPP2_ERR_FILE_NOT_FOUND: File does not exist or is not readable
 *   - PISSTVPP2_ERR_IMAGE_LOAD: libvips failed to load (format issue, corrupt, etc.)
 *   - PISSTVPP2_ERR_IMAGE_DIMENSIONS_INVALID: Width or height is zero/negative
 *   - PISSTVPP2_ERR_IMAGE_PROCESS: Colorspace conversion or buffering failed
 *   - PISSTVPP2_ERR_MEMORY_ALLOC: Failed to allocate space for pixel data
 * 
 * Effects:
 * - Loads image internally in global state
 * - Converts to RGB (3-band, 8-bit per channel)
 * - Allocates buffer for pixel data
 * - On failure, cleans up automatically
 * - On success with debug_output_dir, saves intermediate image
 * 
 * Notes:
 * - For very large images (>500MB), ensure system has sufficient RAM
 * - VIPS may use memory-mapped files for very large images
 * - Image orientation is NOT automatically corrected (EXIF ignored)
 */
int image_loader_load_image(const char *filename, int verbose, int timestamp_logging,
                           const char *debug_output_dir);

/**
 * image_loader_get_buffer
 * 
 * Access the currently loaded image buffer (read-only).
 * 
 * @return Pointer to ImageBuffer if image is loaded, NULL otherwise
 * 
 * Note: Returned buffer is valid only until next load or shutdown
 */
const ImageBuffer *image_loader_get_buffer(void);

/**
 * image_loader_free_buffer
 * 
 * Release the currently loaded image and its buffer memory.
 * 
 * Effects:
 * - Unref internal VipsImage
 * - Free allocated pixel data buffer
 * - Reset loader state
 * 
 * Safe to call multiple times (idempotent)
 */
void image_loader_free_buffer(void);

/* ============================================================================
   QUERY FUNCTIONS: Image Properties
   ============================================================================ */

/**
 * image_loader_get_dimensions
 * 
 * Get width and height of currently loaded image.
 * 
 * @param width Output: pointer to receive width in pixels (may be NULL)
 * @param height Output: pointer to receive height in pixels (may be NULL)
 * 
 * @return PISSTVPP2_OK if image is loaded, PISSTVPP2_ERR_IMAGE_LOAD otherwise
 */
int image_loader_get_dimensions(int *width, int *height);

/**
 * image_loader_get_pixel_rgb
 * 
 * Read RGB color values from a pixel in the loaded image.
 * 
 * @param x X coordinate (0..width-1)
 * @param y Y coordinate (0..height-1)
 * @param r Output: red channel (0-255)
 * @param g Output: green channel (0-255)
 * @param b Output: blue channel (0-255)
 * 
 * Effects:
 * - If coordinates out of bounds, sets RGB to (0,0,0) (black)
 * - If no image loaded, sets RGB to (0,0,0)
 * - Direct buffer access (O(1) constant time)
 * 
 * Note: Very efficient for individual pixel access
 */
void image_loader_get_pixel_rgb(int x, int y, uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * image_loader_get_original_extension
 * 
 * Get the file extension of the originally loaded image.
 * 
 * @return Pointer to extension string including dot (e.g., ".png", ".jpg")
 *         or empty string "" if no image loaded or no extension
 */
const char *image_loader_get_original_extension(void);

/* ============================================================================
   ADVANCED FUNCTIONS: Direct VIPS Access
   ============================================================================ */

/**
 * image_loader_get_vips_image
 * 
 * Access the internal VipsImage object directly (advanced use only).
 * 
 * @return Pointer to internal VipsImage, or NULL if no image loaded
 * 
 * Responsibility:
 * - Do NOT call g_object_unref() on returned pointer
 * - Image remains owned by loader module
 * - Image becomes invalid after next load or free_buffer call
 * 
 * Use case: When VIPS operations not exposed by other functions are needed
 */
VipsImage *image_loader_get_vips_image(void);

/**
 * image_loader_vips_to_buffer
 * 
 * Convert a VipsImage to RGB pixel buffer (low-level helper).
 * 
 * @param image Valid VipsImage pointer (ownership NOT transferred)
 * @param verbose If non-zero, print diagnostic messages
 * @param timestamp_logging If non-zero (and verbose), add timestamps
 * 
 * @return Allocated ImageBuffer on success, NULL on failure
 *         (Error details logged via error_log)
 * 
 * Responsibility:
 * - Caller must free returned buffer->data with free()
 * - Caller must free returned buffer pointer with free()
 * - Input image is NOT modified or freed
 * 
 * Use case: Converting VIPS images created by other processing modules
 */
ImageBuffer *image_loader_vips_to_buffer(VipsImage *image, int verbose, int timestamp_logging);

/* ============================================================================
   UTILITY FUNCTIONS: Diagnostics
   ============================================================================ */

/**
 * image_loader_print_diagnostics
 * 
 * Print detailed information about currently loaded image to stdout.
 * 
 * Output: filename, dimensions, aspect ratio, memory usage
 */
void image_loader_print_diagnostics(void);

/* ============================================================================
   INITIALIZATION (REQUIRED - Call Once at Startup)
   ============================================================================ */

/**
 * image_loader_init
 * 
 * Initialize the VIPS image processing library.
 * MUST be called once before any image loading operations.
 * 
 * Effects:
 * - Initializes GLib type system
 * - Sets up VIPS plugin system
 * - Enables format auto-detection
 * - Initializes memory management
 * 
 * Typical usage: Call in main() before first image operation
 * 
 * Note: Will exit with error message if initialization fails
 */
void image_loader_init(void);

/**
 * image_loader_shutdown
 * 
 * Clean up VIPS library resources and close image files.
 * Should be called once at program exit.
 * 
 * Effects:
 * - Frees all loaded images and buffers
 * - Closes file handles
 * - Shuts down VIPS plugin system
 * - Releases GLib memory
 * 
 * Safe to call even if no images are loaded (idempotent)
 */
void image_loader_shutdown(void);

#endif /* IMAGE_LOADER_H */
