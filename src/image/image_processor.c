/*
 * image_processor.c
 *
 * Image processing module for PiSSTVpp2
 * 
 * Implements basic image transformations (scaling, cropping, padding, color conversion)
 * using libvips for efficient in-memory operations. These operations build on the
 * image_loader module (which provides RGB-normalized images) to perform geometric
 * and photometric transformations.
 *
 * Color Space Operations:
 * - RGB normalization (sRGB, 3-band, 8-bit) via vips_colourspace
 *
 * Geometric Operations:
 * - Scaling/resizing (vips_resize with linear interpolation)
 * - Cropping (vips_crop to extract rectangular regions)
 * - Embedding/padding (vips_embed with black fill)
 *
 * All operations work on VipsImage objects for memory efficiency.
 * Final pixel access (for SSTV encoding) uses ImageBuffer for fast O(1) access.
 */

#include "../include/image/image_processor.h"
#include "../util/error.h"
#include "../include/logging.h"
#include "../include/overlay_spec.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
   COLOR SPACE OPERATIONS
   ============================================================================ */

int image_processor_to_rgb(VipsImage *image, VipsImage **out_rgb) {
    if (!image) {
        error_log(PISSTVPP2_ERR_ARG_INVALID, "Cannot convert NULL VipsImage to RGB");
        return PISSTVPP2_ERR_ARG_INVALID;
    }

    if (!out_rgb) {
        error_log(PISSTVPP2_ERR_ARG_INVALID, "Output VipsImage pointer is NULL");
        return PISSTVPP2_ERR_ARG_INVALID;
    }

    /* Ensure image is in sRGB format */
    VipsImage *rgb_image = NULL;
    
    if (vips_colourspace(image, &rgb_image, VIPS_INTERPRETATION_sRGB, NULL)) {
        error_log(PISSTVPP2_ERR_IMAGE_PROCESS,
                 "Failed to convert image colorspace to sRGB: %s", vips_error_buffer());
        vips_error_clear();
        return PISSTVPP2_ERR_IMAGE_PROCESS;
    }

    /* If conversion created a new image, use it; otherwise passthrough */
    if (rgb_image != image) {
        *out_rgb = rgb_image;
    } else {
        *out_rgb = image;
    }

    return PISSTVPP2_OK;
}

/* ============================================================================
   GEOMETRIC OPERATIONS: Scaling
   ============================================================================ */

int image_processor_scale(VipsImage *image, int new_width, int new_height,
                         VipsImage **out_scaled, int verbose) {
    if (!image) {
        error_log(PISSTVPP2_ERR_ARG_INVALID, "Cannot scale NULL VipsImage");
        return PISSTVPP2_ERR_ARG_INVALID;
    }

    if (!out_scaled) {
        error_log(PISSTVPP2_ERR_ARG_INVALID, "Output VipsImage pointer is NULL");
        return PISSTVPP2_ERR_ARG_INVALID;
    }

    if (new_width <= 0 || new_height <= 0) {
        error_log(PISSTVPP2_ERR_IMAGE_DIMENSIONS_INVALID,
                 "Invalid target dimensions for scaling: %dx%d", new_width, new_height);
        return PISSTVPP2_ERR_IMAGE_DIMENSIONS_INVALID;
    }

    /* Calculate scale factors for non-uniform resize */
    double scale_x = (double)new_width / image->Xsize;
    double scale_y = (double)new_height / image->Ysize;

    log_verbose(verbose, 0, "   --> Scaling %dx%d → %dx%d (scale: %.4f x, %.4f y)\n",
               image->Xsize, image->Ysize, new_width, new_height, scale_x, scale_y);

    VipsImage *resized = NULL;
    if (vips_resize(image, &resized, scale_x, "vscale", scale_y, NULL)) {
        error_log(PISSTVPP2_ERR_IMAGE_PROCESS,
                 "Image resize failed (%dx%d → %dx%d): %s",
                 image->Xsize, image->Ysize, new_width, new_height, vips_error_buffer());
        vips_error_clear();
        return PISSTVPP2_ERR_IMAGE_PROCESS;
    }

    *out_scaled = resized;
    log_verbose(verbose, 0, "   [OK] Scaled to %dx%d\n", resized->Xsize, resized->Ysize);

    return PISSTVPP2_OK;
}

/* ============================================================================
   GEOMETRIC OPERATIONS: Cropping
   ============================================================================ */

int image_processor_crop(VipsImage *image, int left, int top, int width, int height,
                        VipsImage **out_cropped, int verbose) {
    if (!image) {
        error_log(PISSTVPP2_ERR_ARG_INVALID, "Cannot crop NULL VipsImage");
        return PISSTVPP2_ERR_ARG_INVALID;
    }

    if (!out_cropped) {
        error_log(PISSTVPP2_ERR_ARG_INVALID, "Output VipsImage pointer is NULL");
        return PISSTVPP2_ERR_ARG_INVALID;
    }

    if (width <= 0 || height <= 0) {
        error_log(PISSTVPP2_ERR_IMAGE_DIMENSIONS_INVALID,
                 "Invalid crop dimensions: %dx%d", width, height);
        return PISSTVPP2_ERR_IMAGE_DIMENSIONS_INVALID;
    }

    /* Validate crop box is within image bounds */
    if (left < 0 || top < 0 || left + width > image->Xsize || top + height > image->Ysize) {
        error_log(PISSTVPP2_ERR_IMAGE_PROCESS,
                 "Crop box out of bounds: image %dx%d, crop at (%d,%d) size %dx%d",
                 image->Xsize, image->Ysize, left, top, width, height);
        return PISSTVPP2_ERR_IMAGE_PROCESS;
    }

    log_verbose(verbose, 0, "   --> Cropping at (%d,%d) size %dx%d from %dx%d image\n",
               left, top, width, height, image->Xsize, image->Ysize);

    VipsImage *cropped = NULL;
    if (vips_crop(image, &cropped, left, top, width, height, NULL)) {
        error_log(PISSTVPP2_ERR_IMAGE_PROCESS,
                 "Image crop failed at (%d,%d) size %dx%d: %s",
                 left, top, width, height, vips_error_buffer());
        vips_error_clear();
        return PISSTVPP2_ERR_IMAGE_PROCESS;
    }

    *out_cropped = cropped;
    log_verbose(verbose, 0, "   [OK] Cropped to %dx%d\n", cropped->Xsize, cropped->Ysize);

    return PISSTVPP2_OK;
}

/* ============================================================================
   GEOMETRIC OPERATIONS: Embedding/Padding
   ============================================================================ */

int image_processor_embed(VipsImage *image, int left, int top, int canvas_width, int canvas_height,
                         VipsImage **out_padded, int verbose) {
    if (!image) {
        error_log(PISSTVPP2_ERR_ARG_INVALID, "Cannot embed NULL VipsImage");
        return PISSTVPP2_ERR_ARG_INVALID;
    }

    if (!out_padded) {
        error_log(PISSTVPP2_ERR_ARG_INVALID, "Output VipsImage pointer is NULL");
        return PISSTVPP2_ERR_ARG_INVALID;
    }

    if (canvas_width <= 0 || canvas_height <= 0) {
        error_log(PISSTVPP2_ERR_IMAGE_DIMENSIONS_INVALID,
                 "Invalid canvas dimensions: %dx%d", canvas_width, canvas_height);
        return PISSTVPP2_ERR_IMAGE_DIMENSIONS_INVALID;
    }

    /* Validate image fits in canvas */
    if (left < 0 || top < 0 || left + image->Xsize > canvas_width || top + image->Ysize > canvas_height) {
        error_log(PISSTVPP2_ERR_IMAGE_PROCESS,
                 "Image does not fit in canvas: %dx%d image at (%d,%d) in %dx%d canvas",
                 image->Xsize, image->Ysize, left, top, canvas_width, canvas_height);
        return PISSTVPP2_ERR_IMAGE_PROCESS;
    }

    log_verbose(verbose, 0, "   --> Embedding %dx%d at offset (%d,%d) in %dx%d canvas\n",
               image->Xsize, image->Ysize, left, top, canvas_width, canvas_height);

    VipsImage *padded = NULL;
    if (vips_embed(image, &padded, left, top, canvas_width, canvas_height,
                   "extend", VIPS_EXTEND_BLACK, NULL)) {
        error_log(PISSTVPP2_ERR_IMAGE_PROCESS,
                 "Image embed failed to canvas %dx%d: %s",
                 canvas_width, canvas_height, vips_error_buffer());
        vips_error_clear();
        return PISSTVPP2_ERR_IMAGE_PROCESS;
    }

    *out_padded = padded;
    log_verbose(verbose, 0, "   [OK] Embedded to %dx%d with black padding\n",
               padded->Xsize, padded->Ysize);

    return PISSTVPP2_OK;
}

/* ============================================================================
   PIXEL ACCESS OPERATIONS
   ============================================================================ */

void image_processor_get_pixel_rgb(const ImageBuffer *buffer, int x, int y,
                                  uint8_t *r, uint8_t *g, uint8_t *b) {
    /* Default to black on error */
    *r = *g = *b = 0;

    if (!buffer || !buffer->data) {
        return;
    }

    /* Bounds check */
    if (x < 0 || x >= buffer->width || y < 0 || y >= buffer->height) {
        return;
    }

    /* Direct buffer access: RGB format, 3 bytes per pixel */
    int offset = (y * buffer->rowstride) + (x * 3);
    const uint8_t *pixel = &buffer->data[offset];

    *r = pixel[0];
    *g = pixel[1];
    *b = pixel[2];
}

/**
 * image_processor_get_dimensions
 * Extract width and height from ImageBuffer
 */
void image_processor_get_dimensions(const ImageBuffer *buffer, int *width, int *height) {
    if (!buffer) {
        if (width) *width = 0;
        if (height) *height = 0;
        return;
    }

    if (width) *width = buffer->width;
    if (height) *height = buffer->height;
}

/* ============================================================================
   INITIALIZATION
   ============================================================================ */

/**
 * image_processor_init
 * Initialize image processor (VIPS must be initialized first)
 * This is a no-op for now but reserved for future module setup
 */
void image_processor_init(void) {
    /* Currently no per-module initialization needed */
    /* All operations delegate to already-initialized VIPS */
}

/**
 * image_processor_shutdown
 * Clean up image processor resources
 * This is a no-op for now but reserved for future cleanup
 */
void image_processor_shutdown(void) {
    /* Currently no per-module cleanup needed */
    /* VIPS shutdown handled by image_loader_shutdown() */
}
