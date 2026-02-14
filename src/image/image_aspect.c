/*
 * image_aspect.c
 *
 * Aspect ratio correction module for PiSSTVpp2
 * 
 * Implements three strategies for adapting images to target aspect ratios:
 *
 * 1. CENTER mode (crop-based):
 *    - Calculates the largest centered rectangular region matching target aspect
 *    - Crops image to that region
 *    - Resizes to exact target dimensions if needed
 *    - Preserves maximum detail from original image
 *    - May lose content at edges
 *
 * 2. PAD mode (padding-based):
 *    - Preserves original aspect ratio and all content
 *    - Adds black letterbox/pillarbox padding to reach target dimensions
 *    - Centers image within padded canvas
 *    - No content loss, but may have black bars
 *
 * 3. STRETCH mode (scaling-based):
 *    - Direct non-uniform scaling to exact target dimensions
 *    - No content loss, but may distort image
 *    - Fastest computation, suitable when distortion is acceptable
 *
 * Uses image_processor functions (scale, crop, embed) to perform transformations
 * on VipsImage objects for memory efficiency.
 */

#include "../include/image/image_aspect.h"
#include "../util/error.h"
#include "../include/logging.h"
#include <math.h>
#include <stdio.h>

/* Tolerance for aspect ratio comparison (allows small floating point differences) */
#define ASPECT_TOLERANCE 0.001

/* ============================================================================
   INTERNAL HELPERS
   ============================================================================ */

/**
 * calculate_centered_crop_box
 * 
 * Calculate dimensions and position of largest centered rectangular region
 * that matches target aspect ratio, extracted from source image.
 * 
 * If source is wider than target aspect:
 *   - Crop horizontally (remove left/right)
 *   - height stays same, width shrinks
 * 
 * If source is taller than target aspect:
 *   - Crop vertically (remove top/bottom)
 *   - width stays same, height shrinks
 * 
 * Returns: crop region in (left, top, width, height) format
 */
static void calculate_centered_crop_box(int src_width, int src_height, double target_aspect,
                                       int *out_left, int *out_top, int *out_width, int *out_height) {
    double src_aspect = (double)src_width / src_height;

    if (src_aspect > target_aspect) {
        /* Source too wide relative to target - crop from sides */
        *out_height = src_height;
        *out_width = (int)(src_height * target_aspect + 0.5);
        *out_top = 0;
        *out_left = (src_width - *out_width) / 2;
    } else {
        /* Source too tall relative to target - crop from top/bottom */
        *out_width = src_width;
        *out_height = (int)(src_width / target_aspect + 0.5);
        *out_left = 0;
        *out_top = (src_height - *out_height) / 2;
    }
}

/**
 * calculate_centered_padding
 * 
 * Calculate positioning for centering source image within larger target canvas.
 * Returns offset to place source image centered in canvas with black padding.
 */
static void calculate_centered_padding(int src_width, int src_height, int canvas_width, int canvas_height,
                                      int *out_left, int *out_top) {
    *out_left = (canvas_width - src_width) / 2;
    *out_top = (canvas_height - src_height) / 2;
}

/* ============================================================================
   ASPECT RATIO CORRECTION: CENTER Mode
   ============================================================================ */

/**
 * correct_center_mode
 * 
 * Crop to aspect ratio, then resize to exact target dimensions.
 * Preserves maximum detail from original but loses edge content.
 */
static int correct_center_mode(VipsImage *image, int target_width, int target_height,
                              VipsImage **out_corrected, int verbose, int timestamp_logging) {
    int crop_left, crop_top, crop_width, crop_height;
    double target_aspect = (double)target_width / target_height;

    log_verbose(verbose, timestamp_logging,
               "   --> CENTER mode: crop to aspect, then resize to exact size\n");

    /* Calculate centered crop box matching target aspect ratio */
    calculate_centered_crop_box(image->Xsize, image->Ysize, target_aspect,
                               &crop_left, &crop_top, &crop_width, &crop_height);

    if (verbose) {
        log_verbose(verbose, timestamp_logging,
                   "       Crop calculation: %dx%d → %dx%d at (%d,%d)\n",
                   image->Xsize, image->Ysize, crop_width, crop_height, crop_left, crop_top);
    }

    /* Step 1: Crop to match target aspect ratio */
    VipsImage *cropped = NULL;
    int result = image_processor_crop(image, crop_left, crop_top, crop_width, crop_height,
                                     &cropped, verbose);
    if (result != PISSTVPP2_OK) {
        return result;
    }

    /* Step 2: Resize cropped image to exact target dimensions (if needed) */
    VipsImage *resized = NULL;
    if (cropped->Xsize != target_width || cropped->Ysize != target_height) {
        result = image_processor_scale(cropped, target_width, target_height, &resized, verbose);
        if (result != PISSTVPP2_OK) {
            g_object_unref(cropped);
            return result;
        }
        g_object_unref(cropped);
        *out_corrected = resized;
    } else {
        /* Already at target size after crop */
        *out_corrected = cropped;
    }

    if (verbose) {
        log_verbose(verbose, timestamp_logging,
                   "       Result: %dx%d (cropped to aspect, then resized)\n",
                   (*out_corrected)->Xsize, (*out_corrected)->Ysize);
    }

    return PISSTVPP2_OK;
}

/* ============================================================================
   ASPECT RATIO CORRECTION: PAD Mode
   ============================================================================ */

/**
 * correct_pad_mode
 * 
 * Preserve original aspect ratio by adding black padding to reach target dimensions.
 * No content loss, but results in letterbox/pillarbox bars.
 */
static int correct_pad_mode(VipsImage *image, int target_width, int target_height,
                           VipsImage **out_corrected, int verbose, int timestamp_logging) {
    int pad_left, pad_top;

    log_verbose(verbose, timestamp_logging,
               "   --> PAD mode: preserve aspect ratio, add black padding\n");

    /* Calculate centering offset */
    calculate_centered_padding(image->Xsize, image->Ysize, target_width, target_height,
                              &pad_left, &pad_top);

    if (verbose) {
        log_verbose(verbose, timestamp_logging,
                   "       Padding: source %dx%d → canvas %dx%d at offset (%d,%d)\n",
                   image->Xsize, image->Ysize, target_width, target_height, pad_left, pad_top);
    }

    /* Embed image in larger canvas with black padding */
    VipsImage *padded = NULL;
    int result = image_processor_embed(image, pad_left, pad_top, target_width, target_height,
                                      &padded, verbose);
    if (result != PISSTVPP2_OK) {
        return result;
    }

    *out_corrected = padded;

    if (verbose) {
        log_verbose(verbose, timestamp_logging,
                   "       Result: %dx%d (original centered with black bars)\n",
                   padded->Xsize, padded->Ysize);
    }

    return PISSTVPP2_OK;
}

/* ============================================================================
   ASPECT RATIO CORRECTION: STRETCH Mode
   ============================================================================ */

/**
 * correct_stretch_mode
 * 
 * Non-uniform scaling to exact target dimensions.
 * Fastest but may distort image if aspect ratios differ significantly.
 */
static int correct_stretch_mode(VipsImage *image, int target_width, int target_height,
                               VipsImage **out_corrected, int verbose, int timestamp_logging) {
    log_verbose(verbose, timestamp_logging,
               "   --> STRETCH mode: direct non-uniform resize (may distort)\n");

    VipsImage *resized = NULL;
    int result = image_processor_scale(image, target_width, target_height, &resized, verbose);
    if (result != PISSTVPP2_OK) {
        return result;
    }

    *out_corrected = resized;

    if (verbose) {
        double src_aspect = (double)image->Xsize / image->Ysize;
        double tgt_aspect = (double)target_width / target_height;
        log_verbose(verbose, timestamp_logging,
                   "       WARNING: Aspect change %.3f → %.3f (%.1f%% distortion)\n",
                   src_aspect, tgt_aspect, fabs(src_aspect - tgt_aspect) * 100);
    }

    return PISSTVPP2_OK;
}

/* ============================================================================
   PUBLIC: ASPECT RATIO CORRECTION
   ============================================================================ */

int image_aspect_correct(VipsImage *image, int target_width, int target_height, AspectMode mode,
                        VipsImage **out_corrected, int verbose, int timestamp_logging) {
    if (!image) {
        error_log(PISSTVPP2_ERR_ARG_INVALID, "Cannot correct aspect of NULL VipsImage");
        return PISSTVPP2_ERR_ARG_INVALID;
    }

    if (!out_corrected) {
        error_log(PISSTVPP2_ERR_ARG_INVALID, "Output VipsImage pointer is NULL");
        return PISSTVPP2_ERR_ARG_INVALID;
    }

    if (target_width <= 0 || target_height <= 0) {
        error_log(PISSTVPP2_ERR_IMAGE_DIMENSIONS_INVALID,
                 "Invalid target dimensions for aspect correction: %dx%d",
                 target_width, target_height);
        return PISSTVPP2_ERR_IMAGE_DIMENSIONS_INVALID;
    }

    int img_width = image->Xsize;
    int img_height = image->Ysize;
    double img_aspect = (double)img_width / img_height;
    double target_aspect = (double)target_width / target_height;

    log_verbose(verbose, timestamp_logging,
               "   Aspect ratio correction\n");
    log_verbose(verbose, timestamp_logging,
               "   Source:  %dx%d (aspect %.4f)\n", img_width, img_height, img_aspect);
    log_verbose(verbose, timestamp_logging,
               "   Target:  %dx%d (aspect %.4f)\n", target_width, target_height, target_aspect);
    log_verbose(verbose, timestamp_logging,
               "   Mode:    %s\n", image_aspect_get_name(mode));

    /* Check if correction is needed */
    int size_matches = (img_width == target_width && img_height == target_height);
    int aspect_matches = (fabs(img_aspect - target_aspect) < ASPECT_TOLERANCE);

    if (size_matches && aspect_matches) {
        log_verbose(verbose, timestamp_logging,
                   "   [OK] Image already correct size and aspect - no correction needed\n");
        *out_corrected = image;
        return PISSTVPP2_OK;
    }

    /* Apply correction based on mode */
    int result = PISSTVPP2_OK;
    switch (mode) {
        case ASPECT_CENTER:
            result = correct_center_mode(image, target_width, target_height, out_corrected,
                                        verbose, timestamp_logging);
            break;

        case ASPECT_PAD:
            result = correct_pad_mode(image, target_width, target_height, out_corrected,
                                     verbose, timestamp_logging);
            break;

        case ASPECT_STRETCH:
            result = correct_stretch_mode(image, target_width, target_height, out_corrected,
                                         verbose, timestamp_logging);
            break;

        default:
            error_log(PISSTVPP2_ERR_ARG_ASPECT_MODE_INVALID,
                     "Unknown aspect mode: %d", mode);
            return PISSTVPP2_ERR_ARG_ASPECT_MODE_INVALID;
    }

    if (result != PISSTVPP2_OK) {
        error_log(result, "Aspect ratio correction failed");
        return result;
    }

    /* Verify result dimensions */
    if ((*out_corrected)->Xsize != target_width || (*out_corrected)->Ysize != target_height) {
        error_log(PISSTVPP2_ERR_IMAGE_PROCESS,
                 "Aspect correction produced wrong dimensions: got %dx%d, expected %dx%d",
                 (*out_corrected)->Xsize, (*out_corrected)->Ysize, target_width, target_height);
        if (*out_corrected != image) {
            g_object_unref(*out_corrected);
        }
        return PISSTVPP2_ERR_IMAGE_PROCESS;
    }

    if (verbose) {
        log_verbose(verbose, timestamp_logging,
                   "   [OK] Aspect correction complete: %dx%d\n",
                   (*out_corrected)->Xsize, (*out_corrected)->Ysize);
    }

    return PISSTVPP2_OK;
}

/* ============================================================================
   PUBLIC: UTILITY FUNCTIONS
   ============================================================================ */

const char *image_aspect_get_name(AspectMode mode) {
    switch (mode) {
        case ASPECT_CENTER:  return "CENTER (crop to aspect, resize to fit)";
        case ASPECT_PAD:     return "PAD (add black bars, preserve aspect)";
        case ASPECT_STRETCH: return "STRETCH (non-uniform resize, may distort)";
        default:             return "UNKNOWN";
    }
}

int image_aspect_parse(const char *mode_str, AspectMode *out_mode) {
    if (!mode_str || !out_mode) {
        error_log(PISSTVPP2_ERR_ARG_INVALID, "NULL pointer in aspect mode parsing");
        return PISSTVPP2_ERR_ARG_INVALID;
    }

    if (strcasecmp(mode_str, "center") == 0) {
        *out_mode = ASPECT_CENTER;
        return PISSTVPP2_OK;
    }
    if (strcasecmp(mode_str, "pad") == 0) {
        *out_mode = ASPECT_PAD;
        return PISSTVPP2_OK;
    }
    if (strcasecmp(mode_str, "stretch") == 0) {
        *out_mode = ASPECT_STRETCH;
        return PISSTVPP2_OK;
    }

    error_log(PISSTVPP2_ERR_ARG_ASPECT_MODE_INVALID,
             "Unknown aspect mode: '%s' (valid: center, pad, stretch)", mode_str);
    return PISSTVPP2_ERR_ARG_ASPECT_MODE_INVALID;
}
