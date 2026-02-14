/**
 * @file overlay_spec.h
 * @brief Flexible Text Overlay Specification System
 *
 * This module provides a comprehensive, extensible system for defining text
 * overlays with full control over:
 * - Text content (arbitrary string, fixed length)
 * - Placement (top, bottom, left, right, center)
 * - Text color (RGB)
 * - Font size (pixels)
 * - Background color (RGB, with transparency option)
 * - Text alignment (left, center, right)
 * - Padding and margins
 *
 * Multiple overlays can be specified and applied in sequence to create
 * complex text compositions on images.
 *
 * ## Usage Example
 * @code
 * // Create a list to hold overlay specifications
 * OverlaySpecList overlay_list;
 * overlay_spec_list_init(&overlay_list, 10);  // Max 10 overlays
 *
 * // Add first overlay: station ID at top
 * TextOverlaySpec overlay1 = {
 *     .text = "W5ZZZ EM12ab",
 *     .placement = OVERLAY_PLACE_TOP,
 *     .text_color = {255, 255, 255},      // White text
 *     .bg_color = {0, 0, 0},              // Black background
 *     .font_size = 12,
 *     .text_align = TEXT_ALIGN_CENTER,
 *     .padding = 4
 * };
 * overlay_spec_list_add(&overlay_list, &overlay1);
 *
 * // Add second overlay: timestamp at bottom
 * TextOverlaySpec overlay2 = {
 *     .text = "2026-02-11 17:30 UTC",
 *     .placement = OVERLAY_PLACE_BOTTOM,
 *     .text_color = {200, 200, 200},      // Gray text
 *     .bg_color = {50, 50, 50},
 *     .font_size = 10,
 *     .text_align = TEXT_ALIGN_RIGHT,
 *     .padding = 2
 * };
 * overlay_spec_list_add(&overlay_list, &overlay2);
 *
 * // Apply all overlays to image
 * image_apply_overlay_list(&overlay_list, image);
 * @endcode
 */

#ifndef OVERLAY_SPEC_H
#define OVERLAY_SPEC_H

#include <stdint.h>
#include <stddef.h>

// ===========================================================================
// CONSTANTS
// ===========================================================================

/** Maximum length of overlay text (including null terminator) */
#define OVERLAY_MAX_TEXT_LENGTH 256

/** Maximum number of overlays in a list */
#define OVERLAY_MAX_COUNT 20

/** Background bar width modes */
#define BGBAR_WIDTH_AUTO 0      /**< Width auto-sized to text + padding */
#define BGBAR_WIDTH_FULL 1      /**< Bar spans entire image width */
#define BGBAR_WIDTH_HALF 2      /**< Bar spans half image width */
#define BGBAR_WIDTH_FIXED 3     /**< Bar spans custom pixel width */

/** Background bar orientation */
#define BGBAR_ORIENT_HORIZONTAL 0  /**< Horizontal bar (text reads normally) */
#define BGBAR_ORIENT_VERTICAL 1    /**< Vertical bar (text rotated 90 degrees) */
#define BGBAR_ORIENT_VERTICAL_STACKED 2  /**< Vertical bar (text stacked top-to-bottom, letters normal) */

// ===========================================================================
// ENUMERATIONS
// ===========================================================================

/**
 * @brief Text overlay placement on image
 */
typedef enum {
    OVERLAY_PLACE_TOP = 0,      /**< Top edge of image */
    OVERLAY_PLACE_BOTTOM = 1,   /**< Bottom edge of image */
    OVERLAY_PLACE_LEFT = 2,     /**< Left edge of image */
    OVERLAY_PLACE_RIGHT = 3,    /**< Right edge of image */
    OVERLAY_PLACE_CENTER = 4,   /**< Center of image (overlay box) */
    OVERLAY_PLACE_TOP_LEFT = 5,     /**< Top-left corner */
    OVERLAY_PLACE_TOP_RIGHT = 6,    /**< Top-right corner */
    OVERLAY_PLACE_BOTTOM_LEFT = 7,  /**< Bottom-left corner */
    OVERLAY_PLACE_BOTTOM_RIGHT = 8  /**< Bottom-right corner */
} OverlayPlacement;

/**
 * @brief Text alignment within overlay box
 */
typedef enum {
    TEXT_ALIGN_LEFT = 0,    /**< Left-aligned text */
    TEXT_ALIGN_CENTER = 1,  /**< Center-aligned text */
    TEXT_ALIGN_RIGHT = 2    /**< Right-aligned text */
} TextAlignment;

/**
 * @brief Vertical alignment within overlay box
 */
typedef enum {
    VALIGN_TOP = 0,      /**< Top-aligned */
    VALIGN_CENTER = 1,   /**< Center-aligned */
    VALIGN_BOTTOM = 2    /**< Bottom-aligned */
} VerticalAlignment;

/**
 * @brief Background rendering mode
 */
typedef enum {
    BG_OPAQUE = 0,      /**< Solid background (fully opaque) */
    BG_TRANSPARENT = 1, /**< Transparent background (no fill) */
    BG_SEMI = 2         /**< Semi-transparent background */
} BackgroundMode;

// ===========================================================================
// COLOR STRUCTURES
// ===========================================================================

/**
 * @brief RGB color with optional alpha channel
 * Used for text color and background color specifications
 */
typedef struct {
    uint8_t r;      /**< Red channel (0-255) */
    uint8_t g;      /**< Green channel (0-255) */
    uint8_t b;      /**< Blue channel (0-255) */
    uint8_t alpha;  /**< Alpha channel: 0=transparent, 255=opaque */
} RGBAColor;

// ===========================================================================
// TEXT OVERLAY SPECIFICATION
// ===========================================================================

/**
 * @brief Complete specification for a single text overlay
 *
 * This structure fully defines how a text overlay should be rendered on an
 * image, including position, styling, colors, and sizing.
 */
typedef struct {
    // Text content
    char text[OVERLAY_MAX_TEXT_LENGTH];     /**< Text to display (null-terminated) */
    char timestamp_format[128];             /**< Timestamp format (strftime format, empty = disabled) */
    
    // Positioning and placement
    OverlayPlacement placement;             /**< Where to place the overlay */
    uint16_t offset_x;                      /**< Horizontal offset from placement edge (pixels) */
    uint16_t offset_y;                      /**< Vertical offset from placement edge (pixels) */
    
    // Text styling
    uint16_t font_size;                     /**< Font size in pixels (typically 8-32) */
    TextAlignment text_align;               /**< Horizontal text alignment */
    VerticalAlignment valign;               /**< Vertical text alignment */
    RGBAColor text_color;                   /**< Text/foreground color */
    
    // Background styling
    BackgroundMode bg_mode;                 /**< How to render background */
    RGBAColor bg_color;                     /**< Background fill color */
    uint16_t padding;                       /**< Padding around text (pixels) */
    uint16_t border_width;                  /**< Border width (0 = no border) */
    RGBAColor border_color;                 /**< Border color (if border_width > 0) */
    
    // Background bar styling (solid color bar behind text for visibility)
    int bg_bar_enable;                      /**< 1 to render solid color bar behind text, 0 to disable */
    uint16_t bg_bar_margin;                 /**< Extra margin around bar beyond padding (0-30 pixels) */
    int bg_bar_width_mode;                  /**< 0=auto(text), 1=full(image), 2=half(image), 3=fixed(pixels) */
    uint16_t bg_bar_custom_width;           /**< Custom width in pixels when bg_bar_width_mode==3 */
    int bg_bar_orientation;                 /**< 0=horizontal (default), 1=vertical (spans image height) */
    RGBAColor bg_bar_color;                 /**< Bar background color (use when bg_mode is transparent) */
    uint16_t min_width;                     /**< Minimum overlay box width (0 = auto) */
    uint16_t max_width;                     /**< Maximum overlay box width (0 = unlimited) */
    uint16_t min_height;                    /**< Minimum overlay box height (0 = auto) */
    uint16_t max_height;                    /**< Maximum overlay box height (0 = unlimited) */
    
    // Effects and attributes
    int enabled;                            /**< 1 if overlay should be applied, 0 to skip */
    int rotation;                           /**< Rotation angle in degrees (0, 90, 180, 270) */
    
} TextOverlaySpec;

// ===========================================================================
// OVERLAY LIST (COLLECTION OF SPECS)
// ===========================================================================

/**
 * @brief Collection of text overlay specifications
 *
 * Holds multiple overlay specifications that can be applied sequentially
 * to an image. Useful for creating complex text compositions.
 */
typedef struct {
    TextOverlaySpec *overlays;      /**< Array of overlay specifications */
    size_t count;                   /**< Number of overlays currently in list */
    size_t capacity;                /**< Maximum capacity of the list */
} OverlaySpecList;

// ===========================================================================
// PUBLIC FUNCTION DECLARATIONS
// ===========================================================================

/**
 * @brief Initialize an empty overlay specification list
 *
 * @param list Pointer to OverlaySpecList to initialize
 * @param capacity Maximum number of overlays to support
 *
 * @return PISSTVPP2_OK on success, error code on failure
 *
 * @note Must be called before adding overlays
 * @note Call overlay_spec_list_cleanup() when done
 */
int overlay_spec_list_init(OverlaySpecList *list, size_t capacity);

/**
 * @brief Add an overlay specification to the list
 *
 * @param list Pointer to initialized OverlaySpecList
 * @param spec Pointer to TextOverlaySpec to add
 *
 * @return PISSTVPP2_OK on success, error code if list is full
 *
 * @note Makes a copy of the spec, does not store pointer
 */
int overlay_spec_list_add(OverlaySpecList *list, const TextOverlaySpec *spec);

/**
 * @brief Get an overlay specification from the list
 *
 * @param list Pointer to OverlaySpecList
 * @param index Index of overlay (0-based)
 *
 * @return Pointer to TextOverlaySpec at index, or NULL if invalid index
 */
TextOverlaySpec* overlay_spec_list_get(OverlaySpecList *list, size_t index);

/**
 * @brief Remove an overlay specification from the list
 *
 * @param list Pointer to OverlaySpecList
 * @param index Index of overlay to remove (0-based)
 *
 * @return PISSTVPP2_OK on success, error code if invalid index
 */
int overlay_spec_list_remove(OverlaySpecList *list, size_t index);

/**
 * @brief Get the number of overlays in the list
 *
 * @param list Pointer to OverlaySpecList
 *
 * @return Number of overlays in the list
 */
size_t overlay_spec_list_count(const OverlaySpecList *list);

/**
 * @brief Clear all overlays from the list
 *
 * @param list Pointer to OverlaySpecList
 */
void overlay_spec_list_clear(OverlaySpecList *list);

/**
 * @brief Free resources allocated for the list
 *
 * @param list Pointer to OverlaySpecList
 *
 * @note Must be called when list is no longer needed
 * @note List pointer should not be used after this call
 */
void overlay_spec_list_cleanup(OverlaySpecList *list);

/**
 * @brief Create a default overlay specification
 *
 * @return TextOverlaySpec with sensible defaults
 *
 * Defaults:
 * - Text: empty
 * - Placement: TOP
 * - Font size: 12 pixels
 * - Text color: white (255, 255, 255, 255)
 * - Background: black (0, 0, 0, 255)
 * - Padding: 4 pixels
 * - Text alignment: CENTER
 * - Enabled: 1 (true)
 */
TextOverlaySpec overlay_spec_create_default(void);

/**
 * @brief Create a station ID overlay specification
 *
 * Convenience function to create a standard FCC station ID overlay
 * with callsign and grid square.
 *
 * @param callsign Amateur radio callsign (e.g., "W5ZZZ")
 * @param grid_square Grid square locator (e.g., "EM12ab")
 * @param placement Where to place the station ID
 *
 * @return TextOverlaySpec configured for station ID display
 */
TextOverlaySpec overlay_spec_create_station_id(const char *callsign, 
                                                const char *grid_square,
                                                OverlayPlacement placement);

/**
 * @brief Parse placement string to enum
 *
 * @param placement_str String like "top", "bottom", "center", etc.
 *
 * @return OverlayPlacement enum value, or OVERLAY_PLACE_TOP if unrecognized
 */
OverlayPlacement overlay_parse_placement(const char *placement_str);

/**
 * @brief Parse alignment string to enum
 *
 * @param align_str String like "left", "center", "right"
 *
 * @return TextAlignment enum value, or TEXT_ALIGN_CENTER if unrecognized
 */
TextAlignment overlay_parse_alignment(const char *align_str);

/**
 * @brief Parse color string (hex or named color)
 *
 * @param color_str Hex string like "FF0000" or named color like "red"
 * @param out_color Output: parsed color with alpha=255
 *
 * @return PISSTVPP2_OK on success, error code on failure
 */
int overlay_parse_color(const char *color_str, RGBAColor *out_color);

// ===========================================================================
// COLOR BAR SPECIFICATIONS (For visual separation of overlay areas)
// ===========================================================================

/**
 * @brief Position of a color bar band
 */
typedef enum {
    COLORBAR_POSITION_TOP = 0,      /**< Top edge of image */
    COLORBAR_POSITION_BOTTOM = 1,   /**< Bottom edge of image */
    COLORBAR_POSITION_CUSTOM = 2    /**< Custom vertical position (pixel offset) */
} ColorBarPosition;

/**
 * @brief Maximum number of color segments in a single color bar
 */
#define COLORBAR_MAX_COLORS 8

/**
 * @brief Single color bar specification
 *
 * Represents a colored band/stripe on the image, useful for visually
 * separating or highlighting overlay areas. Can have multiple colors
 * for striped/gradient effects.
 */
typedef struct {
    ColorBarPosition position;                          /**< Where to place the bar */
    uint16_t custom_offset;                             /**< Pixel offset if position == COLORBAR_POSITION_CUSTOM */
    uint16_t height;                                    /**< Height of color bar in pixels */
    RGBAColor colors[COLORBAR_MAX_COLORS];              /**< Array of colors for multi-color bars */
    uint8_t num_colors;                                 /**< Number of actual colors (1 to COLORBAR_MAX_COLORS) */
    int enabled;                                        /**< 1 if bar should be applied, 0 to skip */
} ColorBar;

/**
 * @brief Collection of color bar specifications
 */
typedef struct {
    ColorBar *bars;                 /**< Array of color bar specifications */
    size_t count;                   /**< Number of bars currently in list */
    size_t capacity;                /**< Maximum capacity of the list */
} ColorBarList;

/**
 * @brief Initialize an empty color bar list
 *
 * @param list Pointer to ColorBarList to initialize
 * @param capacity Maximum number of color bars to support
 *
 * @return PISSTVPP2_OK on success, error code on failure
 *
 * @note Must be called before adding color bars
 * @note Call colorbar_list_cleanup() when done
 */
int colorbar_list_init(ColorBarList *list, size_t capacity);

/**
 * @brief Add a color bar specification to the list
 *
 * @param list Pointer to initialized ColorBarList
 * @param bar Pointer to ColorBar to add
 *
 * @return PISSTVPP2_OK on success, error code if list is full
 *
 * @note Makes a copy of the bar, does not store pointer
 */
int colorbar_list_add(ColorBarList *list, const ColorBar *bar);

/**
 * @brief Get a color bar specification from the list
 *
 * @param list Pointer to ColorBarList
 * @param index Index of color bar (0-based)
 *
 * @return Pointer to ColorBar at index, or NULL if invalid index
 */
ColorBar* colorbar_list_get(ColorBarList *list, size_t index);

/**
 * @brief Get the number of color bars in the list
 *
 * @param list Pointer to ColorBarList
 *
 * @return Number of color bars in the list
 */
size_t colorbar_list_count(const ColorBarList *list);

/**
 * @brief Clear all color bars from the list
 *
 * @param list Pointer to ColorBarList
 */
void colorbar_list_clear(ColorBarList *list);

/**
 * @brief Free resources allocated for the list
 *
 * @param list Pointer to ColorBarList
 *
 * @note Must be called when list is no longer needed
 * @note List pointer should not be used after this call
 */
void colorbar_list_cleanup(ColorBarList *list);

/**
 * @brief Create a default color bar specification
 *
 * @return ColorBar with sensible defaults
 *
 * Defaults:
 * - Position: TOP
 * - Height: 20 pixels
 * - Color: white (255, 255, 255, 255)
 * - Enabled: 1 (true)
 */
ColorBar colorbar_create_default(void);

/**
 * @brief Parse color bar position string
 *
 * @param position_str String like "top", "bottom"
 *
 * @return ColorBarPosition enum value, or COLORBAR_POSITION_TOP if unrecognized
 */
ColorBarPosition colorbar_parse_position(const char *position_str);

/**
 * @brief Parse unified text overlay specification (pipe-separated key=value format)
 *
 * Parses a string like: "text|size=20|align=center|pos=top|color=white|bg=blue"
 * and populates a TextOverlaySpec structure with the parsed values.
 *
 * Format: "text[|key=value|key=value|...]"
 * - First part (before first pipe) is the overlay text
 * - Subsequent pipe-separated key=value pairs set properties
 *
 * Recognized keys:
 *   - size, s: font size (8-96)
 *   - align, a: text alignment (left, center, right)
 *   - pos, p: position (top, bottom, left, right, center)
 *   - color, c: text color (named or hex)
 *   - bg: background color (named or hex)
 *   - mode, m: background mode (opaque, transparent, semi)
 *   - pad, padding, x: padding in pixels
 *   - border, b, d: border width in pixels
 *   - v-align, va: vertical alignment (top, center, bottom)
 *
 * @param spec_str Pipe-separated specification string
 * @param out_spec Output: populated TextOverlaySpec structure
 *
 * @return PISSTVPP2_OK on success, error code on failure
 */
int overlay_parse_unified_spec(const char *spec_str, TextOverlaySpec *out_spec);

#endif /* OVERLAY_SPEC_H */
