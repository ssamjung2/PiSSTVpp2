/**
 * @file overlay_spec.c
 * @brief Flexible Text Overlay Specification System - Implementation
 */

#include "overlay_spec.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

// ===========================================================================
// OVERLAY LIST MANAGEMENT
// ===========================================================================

int overlay_spec_list_init(OverlaySpecList *list, size_t capacity) {
    if (!list) {
        error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL, "Invalid list pointer");
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }

    if (capacity == 0 || capacity > OVERLAY_MAX_COUNT) {
        error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL, 
                  "Overlay list capacity must be 1-%d", OVERLAY_MAX_COUNT);
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }

    list->overlays = (TextOverlaySpec *)malloc(capacity * sizeof(TextOverlaySpec));
    if (!list->overlays) {
        error_log(SLOWFRAME_ERR_MEMORY_ALLOC, "Failed to allocate overlay list");
        return SLOWFRAME_ERR_MEMORY_ALLOC;
    }

    list->count = 0;
    list->capacity = capacity;
    return SLOWFRAME_OK;
}

int overlay_spec_list_add(OverlaySpecList *list, const TextOverlaySpec *spec) {
    if (!list || !spec) {
        error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL, "Invalid list or spec pointer");
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }

    if (list->count >= list->capacity) {
        error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL, 
                  "Overlay list is full (capacity %zu)", list->capacity);
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }

    // Copy the spec into the list
    memcpy(&list->overlays[list->count], spec, sizeof(TextOverlaySpec));
    list->count++;
    return SLOWFRAME_OK;
}

TextOverlaySpec* overlay_spec_list_get(OverlaySpecList *list, size_t index) {
    if (!list || index >= list->count) {
        return NULL;
    }
    return &list->overlays[index];
}

int overlay_spec_list_remove(OverlaySpecList *list, size_t index) {
    if (!list || index >= list->count) {
        error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL, "Invalid list or index");
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }

    // Shift remaining elements
    for (size_t i = index; i < list->count - 1; i++) {
        memcpy(&list->overlays[i], &list->overlays[i + 1], sizeof(TextOverlaySpec));
    }
    list->count--;
    return SLOWFRAME_OK;
}

size_t overlay_spec_list_count(const OverlaySpecList *list) {
    if (!list) return 0;
    return list->count;
}

void overlay_spec_list_clear(OverlaySpecList *list) {
    if (!list) return;
    list->count = 0;
}

void overlay_spec_list_cleanup(OverlaySpecList *list) {
    if (!list) return;
    if (list->overlays) {
        free(list->overlays);
        list->overlays = NULL;
    }
    list->count = 0;
    list->capacity = 0;
}

// ===========================================================================
// CREATION AND DEFAULTS
// ===========================================================================

TextOverlaySpec overlay_spec_create_default(void) {
    TextOverlaySpec spec = {0};
    
    // Text content (empty by default)
    spec.text[0] = '\0';
    spec.timestamp_format[0] = '\0';  // Timestamp disabled by default
    
    // Placement and positioning (SSTV-optimized defaults)
    spec.placement = OVERLAY_PLACE_BOTTOM_RIGHT;  // Common for callsign in SSTV
    spec.offset_x = 0;
    spec.offset_y = 0;
    
    // Text styling with SSTV-friendly defaults
    spec.font_size = 16;  // 16px fits well on 320x240 and larger images
    strncpy(spec.font_family, "sans", sizeof(spec.font_family) - 1);
    spec.text_align = TEXT_ALIGN_CENTER;
    spec.valign = VALIGN_CENTER;
    spec.text_color = (RGBAColor){255, 255, 255, 255};    // White text (best contrast)
    
    // Text wrapping (for multi-line support)
    spec.text_width = 0;  // No limit by default
    spec.wrap_mode = WRAP_WORD;  // Wrap at word boundaries
    spec.line_spacing = 1.0f;  // Single line spacing
    
    // Background styling (SSTV-optimized)
    spec.bg_mode = BG_SEMI;  // Semi-transparent (50% opacity)
    spec.bg_color = (RGBAColor){0, 0, 0, 255};     // Black background (standard SSTV)
    spec.opacity = 50;  // 50% opacity (alternative to bg_mode)
    spec.padding = 5;  // 5px padding around text
    spec.border_width = 0;  // No border by default
    spec.border_color = (RGBAColor){255, 255, 255, 255};  // White border (if enabled)
    
    // Background bar styling (enabled for signal protection in weak SSTV)
    spec.bg_bar_enable = 0;  // User enables when needed
    spec.bg_bar_margin = 2;  // 2px margin (reduced from 4 for SSTV screens)
    spec.bg_bar_width_mode = BGBAR_WIDTH_FULL;  // Full width (SSTV protection, user request)
    spec.bg_bar_custom_width = 0;
    spec.bg_bar_orientation = BGBAR_ORIENT_HORIZONTAL;  // Horizontal (standard)
    spec.bg_bar_color = (RGBAColor){255, 255, 255, 255};      // White bar (high visibility)
    
    // Sizing constraints (auto by default)
    spec.min_width = 0;
    spec.max_width = 0;
    spec.min_height = 0;
    spec.max_height = 0;
    
    // Effects
    spec.enabled = 1;
    spec.rotation = 0;
    
    return spec;
}

// ===========================================================================
// PARSING UTILITIES
// ===========================================================================

OverlayPlacement overlay_parse_placement(const char *placement_str) {
    if (!placement_str) return OVERLAY_PLACE_TOP;
    
    if (strcmp(placement_str, "top") == 0 || strcmp(placement_str, "TOP") == 0) {
        return OVERLAY_PLACE_TOP;
    } else if (strcmp(placement_str, "bottom") == 0 || strcmp(placement_str, "BOTTOM") == 0) {
        return OVERLAY_PLACE_BOTTOM;
    } else if (strcmp(placement_str, "left") == 0 || strcmp(placement_str, "LEFT") == 0) {
        return OVERLAY_PLACE_LEFT;
    } else if (strcmp(placement_str, "right") == 0 || strcmp(placement_str, "RIGHT") == 0) {
        return OVERLAY_PLACE_RIGHT;
    } else if (strcmp(placement_str, "center") == 0 || strcmp(placement_str, "CENTER") == 0) {
        return OVERLAY_PLACE_CENTER;
    } else if (strcmp(placement_str, "top-left") == 0 || strcmp(placement_str, "topleft") == 0) {
        return OVERLAY_PLACE_TOP_LEFT;
    } else if (strcmp(placement_str, "top-right") == 0 || strcmp(placement_str, "topright") == 0) {
        return OVERLAY_PLACE_TOP_RIGHT;
    } else if (strcmp(placement_str, "bottom-left") == 0 || strcmp(placement_str, "bottomleft") == 0) {
        return OVERLAY_PLACE_BOTTOM_LEFT;
    } else if (strcmp(placement_str, "bottom-right") == 0 || strcmp(placement_str, "bottomright") == 0) {
        return OVERLAY_PLACE_BOTTOM_RIGHT;
    }
    
    return OVERLAY_PLACE_TOP;  // Default
}

TextAlignment overlay_parse_alignment(const char *align_str) {
    if (!align_str) return TEXT_ALIGN_CENTER;
    
    if (strcmp(align_str, "left") == 0 || strcmp(align_str, "LEFT") == 0) {
        return TEXT_ALIGN_LEFT;
    } else if (strcmp(align_str, "center") == 0 || strcmp(align_str, "CENTER") == 0) {
        return TEXT_ALIGN_CENTER;
    } else if (strcmp(align_str, "right") == 0 || strcmp(align_str, "RIGHT") == 0) {
        return TEXT_ALIGN_RIGHT;
    }
    
    return TEXT_ALIGN_CENTER;  // Default
}

/**
 * @brief Parse hex color string (e.g., "FF0000" for red)
 */
static int parse_hex_color(const char *hex_str, uint8_t *r, uint8_t *g, uint8_t *b) {
    if (!hex_str || strlen(hex_str) < 6) {
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }
    
    // Validate all characters are hex digits
    for (int i = 0; i < 6; i++) {
        if (!isxdigit((unsigned char)hex_str[i])) {
            return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
        }
    }
    
    char r_str[3] = {hex_str[0], hex_str[1], '\0'};
    char g_str[3] = {hex_str[2], hex_str[3], '\0'};
    char b_str[3] = {hex_str[4], hex_str[5], '\0'};
    
    long r_val = strtol(r_str, NULL, 16);
    long g_val = strtol(g_str, NULL, 16);
    long b_val = strtol(b_str, NULL, 16);
    
    if (r_val > 255 || g_val > 255 || b_val > 255) {
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }
    
    *r = (uint8_t)r_val;
    *g = (uint8_t)g_val;
    *b = (uint8_t)b_val;
    
    return SLOWFRAME_OK;
}

/**
 * @brief Parse named color (e.g., "red", "white", "black")
 */
static int parse_named_color(const char *name, uint8_t *r, uint8_t *g, uint8_t *b) {
    if (!name) return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    
    // Convert to lowercase for comparison
    char lower_name[32];
    strncpy(lower_name, name, sizeof(lower_name) - 1);
    lower_name[sizeof(lower_name) - 1] = '\0';
    for (char *p = lower_name; *p; p++) *p = tolower(*p);
    
    // Primary colors
    if (strcmp(lower_name, "red") == 0) {
        *r = 255; *g = 0; *b = 0;
    } else if (strcmp(lower_name, "green") == 0) {
        *r = 0; *g = 255; *b = 0;
    } else if (strcmp(lower_name, "blue") == 0) {
        *r = 0; *g = 0; *b = 255;
    } else if (strcmp(lower_name, "white") == 0) {
        *r = 255; *g = 255; *b = 255;
    } else if (strcmp(lower_name, "black") == 0) {
        *r = 0; *g = 0; *b = 0;
    } 
    // Secondary colors
    else if (strcmp(lower_name, "yellow") == 0) {
        *r = 255; *g = 255; *b = 0;
    } else if (strcmp(lower_name, "cyan") == 0) {
        *r = 0; *g = 255; *b = 255;
    } else if (strcmp(lower_name, "magenta") == 0) {
        *r = 255; *g = 0; *b = 255;
    } 
    // Grays
    else if (strcmp(lower_name, "gray") == 0 || strcmp(lower_name, "grey") == 0) {
        *r = 128; *g = 128; *b = 128;
    } else if (strcmp(lower_name, "silver") == 0) {
        *r = 192; *g = 192; *b = 192;
    } else if (strcmp(lower_name, "dark-gray") == 0 || strcmp(lower_name, "darkgray") == 0 ||
               strcmp(lower_name, "dark-grey") == 0 || strcmp(lower_name, "darkgrey") == 0) {
        *r = 64; *g = 64; *b = 64;
    }
    // Extended colors
    else if (strcmp(lower_name, "lime") == 0) {
        *r = 0; *g = 255; *b = 0;  // Bright green
    } else if (strcmp(lower_name, "orange") == 0) {
        *r = 255; *g = 165; *b = 0;
    } else if (strcmp(lower_name, "purple") == 0 || strcmp(lower_name, "violet") == 0) {
        *r = 128; *g = 0; *b = 128;
    } else if (strcmp(lower_name, "pink") == 0) {
        *r = 255; *g = 192; *b = 203;
    } else if (strcmp(lower_name, "brown") == 0) {
        *r = 165; *g = 42; *b = 42;
    } else if (strcmp(lower_name, "navy") == 0 || strcmp(lower_name, "navy-blue") == 0) {
        *r = 0; *g = 0; *b = 128;
    } else if (strcmp(lower_name, "teal") == 0) {
        *r = 0; *g = 128; *b = 128;
    } else if (strcmp(lower_name, "maroon") == 0) {
        *r = 128; *g = 0; *b = 0;
    } else if (strcmp(lower_name, "olive") == 0) {
        *r = 128; *g = 128; *b = 0;
    } else {
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }
    
    return SLOWFRAME_OK;
}

int overlay_parse_color(const char *color_str, RGBAColor *out_color) {
    if (!color_str || !out_color) {
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }
    
    int result;
    const char *hex_start = color_str;
    
    // Skip leading '#' if present
    if (color_str[0] == '#') {
        hex_start = color_str + 1;
    }
    
    // Try hex color first (e.g., "FF0000" or "#FF0000")
    if (strlen(hex_start) >= 6 && isxdigit((unsigned char)hex_start[0])) {
        result = parse_hex_color(hex_start, &out_color->r, &out_color->g, &out_color->b);
        if (result == SLOWFRAME_OK) {
            out_color->alpha = 255;  // Fully opaque by default
            return SLOWFRAME_OK;
        }
    }
    
    // Try named color (e.g., "red", "white")
    result = parse_named_color(color_str, &out_color->r, &out_color->g, &out_color->b);
    if (result == SLOWFRAME_OK) {
        out_color->alpha = 255;  // Fully opaque by default
        return SLOWFRAME_OK;
    }
    
    error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL, 
              "Invalid color format: %s (use hex like FF0000 or #FF0000, or name like red)", color_str);
    return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
}

// ===========================================================================
// COLOR BAR LIST MANAGEMENT
// ===========================================================================

int colorbar_list_init(ColorBarList *list, size_t capacity) {
    if (!list) {
        error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL, "Invalid list pointer");
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }

    if (capacity == 0 || capacity > 20) {  // Max 20 color bars
        error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL, 
                  "Color bar list capacity must be 1-20");
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }

    list->bars = (ColorBar *)malloc(capacity * sizeof(ColorBar));
    if (!list->bars) {
        error_log(SLOWFRAME_ERR_MEMORY_ALLOC, "Failed to allocate color bar list");
        return SLOWFRAME_ERR_MEMORY_ALLOC;
    }

    list->count = 0;
    list->capacity = capacity;
    return SLOWFRAME_OK;
}

int colorbar_list_add(ColorBarList *list, const ColorBar *bar) {
    if (!list || !bar) {
        error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL, "Invalid list or bar pointer");
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }

    if (list->count >= list->capacity) {
        error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL, 
                  "Color bar list is full (capacity %zu)", list->capacity);
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }

    // Copy the bar to the list
    list->bars[list->count] = *bar;
    list->count++;
    return SLOWFRAME_OK;
}

ColorBar* colorbar_list_get(ColorBarList *list, size_t index) {
    if (!list || index >= list->count) {
        return NULL;
    }
    return &list->bars[index];
}

size_t colorbar_list_count(const ColorBarList *list) {
    if (!list) {
        return 0;
    }
    return list->count;
}

void colorbar_list_clear(ColorBarList *list) {
    if (list) {
        list->count = 0;
    }
}

void colorbar_list_cleanup(ColorBarList *list) {
    if (list && list->bars) {
        free(list->bars);
        list->bars = NULL;
        list->count = 0;
        list->capacity = 0;
    }
}

ColorBar colorbar_create_default(void) {
    ColorBar bar = {
        .position = COLORBAR_POSITION_TOP,
        .custom_offset = 0,
        .height = 20,
        .num_colors = 1,
        .enabled = 1
    };
    
    // Default color: white
    bar.colors[0].r = 255;
    bar.colors[0].g = 255;
    bar.colors[0].b = 255;
    bar.colors[0].alpha = 255;
    
    return bar;
}

ColorBarPosition colorbar_parse_position(const char *position_str) {
    if (!position_str) {
        return COLORBAR_POSITION_TOP;
    }
    
    // Convert to lowercase for comparison
    char lower_pos[32];
    strncpy(lower_pos, position_str, sizeof(lower_pos) - 1);
    lower_pos[sizeof(lower_pos) - 1] = '\0';
    
    for (int i = 0; lower_pos[i]; i++) {
        lower_pos[i] = tolower((unsigned char)lower_pos[i]);
    }
    
    if (strcmp(lower_pos, "top") == 0) {
        return COLORBAR_POSITION_TOP;
    } else if (strcmp(lower_pos, "bottom") == 0) {
        return COLORBAR_POSITION_BOTTOM;
    } else if (strcmp(lower_pos, "custom") == 0) {
        return COLORBAR_POSITION_CUSTOM;
    } else {
        return COLORBAR_POSITION_TOP;  // Default to top if unrecognized
    }
}

// ===========================================================================
// UNIFIED TEXT OVERLAY SPECIFICATION PARSER
// ===========================================================================

/**
 * @brief Parse unified text overlay specification
 *
 * Format: "text|size=20|align=center|pos=top|color=white|bg=blue|mode=opaque|pad=8|border=2|v-align=center"
 *
 * The first segment (before the first pipe) is the text content.
 * All subsequent segments are key=value pairs separated by pipes.
 */
int overlay_parse_unified_spec(const char *spec_str, TextOverlaySpec *out_spec) {
    if (!spec_str || !out_spec) {
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }
    
    // Initialize output with defaults
    *out_spec = overlay_spec_create_default();
    
    // Make a working copy we can modify with strtok
    char spec_copy[512];
    strncpy(spec_copy, spec_str, sizeof(spec_copy) - 1);
    spec_copy[sizeof(spec_copy) - 1] = '\0';
    
    // Parse text (everything up to first pipe, or entire string if no pipe)
    char *pipe_pos = strchr(spec_copy, '|');
    if (pipe_pos) {
        *pipe_pos = '\0';  // Terminate text at first pipe
        strncpy(out_spec->text, spec_copy, OVERLAY_MAX_TEXT_LENGTH - 1);
        out_spec->text[OVERLAY_MAX_TEXT_LENGTH - 1] = '\0';
        
        // Continue parsing from after the first pipe
        char *token = pipe_pos + 1;
        
        while (token && *token) {
            // Find next pipe
            char *next_pipe = strchr(token, '|');
            if (next_pipe) {
                *next_pipe = '\0';
            }
            
            // Parse key=value pair
            char *eq_pos = strchr(token, '=');
            if (eq_pos) {
                *eq_pos = '\0';
                char *key = token;
                char *value = eq_pos + 1;
                
                // Trim whitespace from key
                while (*key == ' ') key++;
                char *key_end = key + strlen(key) - 1;
                while (key_end > key && *key_end == ' ') *key_end-- = '\0';
                
                // Trim whitespace from value
                while (*value == ' ') value++;
                char *val_end = value + strlen(value) - 1;
                while (val_end > value && *val_end == ' ') *val_end-- = '\0';
                
                // Convert key to lowercase for comparison
                char lower_key[64];
                strncpy(lower_key, key, sizeof(lower_key) - 1);
                lower_key[sizeof(lower_key) - 1] = '\0';
                for (char *p = lower_key; *p; p++) *p = tolower((unsigned char)*p);
                
                // Handle known keys
                if (strcmp(lower_key, "size") == 0 || strcmp(lower_key, "s") == 0) {
                    char *endptr;
                    errno = 0;
                    long size = strtol(value, &endptr, 10);
                    if (errno == 0 && size >= 8 && size <= 72) {
                        out_spec->font_size = (uint16_t)size;
                    }
                } else if (strcmp(lower_key, "align") == 0 || strcmp(lower_key, "a") == 0) {
                    out_spec->text_align = overlay_parse_alignment(value);
                } else if (strcmp(lower_key, "pos") == 0 || strcmp(lower_key, "p") == 0 || strcmp(lower_key, "position") == 0) {
                    out_spec->placement = overlay_parse_placement(value);
                } else if (strcmp(lower_key, "color") == 0 || strcmp(lower_key, "c") == 0) {
                    overlay_parse_color(value, &out_spec->text_color);
                } else if (strcmp(lower_key, "bg") == 0 || strcmp(lower_key, "background") == 0) {
                    overlay_parse_color(value, &out_spec->bg_color);
                } else if (strcmp(lower_key, "mode") == 0 || strcmp(lower_key, "m") == 0) {
                    if (strcmp(value, "opaque") == 0) {
                        out_spec->bg_mode = BG_OPAQUE;
                    } else if (strcmp(value, "transparent") == 0) {
                        out_spec->bg_mode = BG_TRANSPARENT;
                    } else if (strcmp(value, "semi") == 0 || strcmp(value, "semi-transparent") == 0) {
                        out_spec->bg_mode = BG_SEMI;
                    }
                } else if (strcmp(lower_key, "pad") == 0 || strcmp(lower_key, "padding") == 0) {
                    char *endptr;
                    errno = 0;
                    long pad = strtol(value, &endptr, 10);
                    if (errno == 0 && pad >= 0 && pad <= 30) {
                        out_spec->padding = (uint16_t)pad;
                    }
                } else if (strcmp(lower_key, "offset-x") == 0 || strcmp(lower_key, "offsetx") == 0 || strcmp(lower_key, "x") == 0) {
                    char *endptr;
                    errno = 0;
                    long offset = strtol(value, &endptr, 10);
                    if (errno == 0 && offset >= -100 && offset <= 100) {
                        out_spec->offset_x = (int16_t)offset;
                    }
                } else if (strcmp(lower_key, "offset-y") == 0 || strcmp(lower_key, "offsety") == 0 || strcmp(lower_key, "y") == 0) {
                    char *endptr;
                    errno = 0;
                    long offset = strtol(value, &endptr, 10);
                    if (errno == 0 && offset >= -100 && offset <= 100) {
                        out_spec->offset_y = (int16_t)offset;
                    }
                } else if (strcmp(lower_key, "border") == 0 || strcmp(lower_key, "b") == 0 || strcmp(lower_key, "d") == 0) {
                    char *endptr;
                    errno = 0;
                    long border = strtol(value, &endptr, 10);
                    if (errno == 0 && border >= 0 && border <= 8) {
                        out_spec->border_width = (uint16_t)border;
                    }
                } else if (strcmp(lower_key, "v-align") == 0 || strcmp(lower_key, "va") == 0 || strcmp(lower_key, "valign") == 0) {
                    if (strcmp(value, "top") == 0) {
                        out_spec->valign = VALIGN_TOP;
                    } else if (strcmp(value, "center") == 0 || strcmp(value, "middle") == 0) {
                        out_spec->valign = VALIGN_CENTER;
                    } else if (strcmp(value, "bottom") == 0) {
                        out_spec->valign = VALIGN_BOTTOM;
                    }
                } else if (strcmp(lower_key, "bgbar") == 0 || strcmp(lower_key, "backgroundbar") == 0 || strcmp(lower_key, "bg-bar") == 0) {
                    // Enable background bar: bgbar=true, bgbar=1, bgbar=yes, etc.
                    if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0 || 
                        strcmp(value, "yes") == 0 || strcmp(value, "enable") == 0) {
                        out_spec->bg_bar_enable = 1;
                    } else {
                        out_spec->bg_bar_enable = 0;
                    }
                } else if (strcmp(lower_key, "bgbar-margin") == 0 || strcmp(lower_key, "bgbarmargin") == 0 || strcmp(lower_key, "bgbar_margin") == 0) {
                    char *endptr;
                    errno = 0;
                    long margin = strtol(value, &endptr, 10);
                    if (errno == 0 && margin >= 0 && margin <= 15) {
                        out_spec->bg_bar_margin = (uint16_t)margin;
                    }
                } else if (strcmp(lower_key, "bgbar-width") == 0 || strcmp(lower_key, "bgbarwidth") == 0 || strcmp(lower_key, "bgbar_width") == 0) {
                    // Parse background bar width: "auto", "full", "half", or pixel value
                    if (strcmp(value, "full") == 0) {
                        out_spec->bg_bar_width_mode = BGBAR_WIDTH_FULL;
                    } else if (strcmp(value, "half") == 0) {
                        out_spec->bg_bar_width_mode = BGBAR_WIDTH_HALF;
                    } else if (strcmp(value, "auto") == 0) {
                        out_spec->bg_bar_width_mode = BGBAR_WIDTH_AUTO;
                    } else {
                        // Try to parse as pixel value
                        char *endptr;
                        errno = 0;
                        long width = strtol(value, &endptr, 10);
                        if (errno == 0 && width > 0 && width <= 2000) {
                            out_spec->bg_bar_width_mode = BGBAR_WIDTH_FIXED;
                            out_spec->bg_bar_custom_width = (uint16_t)width;
                        }
                    }
                } else if (strcmp(lower_key, "bgbar-orient") == 0 || strcmp(lower_key, "bgarorient") == 0 || 
                          strcmp(lower_key, "bgbar-orientation") == 0 || strcmp(lower_key, "bgbarorientation") == 0) {
                    // Parse background bar orientation: "horizontal", "vertical", "vertical-stacked", "h", "v", "vs"
                    if (strcmp(value, "vertical-stacked") == 0 || strcmp(value, "vs") == 0 || strcmp(value, "stacked") == 0) {
                        out_spec->bg_bar_orientation = BGBAR_ORIENT_VERTICAL_STACKED;
                    } else if (strcmp(value, "vertical") == 0 || strcmp(value, "v") == 0) {
                        out_spec->bg_bar_orientation = BGBAR_ORIENT_VERTICAL;
                    } else {
                        out_spec->bg_bar_orientation = BGBAR_ORIENT_HORIZONTAL;
                    }
                } else if (strcmp(lower_key, "bgbar-color") == 0 || strcmp(lower_key, "bgbarcolor") == 0) {
                    // Parse background bar color
                    if (overlay_parse_color(value, &out_spec->bg_bar_color) != 0) {
                        out_spec->bg_bar_color = (RGBAColor){0, 0, 0, 255};  // Default to black on parse error
                    }
                } else if (strcmp(lower_key, "timestamp-format") == 0 || strcmp(lower_key, "timestamp_format") == 0 || 
                          strcmp(lower_key, "timestampformat") == 0 || strcmp(lower_key, "ts") == 0) {
                    // Set timestamp format (strftime format string)
                    strncpy(out_spec->timestamp_format, value, sizeof(out_spec->timestamp_format) - 1);
                    out_spec->timestamp_format[sizeof(out_spec->timestamp_format) - 1] = '\0';
                } else if (strcmp(lower_key, "border-color") == 0 || strcmp(lower_key, "bordercolor") == 0) {
                    // Parse border color
                    if (overlay_parse_color(value, &out_spec->border_color) != 0) {
                        out_spec->border_color = out_spec->text_color;  // Default to text color
                    }
                } else if (strcmp(lower_key, "opacity") == 0 || strcmp(lower_key, "alpha") == 0) {
                    // Parse opacity as percentage (0-100)
                    char *endptr;
                    errno = 0;
                    long opacity = strtol(value, &endptr, 10);
                    if (errno == 0 && opacity >= 0 && opacity <= 100) {
                        out_spec->opacity = (uint8_t)opacity;
                        // Convert to bg_mode for compatibility
                        if (opacity == 0) {
                            out_spec->bg_mode = BG_TRANSPARENT;
                        } else if (opacity >= 90) {
                            out_spec->bg_mode = BG_OPAQUE;
                        } else {
                            out_spec->bg_mode = BG_SEMI;
                        }
                    }
                } else if (strcmp(lower_key, "font") == 0 || strcmp(lower_key, "font-family") == 0 || strcmp(lower_key, "fontfamily") == 0) {
                    // Parse font family (fontconfig style)
                    strncpy(out_spec->font_family, value, sizeof(out_spec->font_family) - 1);
                    out_spec->font_family[sizeof(out_spec->font_family) - 1] = '\0';
                } else if (strcmp(lower_key, "width") == 0 || strcmp(lower_key, "text-width") == 0 || strcmp(lower_key, "textwidth") == 0) {
                    // Parse max text width for wrapping
                    char *endptr;
                    errno = 0;
                    long width = strtol(value, &endptr, 10);
                    if (errno == 0 && width >= 0 && width <= 2000) {
                        out_spec->text_width = (uint16_t)width;
                    }
                } else if (strcmp(lower_key, "wrap") == 0 || strcmp(lower_key, "text-wrap") == 0 || strcmp(lower_key, "textwrap") == 0) {
                    // Parse text wrapping mode
                    if (strcmp(value, "char") == 0 || strcmp(value, "character") == 0) {
                        out_spec->wrap_mode = WRAP_CHAR;
                    } else if (strcmp(value, "none") == 0 || strcmp(value, "no") == 0) {
                        out_spec->wrap_mode = WRAP_NONE;
                    } else {
                        out_spec->wrap_mode = WRAP_WORD;  // Default to word wrap
                    }
                } else if (strcmp(lower_key, "line-spacing") == 0 || strcmp(lower_key, "linespacing") == 0 || strcmp(lower_key, "spacing") == 0) {
                    // Parse line spacing as multiplier (0.8-1.5)
                    char *endptr;
                    errno = 0;
                    double spacing = strtod(value, &endptr);
                    if (errno == 0 && spacing >= 0.5 && spacing <= 2.0) {
                        out_spec->line_spacing = (float)spacing;
                    }
                }
            }
            
            // Move to next token
            if (next_pipe) {
                token = next_pipe + 1;
            } else {
                token = NULL;
            }
        }
    } else {
        // No pipe - entire string is text with default properties
        strncpy(out_spec->text, spec_copy, OVERLAY_MAX_TEXT_LENGTH - 1);
        out_spec->text[OVERLAY_MAX_TEXT_LENGTH - 1] = '\0';
    }
    
    return SLOWFRAME_OK;
}
