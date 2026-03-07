/*
 * slowframe_image.h
 * 
 * Image processing module for SlowFrame
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

#ifndef SLOWFRAME_IMAGE_H
#define SLOWFRAME_IMAGE_H

#include <stdint.h>
#include <vips/vips.h>
#include "error.h"
#include "overlay_spec.h"

/* Image buffer structure - stores decoded RGB pixel data */
typedef struct {
    uint8_t *data;      /**< Contiguous RGB pixel data (3 bytes per pixel) */
    int width;          /**< Width in pixels */
    int height;         /**< Height in pixels */
    int rowstride;      /**< Bytes per row (may include padding) */
} ImageBuffer;

/* EXIF metadata extracted from source image */
typedef struct {
    uint8_t f_stop;              /**< F-stop × 10 (28 = f/2.8) */
    uint8_t metering_mode;       /**< 0=unknown, 1=avg, 2=center, 3=spot, etc */
    uint8_t exposure_program;    /**< 0=undef, 1=manual, 2=normal, 3=aperture, etc */
    uint8_t exposure_time_log;   /**< Log2 scale for exposure time */
    uint16_t focal_length;       /**< Focal length in mm */
    uint8_t brightness_ev;       /**< EV with +10 offset */
    uint16_t iso_speed;          /**< ISO speed */
    uint8_t white_balance;       /**< 0=auto, 1=daylight, 2=cloudy, etc */
    uint8_t color_space;         /**< 0=sRGB, 1=Adobe RGB, etc */
    uint8_t color_profile_id;    /**< ICC profile or standard */
    uint16_t device_make_id;     /**< Camera manufacturer hash */
    uint16_t device_model_id;    /**< Camera model hash */
    uint8_t date_year;           /**< Year - 2000 */
    uint8_t date_month;          /**< Month (1-12) */
    uint8_t date_day;            /**< Day (1-31) */
    uint8_t date_hour;           /**< Hour (0-23) */
    uint8_t date_minute;         /**< Minute (0-59) */
    uint8_t date_second;         /**< Second (0-59) */
} ExifMetadata;

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
 * @return Error code: SLOWFRAME_OK on success, or one of:
 *   - SLOWFRAME_ERR_FILE_NOT_FOUND: File does not exist
 *   - SLOWFRAME_ERR_IMAGE_LOAD: libvips failed to load the image
 *   - SLOWFRAME_ERR_IMAGE_FORMAT_UNSUPPORTED: Format not recognized by libvips
 *   - SLOWFRAME_ERR_MEMORY_ALLOC: Failed to allocate buffer for pixel data
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
 * @return SLOWFRAME_OK if image is loaded, SLOWFRAME_ERR_IMAGE_LOAD if no image
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
 * image_get_rgb_data
 * 
 * Get direct access to the loaded image's RGB pixel buffer.
 * This is primarily used by external-encoding modules (e.g., MMSSTV library).
 * 
 * @return Pointer to ImageBuffer structure, or NULL if no image is loaded
 * 
 * Notes:
 * - Pixel data is in row-major format with RGB bytes (R,G,B,R,G,B,...)
 * - Rowstride may differ from width*3 due to alignment padding
 * - Buffer remains valid until image_free() or next image_load_from_file()
 * - Do NOT free the returned pointer or modify the buffer
 */
const ImageBuffer* image_get_rgb_data(void);

/**
 * image_get_vips_image
 * 
 * Get direct access to the current VipsImage object.
 * This is used for advanced image processing operations (e.g., recovery marker embedding).
 * 
 * @return Pointer to VipsImage, or NULL if no image is loaded
 * 
 * Notes:
 * - Do NOT unref or modify the returned image pointer
 * - Image remains valid until image_free() or next image_load_from_file()
 * - The image may be modified by other functions (e.g., image_apply_overlay_list)
 * - For external operations, get the pointer just before use
 */
VipsImage* image_get_vips_image(void);

/**
 * image_get_exif_f_stop
 * 
 * Get the extracted f-stop value from the loaded image's EXIF metadata.
 * 
 * @return F-stop value × 10 (e.g., 28 = f/2.8, 56 = f/5.6)
 *         Returns default 28 (f/2.8) if no EXIF or default if image not loaded
 */
uint8_t image_get_exif_f_stop(void);

/**
 * image_get_exif_iso
 * 
 * Get the extracted ISO speed from the loaded image's EXIF metadata.
 * 
 * @return ISO speed (e.g., 100, 200, 400, 800, 1600, 3200)
 *         Returns default 400 if no EXIF or if image not loaded
 */
uint16_t image_get_exif_iso(void);

/**
 * image_get_exif_focal_length
 * 
 * Get the extracted focal length from the loaded image's EXIF metadata.
 * 
 * @return Focal length in millimeters (e.g., 50 = 50mm)
 *         Returns default 50 if no EXIF or if image not loaded
 */
uint16_t image_get_exif_focal_length(void);

/**
 * image_get_exif_data
 * 
 * Get the full extracted EXIF data structure.
 * This is used by the encoder to populate tile headers with actual metadata.
 * 
 * @param dest Output buffer: pointer to receive EXIF data structure
 * 
 * @return SLOWFRAME_OK on success, SLOWFRAME_ERR_IMAGE_LOAD if no image loaded
 * 
 * Notes:
 * - The returned structure contains all extracted EXIF fields
 * - If EXIF was not found in the image, defaults are returned
 * - Use this function in the encoder when creating SFTileInfo
 */
int image_get_exif_data(void *dest);

/**
 * image_embed_recovery_markers
 * 
 * Embed color-coded position markers in the 4 corners of the buffered image.
 * Each corner gets a 5x5 pixel marker with distinctive colors based on tile position.
 * 
 * Parameters:
 *   tile_row - Row number of the tile (0 = top)
 *   tile_col - Column number of the tile (0 = left)
 * 
 * Returns: SLOWFRAME_OK on success, error code on failure
 */
int image_embed_recovery_markers(int tile_row, int tile_col);

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
 * @return Error code: SLOWFRAME_OK on success, or one of:
 *   - SLOWFRAME_ERR_IMAGE_LOAD: No image loaded
 *   - SLOWFRAME_ERR_IMAGE_PROCESS: libvips transformation failed
 *   - SLOWFRAME_ERR_IMAGE_ASPECT_CORRECTION: Aspect correction failed
 *   - SLOWFRAME_ERR_MEMORY_ALLOC: Failed to allocate for transformed image
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

/**
 * image_rotate
 *
 * Rotate the currently loaded image by the specified number of degrees.
 *
 * @param degrees  Rotation angle in degrees. Positive = clockwise, negative = counterclockwise.
 *                 Valid range: -360 to 360.  0 is a no-op.
 * @param verbose  If non-zero, print diagnostic messages
 * @param timestamp_logging  If non-zero (and verbose is non-zero), add timestamps
 *
 * @return Error code: SLOWFRAME_OK on success, or:
 *   - SLOWFRAME_ERR_IMAGE_LOAD: No image loaded
 *   - SLOWFRAME_ERR_IMAGE_PROCESS: libvips rotation failed
 *   - SLOWFRAME_ERR_MEMORY_ALLOC: Failed to buffer rotated image
 *
 * Notes:
 * - For 90/180/270-degree rotations the output dimensions are swapped (or same)
 *   with no quality loss.
 * - For arbitrary angles the image is embedded in a larger canvas (black fill)
 *   sized to contain the full rotated content; use image_correct_aspect_and_resize()
 *   after to fit the SSTV target resolution.
 */
int image_rotate(int degrees, int verbose, int timestamp_logging);

/* ============================================================================
   TILING SUPPORT (Layer 1: Color-Bar Header Rows)
   ============================================================================ */

/**
 * Number of rows reserved at the top of each tile for structured metadata.
 *
 * Layout (8 rows total):
 *   Rows 0–3  Primary header  (written first, used by default)
 *   Rows 4–7  Backup header   (byte-exact copy of rows 0–3)
 *
 * If burst noise corrupts the primary header, the decoder can fall back to
 * the backup copy to recover grid geometry, dimensions, and session ID.
 */
#define SF_TILE_HEADER_ROWS 8

/** Number of rows in one header block (primary = backup = SF_TILE_HEADER_ROWS/2). */
#define SF_TILE_HEADER_BLOCK 4

/**
 * SFTileInfo
 *
 * Metadata embedded into every tile's header rows.
 * Encoded as 16 grayscale macroblocks per row (20px width), CRC-8 protected.
 *
 * ENCODING:
 * - 320px header width ÷ 16 blocks = 20px per block
 * - Each row holds 16 bytes of data
 * - 3 data rows × 16 bytes = 48 bytes total capacity
 * - Backup rows 4-7 duplicate rows 0-3 for redundancy
 *
 * PRIMARY block (rows 0–3):
 * Row 0  (sync)  : 16 color sync blocks — identifies a SlowFrame tile.
 * Row 1  (grid)  : version | grid_cols | grid_rows | tile_col | tile_row |
 *                  total_tiles | seq_num | exif_version | f_stop |
 *                  metering_mode | exposure_prog | exposure_time | focal_len_hi | [crc]
 * Row 2  (geom)  : orig_width_hi | orig_width_lo | orig_height_hi | orig_height_lo |
 *                  session_id[3] | session_id[2] | session_id[1] | session_id[0] |
 *                  overlap_px | brightness_ev | iso_hi | iso_lo | white_balance |
 *                  color_space | color_profile | [crc]
 * Row 3  (device): device_make_hi | device_make_lo | device_model_hi | device_model_lo |
 *                  date_year | date_month | date_day | date_hour | date_minute |
 *                  date_second | focal_len_lo | [reserved] | [reserved] | [crc]
 *
 * BACKUP block (rows 4–7): byte-exact copy of rows 0–3.
 */
typedef struct {
    /* Tile geometry (original fields) */
    uint8_t  version;            /**< Format version (currently 2) */
    uint8_t  grid_cols;          /**< Total columns in grid */
    uint8_t  grid_rows;          /**< Total rows in grid */
    uint8_t  tile_col;           /**< This tile's column (0-based) */
    uint8_t  tile_row;           /**< This tile's row (0-based) */
    uint8_t  total_tiles;        /**< grid_cols × grid_rows */
    uint8_t  seq_num;            /**< Sequence number (1-based) */
    uint16_t orig_width;         /**< Original image width (px) */
    uint16_t orig_height;        /**< Original image height (px) */
    uint8_t  overlap_px;         /**< Overlap on shared edges */
    uint32_t session_id;         /**< Session identifier */
    
    /* EXIF-like metadata (new fields) */
    uint8_t  exif_version;       /**< EXIF version: 0x23 = 2.3 */
    uint8_t  f_stop;             /**< F-stop × 10 (28 = f/2.8, 56 = f/5.6, etc) */
    uint8_t  metering_mode;      /**< 0=unknown, 1=avg, 2=center, 3=spot, 4=matrix, 5=partial */
    uint8_t  exposure_program;   /**< 0=undef, 1=manual, 2=normal, 3=aperture, 4=shutter, etc */
    uint8_t  exposure_time_log;  /**< Log2 scale: -10 to +10 (−10=1/1000s, 0=1s, +10=1000s) */
    uint16_t focal_length;       /**< Focal length in mm (e.g., 50, 135, 200) */
    uint8_t  brightness_ev;      /**< EV with +10 offset: 0-20 (0 = -10EV, 20 = +10EV) */
    uint16_t iso_speed;          /**< ISO speed (100, 200, 400, 800, 1600, 3200, etc) */
    uint8_t  white_balance;      /**< 0=auto, 1=daylight, 2=cloudy, 3=shade, 4=tungsten, 5=fluorescent, 6=flash */
    uint8_t  color_space;        /**< 0=sRGB, 1=Adobe RGB, 2=ProPhoto RGB */
    uint8_t  color_profile_id;   /**< Hash/ID of ICC profile or standard (0=unspecified) */
    uint16_t device_make_id;     /**< Camera manufacturer hash (1=Canon, 2=Sony, 3=Nikon, etc) */
    uint16_t device_model_id;    /**< Camera model hash */
    uint8_t  date_year;          /**< Year - 2000 (0-99) */
    uint8_t  date_month;         /**< Month (1-12) */
    uint8_t  date_day;           /**< Day (1-31) */
    uint8_t  date_hour;          /**< Hour (0-23) */
    uint8_t  date_minute;        /**< Minute (0-59) */
    uint8_t  date_second;        /**< Second (0-59) */
} SFTileInfo;

/**
 * image_crop_region
 *
 * Replace the currently loaded image with a cropped sub-region of it.
 * The image must already be loaded.  On success the module holds the
 * cropped region as the new current image.
 *
 * @param left   Left edge of crop box in pixels
 * @param top    Top edge of crop box in pixels
 * @param width  Width of crop box in pixels
 * @param height Height of crop box in pixels
 * @param verbose  If non-zero, print diagnostic messages
 * @param timestamp_logging  If non-zero (and verbose non-zero), add timestamps
 *
 * @return SLOWFRAME_OK on success, or:
 *   - SLOWFRAME_ERR_IMAGE_LOAD: No image loaded
 *   - SLOWFRAME_ERR_IMAGE_PROCESS: libvips crop failed
 *   - SLOWFRAME_ERR_MEMORY_ALLOC: Failed to buffer cropped image
 */
int image_crop_region(int left, int top, int width, int height,
                      int verbose, int timestamp_logging);

/**
 * image_pad_top
 *
 * Prepend @rows blank (black) rows at the top of the current image.
 * The image grows from W×H to W×(H+rows); existing pixels shift down.
 *
 * Used before image_apply_tile_header so that header rows overwrite black
 * padding instead of source content, eliminating interior pixel loss.
 *
 * @param rows               Number of blank rows to prepend
 * @param verbose            If non-zero, print diagnostic messages
 * @param timestamp_logging  If non-zero (and verbose non-zero), add timestamps
 *
 * @return SLOWFRAME_OK on success, or:
 *   - SLOWFRAME_ERR_IMAGE_LOAD: No image loaded
 *   - SLOWFRAME_ERR_IMAGE_PROCESS: libvips embed failed
 *   - SLOWFRAME_ERR_MEMORY_ALLOC: Failed to buffer padded image
 */
int image_pad_top(int rows, int verbose, int timestamp_logging);

/**
 * image_apply_tile_header
 *
 * Overwrite the top SF_TILE_HEADER_ROWS rows of the current image with
 * structured tile metadata.
 *
 * The image must be loaded and at least SF_TILE_HEADER_ROWS tall.
 * Row 0 is a fixed RGB/CMY/W/K sync pattern for frame identification.
 * Rows 1-3 contain CRC-8 protected metadata bytes encoded as grayscale
 * macroblocks (macroblock width = image_width / 8).
 *
 * @param info   Pointer to tile metadata to embed
 * @param verbose  If non-zero, print diagnostic messages
 * @param timestamp_logging  If non-zero (and verbose non-zero), add timestamps
 *
 * @return SLOWFRAME_OK on success, or:
 *   - SLOWFRAME_ERR_IMAGE_LOAD: No image loaded
 *   - SLOWFRAME_ERR_IMAGE_PROCESS: Image too small for header rows
 */
int image_apply_tile_header(const SFTileInfo *info,
                            int verbose, int timestamp_logging);


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
 * @return Error code: SLOWFRAME_OK on success, or:
 *   - SLOWFRAME_ERR_FILE_WRITE: Failed to write output file
 *   - SLOWFRAME_ERR_IMAGE_PROCESS: libvips save operation failed
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

/**
 * image_apply_overlay_list
 * 
 * Apply a list of text overlay specifications to the current image.
 * 
 * @param overlay_specs Pointer to OverlaySpecList with specifications
 * @param verbose If non-zero, print debug messages
 * @param timestamp_logging If non-zero (and verbose is non-zero), add timestamps to output
 * 
 * @return Error code: SLOWFRAME_OK on success, or error code on failure
 * 
 * Effects:
 * - Applies all enabled overlays from the list to the current image
 * - Overlays are applied in order (first to last in the list)
 * - Modifies the current image in the pipeline
 * - Returns updated image ready for SSTV encoding
 * 
 * @note This function will be fully implemented in Phase 2.5 (text rendering)
 * @note Currently prepares specifications but does not render overlays
 */
int image_apply_overlay_list(const OverlaySpecList *overlay_specs, 
                            int verbose, int timestamp_logging);

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

#endif /* SLOWFRAME_IMAGE_H */

