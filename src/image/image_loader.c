/*
 * image_loader.c
 *
 * Image loading module for PiSSTVpp2
 * 
 * Handles loading images from files in multiple formats (PNG, JPEG, GIF, BMP, TIFF, WebP, PPM, etc.)
 * using libvips for format detection and decoding. Supports a wide range of image dimensions
 * from small (320x240) to very large (19200x10800) and automatically converts all images to
 * RGB format for consistent downstream processing.
 *
 * Format Support:
 * - PNG: PNG 8-bit RGB, RGBA (with alpha channel)
 * - JPEG: Baseline and progressive, with EXIF metadata support
 * - GIF: Static and animated (loads as single frame)
 * - BMP: Windows bitmap formats
 * - TIFF: Tagged Image File Format
 * - WebP: VP8/VP9 encoded images
 * - PPM/PGM: Netpbm formats (ASCII and binary)
 * - And any other format supported by libvips
 *
 * Size Support:
 * - Minimum: Limited only by VIPS integer constraints
 * - Maximum: Tested up to 19200x10800 pixels
 * - Memory allocation: Scales with image size (3 bytes/pixel for RGB)
 * - Large images are buffered in system RAM
 *
 * Color Space Handling:
 * - Automatic conversion to sRGB (3-band, 8-bit per channel)
 * - Handles grayscale, paletted, and alpha channel inputs
 * - Preserves color fidelity from original format
 *
 * Error Handling:
 * - File not found detection
 * - Format validation with VIPS error messages
 * - Memory allocation failures with size context
 * - Region preparation errors
 * - All errors use standardized PISSTVPP2_ERR_* codes
 */

#include "../include/image/image_loader.h"
#include "../util/error.h"
#include "../include/logging.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* ============================================================================
   INTERNAL STATE
   ============================================================================ */

/**
 * LoaderState
 * Encapsulates per-loader instance state
 */
typedef struct {
    VipsImage *image;                  /* Current VipsImage loaded from disk */
    ImageBuffer buffer;                /* Pixel data buffer (struct, not pointer) */
    char original_filename[1024];      /* Original image filename for extension extraction */
    int initialized;                   /* State validity flag */
} LoaderState;

/**
 * Global loader state
 * Used by legacy compatibility functions; modular code should use loaders directly
 */
static LoaderState g_loader = {
    .image = NULL,
    .buffer = {.data = NULL, .width = 0, .height = 0, .rowstride = 0},
    .original_filename = {0},
    .initialized = 0
};

/* ============================================================================
   INTERNAL HELPERS: File Validation
   ============================================================================ */

/**
 * file_exists
 * Check if a file exists and is readable
 * 
 * @param filename Path to check
 * @return 1 if file exists and is readable, 0 otherwise
 */
static int file_exists(const char *filename) {
    if (!filename || filename[0] == '\0') {
        return 0;
    }
    
    /* Use access() to check if file is readable */
    if (access(filename, R_OK) == 0) {
        return 1;
    }
    
    return 0;
}

/**
 * get_file_extension
 * Extract file extension from a filename path
 * 
 * @param filename Path with extension
 * @return Pointer to extension (including dot), or empty string if none
 *
 * Examples:
 *   "image.png"     -> ".png"
 *   "photo.jpg"     -> ".jpg"
 *   "noext"         -> ""
 *   "/path/img.gif" -> ".gif"
 */
static const char *get_file_extension(const char *filename) {
    if (!filename) return "";
    
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    
    return dot;
}

/* ============================================================================
   INTERNAL HELPERS: Buffer Management
   ============================================================================ */

/**
 * buffer_vips_image_internal
 * Convert a VipsImage to internal pixel buffer
 *
 * This is the core buffering operation. It:
 * 1. Ensures RGB colorspace conversion (sRGB, 3 bands, 8-bit)
 * 2. Allocates ImageBuffer structure
 * 3. Allocates pixel data array
 * 4. Uses VipsRegion to safely extract pixel data
 * 5. Handles memory allocation failures
 * 6. Tracks image metadata (width, height, rowstride)
 *
 * @param image VipsImage to buffer (ownership NOT transferred)
 * @param verbose Enable diagnostic output
 * @param timestamp_logging Add timestamps to output
 * @return Allocated ImageBuffer, or NULL on failure (with error logged)
 *
 * Note: Caller must free returned buffer with image_loader_free_buffer()
 */
static ImageBuffer *buffer_vips_image_internal(VipsImage *image, int verbose, int timestamp_logging) {
    if (!image) {
        error_log(PISSTVPP2_ERR_IMAGE_LOAD, "Cannot buffer NULL VipsImage");
        return NULL;
    }

    /* Ensure image is in RGB format (colorspace conversion) */
    VipsImage *rgb_image = NULL;
    
    if (vips_colourspace(image, &rgb_image, VIPS_INTERPRETATION_sRGB, NULL)) {
        error_log(PISSTVPP2_ERR_IMAGE_PROCESS,
                 "Failed to convert image colorspace to sRGB: %s", vips_error_buffer());
        vips_error_clear();
        return NULL;
    }

    /* If colorspace conversion created a new image, use it */
    if (rgb_image != image) {
        image = rgb_image;
    }

    log_verbose(verbose, timestamp_logging,
               "   --> Buffering %dx%d RGB image (%d bands, 8-bit)...\n",
               image->Xsize, image->Ysize, image->Bands);

    /* Allocate buffer structure */
    ImageBuffer *buf = (ImageBuffer *)malloc(sizeof(ImageBuffer));
    if (!buf) {
        error_log(PISSTVPP2_ERR_MEMORY_ALLOC, "Failed to allocate ImageBuffer structure");
        if (rgb_image != image) g_object_unref(image);
        return NULL;
    }

    /* Store metadata */
    buf->width = image->Xsize;
    buf->height = image->Ysize;
    buf->rowstride = VIPS_IMAGE_SIZEOF_LINE(image);

    /* Calculate and allocate pixel data */
    int data_size = buf->height * buf->rowstride;
    log_verbose(verbose, timestamp_logging,
               "   --> Allocating %d bytes (%dx%d * %d bytes/row)...\n",
               data_size, buf->width, buf->height, buf->rowstride);

    buf->data = (uint8_t *)malloc(data_size);
    if (!buf->data) {
        error_log(PISSTVPP2_ERR_MEMORY_ALLOC,
                 "Failed to allocate %d bytes for pixel data (image %dx%d, %d bytes/row)",
                 data_size, buf->width, buf->height, buf->rowstride);
        free(buf);
        if (rgb_image != image) g_object_unref(image);
        return NULL;
    }

    /* Create VipsRegion to safely access pixel data */
    VipsRegion *region = vips_region_new(image);
    if (!region) {
        error_log(PISSTVPP2_ERR_IMAGE_PROCESS,
                 "Failed to create VipsRegion for buffering image");
        free(buf->data);
        free(buf);
        if (rgb_image != image) g_object_unref(image);
        return NULL;
    }

    /* Prepare the region for the entire image */
    VipsRect rect = {0, 0, image->Xsize, image->Ysize};
    if (vips_region_prepare(region, &rect)) {
        error_log(PISSTVPP2_ERR_IMAGE_PROCESS,
                 "Failed to prepare VipsRegion for buffering: %s", vips_error_buffer());
        vips_error_clear();
        g_object_unref(region);
        free(buf->data);
        free(buf);
        if (rgb_image != image) g_object_unref(image);
        return NULL;
    }

    /* Copy pixel data from region to buffer */
    const uint8_t *src = VIPS_REGION_ADDR(region, 0, 0);
    if (!src) {
        error_log(PISSTVPP2_ERR_IMAGE_PROCESS,
                 "Failed to access pixel data from VipsRegion (region may be out of bounds)");
        g_object_unref(region);
        free(buf->data);
        free(buf);
        if (rgb_image != image) g_object_unref(image);
        return NULL;
    }

    memcpy(buf->data, src, data_size);
    g_object_unref(region);

    log_verbose(verbose, timestamp_logging,
               "   [OK] Buffer ready: %dx%d RGB (%d bytes total)\n",
               buf->width, buf->height, data_size);

    if (rgb_image != image) {
        g_object_unref(image);
    }

    return buf;
}

/**
 * clear_state
 * Release VipsImage and buffer from global state, maintain initialized flag
 */
static void clear_state(void) {
    if (g_loader.image) {
        g_object_unref(g_loader.image);
        g_loader.image = NULL;
    }
    if (g_loader.buffer.data) {
        free(g_loader.buffer.data);
        g_loader.buffer.data = NULL;
        g_loader.buffer.width = 0;
        g_loader.buffer.height = 0;
        g_loader.buffer.rowstride = 0;
    }
}

/* ============================================================================
   PUBLIC: IMAGE LOADING
   ============================================================================ */

int image_loader_load_image(const char *filename, int verbose, int timestamp_logging,
                           const char *debug_output_dir) {
    if (!filename) {
        error_log(PISSTVPP2_ERR_ARG_FILENAME_INVALID, "Filename pointer is NULL");
        return PISSTVPP2_ERR_ARG_FILENAME_INVALID;
    }

    log_verbose(verbose, timestamp_logging, "   Loading image from: %s\n", filename);

    /* Check if file exists before attempting to load */
    if (!file_exists(filename)) {
        error_log(PISSTVPP2_ERR_FILE_NOT_FOUND,
                 "Image file not found or not readable: %s (errno: %d)",
                 filename, errno);
        return PISSTVPP2_ERR_FILE_NOT_FOUND;
    }

    /* Clear any existing state */
    clear_state();

    /* Store original filename for later extension extraction */
    strncpy(g_loader.original_filename, filename, sizeof(g_loader.original_filename) - 1);
    g_loader.original_filename[sizeof(g_loader.original_filename) - 1] = '\0';

    /* Load image with libvips auto-format detection */
    log_verbose(verbose, timestamp_logging, "   --> Detecting format with libvips...\n");
    
    VipsImage *image = vips_image_new_from_file(filename, NULL);
    if (!image) {
        error_log(PISSTVPP2_ERR_IMAGE_LOAD,
                 "Failed to load image: %s (Details: %s)",
                 filename, vips_error_buffer());
        vips_error_clear();
        return PISSTVPP2_ERR_IMAGE_LOAD;
    }

    log_verbose(verbose, timestamp_logging,
               "   --> Loaded: %dx%d, %d-band image\n",
               image->Xsize, image->Ysize, image->Bands);

    /* Validate image dimensions against reasonable limits */
    if (image->Xsize <= 0 || image->Ysize <= 0) {
        error_log(PISSTVPP2_ERR_IMAGE_DIMENSIONS_INVALID,
                 "Invalid image dimensions: %dx%d",
                 image->Xsize, image->Ysize);
        g_object_unref(image);
        return PISSTVPP2_ERR_IMAGE_DIMENSIONS_INVALID;
    }

    /* Buffer the image (includes RGB conversion) */
    ImageBuffer *buf = buffer_vips_image_internal(image, verbose, timestamp_logging);
    if (!buf) {
        /* Error already logged by buffer function */
        g_object_unref(image);
        return PISSTVPP2_ERR_IMAGE_PROCESS;
    }

    /* Update global state with successfully loaded image */
    g_loader.image = image;
    g_loader.buffer = *buf;
    g_loader.initialized = 1;
    free(buf);  /* We copied the struct, so free the pointer */

    log_verbose(verbose, timestamp_logging, "   [OK] Image loaded successfully\n");

    /* Debug: Save loaded image if directory provided */
    if (debug_output_dir && debug_output_dir[0] != '\0') {
        char debug_path[1024];
        snprintf(debug_path, sizeof(debug_path), "%s/01_loaded.png", debug_output_dir);
        if (vips_image_write_to_file(g_loader.image, debug_path, NULL) == 0) {
            if (verbose) {
                printf("[DEBUG] Saved loaded image to: %s\n", debug_path);
            }
        } else {
            log_verbose(verbose, 0, "   [WARNING] Failed to save debug image: %s\n", debug_path);
        }
    }

    return PISSTVPP2_OK;
}

/* ============================================================================
   PUBLIC: BUFFER MANAGEMENT
   ============================================================================ */

const ImageBuffer *image_loader_get_buffer(void) {
    if (!g_loader.initialized || !g_loader.buffer.data) {
        return NULL;
    }
    return &g_loader.buffer;
}

void image_loader_free_buffer(void) {
    clear_state();
    g_loader.initialized = 0;
}

int image_loader_get_dimensions(int *width, int *height) {
    if (!g_loader.initialized || !g_loader.buffer.data) {
        error_log(PISSTVPP2_ERR_IMAGE_LOAD, "No image loaded");
        return PISSTVPP2_ERR_IMAGE_LOAD;
    }
    
    if (width) *width = g_loader.buffer.width;
    if (height) *height = g_loader.buffer.height;
    return PISSTVPP2_OK;
}

void image_loader_get_pixel_rgb(int x, int y, uint8_t *r, uint8_t *g, uint8_t *b) {
    /* Default to black on error */
    *r = *g = *b = 0;

    if (!g_loader.initialized || !g_loader.buffer.data) {
        return;
    }

    /* Bounds check */
    if (x < 0 || x >= g_loader.buffer.width || y < 0 || y >= g_loader.buffer.height) {
        return;
    }

    /* Direct buffer access: RGB format, 3 bytes per pixel */
    int offset = (y * g_loader.buffer.rowstride) + (x * 3);
    const uint8_t *pixel = &g_loader.buffer.data[offset];

    *r = pixel[0];
    *g = pixel[1];
    *b = pixel[2];
}

const char *image_loader_get_original_extension(void) {
    if (!g_loader.initialized || g_loader.original_filename[0] == '\0') {
        return "";
    }
    return get_file_extension(g_loader.original_filename);
}

/* ============================================================================
   PUBLIC: VIPS IMAGE ACCESS (Advanced)
   ============================================================================ */

VipsImage *image_loader_get_vips_image(void) {
    if (!g_loader.initialized) {
        return NULL;
    }
    return g_loader.image;
}

ImageBuffer *image_loader_vips_to_buffer(VipsImage *image, int verbose, int timestamp_logging) {
    if (!image) {
        error_log(PISSTVPP2_ERR_ARG_INVALID, "Cannot buffer NULL VipsImage");
        return NULL;
    }

    return buffer_vips_image_internal(image, verbose, timestamp_logging);
}

/* ============================================================================
   PUBLIC: DIAGNOSTICS
   ============================================================================ */

void image_loader_print_diagnostics(void) {
    if (!g_loader.initialized || !g_loader.buffer.data) {
        printf("[IMAGE_LOADER] No image loaded\n");
        return;
    }

    double aspect = (double)g_loader.buffer.width / g_loader.buffer.height;
    printf("[IMAGE_LOADER] Original file: %s\n", g_loader.original_filename);
    printf("[IMAGE_LOADER] Dimensions: %dx%d pixels\n", g_loader.buffer.width, g_loader.buffer.height);
    printf("[IMAGE_LOADER] Aspect ratio: %.3f (%.1f:%.1f)\n", aspect,
           (double)g_loader.buffer.width, (double)g_loader.buffer.height);
    printf("[IMAGE_LOADER] Rowstride: %d bytes\n", g_loader.buffer.rowstride);
    printf("[IMAGE_LOADER] Total buffer size: %d bytes\n",
           g_loader.buffer.height * g_loader.buffer.rowstride);
}

/* ============================================================================
   INITIALIZATION (Required for VIPS)
   ============================================================================ */

/** 
 * image_loader_init
 * Initialize VIPS system (called once at startup)
 * Must be called before any VIPS operations
 */
void image_loader_init(void) {
    if (VIPS_INIT("pisstvpp2")) {
        vips_error_exit("Unable to start VIPS");
    }
    g_loader.initialized = 0;  /* No image loaded yet, but VIPS is ready */
}

/**
 * image_loader_shutdown
 * Clean up VIPS and module resources (called at exit)
 */
void image_loader_shutdown(void) {
    clear_state();
    vips_shutdown();
    g_loader.initialized = 0;
}
