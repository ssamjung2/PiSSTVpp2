/*
 * image_processor.h
 *
 * Basic image processing operations for PiSSTVpp2
 * 
 * Provides color space conversion, scaling, cropping, padding, and pixel access
 * utilities built on top of libvips for efficient image transformation.
 *
 * Module Design:
 * - All operations work on VipsImage objects (memory-efficient)
 * - Operations are composable (output of one is input to next)
 * - Pixel access operates on ImageBuffer (from image_loader)
 * - All errors use standardized PISSTVPP2_ERR_* codes
 *
 * Typical Usage Flow:
 * 1. image_loader_load_image() → get RGB ImageBuffer + VipsImage
 * 2. image_processor_scale() → resize VipsImage to target size
 * 3. image_processor_crop() or image_processor_embed() → adjust aspect ratio
 * 4. image_loader_vips_to_buffer() → convert result back to ImageBuffer
 * 5. image_processor_get_pixel_rgb() → access pixels for SSTV encoding
 */

#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

#include <stdint.h>
#include <vips/vips.h>
#include "image_loader.h"

/**
 * image_processor_to_rgb
 * 
 * Ensure VipsImage is in RGB format (sRGB color space).
 * 
 * @param image VipsImage to convert
 * @param out_rgb Output: converted VipsImage (or same if already RGB)
 * 
 * @return PISSTVPP2_OK on success
 * Caller responsibility: g_object_unref(out_rgb) after use
 */
int image_processor_to_rgb(VipsImage *image, VipsImage **out_rgb);

/**
 * image_processor_scale
 * 
 * Scale image to new dimensions.
 * 
 * @param image Input VipsImage
 * @param new_width Target width in pixels
 * @param new_height Target height in pixels  
 * @param out_scaled Output: scaled VipsImage
 * @param verbose If non-zero, print progress
 * 
 * @return PISSTVPP2_OK on success, PISSTVPP2_ERR_IMAGE_PROCESS on failure
 * Caller responsibility: g_object_unref(out_scaled) after use
 */
int image_processor_scale(VipsImage *image, int new_width, int new_height,
                         VipsImage **out_scaled, int verbose);

/**
 * image_processor_crop
 * 
 * Extract rectangular region from image.
 * 
 * @param image Input VipsImage
 * @param left Left edge in pixels
 * @param top Top edge in pixels
 * @param width Crop width
 * @param height Crop height
 * @param out_cropped Output: cropped VipsImage
 * @param verbose If non-zero, print progress
 * 
 * @return PISSTVPP2_OK on success
 * Caller responsibility: g_object_unref(out_cropped) after use
 */
int image_processor_crop(VipsImage *image, int left, int top, int width, int height,
                        VipsImage **out_cropped, int verbose);

/**
 * image_processor_embed
 * 
 * Embed image in larger canvas with padding.
 * 
 * @param image Input VipsImage
 * @param left Left offset in canvas
 * @param top Top offset in canvas
 * @param canvas_width Total canvas width
 * @param canvas_height Total canvas height
 * @param out_padded Output: padded VipsImage
 * @param verbose If non-zero, print progress
 * 
 * @return PISSTVPP2_OK on success
 * Caller responsibility: g_object_unref(out_padded) after use
 */
int image_processor_embed(VipsImage *image, int left, int top, int canvas_width, int canvas_height,
                         VipsImage **out_padded, int verbose);

/**
 * image_processor_get_pixel_rgb
 * 
 * Read RGB values from ImageBuffer at (x,y).
 * Returns black (0,0,0) for out-of-bounds coordinates.
 * 
 * @param buffer ImageBuffer to sample
 * @param x X coordinate (0..width-1)
 * @param y Y coordinate (0..height-1)
 * @param r Output: red (0-255)
 * @param g Output: green (0-255)
 * @param b Output: blue (0-255)
 */
void image_processor_get_pixel_rgb(const ImageBuffer *buffer, int x, int y,
                                  uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * image_processor_get_dimensions
 * 
 * Get dimensions from ImageBuffer.
 * 
 * @param buffer ImageBuffer to query
 * @param width Output: width in pixels (may be NULL)
 * @param height Output: height in pixels (may be NULL)
 */
void image_processor_get_dimensions(const ImageBuffer *buffer, int *width, int *height);

/* ============================================================================
   INITIALIZATION (No-ops, reserved for future module setup)
   ============================================================================ */

/**
 * image_processor_init
 * 
 * Initialize image processor module.
 * Currently a no-op (VIPS is initialized by image_loader_init).
 * Reserved for future per-module setup or optimization.
 */
void image_processor_init(void);

/**
 * image_processor_shutdown
 * 
 * Clean up image processor resources.
 * Currently a no-op (VIPS cleanup handled by image_loader_shutdown).
 * Reserved for future per-module cleanup.
 */
void image_processor_shutdown(void);

#endif /* IMAGE_PROCESSOR_H */
