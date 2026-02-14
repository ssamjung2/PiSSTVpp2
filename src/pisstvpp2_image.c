/*
 * pisstvpp2_image.c
 * 
 * Image processing module for PiSSTVpp2
 * Implements image loading, resizing, aspect ratio correction, and debug export
 */

#include "pisstvpp2_image.h"
#include "error.h"
#include "logging.h"
#include "overlay_spec.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <time.h>

/* ============================================================================
   INTERNAL STATE
   ============================================================================ */

/**
 * ImageState
 * Encapsulates all module-level image state
 */
typedef struct {
    VipsImage *image;                  /* Current image in memory */
    ImageBuffer *buffer;                /* Buffered pixel data */
    char original_filename[1024];      /* Original image filename (to extract extension) */
} ImageState;

static ImageState g_img = {
    .image = NULL,
    .buffer = NULL,
    .original_filename = {0}
};

/* Aspect ratio tolerance for "already correct" check */
#define ASPECT_TOLERANCE 0.001

/* ============================================================================
   INTERNAL HELPERS
   ============================================================================ */

/**
 * get_file_extension
 * Extract file extension from a filename path
 * Returns pointer to extension (including the dot), or empty string if no extension
 */
static const char *get_file_extension(const char *filename) {
    if (!filename) return "";
    
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    
    return dot;
}

/**
 * clear_image_state
 * Release VipsImage and buffer, reset to clean state
 */
static void clear_image_state(void) {
    if (g_img.image) {
        g_object_unref(g_img.image);
        g_img.image = NULL;
    }
    if (g_img.buffer) {
        if (g_img.buffer->data) {
            free(g_img.buffer->data);
        }
        free(g_img.buffer);
        g_img.buffer = NULL;
    }
}

/**
 * buffer_vips_image
 * Convert a VipsImage to internal pixel buffer
 * Returns PISSTVPP2_OK on success, error code on failure
 */
static int buffer_vips_image(VipsImage *image, int verbose, int timestamp_logging) {
    if (!image) {
        error_log(PISSTVPP2_ERR_IMAGE_LOAD, "No image to buffer");
        return PISSTVPP2_ERR_IMAGE_LOAD;
    }

    /* Ensure image is in RGB format */
    VipsImage *rgb_image = NULL;
    if (vips_colourspace(image, &rgb_image, VIPS_INTERPRETATION_sRGB, NULL)) {
        error_log(PISSTVPP2_ERR_IMAGE_PROCESS, "Colorspace conversion failed: %s", vips_error_buffer());
        vips_error_clear();
        return PISSTVPP2_ERR_IMAGE_PROCESS;
    }

    /* Replace with RGB version if conversion needed */
    if (rgb_image != image) {
        g_object_unref(image);
        image = rgb_image;
    }

    /* Allocate buffer structure */
    ImageBuffer *buf = (ImageBuffer *)malloc(sizeof(ImageBuffer));
    if (!buf) {
        error_log(PISSTVPP2_ERR_MEMORY_ALLOC, "Failed to allocate ImageBuffer structure");
        g_object_unref(image);
        return PISSTVPP2_ERR_MEMORY_ALLOC;
    }

    /* Get metadata */
    buf->width = image->Xsize;
    buf->height = image->Ysize;
    buf->rowstride = VIPS_IMAGE_SIZEOF_LINE(image);

    /* Allocate pixel data */
    int data_size = buf->height * buf->rowstride;
    buf->data = (uint8_t *)malloc(data_size);
    if (!buf->data) {
        error_log(PISSTVPP2_ERR_MEMORY_ALLOC, "Failed to allocate %d bytes for pixel data (image %dx%d, %d bytes/row)", 
                data_size, buf->width, buf->height, buf->rowstride);
        free(buf);
        g_object_unref(image);
        return PISSTVPP2_ERR_MEMORY_ALLOC;
    }

    log_verbose(verbose, timestamp_logging, "   --> Buffering %dx%d RGB image (%d bytes)...\n", buf->width, buf->height, data_size);

    /* Prepare image region and copy data */
    VipsRegion *region = vips_region_new(image);
    if (!region) {
        error_log(PISSTVPP2_ERR_IMAGE_PROCESS, "Failed to create VipsRegion for image");
        free(buf->data);
        free(buf);
        g_object_unref(image);
        return PISSTVPP2_ERR_IMAGE_PROCESS;
    }

    VipsRect rect = { 0, 0, image->Xsize, image->Ysize };
    if (vips_region_prepare(region, &rect)) {
        error_log(PISSTVPP2_ERR_IMAGE_PROCESS, "Failed to prepare image region: %s", vips_error_buffer());
        vips_error_clear();
        g_object_unref(region);
        free(buf->data);
        free(buf);
        g_object_unref(image);
        return PISSTVPP2_ERR_IMAGE_PROCESS;
    }

    /* Copy pixel data */
    const uint8_t *src = VIPS_REGION_ADDR(region, 0, 0);
    if (!src) {
        error_log(PISSTVPP2_ERR_IMAGE_PROCESS, "Failed to access pixel data from VipsRegion");
        g_object_unref(region);
        free(buf->data);
        free(buf);
        g_object_unref(image);
        return PISSTVPP2_ERR_IMAGE_PROCESS;
    }

    memcpy(buf->data, src, data_size);
    g_object_unref(region);

    log_verbose(verbose, timestamp_logging, "   [OK] Buffer ready: %dx%d RGB\n", buf->width, buf->height);

    /* Update module state */
    if (g_img.buffer) {
        if (g_img.buffer->data) free(g_img.buffer->data);
        free(g_img.buffer);
    }
    g_img.buffer = buf;
    g_img.image = image;

    return PISSTVPP2_OK;
}

/* ============================================================================
   PUBLIC: IMAGE LIFECYCLE
   ============================================================================ */

int image_load_from_file(const char *filename, int verbose, int timestamp_logging, const char *debug_output_dir) {
    if (!filename) {
        error_log(PISSTVPP2_ERR_ARG_FILENAME_INVALID, "Filename pointer is NULL");
        return PISSTVPP2_ERR_ARG_FILENAME_INVALID;
    }

    log_verbose(verbose, timestamp_logging, "   Loading image from: %s\n", filename);

    /* Clear any existing image */
    clear_image_state();
    
    /* Store original filename for extension extraction */
    strncpy(g_img.original_filename, filename, sizeof(g_img.original_filename) - 1);
    g_img.original_filename[sizeof(g_img.original_filename) - 1] = '\0';

    /* Load image with libvips auto-detect */
    VipsImage *image = vips_image_new_from_file(filename, NULL);
    if (!image) {
        error_log(PISSTVPP2_ERR_IMAGE_LOAD, "Failed to load image: %s (Details: %s)", filename, vips_error_buffer());
        vips_error_clear();
        return PISSTVPP2_ERR_IMAGE_LOAD;
    }

    log_verbose(verbose, timestamp_logging, "   --> Loaded: %dx%d, %d-band image\n", image->Xsize, image->Ysize, image->Bands);

    /* Buffer the image (includes RGB conversion) */
    int buffer_result = buffer_vips_image(image, verbose, timestamp_logging);
    if (buffer_result != PISSTVPP2_OK) {
        error_log(buffer_result, "Failed to buffer image data");
        g_object_unref(image);
        return buffer_result;
    }

    log_verbose(verbose, timestamp_logging, "   [OK] Image loaded successfully\n");

    /* Debug: Save loaded image if directory provided */
    if (debug_output_dir) {
        char debug_path[1024];
        snprintf(debug_path, sizeof(debug_path), "%s/01_loaded.png", debug_output_dir);
        if (image_save_to_file(debug_path, 0) == PISSTVPP2_OK && verbose) {
            printf("[DEBUG] Saved loaded image to: %s\n", debug_path);
        }
    }

    return PISSTVPP2_OK;
}

int image_get_dimensions(int *width, int *height) {
    if (!g_img.buffer) {
        error_log(PISSTVPP2_ERR_IMAGE_LOAD, "No image loaded");
        return PISSTVPP2_ERR_IMAGE_LOAD;
    }
    if (width) *width = g_img.buffer->width;
    if (height) *height = g_img.buffer->height;
    return PISSTVPP2_OK;
}

void image_get_pixel_rgb(int x, int y, uint8_t *r, uint8_t *g, uint8_t *b) {
    /* Default to black on error */
    *r = *g = *b = 0;

    if (!g_img.buffer) {
        return;
    }

    /* Bounds check */
    if (x < 0 || x >= g_img.buffer->width || y < 0 || y >= g_img.buffer->height) {
        return;
    }

    /* Direct buffer access: RGB format, 3 bytes per pixel */
    int offset = (y * g_img.buffer->rowstride) + (x * 3);
    const uint8_t *pixel = &g_img.buffer->data[offset];

    *r = pixel[0];
    *g = pixel[1];
    *b = pixel[2];
}

const char *image_get_original_extension(void) {
    if (g_img.original_filename[0] == '\0') {
        return "";
    }
    return get_file_extension(g_img.original_filename);
}

void image_free(void) {
    clear_image_state();
}

/* ============================================================================
   PUBLIC: IMAGE TRANSFORMATION
   ============================================================================ */

/**
 * apply_center_transformation
 * Crop from center to exact target dimensions
 */
static int apply_center_transformation(int target_width, int target_height, 
                                       int *crop_left, int *crop_top, 
                                       int *crop_width, int *crop_height,
                                       VipsImage **out_corrected, int verbose, int timestamp_logging) {
    VipsImage *cropped = NULL;
    VipsImage *resized = NULL;

    log_verbose(verbose, timestamp_logging, "   --> CENTER mode: center-crop to exact target dimensions\n");
    log_verbose(verbose, timestamp_logging, "       Crop box: x=%d y=%d w=%d h=%d\n", *crop_left, *crop_top, *crop_width, *crop_height);

    /* Ensure crop dimensions are valid */
    int actual_crop_width = *crop_width;
    int actual_crop_height = *crop_height;
    
    if (actual_crop_width > target_width) {
        actual_crop_width = target_width;
    }
    if (actual_crop_height > target_height) {
        actual_crop_height = target_height;
    }

    /* Crop from center to extract the requested region */
    if (vips_crop(g_img.image, &cropped, *crop_left, *crop_top, actual_crop_width, actual_crop_height, NULL)) {
        error_log(PISSTVPP2_ERR_IMAGE_ASPECT_CORRECTION, "Image cropping",
                 "vips_crop failed: %s", vips_error_buffer());
        vips_error_clear();
        return PISSTVPP2_ERR_IMAGE_ASPECT_CORRECTION;
    }

    /* Resize to exact target dimensions if needed */
    if (cropped->Xsize != target_width || cropped->Ysize != target_height) {
        double scale_x = (double)target_width / cropped->Xsize;
        double scale_y = (double)target_height / cropped->Ysize;

        if (vips_resize(cropped, &resized, scale_x, "vscale", scale_y, NULL)) {
            error_log(PISSTVPP2_ERR_IMAGE_ASPECT_CORRECTION, "Image resizing",
                     "vips_resize failed: %s", vips_error_buffer());
            vips_error_clear();
            g_object_unref(cropped);
            return PISSTVPP2_ERR_IMAGE_ASPECT_CORRECTION;
        }

        g_object_unref(cropped);
        *out_corrected = resized;

        if (verbose) {
            log_verbose(verbose, timestamp_logging, "       Result: %dx%d (cropped + resized)\n", target_width, target_height);
        }
    } else {
        if (verbose) {
            log_verbose(verbose, timestamp_logging, "       Result: %dx%d (cropped, no resize)\n", actual_crop_width, actual_crop_height);
        }
        *out_corrected = cropped;
    }
    return PISSTVPP2_OK;
}

/**
 * apply_pad_transformation
 * Add black padding to reach target, preserving original aspect ratio
 */
static int apply_pad_transformation(int target_width, int target_height,
                                    int *pad_left, int *pad_top,
                                    int *pad_width, int *pad_height,
                                    VipsImage **out_corrected, int verbose, int timestamp_logging) {
    VipsImage *padded = NULL;

    log_verbose(verbose, timestamp_logging, "   --> PAD mode: add black padding to reach target\n");
    log_verbose(verbose, timestamp_logging, "       Padding: x=%d y=%d total_w=%d total_h=%d\n", *pad_left, *pad_top, *pad_width, *pad_height);
    log_verbose(verbose, timestamp_logging, "       (Preserves original image aspect ratio, centers image with black bars)\n");

    /* Pad with black bars to exact target dimensions */
    if (vips_embed(g_img.image, &padded, *pad_left, *pad_top, target_width, target_height, 
                   "extend", VIPS_EXTEND_BLACK, NULL)) {
        error_log(PISSTVPP2_ERR_IMAGE_ASPECT_CORRECTION, "Image padding",
                 "vips_embed failed: %s", vips_error_buffer());
        vips_error_clear();
        return PISSTVPP2_ERR_IMAGE_ASPECT_CORRECTION;
    }

    if (verbose) {
        log_verbose(verbose, timestamp_logging, "       Result: %dx%d with black padding\n", target_width, target_height);
    }

    *out_corrected = padded;
    return PISSTVPP2_OK;
}

/**
 * apply_stretch_transformation
 * Direct resize without aspect correction
 */
static int apply_stretch_transformation(int target_width, int target_height,
                                         VipsImage **out_corrected, int verbose, int timestamp_logging) {
    VipsImage *resized = NULL;

    log_verbose(verbose, timestamp_logging, "   --> STRETCH mode: direct non-uniform resize\n");
    log_verbose(verbose, timestamp_logging, "       WARNING: Image will be distorted\n");

    double scale_x = (double)target_width / g_img.image->Xsize;
    double scale_y = (double)target_height / g_img.image->Ysize;

    log_verbose(verbose, timestamp_logging, "       Resize scales: x=%.4f y=%.4f\n", scale_x, scale_y);

    if (vips_resize(g_img.image, &resized, scale_x, "vscale", scale_y, NULL)) {
        error_log(PISSTVPP2_ERR_IMAGE_ASPECT_CORRECTION, "Image stretching",
                 "vips_resize failed: %s", vips_error_buffer());
        vips_error_clear();
        return PISSTVPP2_ERR_IMAGE_ASPECT_CORRECTION;
    }

    *out_corrected = resized;
    return PISSTVPP2_OK;
}

int image_correct_aspect_and_resize(int target_width, int target_height, AspectMode mode, 
                                    int verbose, int timestamp_logging, const char *debug_output_dir) {
    if (!g_img.buffer || !g_img.image) {
        error_log(PISSTVPP2_ERR_IMAGE_LOAD, "No image loaded");
        return PISSTVPP2_ERR_IMAGE_LOAD;
    }

    int img_width = g_img.buffer->width;
    int img_height = g_img.buffer->height;
    double img_aspect = (double)img_width / img_height;
    double target_aspect = (double)target_width / target_height;

    log_verbose(verbose, timestamp_logging, "   Correcting image aspect and resolution\n");
    log_verbose(verbose, timestamp_logging, "   Current: %dx%d (aspect %.3f)\n", img_width, img_height, img_aspect);
    log_verbose(verbose, timestamp_logging, "   Target:  %dx%d (aspect %.3f)\n", target_width, target_height, target_aspect);
    log_verbose(verbose, timestamp_logging, "   Mode: %s\n", aspect_mode_to_string(mode));

    /* Check if correction needed */
    int needs_size = (img_width != target_width || img_height != target_height);
    int needs_aspect = (fabs(img_aspect - target_aspect) > ASPECT_TOLERANCE);

    if (!needs_size && !needs_aspect) {
        log_verbose(verbose, timestamp_logging, "   [OK] Image already correct size and aspect - no correction needed\n");
        return PISSTVPP2_OK;
    }

    VipsImage *corrected = NULL;

    /* Pre-calculate transformation parameters */
    int crop_left, crop_top, crop_width, crop_height;
    int pad_left, pad_top, pad_width, pad_height;

    /* For CENTER mode: crop to exact target dimensions from center */
    if (mode == ASPECT_CENTER) {
        /* Calculate centered crop box to match target aspect ratio */
        image_calculate_crop_box(img_width, img_height, target_aspect, 
                                &crop_left, &crop_top, &crop_width, &crop_height);
        
        if (verbose) {
            log_verbose(verbose, timestamp_logging, "   CENTER crop calculation: src %dx%d → crop box %dx%d at (%d,%d)\n",
                   img_width, img_height, crop_width, crop_height, crop_left, crop_top);
        }
    } else {
        /* For other modes: calculate based on aspect ratio */
        image_calculate_crop_box(img_width, img_height, target_aspect, 
                                &crop_left, &crop_top, &crop_width, &crop_height);
    }

    /* For padding, center image within target dimensions */
    pad_width = target_width;
    pad_height = target_height;
    pad_left = (target_width - img_width) / 2;
    pad_top = (target_height - img_height) / 2;

    /* Apply transformation based on mode */
    int result = PISSTVPP2_OK;
    switch (mode) {
        case ASPECT_CENTER:
            result = apply_center_transformation(target_width, target_height, 
                                                &crop_left, &crop_top, &crop_width, &crop_height,
                                                &corrected, verbose, timestamp_logging);
            break;
        case ASPECT_PAD:
            result = apply_pad_transformation(target_width, target_height,
                                             &pad_left, &pad_top, &pad_width, &pad_height,
                                             &corrected, verbose, timestamp_logging);
            break;
        case ASPECT_STRETCH:
            result = apply_stretch_transformation(target_width, target_height, &corrected, verbose, timestamp_logging);
            break;
        default:
            error_log(PISSTVPP2_ERR_IMAGE_ASPECT_CORRECTION, "Unknown aspect mode: %d", mode);
            return PISSTVPP2_ERR_IMAGE_ASPECT_CORRECTION;
    }

    if (result != PISSTVPP2_OK) {
        error_log(result, "Image aspect correction transformation failed");
        if (corrected) g_object_unref(corrected);
        return result;
    }

    /* Debug: Save intermediate image */
    if (debug_output_dir && debug_output_dir[0] != '\0') {
        /* Save the corrected image before clearing state */
        /* We need to use corrected VipsImage directly */
        if (vips_image_write_to_file(corrected, debug_output_dir, NULL)) {
            log_verbose(1, timestamp_logging, "   [WARNING] Failed to save intermediate image to: %s\n", debug_output_dir);
        } else {
            log_verbose(1, timestamp_logging, "   --> Saved intermediate image: %s\n", debug_output_dir);
        }
    }

    /* Release old image and buffer */
    clear_image_state();

    /* Buffer the corrected image */
    int buffer_result = buffer_vips_image(corrected, verbose, timestamp_logging);
    if (buffer_result != PISSTVPP2_OK) {
        error_log(buffer_result, "Failed to buffer corrected image");
        g_object_unref(corrected);
        return buffer_result;
    }

    /* Verify result */
    if (g_img.buffer->width != target_width || g_img.buffer->height != target_height) {
        error_log(PISSTVPP2_ERR_IMAGE_PROCESS, "Correction failed: got %dx%d, expected %dx%d",
                g_img.buffer->width, g_img.buffer->height, target_width, target_height);
        return PISSTVPP2_ERR_IMAGE_PROCESS;
    }

    if (verbose) {
        log_verbose(verbose, timestamp_logging, "   [OK] Image corrected to %dx%d\n", g_img.buffer->width, g_img.buffer->height);
    }

    return PISSTVPP2_OK;
}

/* ============================================================================
   PUBLIC: DEBUG AND INSPECTION
   ============================================================================ */

int image_save_to_file(const char *output_path, int verbose) {
    if (!output_path) {
        error_log(PISSTVPP2_ERR_ARG_FILENAME_INVALID, "Output path pointer is NULL");
        return PISSTVPP2_ERR_ARG_FILENAME_INVALID;
    }

    if (!g_img.image) {
        error_log(PISSTVPP2_ERR_IMAGE_LOAD, "No image loaded");
        return PISSTVPP2_ERR_IMAGE_LOAD;
    }

    if (vips_image_write_to_file(g_img.image, output_path, NULL)) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "Failed to save image to '%s' (Details: %s)", output_path, vips_error_buffer());
        vips_error_clear();
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    if (verbose) {
        printf("[DEBUG] Image saved to: %s\n", output_path);
    }

    return PISSTVPP2_OK;
}

void image_print_diagnostics(void) {
    if (!g_img.buffer) {
        printf("[IMAGE] No image loaded\n");
        return;
    }

    double aspect = (double)g_img.buffer->width / g_img.buffer->height;
    printf("[IMAGE] Dimensions: %dx%d pixels\n", g_img.buffer->width, g_img.buffer->height);
    printf("[IMAGE] Aspect ratio: %.3f (%.1f:%.1f)\n", aspect, 
           (double)g_img.buffer->width, (double)g_img.buffer->height);
    printf("[IMAGE] Rowstride: %d bytes\n", g_img.buffer->rowstride);
    printf("[IMAGE] Total size: %d bytes\n", g_img.buffer->height * g_img.buffer->rowstride);
}

/**
 * apply_single_overlay - Apply a single text overlay to the image
 * 
 * @param spec TextOverlaySpec with overlay configuration
 * @param verbose Debug output
 * @param timestamp_logging Timestamp flag
 * 
 * @return Error code
 * 
 * Renders text with blue color, white background, and appropriate sizing
 */
static int apply_single_overlay(const TextOverlaySpec *spec, int verbose, int timestamp_logging)
{
    if (!spec || !g_img.image) {
        return PISSTVPP2_OK;  // Skip empty specs
    }

    // Prepare text content - either use provided text or generate timestamp
    char display_text[OVERLAY_MAX_TEXT_LENGTH];
    
    if (spec->timestamp_format[0] != '\0') {
        // Generate timestamp using current system time
        time_t now = time(NULL);
        struct tm *timeinfo = localtime(&now);
        if (timeinfo == NULL) {
            // Fall back to empty text if time generation fails
            display_text[0] = '\0';
        } else {
            // Format timestamp using strftime
            strftime(display_text, sizeof(display_text), spec->timestamp_format, timeinfo);
        }
        log_verbose(verbose, timestamp_logging,
                   "      Generated timestamp: '%s' (format: %s)\n", 
                   display_text, spec->timestamp_format);
    } else if (strlen(spec->text) > 0) {
        // Use provided text
        strncpy(display_text, spec->text, sizeof(display_text) - 1);
        display_text[sizeof(display_text) - 1] = '\0';
    } else {
        // No text or timestamp specified
        return PISSTVPP2_OK;
    }
    
    if (display_text[0] == '\0') {
        return PISSTVPP2_OK;  // Skip if text is empty
    }

    int font_size = spec->font_size > 0 ? spec->font_size : 24;
    
    // Format color for display
    char color_str[32];
    snprintf(color_str, sizeof(color_str), "#%02X%02X%02X", 
             spec->text_color.r, spec->text_color.g, spec->text_color.b);
    
    log_verbose(verbose, timestamp_logging,
               "      Rendering overlay text '%s' (font size: %d, color: %s)\n", 
               display_text, font_size, color_str);

    // Create font description
    char font_str[256];
    snprintf(font_str, sizeof(font_str), "sans bold %d", font_size);

    // Create Pango markup with color using hex notation
    // vips_text supports Pango markup like <span foreground="#RRGGBB">text</span>
    char markup_text[512];
    snprintf(markup_text, sizeof(markup_text), 
             "<span foreground=\"#%02X%02X%02X\">%s</span>",
             spec->text_color.r, spec->text_color.g, spec->text_color.b, 
             display_text);

    // Create text image with RGBA support for proper alpha blending
    // vips_text will render colored text via Pango markup
    VipsImage *text_image = NULL;
    if (vips_text(&text_image, markup_text, 
                 "font", font_str,
                 "rgba", TRUE,
                 "dpi", 72,
                 NULL)) {
        log_verbose(verbose, timestamp_logging,
                   "      Warning: Failed to render text '%s': %s\n", 
                   spec->text, vips_error_buffer());
        vips_error_clear();
        return PISSTVPP2_OK;  // Don't fail, just skip this overlay
    }

    if (!text_image) {
        return PISSTVPP2_OK;
    }
    
    // Handle vertical bar text orientations
    if (spec->bg_bar_enable && spec->bg_bar_orientation == BGBAR_ORIENT_VERTICAL_STACKED) {
        // Stack text vertically with normal letter orientation
        // Create text with line breaks between characters for vertical stacking
        char stacked_markup[512] = {0};
        int pos = 0;
        
        for (int i = 0; display_text[i] != '\0' && pos < (int)sizeof(stacked_markup) - 10; i++) {
            stacked_markup[pos++] = display_text[i];
            // Add newline after each character except the last
            if (display_text[i + 1] != '\0') {
                stacked_markup[pos++] = '\n';
            }
        }
        
        // Re-render text with vertical layout (one character per line)
        VipsImage *stacked_image = NULL;
        if (vips_text(&stacked_image, stacked_markup,
                     "font", font_str,
                     "rgba", TRUE,
                     "dpi", 72,
                     NULL) == 0) {
            g_object_unref(text_image);
            text_image = stacked_image;
            log_verbose(verbose, timestamp_logging,
                       "      Text arranged vertically (stacked, %d chars)\n",
                       (int)strlen(display_text));
        } else {
            log_verbose(verbose, timestamp_logging,
                       "      Note: Could not render vertically-stacked text\n");
            vips_error_clear();
        }
    } else if (spec->bg_bar_enable && spec->bg_bar_orientation == BGBAR_ORIENT_VERTICAL) {
        // Rotate text 90 degrees for vertical orientation
        VipsImage *rotated_text = NULL;
        // Rotate 90 degrees clockwise using vips_rot90 (only takes in/out, no direction param)
        if (vips_rot90(text_image, &rotated_text, NULL) == 0) {
            g_object_unref(text_image);
            text_image = rotated_text;
            log_verbose(verbose, timestamp_logging, 
                       "      Rotated text 90 degrees for vertical bar\n");
        } else {
            log_verbose(verbose, timestamp_logging,
                       "      Warning: Failed to rotate text for vertical bar\n");
            vips_error_clear();
        }
    }
    // Get image dimensions to calculate placement
    int img_width = g_img.image->Xsize;
    int img_height = g_img.image->Ysize;
    int text_width = text_image->Xsize;
    int text_height = text_image->Ysize;

    // Calculate position based on placement spec
    int x_pos = 10;  // Default: 10 pixels from left
    int y_pos = 10;  // Default: 10 pixels from top

    switch (spec->placement) {
        case OVERLAY_PLACE_TOP:
            x_pos = (img_width - text_width) / 2;  // Center horizontally
            y_pos = 10;
            break;
        case OVERLAY_PLACE_BOTTOM:
            x_pos = (img_width - text_width) / 2;
            y_pos = img_height - text_height - 10;
            break;
        case OVERLAY_PLACE_LEFT:
            x_pos = 10;
            y_pos = (img_height - text_height) / 2;
            break;
        case OVERLAY_PLACE_RIGHT:
            x_pos = img_width - text_width - 10;
            y_pos = (img_height - text_height) / 2;
            break;
        case OVERLAY_PLACE_CENTER:
            x_pos = (img_width - text_width) / 2;
            y_pos = (img_height - text_height) / 2;
            break;
        default:
            x_pos = 10;
            y_pos = 10;
    }

    // Clamp to valid range
    if (x_pos < 0) x_pos = 0;
    if (y_pos < 0) y_pos = 0;
    if (x_pos + text_width > img_width) x_pos = img_width - text_width;
    if (y_pos + text_height > img_height) y_pos = img_height - text_height;

    // If background bar is enabled, render it first
    VipsImage *current_result = g_img.image;
    if (spec->bg_bar_enable) {
        // Calculate bar dimensions based on orientation
        uint16_t total_margin = spec->padding + spec->bg_bar_margin;
        int bar_width;
        int bar_height;
        int bar_x;
        int bar_y;
        
        // Handle vertical vs horizontal orientation
        if (spec->bg_bar_orientation == BGBAR_ORIENT_VERTICAL || 
            spec->bg_bar_orientation == BGBAR_ORIENT_VERTICAL_STACKED) {
            // VERTICAL/STACKED BAR: spans image height, positioned on left or right
            bar_height = img_height;  // Full height
            bar_y = 0;
            
            // Determine bar width (thickness of the vertical bar)
            if (spec->placement == OVERLAY_PLACE_LEFT) {
                bar_width = spec->bg_bar_custom_width > 0 ? spec->bg_bar_custom_width : 50;
                bar_x = 0;
            } else if (spec->placement == OVERLAY_PLACE_RIGHT) {
                bar_width = spec->bg_bar_custom_width > 0 ? spec->bg_bar_custom_width : 50;
                bar_x = img_width - bar_width;
            } else {
                // For non-left/right placements, default to left vertical bar
                bar_width = spec->bg_bar_custom_width > 0 ? spec->bg_bar_custom_width : 50;
                bar_x = 0;
            }
        } else {
            // HORIZONTAL BAR: original behavior
            bar_height = text_height + (total_margin * 2);
            bar_x = x_pos - total_margin;
            bar_y = y_pos - total_margin;
            
            // Determine bar width based on mode
            switch (spec->bg_bar_width_mode) {
                case BGBAR_WIDTH_FULL:
                    // Full image width
                    bar_width = img_width;
                    bar_x = 0;
                    break;
                case BGBAR_WIDTH_HALF:
                    // Half image width, centered on text
                    bar_width = img_width / 2;
                    bar_x = (img_width - bar_width) / 2;
                    break;
                case BGBAR_WIDTH_FIXED:
                    // Custom pixel width, centered on text
                    bar_width = spec->bg_bar_custom_width;
                    bar_x = x_pos - (bar_width - text_width) / 2;
                    break;
                case BGBAR_WIDTH_AUTO:
                default:
                    // Auto: text + padding + margin (original behavior)
                    bar_width = text_width + (total_margin * 2);
                    bar_x = x_pos - total_margin;
                    break;
            }
        }
        
        // Clamp bar position to valid range
        if (bar_x < 0) bar_x = 0;
        if (bar_y < 0) bar_y = 0;
        // Clamp bar dimensions to image bounds
        if (bar_width > img_width) bar_width = img_width;
        if (bar_height > img_height) bar_height = img_height;
        if (bar_x + bar_width > img_width) bar_x = img_width - bar_width;
        if (bar_y + bar_height > img_height) bar_y = img_height - bar_height;
        
        // Render background bar (always render when enabled, use bg_bar_color)
        VipsImage *bar_image = NULL;
        
        // Get the number of bands from current_result to match
        int bands = current_result->Bands;
        
        // Create black image of bar dimensions with matching band count
        if (vips_black(&bar_image, bar_width, bar_height, "bands", bands, NULL) == 0) {
            // Create new image with the background bar color
            if (bar_image != NULL) {
                // Iterate through pixels and set color
                VipsImage *colored_bar = NULL;
                
                // Use vips_copy to create a modifiable copy
                if (vips_copy(bar_image, &colored_bar, NULL) == 0) {
                    g_object_unref(bar_image);
                    
                    // Access the region to modify pixels
                    VipsRegion *region = vips_region_new(colored_bar);
                    if (region != NULL) {
                        // Get the region for the entire image
                        VipsRect rect = {0, 0, bar_width, bar_height};
                        if (vips_region_prepare(region, &rect) == 0) {
                            // Fill all pixels with bar color
                            PEL *p = (PEL *)VIPS_REGION_ADDR(region, 0, 0);
                            
                            for (int row = 0; row < bar_height; row++) {
                                PEL *row_ptr = p + row * VIPS_REGION_LSKIP(region);
                                for (int col = 0; col < bar_width; col++) {
                                    PEL *pixel = row_ptr + col * bands;
                                    pixel[0] = spec->bg_bar_color.r;
                                    pixel[1] = spec->bg_bar_color.g;
                                    pixel[2] = spec->bg_bar_color.b;
                                    // If there's an alpha channel, set it to fully opaque
                                    if (bands > 3) {
                                        pixel[3] = 255;
                                    }
                                }
                            }
                            
                            log_verbose(verbose, timestamp_logging,
                                       "      Rendered background bar at position (%d, %d), size %dx%d, color RGB(%d,%d,%d)\n",
                                       bar_x, bar_y, bar_width, bar_height,
                                       spec->bg_bar_color.r, spec->bg_bar_color.g, spec->bg_bar_color.b);
                        }
                        g_object_unref(region);
                    }
                    
                    // Now composite the colored bar into current result
                    VipsImage *result_with_bar = NULL;
                    if (vips_insert(current_result, colored_bar, &result_with_bar,
                                   bar_x, bar_y, NULL) == 0) {
                        g_object_unref(colored_bar);
                        if (current_result != g_img.image) {
                            g_object_unref(current_result);
                        }
                        current_result = result_with_bar;
                    } else {
                        log_verbose(verbose, timestamp_logging,
                                   "      Warning: Failed to insert background bar: %s\n",
                                   vips_error_buffer());
                        vips_error_clear();
                        g_object_unref(colored_bar);
                    }
                } else {
                    g_object_unref(bar_image);
                }
            }
        } else {
            log_verbose(verbose, timestamp_logging,
                       "      Warning: Failed to create bar background image\n");
            vips_error_clear();
        }
    }

    // Composite text_image onto current result (which may have background bar)
    // vips_composite2() handles RGBA->RGB properly with blend mode OVER
    VipsImage *composited = NULL;
    if (vips_composite2(current_result, text_image, &composited,
                       VIPS_BLEND_MODE_OVER,
                       "x", x_pos,
                       "y", y_pos,
                       NULL)) {
        error_log(PISSTVPP2_ERR_IMAGE_PROCESS, 
                  "Failed to composite text overlay: %s", 
                  vips_error_buffer());
        vips_error_clear();
        g_object_unref(text_image);
        if (current_result != g_img.image) {
            g_object_unref(current_result);
        }
        return PISSTVPP2_ERR_IMAGE_PROCESS;
    }

    // Verify compositing succeeded
    if (!composited) {
        error_log(PISSTVPP2_ERR_IMAGE_PROCESS, 
                  "vips_composite2 returned NULL image");
        g_object_unref(text_image);
        if (current_result != g_img.image) {
            g_object_unref(current_result);
        }
        return PISSTVPP2_ERR_IMAGE_PROCESS;
    }

    log_verbose(verbose, timestamp_logging,
               "      Composited text at position (%d, %d) with alpha blending\n", 
               x_pos, y_pos);

    // Release old image reference and update to composited result
    if (current_result != g_img.image) {
        g_object_unref(current_result);
    }
    g_object_unref(g_img.image);
    g_img.image = composited;
    g_object_unref(text_image);

    return PISSTVPP2_OK;
}

/* ============================================================================
   PUBLIC: TEXT OVERLAY
   ============================================================================ */

/**
 * image_apply_overlay_list - Apply overlay specifications to image
 * 
 * @param overlay_specs Pointer to OverlaySpecList with specifications
 * @param verbose Debug output flag
 * @param timestamp_logging Timestamp flag (requires verbose)
 * 
 * @return PISSTVPP2_OK on success, error code on failure
 * 
 * Renders text overlays with configurable:
 * - Text content (arbitrary)
 * - Font size
 * - Placement (top, bottom, left, right, center)
 * - Color and background (to be rendered in Phase 2.5)
 */
int image_apply_overlay_list(const OverlaySpecList *overlay_specs,
                            int verbose, int timestamp_logging)
{
    if (!overlay_specs) {
        error_log(PISSTVPP2_ERR_IMAGE_TEXT_OVERLAY, "No overlay specifications provided");
        return PISSTVPP2_ERR_IMAGE_TEXT_OVERLAY;
    }

    if (!g_img.image) {
        error_log(PISSTVPP2_ERR_IMAGE_LOAD, "No image loaded for overlay");
        return PISSTVPP2_ERR_IMAGE_LOAD;
    }

    size_t overlay_count = overlay_spec_list_count((OverlaySpecList *)overlay_specs);
    
    if (overlay_count == 0) {
        return PISSTVPP2_OK;
    }

    log_verbose(verbose, timestamp_logging,
               "   Applying %zu text overlay(s) to image...\n", overlay_count);

    // Apply each overlay spec
    for (size_t i = 0; i < overlay_count; i++) {
        TextOverlaySpec *spec = overlay_spec_list_get((OverlaySpecList *)overlay_specs, i);
        
        if (!spec || !spec->enabled) {
            continue;
        }

        int result = apply_single_overlay(spec, verbose, timestamp_logging);
        if (result != PISSTVPP2_OK) {
            log_verbose(verbose, timestamp_logging,
                       "      Warning: Overlay %zu processing\n", i + 1);
        }
    }

    // Re-buffer the modified image
    int result = buffer_vips_image(g_img.image, verbose, timestamp_logging);
    if (result != PISSTVPP2_OK) {
        error_log(result, "Failed to buffer image after overlays");
        return result;
    }

    log_verbose(verbose, timestamp_logging,
               "   [OK] All overlay specifications processed\n");

    return PISSTVPP2_OK;
}


/* ============================================================================
   PUBLIC: HELPER UTILITIES
   ============================================================================ */

const char* aspect_mode_to_string(AspectMode mode) {
    switch (mode) {
        case ASPECT_CENTER: return "CENTER";
        case ASPECT_PAD: return "PAD";
        case ASPECT_STRETCH: return "STRETCH";
        default: return "UNKNOWN";
    }
}

void image_calculate_crop_box(int src_width, int src_height, double target_aspect,
                              int *out_left, int *out_top, int *out_width, int *out_height) {
    double src_aspect = (double)src_width / src_height;

    if (src_aspect > target_aspect) {
        /* Source too wide - crop left/right */
        *out_height = src_height;
        *out_width = (int)(src_height * target_aspect + 0.5);
        *out_top = 0;
        *out_left = (src_width - *out_width) / 2;
    } else {
        /* Source too tall - crop top/bottom */
        *out_width = src_width;
        *out_height = (int)(src_width / target_aspect + 0.5);
        *out_left = 0;
        *out_top = (src_height - *out_height) / 2;
    }
}
