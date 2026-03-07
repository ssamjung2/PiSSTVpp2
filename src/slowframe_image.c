/*
 * slowframe_image.c
 * 
 * Image processing module for SlowFrame
 * Implements image loading, resizing, aspect ratio correction, and debug export
 */

#include "slowframe_image.h"
#include "error.h"
#include "logging.h"
#include "overlay_spec.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <time.h>

/* Forward declaration from image_aspect.c */
extern int image_aspect_correct(VipsImage *image, int target_width, int target_height,
                                AspectMode mode, VipsImage **out_corrected,
                                int verbose, int timestamp_logging);

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
    ExifMetadata exif;                  /* Extracted EXIF metadata */
} ImageState;

static ImageState g_img = {
    .image = NULL,
    .buffer = NULL,
    .original_filename = {0},
    .exif = {0}
};

/* Aspect ratio tolerance for "already correct" check */
#define ASPECT_TOLERANCE 0.001

/* ============================================================================
   INTERNAL HELPERS
   ============================================================================ */

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
 * extract_exif_from_image
 * Extract EXIF metadata from a VipsImage and store in ExifMetadata structure
 * Uses defaults for any missing EXIF tags
 * 
 * Tries multiple metadata key variations since libvips EXIF key naming varies
 */
static void extract_exif_from_image(VipsImage *image, ExifMetadata *exif) {
    if (!image || !exif) return;

    /* Initialize to defaults */
    memset(exif, 0, sizeof(ExifMetadata));
    exif->f_stop = 28;              /* f/2.8 default */
    exif->metering_mode = 1;        /* Average metering */
    exif->exposure_program = 2;     /* Normal program */
    exif->exposure_time_log = 0;    /* 1 second baseline */
    exif->focal_length = 50;        /* 50mm default */
    exif->brightness_ev = 10;       /* 0 EV (neutral) */
    exif->iso_speed = 400;          /* ISO 400 */
    exif->white_balance = 0;        /* Auto WB */
    exif->color_space = 0;          /* sRGB */

    /* Try to extract individual EXIF fields using libvips metadata API */
    
    /* Extract ISO Speed */
    int iso_val = 0;
    const char *iso_keys[] = {
        "exif-photo-iso-speed-ratings",
        "exif-iop-iso-speed-ratings", 
        "Exif.Photo.ISOSpeedRatings",
        NULL
    };
    for (int i = 0; iso_keys[i]; i++) {
        if (!vips_image_get_int(image, iso_keys[i], &iso_val) && iso_val > 0) {
            exif->iso_speed = (uint16_t)iso_val;
            break;
        }
    }
    
    /* Extract F-Number (FNumber is stored as Rational, e.g., "2/1" for f/2) */
    const char *fnumber = NULL;
    const char *fnumber_keys[] = {
        "exif-photo-f-number",
        "exif-photo-fnumber",
        "Exif.Photo.FNumber",
        NULL
    };
    for (int i = 0; fnumber_keys[i]; i++) {
        if (!vips_image_get_string(image, fnumber_keys[i], &fnumber) && fnumber) {
            /* Parse rational format: "numerator/denominator" */
            float f_val = 0;
            if (sscanf(fnumber, "%f", &f_val) == 1 && f_val > 0) {
                exif->f_stop = (uint8_t)(f_val * 10 + 0.5);  /* f/2.0 → 20, f/2.8 → 28 */
            }
            break;
        }
    }
    
    /* Extract Focal Length (Rational, e.g., "50/1" for 50mm) */
    const char *focal = NULL;
    const char *focal_keys[] = {
        "exif-photo-focal-length",
        "Exif.Photo.FocalLength",
        NULL
    };
    for (int i = 0; focal_keys[i]; i++) {
        if (!vips_image_get_string(image, focal_keys[i], &focal) && focal) {
            float focal_val = 0;
            if (sscanf(focal, "%f", &focal_val) == 1 && focal_val > 0) {
                exif->focal_length = (uint16_t)(focal_val + 0.5);
            }
            break;
        }
    }
    
    /* Extract Exposure Time / Shutter Speed (Rational, e.g., "1/100" for 1/100s) */
    const char *exposure_time = NULL;
    const char *exposure_keys[] = {
        "exif-photo-exposure-time",
        "Exif.Photo.ExposureTime",
        NULL
    };
    for (int i = 0; exposure_keys[i]; i++) {
        if (!vips_image_get_string(image, exposure_keys[i], &exposure_time) && exposure_time) {
            /* Parse as fraction or decimal and convert to log2 scale */
            float exp_val = 0;
            int num = 0, denom = 1;
            if (sscanf(exposure_time, "%d/%d", &num, &denom) == 2 && denom > 0) {
                exp_val = (float)num / denom;
            } else {
                sscanf(exposure_time, "%f", &exp_val);
            }
            if (exp_val > 0) {
                /* Convert to log2 scale: log2(exposure_time) with +10 offset for range */
                /* -10=1/1000s, 0=1s, +10=1000s */
                float log_val = log2f(exp_val) + 10;
                if (log_val >= -10 && log_val <= 10) {
                    exif->exposure_time_log = (uint8_t)(log_val + 0.5);
                }
            }
            break;
        }
    }
    
    /* Extract Brightness Value (Rational) */
    const char *brightness = NULL;
    const char *brightness_keys[] = {
        "exif-photo-brightness-value",
        "Exif.Photo.BrightnessValue",
        NULL
    };
    for (int i = 0; brightness_keys[i]; i++) {
        if (!vips_image_get_string(image, brightness_keys[i], &brightness) && brightness) {
            float bright_val = 0;
            if (sscanf(brightness, "%f", &bright_val) == 1) {
                exif->brightness_ev = (uint8_t)(bright_val + 10 + 0.5);  /* +10 offset */
            }
            break;
        }
    }
    
    /* Extract Metering Mode */
    int metering = 0;
    const char *metering_keys[] = {
        "exif-photo-metering-mode",
        "Exif.Photo.MeteringMode",
        NULL
    };
    for (int i = 0; metering_keys[i]; i++) {
        if (!vips_image_get_int(image, metering_keys[i], &metering) && metering >= 0) {
            exif->metering_mode = (uint8_t)metering;
            break;
        }
    }
    
    /* Extract Exposure Program */
    int exp_prog = 0;
    const char *prog_keys[] = {
        "exif-photo-exposure-program",
        "Exif.Photo.ExposureProgram",
        NULL
    };
    for (int i = 0; prog_keys[i]; i++) {
        if (!vips_image_get_int(image, prog_keys[i], &exp_prog) && exp_prog >= 0) {
            exif->exposure_program = (uint8_t)exp_prog;
            break;
        }
    }
    
    /* Extract White Balance */
    int wb = 0;
    const char *wb_keys[] = {
        "exif-photo-white-balance",
        "Exif.Photo.WhiteBalance",
        NULL
    };
    for (int i = 0; wb_keys[i]; i++) {
        if (!vips_image_get_int(image, wb_keys[i], &wb) && wb >= 0) {
            exif->white_balance = (uint8_t)wb;
            break;
        }
    }
    
    /* Extract Color Space */
    int colorspace = 0;
    const char *cs_keys[] = {
        "exif-photo-color-space",
        "Exif.Photo.ColorSpace",
        NULL
    };
    for (int i = 0; cs_keys[i]; i++) {
        if (!vips_image_get_int(image, cs_keys[i], &colorspace) && colorspace >= 0) {
            exif->color_space = (uint8_t)colorspace;
            break;
        }
    }
    
    /* Extract Date/Time - try multiple formats */
    const char *datetime = NULL;
    const char *datetime_keys[] = {
        "exif-image-datetime",
        "exif-photo-datetime-original",
        "exif-photo-datetime-digitized",
        "Exif.Image.DateTime",
        "Exif.Photo.DateTimeOriginal",
        "Exif.Photo.DateTimeDigitized",
        NULL
    };
    for (int i = 0; datetime_keys[i]; i++) {
        if (!vips_image_get_string(image, datetime_keys[i], &datetime) && datetime) {
            /* Parse datetime: YYYY:MM:DD HH:MM:SS */
            int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
            if (sscanf(datetime, "%d:%d:%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
                if (year >= 2000 && year <= 2099) {
                    exif->date_year = (uint8_t)(year - 2000);
                    exif->date_month = (uint8_t)month;
                    exif->date_day = (uint8_t)day;
                    exif->date_hour = (uint8_t)hour;
                    exif->date_minute = (uint8_t)minute;
                    exif->date_second = (uint8_t)second;
                }
            }
            break;
        }
    }
}

/**
 * buffer_vips_image
 * Convert a VipsImage to internal pixel buffer
 * Returns SLOWFRAME_OK on success, error code on failure
 */
static int buffer_vips_image(VipsImage *image, int verbose, int timestamp_logging) {
    if (!image) {
        error_log(SLOWFRAME_ERR_IMAGE_LOAD, "No image to buffer");
        return SLOWFRAME_ERR_IMAGE_LOAD;
    }

    /* Ensure image is in sRGB format */
    VipsImage *rgb_image = NULL;
    if (vips_colourspace(image, &rgb_image, VIPS_INTERPRETATION_sRGB, NULL)) {
        error_log(SLOWFRAME_ERR_IMAGE_PROCESS, "Colorspace conversion failed: %s", vips_error_buffer());
        vips_error_clear();
        return SLOWFRAME_ERR_IMAGE_PROCESS;
    }
    if (rgb_image != image) {
        g_object_unref(image);
        image = rgb_image;
    }

    /* Strip alpha channel if present — buffer must be exactly 3 bytes/pixel (RGB) */
    if (vips_image_hasalpha(image)) {
        VipsImage *flat = NULL;
        if (vips_flatten(image, &flat, NULL)) {
            error_log(SLOWFRAME_ERR_IMAGE_PROCESS, "Alpha flatten failed: %s", vips_error_buffer());
            vips_error_clear();
            g_object_unref(image);
            return SLOWFRAME_ERR_IMAGE_PROCESS;
        }
        g_object_unref(image);
        image = flat;
    }

    /* Allocate buffer structure */
    ImageBuffer *buf = (ImageBuffer *)malloc(sizeof(ImageBuffer));
    if (!buf) {
        error_log(SLOWFRAME_ERR_MEMORY_ALLOC, "Failed to allocate ImageBuffer structure");
        g_object_unref(image);
        return SLOWFRAME_ERR_MEMORY_ALLOC;
    }

    /* Get metadata */
    buf->width = image->Xsize;
    buf->height = image->Ysize;
    buf->rowstride = VIPS_IMAGE_SIZEOF_LINE(image);

    /* Allocate pixel data */
    int data_size = buf->height * buf->rowstride;
    buf->data = (uint8_t *)malloc(data_size);
    if (!buf->data) {
        error_log(SLOWFRAME_ERR_MEMORY_ALLOC, "Failed to allocate %d bytes for pixel data (image %dx%d, %d bytes/row)", 
                data_size, buf->width, buf->height, buf->rowstride);
        free(buf);
        g_object_unref(image);
        return SLOWFRAME_ERR_MEMORY_ALLOC;
    }

    log_verbose(verbose, timestamp_logging, "   --> Buffering %dx%d RGB image (%d bytes)...\n", buf->width, buf->height, data_size);

    /* Extract pixel data into a densely-packed buffer.
     * vips_image_write_to_memory always produces a flat width×bands×height
     * byte array with no row padding, regardless of how the image was derived
     * (crop, embed, colourspace, etc.).
     *
     * Using VipsRegion + VIPS_REGION_ADDR is NOT safe here: for a lazy derived
     * image such as one produced by vips_crop, the region data is a window
     * into the *parent* image's memory and VIPS_REGION_LSKIP reflects the
     * parent's wider rowstride (e.g. 2880 bytes for a 960-pixel-wide source).
     * Treating that pointer as if it had the crop's logical rowstride (960 bytes
     * for a 320-pixel-wide crop) causes a silent stride mismatch that manifests
     * as a zig-zag / shear corruption in every derived tile.
     */
    size_t written_size = 0;
    void *raw = vips_image_write_to_memory(image, &written_size);
    if (!raw) {
        error_log(SLOWFRAME_ERR_IMAGE_PROCESS,
                  "vips_image_write_to_memory failed: %s", vips_error_buffer());
        vips_error_clear();
        free(buf->data);
        free(buf);
        g_object_unref(image);
        return SLOWFRAME_ERR_IMAGE_PROCESS;
    }

    memcpy(buf->data, raw, data_size);
    g_free(raw);

    log_verbose(verbose, timestamp_logging, "   [OK] Buffer ready: %dx%d RGB\n", buf->width, buf->height);

    /* Update module state */
    if (g_img.buffer) {
        if (g_img.buffer->data) free(g_img.buffer->data);
        free(g_img.buffer);
    }
    g_img.buffer = buf;
    g_img.image = image;

    return SLOWFRAME_OK;
}

/* ============================================================================
   PUBLIC: IMAGE LIFECYCLE
   ============================================================================ */

int image_load_from_file(const char *filename, int verbose, int timestamp_logging, const char *debug_output_dir) {
    if (!filename) {
        error_log(SLOWFRAME_ERR_ARG_FILENAME_INVALID, "Filename pointer is NULL");
        return SLOWFRAME_ERR_ARG_FILENAME_INVALID;
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
        error_log(SLOWFRAME_ERR_IMAGE_LOAD, "Failed to load image: %s (Details: %s)", filename, vips_error_buffer());
        vips_error_clear();
        return SLOWFRAME_ERR_IMAGE_LOAD;
    }

    log_verbose(verbose, timestamp_logging, "   --> Loaded: %dx%d, %d-band image\n", image->Xsize, image->Ysize, image->Bands);

    /* Extract EXIF metadata from loaded image */
    extract_exif_from_image(image, &g_img.exif);
    if (verbose) {
        log_verbose(verbose, timestamp_logging, "   --> EXIF data available: ISO %d, f/%.1f\n", 
                    g_img.exif.iso_speed, g_img.exif.f_stop / 10.0);
    }

    /* Buffer the image (includes RGB conversion) */
    int buffer_result = buffer_vips_image(image, verbose, timestamp_logging);
    if (buffer_result != SLOWFRAME_OK) {
        error_log(buffer_result, "Failed to buffer image data");
        g_object_unref(image);
        return buffer_result;
    }

    log_verbose(verbose, timestamp_logging, "   [OK] Image loaded successfully\n");

    /* Debug: Save loaded image if directory provided */
    if (debug_output_dir) {
        char debug_path[1024];
        snprintf(debug_path, sizeof(debug_path), "%s/01_loaded.png", debug_output_dir);
        if (image_save_to_file(debug_path, 0) == SLOWFRAME_OK && verbose) {
            printf("[DEBUG] Saved loaded image to: %s\n", debug_path);
        }
    }

    return SLOWFRAME_OK;
}

int image_get_dimensions(int *width, int *height) {
    if (!g_img.buffer) {
        error_log(SLOWFRAME_ERR_IMAGE_LOAD, "No image loaded");
        return SLOWFRAME_ERR_IMAGE_LOAD;
    }
    if (width) *width = g_img.buffer->width;
    if (height) *height = g_img.buffer->height;
    return SLOWFRAME_OK;
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

    /* Direct buffer access: always 3 bytes/pixel after alpha strip in buffer_vips_image */
    int bytes_per_pixel = g_img.buffer->rowstride / g_img.buffer->width;
    int offset = (y * g_img.buffer->rowstride) + (x * bytes_per_pixel);
    const uint8_t *pixel = &g_img.buffer->data[offset];

    *r = pixel[0];
    *g = pixel[1];
    *b = pixel[2];
}

const ImageBuffer* image_get_rgb_data(void) {
    return g_img.buffer;
}

const char *image_get_original_extension(void) {
    if (g_img.original_filename[0] == '\0') {
        return "";
    }
    
    /* Extract file extension from original filename */
    const char *dot = strrchr(g_img.original_filename, '.');
    if (!dot || dot == g_img.original_filename) {
        return "";
    }
    return dot;
}

VipsImage* image_get_vips_image(void) {
    return g_img.image;
}

/**
 * image_get_exif_f_stop
 * Return the extracted f-stop value from EXIF
 */
uint8_t image_get_exif_f_stop(void) {
    return g_img.exif.f_stop;
}

/**
 * image_get_exif_iso
 * Return the extracted ISO speed from EXIF
 */
uint16_t image_get_exif_iso(void) {
    return g_img.exif.iso_speed;
}

/**
 * image_get_exif_focal_length
 * Return the extracted focal length from EXIF
 */
uint16_t image_get_exif_focal_length(void) {
    return g_img.exif.focal_length;
}

/**
 * image_get_exif_data
 * Copy the full extracted EXIF data structure to dest
 */
int image_get_exif_data(void *dest) {
    if (!dest) {
        return SLOWFRAME_ERR_ARG_INVALID;
    }
    if (!g_img.buffer) {
        return SLOWFRAME_ERR_IMAGE_LOAD;
    }
    memcpy(dest, &g_img.exif, sizeof(ExifMetadata));
    return SLOWFRAME_OK;
}

/**
 * Embed recovery position markers in the 4 corners of the buffered image.
 * Markers are 5x5 pixels with color-coded position information.
 * Works on the RGB buffer directly (not demand-driven VipsImage).
 */
int image_embed_recovery_markers(int tile_row, int tile_col) {
    if (!g_img.buffer) {
        return SLOWFRAME_ERR_IMAGE_LOAD;
    }

    int w = g_img.buffer->width;
    int h = g_img.buffer->height;
    int corner_size = 5;  /* 5x5 pixel corners for position markers */

    /* Get distinctive color for this position (color-coded by row,col) */
    uint8_t r = (uint8_t)((tile_row + 1) * 80);
    uint8_t g = (uint8_t)((tile_col + 1) * 80);
    uint8_t b = 128;

    uint8_t *data = g_img.buffer->data;
    int rowstride = g_img.buffer->rowstride;
    int bytes_per_pixel = 3;  /* RGB */

    /* Top-left corner */
    for (int y = 0; y < corner_size && y < h; y++) {
        for (int x = 0; x < corner_size && x < w; x++) {
            int offset = y * rowstride + x * bytes_per_pixel;
            data[offset + 0] = r;
            data[offset + 1] = g;
            data[offset + 2] = b;
        }
    }

    /* Top-right corner */
    for (int y = 0; y < corner_size && y < h; y++) {
        for (int x = (w >= corner_size ? w - corner_size : 0); x < w; x++) {
            int offset = y * rowstride + x * bytes_per_pixel;
            data[offset + 0] = r;
            data[offset + 1] = g;
            data[offset + 2] = b;
        }
    }

    /* Bottom-left corner */
    for (int y = (h >= corner_size ? h - corner_size : 0); y < h; y++) {
        for (int x = 0; x < corner_size && x < w; x++) {
            int offset = y * rowstride + x * bytes_per_pixel;
            data[offset + 0] = r;
            data[offset + 1] = g;
            data[offset + 2] = b;
        }
    }

    /* Bottom-right corner */
    for (int y = (h >= corner_size ? h - corner_size : 0); y < h; y++) {
        for (int x = (w >= corner_size ? w - corner_size : 0); x < w; x++) {
            int offset = y * rowstride + x * bytes_per_pixel;
            data[offset + 0] = r;
            data[offset + 1] = g;
            data[offset + 2] = b;
        }
    }

    return SLOWFRAME_OK;
}

void image_free(void) {
    clear_image_state();
}

/* ============================================================================
   PUBLIC: IMAGE TRANSFORMATION
   ============================================================================ */

int image_correct_aspect_and_resize(int target_width, int target_height, AspectMode mode, 
                                    int verbose, int timestamp_logging, const char *debug_output_dir) {
    if (!g_img.buffer || !g_img.image) {
        error_log(SLOWFRAME_ERR_IMAGE_LOAD, "No image loaded");
        return SLOWFRAME_ERR_IMAGE_LOAD;
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
        return SLOWFRAME_OK;
    }

    /* Use modular aspect correction function */
    VipsImage *corrected = NULL;
    int result = image_aspect_correct(g_img.image, target_width, target_height, mode,
                                     &corrected, verbose, timestamp_logging);

    if (result != SLOWFRAME_OK) {
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
    if (buffer_result != SLOWFRAME_OK) {
        error_log(buffer_result, "Failed to buffer corrected image");
        g_object_unref(corrected);
        return buffer_result;
    }

    /* Verify result */
    if (g_img.buffer->width != target_width || g_img.buffer->height != target_height) {
        error_log(SLOWFRAME_ERR_IMAGE_PROCESS, "Correction failed: got %dx%d, expected %dx%d",
                g_img.buffer->width, g_img.buffer->height, target_width, target_height);
        return SLOWFRAME_ERR_IMAGE_PROCESS;
    }

    if (verbose) {
        log_verbose(verbose, timestamp_logging, "   [OK] Image corrected to %dx%d\n", g_img.buffer->width, g_img.buffer->height);
    }

    return SLOWFRAME_OK;
}

/* ============================================================================
   PUBLIC: DEBUG AND INSPECTION
   ============================================================================ */

int image_rotate(int degrees, int verbose, int timestamp_logging) {
    if (!g_img.image) {
        error_log(SLOWFRAME_ERR_IMAGE_LOAD, "No image loaded");
        return SLOWFRAME_ERR_IMAGE_LOAD;
    }

    if (degrees == 0) {
        return SLOWFRAME_OK;
    }

    log_verbose(verbose, timestamp_logging,
                "   Rotating image %+d degrees (clockwise)\n", degrees);

    /* vips_similarity angle is counter-clockwise, so negate for clockwise */
    double angle = -(double)degrees;

    VipsImage *rotated = NULL;
    if (vips_similarity(g_img.image, &rotated, "angle", angle, NULL)) {
        error_log(SLOWFRAME_ERR_IMAGE_PROCESS,
                  "Failed to rotate image: %s", vips_error_buffer());
        vips_error_clear();
        return SLOWFRAME_ERR_IMAGE_PROCESS;
    }

    /* Release old state and buffer the rotated result */
    clear_image_state();

    int buffer_result = buffer_vips_image(rotated, verbose, timestamp_logging);
    if (buffer_result != SLOWFRAME_OK) {
        error_log(buffer_result, "Failed to buffer rotated image");
        g_object_unref(rotated);
        return buffer_result;
    }

    log_verbose(verbose, timestamp_logging,
                "   [OK] Image rotated, new size: %dx%d\n",
                g_img.buffer->width, g_img.buffer->height);

    return SLOWFRAME_OK;
}

/* ============================================================================
   PUBLIC: TILING SUPPORT
   ============================================================================ */

int image_crop_region(int left, int top, int width, int height,
                      int verbose, int timestamp_logging) {
    if (!g_img.image) {
        error_log(SLOWFRAME_ERR_IMAGE_LOAD, "No image loaded for crop");
        return SLOWFRAME_ERR_IMAGE_LOAD;
    }

    log_verbose(verbose, timestamp_logging,
                "   Cropping tile region: left=%d top=%d %dx%d\n",
                left, top, width, height);

    VipsImage *cropped = NULL;
    if (vips_crop(g_img.image, &cropped, left, top, width, height, NULL)) {
        error_log(SLOWFRAME_ERR_IMAGE_PROCESS,
                  "vips_crop failed: %s", vips_error_buffer());
        vips_error_clear();
        return SLOWFRAME_ERR_IMAGE_PROCESS;
    }

    /* Release old state and buffer the cropped region */
    clear_image_state();

    int buffer_result = buffer_vips_image(cropped, verbose, timestamp_logging);
    if (buffer_result != SLOWFRAME_OK) {
        error_log(buffer_result, "Failed to buffer cropped tile");
        g_object_unref(cropped);
        return buffer_result;
    }

    log_verbose(verbose, timestamp_logging,
                "   [OK] Tile cropped: %dx%d\n",
                g_img.buffer->width, g_img.buffer->height);

    return SLOWFRAME_OK;
}

int image_pad_top(int rows, int verbose, int timestamp_logging) {
    if (!g_img.image) {
        error_log(SLOWFRAME_ERR_IMAGE_LOAD, "No image loaded for pad_top");
        return SLOWFRAME_ERR_IMAGE_LOAD;
    }
    if (rows <= 0) return SLOWFRAME_OK;

    int w = vips_image_get_width(g_img.image);
    int h = vips_image_get_height(g_img.image);

    log_verbose(verbose, timestamp_logging,
                "   Padding top: adding %d black rows (%dx%d -> %dx%d)\n",
                rows, w, h, w, h + rows);

    /* Place current image at (0, rows) within a (w x h+rows) black canvas */
    VipsImage *padded = NULL;
    if (vips_embed(g_img.image, &padded, 0, rows, w, h + rows,
                   "extend", VIPS_EXTEND_BLACK, NULL)) {
        error_log(SLOWFRAME_ERR_IMAGE_PROCESS,
                  "vips_embed (pad_top) failed: %s", vips_error_buffer());
        vips_error_clear();
        return SLOWFRAME_ERR_IMAGE_PROCESS;
    }

    clear_image_state();

    int buffer_result = buffer_vips_image(padded, verbose, timestamp_logging);
    if (buffer_result != SLOWFRAME_OK) {
        error_log(buffer_result, "Failed to buffer padded image");
        g_object_unref(padded);
        return buffer_result;
    }

    log_verbose(verbose, timestamp_logging,
                "   [OK] Image padded: %dx%d\n",
                g_img.buffer->width, g_img.buffer->height);

    return SLOWFRAME_OK;
}

/* CRC-8/CCITT (polynomial 0x07) used for tile header rows */
static uint8_t sf_crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 0x80) crc = (uint8_t)((crc << 1) ^ 0x07);
            else            crc = (uint8_t)(crc << 1);
        }
    }
    return crc;
}

int image_apply_tile_header(const SFTileInfo *info,
                            int verbose, int timestamp_logging) {
    if (!g_img.buffer) {
        error_log(SLOWFRAME_ERR_IMAGE_LOAD, "No image loaded for tile header");
        return SLOWFRAME_ERR_IMAGE_LOAD;
    }

    int W = g_img.buffer->width;
    int H = g_img.buffer->height;

    if (H < SF_TILE_HEADER_ROWS) {
        error_log(SLOWFRAME_ERR_IMAGE_PROCESS,
                  "Image height %d < required tile header rows %d",
                  H, SF_TILE_HEADER_ROWS);
        return SLOWFRAME_ERR_IMAGE_PROCESS;
    }

    /* Macroblock width: now divide row into 16 blocks (20px each) for EXIF support.
     * This allows encoding all 14 EXIF metadata fields + tile geometry.
     * Any remainder pixels at right edge use the last block's color. */
    int blk = W / 16;
    if (blk < 1) blk = 1;

    /* ----------------------------------------------------------------
     * Row 0 — Sync: 16 fixed-color blocks repeated R G B Y C M W K
     * These specific primaries/secondaries identify a SlowFrame tile.
     * ---------------------------------------------------------------- */
    static const uint8_t sync[8][3] = {
        {255,   0,   0},  /* R */
        {  0, 255,   0},  /* G */
        {  0,   0, 255},  /* B */
        {255, 255,   0},  /* Y */
        {  0, 255, 255},  /* C */
        {255,   0, 255},  /* M */
        {255, 255, 255},  /* W */
        {  0,   0,   0},  /* K */
    };

    for (int x = 0; x < W; x++) {
        int b = (x / blk < 16) ? (x / blk) : 15;
        uint8_t *px = g_img.buffer->data + (size_t)(0 * g_img.buffer->rowstride + x * 3);
        px[0] = sync[b % 8][0];
        px[1] = sync[b % 8][1];
        px[2] = sync[b % 8][2];
    }

    /* ================================================================
     * Row 1 — Grid / position / basic EXIF (16 bytes)
     *   [0] version       [1] grid_cols     [2] grid_rows     [3] tile_col
     *   [4] tile_row      [5] total_tiles   [6] seq_num       [7] exif_version
     *   [8] f_stop        [9] metering_mode [10] exposure_prog [11] exposure_time_log
     *   [12] focal_len_hi [13] reserved     [14] reserved     [15] CRC-8
     * ================================================================ */
    uint8_t r1[16] = {
        info->version,
        info->grid_cols,
        info->grid_rows,
        info->tile_col,
        info->tile_row,
        info->total_tiles,
        info->seq_num,
        info->exif_version,
        info->f_stop,
        info->metering_mode,
        info->exposure_program,
        info->exposure_time_log,
        (uint8_t)((info->focal_length >> 8) & 0xFF),  /* focal_length hi byte */
        0,  /* reserved */
        0,  /* reserved */
        0   /* CRC placeholder */
    };
    r1[15] = sf_crc8(r1, 15);

    /* ================================================================
     * Row 2 — Image dimensions + session ID + additional EXIF (16 bytes)
     *   [0] orig_width hi     [1] orig_width lo
     *   [2] orig_height hi    [3] orig_height lo
     *   [4..7] session_id bytes (big-endian)
     *   [8] overlap_px        [9] brightness_ev [10] iso_hi    [11] iso_lo
     *   [12] white_balance    [13] color_space  [14] color_profile_id [15] CRC-8
     * ================================================================ */
    uint8_t r2[16] = {
        (uint8_t)((info->orig_width  >> 8) & 0xFF),
        (uint8_t)( info->orig_width        & 0xFF),
        (uint8_t)((info->orig_height >> 8) & 0xFF),
        (uint8_t)( info->orig_height       & 0xFF),
        (uint8_t)((info->session_id >> 24) & 0xFF),
        (uint8_t)((info->session_id >> 16) & 0xFF),
        (uint8_t)((info->session_id >>  8) & 0xFF),
        (uint8_t)( info->session_id        & 0xFF),
        info->overlap_px,
        info->brightness_ev,
        (uint8_t)((info->iso_speed >> 8) & 0xFF),
        (uint8_t)( info->iso_speed       & 0xFF),
        info->white_balance,
        info->color_space,
        info->color_profile_id,
        0   /* CRC placeholder */
    };
    r2[15] = sf_crc8(r2, 15);

    /* ================================================================
     * Row 3 — Device & timestamp info (16 bytes)
     *   [0] device_make_hi     [1] device_make_lo
     *   [2] device_model_hi    [3] device_model_lo
     *   [4] date_year          [5] date_month   [6] date_day
     *   [7] date_hour          [8] date_minute  [9] date_second
     *   [10] focal_len_lo      [11..]  reserved
     * ================================================================ */
    uint8_t r3[16] = {
        (uint8_t)((info->device_make_id >> 8) & 0xFF),
        (uint8_t)( info->device_make_id       & 0xFF),
        (uint8_t)((info->device_model_id >> 8) & 0xFF),
        (uint8_t)( info->device_model_id       & 0xFF),
        info->date_year,
        info->date_month,
        info->date_day,
        info->date_hour,
        info->date_minute,
        info->date_second,
        (uint8_t)(info->focal_length & 0xFF),  /* focal_length lo byte */
        0, 0, 0, 0,
        0   /* CRC placeholder */
    };
    r3[15] = sf_crc8(r3, 15);

    /* Paint data rows 1-3 as grayscale macroblocks */
    const uint8_t *rows[3] = { r1, r2, r3 };
    for (int row_idx = 0; row_idx < 3; row_idx++) {
        const uint8_t *bytes = rows[row_idx];
        int y = 1 + row_idx;
        for (int x = 0; x < W; x++) {
            int b = (x / blk < 16) ? (x / blk) : 15;
            uint8_t v = bytes[b];
            uint8_t *px = g_img.buffer->data +
                          (size_t)(y * g_img.buffer->rowstride + x * 3);
            px[0] = v;
            px[1] = v;
            px[2] = v;
        }
    }

    /* ================================================================
     * Rows 4-7 — Redundant backup copy of rows 0-3.
     * If the primary header is corrupted during transmission (SSTV burst
     * noise, tape dropout, RF fade), the decoder can fall back to this
     * second copy to recover grid geometry, dimensions, and session ID.
     * ================================================================ */
    {
        size_t row_bytes = (size_t)W * 3;
        for (int src_row = 0; src_row < 4; src_row++) {
            const uint8_t *src_p = g_img.buffer->data +
                                   (size_t)(src_row * g_img.buffer->rowstride);
            uint8_t       *dst_p = g_img.buffer->data +
                                   (size_t)((src_row + 4) * g_img.buffer->rowstride);
            memcpy(dst_p, src_p, row_bytes);
        }
    }

    log_verbose(verbose, timestamp_logging,
                "   [OK] Tile header applied: %dx%d grid  tile r%dc%d  seq %d/%d  "
                "session 0x%08X  ISO %d  fnumber %.1f  (16-block EXIF format)\n",
                info->grid_cols, info->grid_rows,
                info->tile_row, info->tile_col,
                info->seq_num, info->total_tiles,
                info->session_id, info->iso_speed, info->f_stop / 10.0);

    return SLOWFRAME_OK;
}

int image_save_to_file(const char *output_path, int verbose) {
    if (!output_path) {
        error_log(SLOWFRAME_ERR_ARG_FILENAME_INVALID, "Output path pointer is NULL");
        return SLOWFRAME_ERR_ARG_FILENAME_INVALID;
    }

    if (!g_img.buffer || !g_img.buffer->data) {
        error_log(SLOWFRAME_ERR_IMAGE_LOAD, "No image loaded");
        return SLOWFRAME_ERR_IMAGE_LOAD;
    }

    /* Construct the output image from the pixel buffer so that in-place
     * modifications (e.g. tile header rows written by image_apply_tile_header)
     * are reflected in the saved file.  vips_image_new_from_memory shares the
     * caller's buffer pointer; the write completes synchronously before we
     * unref, so the pointer remains valid throughout. */
    VipsImage *save_img = vips_image_new_from_memory(
        g_img.buffer->data,
        (size_t)(g_img.buffer->height * g_img.buffer->rowstride),
        g_img.buffer->width,
        g_img.buffer->height,
        3,
        VIPS_FORMAT_UCHAR);
    if (!save_img) {
        error_log(SLOWFRAME_ERR_IMAGE_PROCESS,
                  "Failed to build VipsImage from pixel buffer: %s",
                  vips_error_buffer());
        vips_error_clear();
        return SLOWFRAME_ERR_IMAGE_PROCESS;
    }

    /* Try to preserve EXIF metadata from the original image */
    if (g_img.image) {
        const void *exif_blob = NULL;
        size_t exif_size = 0;
        
        /* Extract EXIF from original image */
        if (!vips_image_get_blob(g_img.image, "exif-data", &exif_blob, &exif_size) &&
            exif_blob && exif_size > 0) {
            /* Copy EXIF blob to the save image
             * Note: vips_image_set_blob requires a free function and takes ownership */
            void *exif_copy = malloc(exif_size);
            if (exif_copy) {
                memcpy(exif_copy, exif_blob, exif_size);
                vips_image_set_blob(save_img, "exif-data", 
                                   (VipsCallbackFn)free, exif_copy, exif_size);
            }
        }
        
        /* Try to copy ICC profile if available */
        const void *icc_blob = NULL;
        size_t icc_size = 0;
        if (!vips_image_get_blob(g_img.image, "icc-profile-data", &icc_blob, &icc_size) &&
            icc_blob && icc_size > 0) {
            void *icc_copy = malloc(icc_size);
            if (icc_copy) {
                memcpy(icc_copy, icc_blob, icc_size);
                vips_image_set_blob(save_img, "icc-profile-data",
                                   (VipsCallbackFn)free, icc_copy, icc_size);
            }
        }
    }

    int write_err = vips_image_write_to_file(save_img, output_path, NULL);
    g_object_unref(save_img);

    if (write_err) {
        error_log(SLOWFRAME_ERR_FILE_WRITE,
                  "Failed to save image to '%s' (Details: %s)",
                  output_path, vips_error_buffer());
        vips_error_clear();
        return SLOWFRAME_ERR_FILE_WRITE;
    }

    if (verbose) {
        printf("[DEBUG] Image saved to: %s\n", output_path);
    }

    return SLOWFRAME_OK;
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
        return SLOWFRAME_OK;  // Skip empty specs
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
        return SLOWFRAME_OK;
    }
    
    if (display_text[0] == '\0') {
        return SLOWFRAME_OK;  // Skip if text is empty
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
        return SLOWFRAME_OK;  // Don't fail, just skip this overlay
    }

    if (!text_image) {
        return SLOWFRAME_OK;
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
        error_log(SLOWFRAME_ERR_IMAGE_PROCESS, 
                  "Failed to composite text overlay: %s", 
                  vips_error_buffer());
        vips_error_clear();
        g_object_unref(text_image);
        if (current_result != g_img.image) {
            g_object_unref(current_result);
        }
        return SLOWFRAME_ERR_IMAGE_PROCESS;
    }

    // Verify compositing succeeded
    if (!composited) {
        error_log(SLOWFRAME_ERR_IMAGE_PROCESS, 
                  "vips_composite2 returned NULL image");
        g_object_unref(text_image);
        if (current_result != g_img.image) {
            g_object_unref(current_result);
        }
        return SLOWFRAME_ERR_IMAGE_PROCESS;
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

    return SLOWFRAME_OK;
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
 * @return SLOWFRAME_OK on success, error code on failure
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
        error_log(SLOWFRAME_ERR_IMAGE_TEXT_OVERLAY, "No overlay specifications provided");
        return SLOWFRAME_ERR_IMAGE_TEXT_OVERLAY;
    }

    if (!g_img.image) {
        error_log(SLOWFRAME_ERR_IMAGE_LOAD, "No image loaded for overlay");
        return SLOWFRAME_ERR_IMAGE_LOAD;
    }

    size_t overlay_count = overlay_spec_list_count((OverlaySpecList *)overlay_specs);
    
    if (overlay_count == 0) {
        return SLOWFRAME_OK;
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
        if (result != SLOWFRAME_OK) {
            log_verbose(verbose, timestamp_logging,
                       "      Warning: Overlay %zu processing\n", i + 1);
        }
    }

    // Re-buffer the modified image
    int result = buffer_vips_image(g_img.image, verbose, timestamp_logging);
    if (result != SLOWFRAME_OK) {
        error_log(result, "Failed to buffer image after overlays");
        return result;
    }

    log_verbose(verbose, timestamp_logging,
               "   [OK] All overlay specifications processed\n");

    return SLOWFRAME_OK;
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
