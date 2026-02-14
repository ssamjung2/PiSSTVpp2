/**
 * @file pisstvpp2_config.c
 * @brief PiSSTVpp2 Configuration Management Module Implementation
 *
 * This module provides the implementation of the configuration management system.
 * It handles command-line parsing, validation, and configuration lifecycle.
 *
 * @author PiSSTVpp2 Contributors
 * @version 2.1.0
 * @date February 2026
 */

#include "pisstvpp2_config.h"
#include "error.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>

// ===========================================================================
// FORWARD DECLARATIONS
// ===========================================================================

static void show_help(const char *program_name);
static void show_detailed_help(const char *program_name);
static int is_valid_protocol(const char *protocol);
static const char* get_format_extension(const char *format);

// ===========================================================================
// STATIC HELPER FUNCTIONS
// ===========================================================================

/**
 * @brief Apply backward compatibility checks (currently a no-op)
 * 
 * Note: The old -S, -G, -O flags were sunset in favor of the unified -T option.
 * This function is retained as a placeholder for potential future extensions.
 */
static int apply_backward_compatibility(PisstvppConfig *config) {
    if (!config) {
        return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
    }

    // All overlay functionality is now through -T flag
    return PISSTVPP2_OK;
}



// ===========================================================================
// PUBLIC FUNCTION IMPLEMENTATIONS
// ===========================================================================

/**
 * @brief Initialize configuration with default values
 */
int pisstvpp_config_init(PisstvppConfig *config) {
    if (!config) {
        return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;  // Generic error
    }

    // Initialize file paths (empty)
    memset(config->input_file, 0, sizeof(config->input_file));
    memset(config->output_file, 0, sizeof(config->output_file));

    // SSTV encoding defaults
    strncpy(config->protocol, CONFIG_DEFAULT_PROTOCOL, 
            sizeof(config->protocol) - 1);
    strncpy(config->format, CONFIG_DEFAULT_FORMAT, 
            sizeof(config->format) - 1);
    config->sample_rate = CONFIG_DEFAULT_SAMPLE_RATE;

    // Aspect ratio mode
    config->aspect_mode = ASPECT_CENTER;

    // CW signature defaults
    config->cw_enabled = 0;
    memset(config->cw_callsign, 0, sizeof(config->cw_callsign));
    config->cw_wpm = CONFIG_DEFAULT_CW_WPM;
    config->cw_tone = CONFIG_DEFAULT_CW_TONE;



    // Initialize overlay specification list (max 20 overlays)
    int overlay_init_result = overlay_spec_list_init(&config->overlay_specs, OVERLAY_MAX_COUNT);
    if (overlay_init_result != PISSTVPP2_OK) {
        error_log(overlay_init_result, "Failed to initialize overlay spec list");
        return overlay_init_result;
    }

    // Initialize color bar specification list (max 10 color bars)
    int colorbar_init_result = colorbar_list_init(&config->colorbar_specs, 10);
    if (colorbar_init_result != PISSTVPP2_OK) {
        error_log(colorbar_init_result, "Failed to initialize color bar spec list");
        return colorbar_init_result;
    }

    // Initialize current overlay pointer (no overlay being configured)
    config->current_overlay = NULL;

    // Audio encoding enabled by default
    config->skip_audio_encoding = 0;

    // Debug and logging defaults
    config->verbose = 0;
    config->timestamp_logging = 0;
    config->keep_intermediate = 0;
    config->text_only = 0;

    return PISSTVPP2_OK;
}

/**
 * @brief Parse command-line arguments into configuration
 */
int pisstvpp_config_parse(PisstvppConfig *config, int argc, char *argv[]) {
    if (!config || !argv) {
        error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL, "Invalid config or argv");
        return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
    }

    // Initialize config with defaults first
    int result = pisstvpp_config_init(config);
    if (result != PISSTVPP2_OK) {
        return result;
    }

    int option;
    int w_flag = 0;  // Track if -W used
    int t_flag = 0;  // Track if -T used

    // Parse all options
    // Overlay text: -T (unified text overlay with embedded styling)
    // Overlay styling: -F (font size), -A (text align), -V (vertical align), -I (image placement)
    //                 -C (text color), -B (bg color), -M (bg mode), -X (padding), -D (border width)
    // Color bars: -R (color bar rows)
    // CW: -Q (CW tone frequency)
    // Testing: -N (skip audio encoding for overlay testing)
    while ((option = getopt(argc, argv, "i:o:p:f:r:vC:W:Q:a:KZhNOP:B:F:A:M:I:R:X:D:V:T:")) != -1) {
        switch (option) {
            // Input file (REQUIRED)
            case 'i':
                if (strlen(optarg) >= sizeof(config->input_file)) {
                    error_log(PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG,
                            "Input filename too long (max %d chars)",
                            CONFIG_MAX_FILENAME);
                    return PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG;
                }
                strncpy(config->input_file, optarg, 
                        sizeof(config->input_file) - 1);
                break;

            // Output file (OPTIONAL)
            case 'o':
                if (strlen(optarg) == 0) {
                    error_log(PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG,
                            "Output filename cannot be empty");
                    return PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG;
                }
                if (strlen(optarg) >= sizeof(config->output_file)) {
                    error_log(PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG,
                            "Output filename too long (max %d chars)",
                            CONFIG_MAX_FILENAME);
                    return PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG;
                }
                // Validate output filename: reject dangerous characters
                for (int i = 0; optarg[i]; i++) {
                    char c = optarg[i];
                    // Reject shell metacharacters and control chars
                    if (c == '&' || c == '|' || c == ';' || c == '`' || c == '$' ||
                        c == '%' || c == '\n' || c == '\r' || c == '\t' ||
                        (unsigned char)c < 32) {
                        error_log(PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG,
                                "Output filename contains invalid character (code %d)", (unsigned char)c);
                        return PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG;
                    }
                }
                strncpy(config->output_file, optarg,
                        sizeof(config->output_file) - 1);
                break;

            // SSTV Protocol
            case 'p':
                if (!is_valid_protocol(optarg)) {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "Unknown protocol: %s", optarg);
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                strncpy(config->protocol, optarg, 
                        sizeof(config->protocol) - 1);
                break;

            // Audio Format
            case 'f':
                if (!pisstvpp_config_is_format_supported(optarg)) {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_FORMAT,
                            "Unsupported format: %s", optarg);
                    return PISSTVPP2_ERR_ARG_INVALID_FORMAT;
                }
                strncpy(config->format, optarg, 
                        sizeof(config->format) - 1);
                break;

            // Sample Rate
            case 'r': {
                char *endptr = NULL;
                errno = 0;
                long tmp = strtol(optarg, &endptr, 10);

                if (errno != 0 || endptr == optarg || *endptr != '\0') {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE,
                            "Invalid sample rate: %s", optarg);
                    return PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE;
                }

                if (tmp < CONFIG_MIN_SAMPLE_RATE || tmp > CONFIG_MAX_SAMPLE_RATE) {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE,
                            "Sample rate must be %d-%d Hz",
                            CONFIG_MIN_SAMPLE_RATE, CONFIG_MAX_SAMPLE_RATE);
                    return PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE;
                }

                config->sample_rate = (uint16_t)tmp;
                break;
            }

            // Aspect Ratio Mode
            case 'a':
                if (strcmp(optarg, "center") == 0) {
                    config->aspect_mode = ASPECT_CENTER;
                } else if (strcmp(optarg, "pad") == 0) {
                    config->aspect_mode = ASPECT_PAD;
                } else if (strcmp(optarg, "stretch") == 0) {
                    config->aspect_mode = ASPECT_STRETCH;
                } else {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_ASPECT,
                            "Aspect mode must be 'center', 'pad', or 'stretch'");
                    return PISSTVPP2_ERR_ARG_INVALID_ASPECT;
                }
                break;

            // CW Callsign OR Overlay Text Color (context-dependent)
            case 'C':
                // Check if we're in overlay styling context
                if (config->current_overlay && strlen(config->current_overlay->text) > 0) {
                    // We're styling current overlay - treat as text color
                    int color_result = overlay_parse_color(optarg, &config->current_overlay->text_color);
                    if (color_result != PISSTVPP2_OK) {
                        error_log(color_result, "Invalid overlay text color: %s", optarg);
                        return color_result;
                    }
                } else {
                    // Traditional CW callsign context
                    if (strlen(optarg) == 0) {
                        error_log(PISSTVPP2_ERR_ARG_CALLSIGN_INVALID,
                                "Callsign cannot be empty");
                        return PISSTVPP2_ERR_ARG_CALLSIGN_INVALID;
                    }
                    if (strlen(optarg) >= sizeof(config->cw_callsign)) {
                        error_log(PISSTVPP2_ERR_ARG_CALLSIGN_INVALID,
                                "Callsign too long (max %d chars)",
                                CONFIG_MAX_CALLSIGN);
                        return PISSTVPP2_ERR_ARG_CALLSIGN_INVALID;
                    }
                    // Validate callsign format: alphanumeric and slash only
                    for (int i = 0; optarg[i]; i++) {
                        char c = optarg[i];
                        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '/')) {
                            error_log(PISSTVPP2_ERR_ARG_CALLSIGN_INVALID,
                                    "Callsign contains invalid character: '%c' (use A-Z, 0-9, / only)", c);
                            return PISSTVPP2_ERR_ARG_CALLSIGN_INVALID;
                        }
                    }
                    strncpy(config->cw_callsign, optarg, 
                            sizeof(config->cw_callsign) - 1);
                    config->cw_enabled = 1;
                }
                break;

            // CW Words Per Minute
            case 'W': {
                w_flag = 1;
                // Reject leading zeros (potential octal confusion)
                if (optarg[0] == '0' && optarg[1] != '\0') {
                    error_log(PISSTVPP2_ERR_ARG_CW_INVALID_WPM,
                            "Invalid CW WPM: %s (no leading zeros allowed)", optarg);
                    return PISSTVPP2_ERR_ARG_CW_INVALID_WPM;
                }
                char *endptr = NULL;
                errno = 0;
                long tmp = strtol(optarg, &endptr, 10);

                // Check for invalid input: non-numeric, partial conversion, etc.
                if (errno != 0 || endptr == optarg || *endptr != '\0') {
                    error_log(PISSTVPP2_ERR_ARG_CW_INVALID_WPM,
                            "Invalid CW WPM: %s (must be decimal number)", optarg);
                    return PISSTVPP2_ERR_ARG_CW_INVALID_WPM;
                }

                if (tmp < CONFIG_MIN_CW_WPM || tmp > CONFIG_MAX_CW_WPM) {
                    error_log(PISSTVPP2_ERR_ARG_CW_INVALID_WPM,
                            "CW WPM must be %d-%d",
                            CONFIG_MIN_CW_WPM, CONFIG_MAX_CW_WPM);
                    return PISSTVPP2_ERR_ARG_CW_INVALID_WPM;
                }
                config->cw_wpm = (uint8_t)tmp;
                break;
            }

            // CW Tone Frequency (-Q <hz>)
            // MOVED from -T (now used for text overlays)
            case 'Q': {
                t_flag = 1;
                // Reject leading zeros (potential octal confusion)
                if (optarg[0] == '0' && optarg[1] != '\0') {
                    error_log(PISSTVPP2_ERR_ARG_CW_INVALID_TONE,
                            "Invalid CW tone: %s (no leading zeros allowed)", optarg);
                    return PISSTVPP2_ERR_ARG_CW_INVALID_TONE;
                }
                char *endptr = NULL;
                errno = 0;
                long tmp = strtol(optarg, &endptr, 10);

                // Check for invalid input: non-numeric, partial conversion, etc.
                if (errno != 0 || endptr == optarg || *endptr != '\0') {
                    error_log(PISSTVPP2_ERR_ARG_CW_INVALID_TONE,
                            "Invalid CW tone: %s (must be decimal number in Hz)", optarg);
                    return PISSTVPP2_ERR_ARG_CW_INVALID_TONE;
                }

                if (tmp < CONFIG_MIN_CW_TONE || tmp > CONFIG_MAX_CW_TONE) {
                    error_log(PISSTVPP2_ERR_ARG_CW_INVALID_TONE,
                            "CW tone must be %d-%d Hz",
                            CONFIG_MIN_CW_TONE, CONFIG_MAX_CW_TONE);
                    return PISSTVPP2_ERR_ARG_CW_INVALID_TONE;
                }
                config->cw_tone = (uint16_t)tmp;
                break;
            }

            // Verbose Output
            case 'v':
                config->verbose = 1;
                break;

            // Timestamp Logging
            case 'Z':
                config->timestamp_logging = 1;
                config->verbose = 1;  // Auto-enable verbose
                break;

            // Keep Intermediate Files
            case 'K':
                config->keep_intermediate = 1;
                break;







            // Overlay Placement (-P <placement>)
            case 'P':
                if (!config->current_overlay) {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "-P flag requires -T <spec> to precede it");
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                config->current_overlay->placement = overlay_parse_placement(optarg);
                if (config->verbose) {
                    printf("[CLI] Set overlay placement: %s\n", optarg);
                }
                break;

            // Overlay Background Color (-B <color>)
            case 'B':
                if (!config->current_overlay) {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "-B flag requires -T <spec> to precede it");
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                result = overlay_parse_color(optarg, &config->current_overlay->bg_color);
                if (result != PISSTVPP2_OK) {
                    error_log(result, "Invalid background color: %s", optarg);
                    return result;
                }
                if (config->verbose) {
                    printf("[CLI] Set overlay background color: %s\n", optarg);
                }
                break;

            // Overlay Font Size (-F <size>)
            case 'F': {
                if (!config->current_overlay) {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "-F flag requires -T <spec> to precede it");
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                char *endptr = NULL;
                errno = 0;
                long font_size = strtol(optarg, &endptr, 10);

                if (errno != 0 || endptr == optarg || *endptr != '\0') {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "Invalid font size: %s (must be decimal number)", optarg);
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }

                if (font_size < 8 || font_size > 96) {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "Font size must be 8-96 pixels, got %ld", font_size);
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                config->current_overlay->font_size = (uint8_t)font_size;
                if (config->verbose) {
                    printf("[CLI] Set overlay font size: %ld\n", font_size);
                }
                break;
            }

            // Overlay Text Alignment (-A <align>)
            case 'A':
                if (!config->current_overlay) {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "-A flag requires -T <spec> to precede it");
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                config->current_overlay->text_align = overlay_parse_alignment(optarg);
                if (config->verbose) {
                    printf("[CLI] Set overlay text alignment: %s\n", optarg);
                }
                break;

            // Overlay Image Alignment/Placement (-I <placement>)
            // Controls where the text overlay appears on the image
            case 'I':
                if (!config->current_overlay) {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "-I flag requires -T <spec> to precede it");
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                
                if (strcmp(optarg, "top") == 0) {
                    config->current_overlay->placement = OVERLAY_PLACE_TOP;
                } else if (strcmp(optarg, "bottom") == 0) {
                    config->current_overlay->placement = OVERLAY_PLACE_BOTTOM;
                } else if (strcmp(optarg, "left") == 0) {
                    config->current_overlay->placement = OVERLAY_PLACE_LEFT;
                } else if (strcmp(optarg, "right") == 0) {
                    config->current_overlay->placement = OVERLAY_PLACE_RIGHT;
                } else if (strcmp(optarg, "center") == 0 || strcmp(optarg, "middle") == 0) {
                    config->current_overlay->placement = OVERLAY_PLACE_CENTER;
                } else {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "Image alignment must be 'top', 'bottom', 'left', 'right', or 'center', got '%s'", optarg);
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                if (config->verbose) {
                    printf("[CLI] Set overlay image alignment: %s\n", optarg);
                }
                break;

            // Overlay Background Mode (-M <mode>)
            // Sets how background is rendered: opaque, transparent, or semi-transparent
            case 'M':
                if (!config->current_overlay) {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "-M flag requires -T <spec> to precede it");
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                
                if (strcmp(optarg, "opaque") == 0) {
                    config->current_overlay->bg_mode = BG_OPAQUE;
                } else if (strcmp(optarg, "transparent") == 0) {
                    config->current_overlay->bg_mode = BG_TRANSPARENT;
                } else if (strcmp(optarg, "semi") == 0 || strcmp(optarg, "semi-transparent") == 0) {
                    config->current_overlay->bg_mode = BG_SEMI;
                } else {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "Background mode must be 'opaque', 'transparent', or 'semi', got '%s'", optarg);
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                if (config->verbose) {
                    printf("[CLI] Set overlay background mode: %s\n", optarg);
                }
                break;

            // Color Bars (-R <position>:<color1>,<color2>,...)
            // Adds colored bands/stripes to image for visual separation of overlay areas
            // Example: -R "top:red" or -R "bottom:white,cyan,magenta"
            case 'R': {
                // Parse the color bar specification
                char spec_copy[256];
                strncpy(spec_copy, optarg, sizeof(spec_copy) - 1);
                spec_copy[sizeof(spec_copy) - 1] = '\0';
                
                // Find the colon separating position from colors
                char *colon_pos = strchr(spec_copy, ':');
                if (!colon_pos) {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "Color bar format error: expected 'position:color1,color2,...', got '%s'", optarg);
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                
                // Split at the colon
                *colon_pos = '\0';
                char *position_str = spec_copy;
                char *colors_str = colon_pos + 1;
                
                // Create a new color bar
                ColorBar bar = colorbar_create_default();
                bar.position = colorbar_parse_position(position_str);
                
                // Parse color list (comma-separated color names or hex codes)
                int color_count = 0;
                char colors_copy[256];
                strncpy(colors_copy, colors_str, sizeof(colors_copy) - 1);
                
                char *color_token = strtok(colors_copy, ",");
                while (color_token && color_count < COLORBAR_MAX_COLORS) {
                    // Trim whitespace
                    while (*color_token == ' ') color_token++;
                    char *end = color_token + strlen(color_token) - 1;
                    while (end > color_token && *end == ' ') *end-- = '\0';
                    
                    // Parse color
                    int color_result = overlay_parse_color(color_token, &bar.colors[color_count]);
                    if (color_result != PISSTVPP2_OK) {
                        error_log(color_result, "Invalid color in bar specification: %s", color_token);
                        return color_result;
                    }
                    
                    color_count++;
                    color_token = strtok(NULL, ",");
                }
                
                if (color_count == 0) {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "Color bar must have at least one color");
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                
                bar.num_colors = color_count;
                
                // Add color bar to list
                result = colorbar_list_add(&config->colorbar_specs, &bar);
                if (result != PISSTVPP2_OK) {
                    error_log(result, "Failed to add color bar specification");
                    return result;
                }
                
                if (config->verbose) {
                    printf("[CLI] Added color bar: position=%s, %d colors\n", position_str, color_count);
                }
                break;
            }

            // Overlay Padding/Extra Space (-X <pixels>)
            // Controls spacing around text within the overlay box
            case 'X': {
                if (!config->current_overlay) {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "-X flag requires -T <spec> to precede it");
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                
                char *endptr = NULL;
                errno = 0;
                long padding = strtol(optarg, &endptr, 10);
                
                if (errno != 0 || endptr == optarg || *endptr != '\0') {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "Invalid padding value: %s (must be decimal number)", optarg);
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                
                if (padding < 0 || padding > 50) {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "Padding must be 0-50 pixels, got %ld", padding);
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                
                config->current_overlay->padding = (uint16_t)padding;
                if (config->verbose) {
                    printf("[CLI] Set overlay padding: %ld pixels\n", padding);
                }
                break;
            }

            // Overlay Border Width (-D <pixels>)
            // Controls border/stroke width around the overlay box
            case 'D': {
                if (!config->current_overlay) {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "-D flag requires -T <spec> to precede it");
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                
                char *endptr = NULL;
                errno = 0;
                long border_width = strtol(optarg, &endptr, 10);
                
                if (errno != 0 || endptr == optarg || *endptr != '\0') {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "Invalid border width: %s (must be decimal number)", optarg);
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                
                if (border_width < 0 || border_width > 10) {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "Border width must be 0-10 pixels, got %ld", border_width);
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                
                config->current_overlay->border_width = (uint16_t)border_width;
                if (config->verbose) {
                    printf("[CLI] Set overlay border width: %ld pixels\n", border_width);
                }
                break;
            }

            // Overlay Vertical Alignment (-V <valign>)
            // Controls vertical alignment of text within overlay box (top, center, bottom)
            case 'V':
                if (!config->current_overlay) {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "-V flag requires -T <spec> to precede it");
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                
                if (strcmp(optarg, "top") == 0) {
                    config->current_overlay->valign = VALIGN_TOP;
                } else if (strcmp(optarg, "center") == 0 || strcmp(optarg, "middle") == 0) {
                    config->current_overlay->valign = VALIGN_CENTER;
                } else if (strcmp(optarg, "bottom") == 0) {
                    config->current_overlay->valign = VALIGN_BOTTOM;
                } else {
                    error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                            "Vertical alignment must be 'top', 'center', or 'bottom', got '%s'", optarg);
                    return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
                }
                if (config->verbose) {
                    printf("[CLI] Set overlay vertical alignment: %s\n", optarg);
                }
                break;

            // Unified Text Overlay Specification (-T "text|size=20|align=center|...")
            // Primary interface for all text overlay functionality
            // Format: "text|key1=value1|key2=value2|..."
            case 'T': {
                if (strlen(optarg) >= OVERLAY_MAX_TEXT_LENGTH) {
                    error_log(PISSTVPP2_ERR_ARG_CALLSIGN_INVALID,
                            "Overlay specification too long (max %d chars)",
                            OVERLAY_MAX_TEXT_LENGTH);
                    return PISSTVPP2_ERR_ARG_CALLSIGN_INVALID;
                }
                
                // Finalize any previous overlay before creating new one
                result = pisstvpp_config_finalize_current_overlay(config);
                if (result != PISSTVPP2_OK) {
                    error_log(result, "Failed to finalize previous overlay");
                    return result;
                }
                
                // Allocate new overlay with malloc and parse unified specification
                config->current_overlay = (TextOverlaySpec *)malloc(sizeof(TextOverlaySpec));
                if (!config->current_overlay) {
                    error_log(PISSTVPP2_ERR_MEMORY_ALLOC, "Failed to allocate overlay spec");
                    return PISSTVPP2_ERR_MEMORY_ALLOC;
                }
                
                // Parse the unified specification format
                int parse_result = overlay_parse_unified_spec(optarg, config->current_overlay);
                if (parse_result != PISSTVPP2_OK) {
                    error_log(parse_result, "Failed to parse overlay specification: %s", optarg);
                    free(config->current_overlay);
                    config->current_overlay = NULL;
                    return parse_result;
                }
                
                if (config->verbose) {
                    printf("[CLI] Created unified overlay: '%s'\n", config->current_overlay->text);
                }
                break;
            }

            // Skip Audio Encoding (-N)
            // Test overlay functionality without audio encoding
            case 'N':
                config->skip_audio_encoding = 1;
                if (config->verbose) {
                    printf("[CLI] Audio encoding disabled (test mode)\n");
                }
                break;

            // Text-Only Overlay (-O)
            // Skip image resizing and aspect ratio correction (only valid with -N)
            case 'O':
                config->text_only = 1;
                if (config->verbose) {
                    printf("[CLI] Text-only overlay mode (no resize/aspect correction)\n");
                }
                break;

            // Help
            case 'h':
                show_detailed_help(argv[0]);
                return PISSTVPP2_OK;

            // Unknown option
            case '?':
            default:
                error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                        "Unknown option or missing argument");
                return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
        }
    }

    // =====================================================================
    // FINALIZE CURRENT OVERLAY (if any was being configured)
    // =====================================================================

    result = pisstvpp_config_finalize_current_overlay(config);
    if (result != PISSTVPP2_OK) {
        error_log(result, "Failed to finalize overlay during parsing");
        return result;
    }
    // =====================================================================
    // BACKWARD COMPATIBILITY
    // =====================================================================
    // Legacy overlay flags have been sunset in favor of unified -T option

    result = apply_backward_compatibility(config);
    if (result != PISSTVPP2_OK) {
        error_log(result, "Failed to apply backward compatibility mapping");
        return result;
    }

    // =====================================================================
    // VALIDATION: INTERDEPENDENCIES
    // =====================================================================

    // Check for required input file
    if (strlen(config->input_file) == 0) {
        error_log(PISSTVPP2_ERR_NO_INPUT_FILE, 
                "Input file (-i) is required");
        return PISSTVPP2_ERR_NO_INPUT_FILE;
    }

    // CW parameters require callsign
    if ((w_flag || t_flag) && strlen(config->cw_callsign) == 0) {
        error_log(PISSTVPP2_ERR_ARG_CW_MISSING_CALLSIGN,
                "-C <callsign> is required if -W or -T are provided");
        return PISSTVPP2_ERR_ARG_CW_MISSING_CALLSIGN;
    }

    // Text-only mode (-O) requires test mode (-N)
    if (config->text_only && !config->skip_audio_encoding) {
        error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                "-O (text-only overlay) can only be used with -N (test mode)");
        return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
    }

    // =====================================================================
    // DERIVED SETTINGS
    // =====================================================================

    // Auto-enable intermediate files in verbose mode or when overlays present
    // (users need to see the overlay result to verify correctness)
    if (config->verbose || overlay_spec_list_count(&config->overlay_specs) > 0) {
        config->keep_intermediate = 1;
    }

    // Auto-enable intermediate files when skipping audio encoding
    // (the intermediate image is the entire output in this case)
    if (config->skip_audio_encoding) {
        config->keep_intermediate = 1;
    }

    // Auto-generate output filename if not specified
    if (strlen(config->output_file) == 0) {
        result = pisstvpp_config_autogen_output_filename(config);
        if (result != PISSTVPP2_OK) {
            error_log(result, "Failed to generate output filename");
            return result;
        }
    }

    return PISSTVPP2_OK;
}

/**
 * @brief Finalize current overlay and add to overlay list
 */
int pisstvpp_config_finalize_current_overlay(PisstvppConfig *config) {
    if (!config) {
        return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
    }

    // No-op if no current overlay
    if (!config->current_overlay) {
        return PISSTVPP2_OK;
    }

    // Only add to list if overlay has text content
    if (strlen(config->current_overlay->text) == 0) {
        free(config->current_overlay);
        config->current_overlay = NULL;
        return PISSTVPP2_OK;
    }

    // Add overlay to list
    int result = overlay_spec_list_add(&config->overlay_specs, config->current_overlay);
    if (result != PISSTVPP2_OK) {
        error_log(result, "Failed to add overlay spec to list");
        free(config->current_overlay);
        config->current_overlay = NULL;
        return result;
    }

    // Clear the pointer
    free(config->current_overlay);
    config->current_overlay = NULL;

    return PISSTVPP2_OK;
}

/**
 * @brief Validate complete configuration for consistency
 */
int pisstvpp_config_validate(const PisstvppConfig *config) {
    if (!config) {
        return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
    }

    // Input file required
    if (strlen(config->input_file) == 0) {
        error_log(PISSTVPP2_ERR_NO_INPUT_FILE,
                "Configuration missing required input file");
        return PISSTVPP2_ERR_NO_INPUT_FILE;
    }

    // Protocol must be valid
    if (!is_valid_protocol(config->protocol)) {
        error_log(PISSTVPP2_ERR_ARG_INVALID_PROTOCOL,
                "Configuration contains invalid protocol: %s",
                config->protocol);
        return PISSTVPP2_ERR_ARG_INVALID_PROTOCOL;
    }

    // Format must be valid
    if (!pisstvpp_config_is_format_supported(config->format)) {
        error_log(PISSTVPP2_ERR_ARG_INVALID_FORMAT,
                "Configuration contains unsupported format: %s",
                config->format);
        return PISSTVPP2_ERR_ARG_INVALID_FORMAT;
    }

    // Sample rate in valid range
    if (config->sample_rate < CONFIG_MIN_SAMPLE_RATE || 
        config->sample_rate > CONFIG_MAX_SAMPLE_RATE) {
        error_log(PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE,
                "Configuration has invalid sample rate: %d Hz",
                config->sample_rate);
        return PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE;
    }

    // CW interdependencies
    if (config->cw_enabled) {
        if (strlen(config->cw_callsign) == 0) {
            error_log(PISSTVPP2_ERR_ARG_CALLSIGN_INVALID,
                    "CW enabled but callsign not set");
            return PISSTVPP2_ERR_ARG_CALLSIGN_INVALID;
        }
        if (config->cw_wpm < CONFIG_MIN_CW_WPM || 
            config->cw_wpm > CONFIG_MAX_CW_WPM) {
            error_log(PISSTVPP2_ERR_ARG_CW_INVALID_WPM,
                    "CW WPM out of range: %d", config->cw_wpm);
            return PISSTVPP2_ERR_ARG_CW_INVALID_WPM;
        }
        if (config->cw_tone < CONFIG_MIN_CW_TONE || 
            config->cw_tone > CONFIG_MAX_CW_TONE) {
            error_log(PISSTVPP2_ERR_ARG_CW_INVALID_TONE,
                    "CW tone out of range: %d Hz", config->cw_tone);
            return PISSTVPP2_ERR_ARG_CW_INVALID_TONE;
        }
    }

    return PISSTVPP2_OK;
}

/**
 * @brief Get human-readable protocol name
 */
const char* pisstvpp_config_get_protocol_name(const char *protocol) {
    if (!protocol) {
        return "Unknown";
    }

    // Martin modes
    if (strcmp(protocol, "m1") == 0) return "Martin 1";
    if (strcmp(protocol, "m2") == 0) return "Martin 2";

    // Scottie modes
    if (strcmp(protocol, "s1") == 0) return "Scottie 1";
    if (strcmp(protocol, "s2") == 0) return "Scottie 2";
    if (strcmp(protocol, "sdx") == 0) return "Scottie DX";

    // Robot modes
    if (strcmp(protocol, "r36") == 0) return "Robot 36";
    if (strcmp(protocol, "r72") == 0) return "Robot 72";

    return "Unknown";
}

/**
 * @brief Print current configuration to output
 */
void pisstvpp_config_print(const PisstvppConfig *config) {
    if (!config) {
        return;
    }

    printf("=== Configuration Summary ===\n");
    printf("Input file:      %s\n", config->input_file);
    printf("Output file:     %s\n", 
                strlen(config->output_file) ? config->output_file : 
                "(auto-generated)");
    printf("Protocol:        %s (%s)\n", config->protocol,
                pisstvpp_config_get_protocol_name(config->protocol));
    printf("Format:          %s\n", config->format);
    printf("Sample rate:     %d Hz\n", config->sample_rate);
    printf("Aspect mode:     %s\n",
                config->aspect_mode == ASPECT_CENTER ? "center-crop" :
                config->aspect_mode == ASPECT_PAD ? "padding" : "stretch");

    if (config->cw_enabled) {
        printf("CW Callsign:     %s\n", config->cw_callsign);
        printf("CW WPM:          %d\n", config->cw_wpm);
        printf("CW Tone:         %d Hz\n", config->cw_tone);
    } else {
        printf("CW Signature:    Disabled\n");
    }

    printf("Verbose:         %s\n", config->verbose ? "Yes" : "No");
    printf("Timestamps:      %s\n", config->timestamp_logging ? "Yes" : "No");
    printf("Keep Intermed:   %s\n", config->keep_intermediate ? "Yes" : "No");
    printf("=============================\n");
}

/**
 * @brief Check if audio format is supported
 */
int pisstvpp_config_is_format_supported(const char *format) {
    if (!format) {
        return 0;
    }

    // Always available
    if (strcmp(format, "wav") == 0 || strcmp(format, "aiff") == 0) {
        return 1;
    }

    // Conditional on OGG support
#ifdef HAVE_OGG_SUPPORT
    if (strcmp(format, "ogg") == 0) {
        return 1;
    }
#endif

    return 0;
}

/**
 * @brief Check if protocol is valid
 */
int pisstvpp_config_is_protocol_valid(const char *protocol) {
    return is_valid_protocol(protocol);
}

/**
 * @brief Clean up configuration
 */
void pisstvpp_config_cleanup(PisstvppConfig *config) {
    if (!config) {
        return;
    }
    
    // Cleanup current overlay if still allocated
    if (config->current_overlay) {
        free(config->current_overlay);
        config->current_overlay = NULL;
    }
    
    // Cleanup overlay spec list
    overlay_spec_list_cleanup(&config->overlay_specs);
    
    // Cleanup color bar spec list
    colorbar_list_cleanup(&config->colorbar_specs);
}

/**
 * @brief Auto-generate output filename
 */
int pisstvpp_config_autogen_output_filename(PisstvppConfig *config) {
    if (!config || strlen(config->input_file) == 0) {
        return PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG;
    }

    // Copy input filename
    const char *input = config->input_file;
    int input_len = strlen(input);
    
    // Find last dot for extension
    int dot_pos = -1;
    for (int i = input_len - 1; i >= 0; i--) {
        if (input[i] == '.') {
            dot_pos = i;
            break;
        }
        if (input[i] == '/') {
            break;  // Stop at directory separator
        }
    }

    // Build output filename
    int base_len = (dot_pos > 0) ? dot_pos : input_len;
    const char *ext = get_format_extension(config->format);
    int output_len = base_len + strlen(ext) + 1;

    if (output_len >= sizeof(config->output_file)) {
        error_log(PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG,
                "Generated filename too long");
        return PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG;
    }

    snprintf(config->output_file, sizeof(config->output_file),
             "%.*s%s", base_len, input, ext);

    return PISSTVPP2_OK;
}

// ===========================================================================
// STATIC HELPER FUNCTIONS
// ===========================================================================

/**
 * @brief Check if protocol code is valid
 */
static int is_valid_protocol(const char *protocol) {
    if (!protocol) {
        return 0;
    }

    // Martin modes
    if (strcmp(protocol, "m1") == 0 || strcmp(protocol, "m2") == 0)
        return 1;

    // Scottie modes
    if (strcmp(protocol, "s1") == 0 || strcmp(protocol, "s2") == 0 ||
        strcmp(protocol, "sdx") == 0)
        return 1;

    // Robot modes
    if (strcmp(protocol, "r36") == 0 || strcmp(protocol, "r72") == 0)
        return 1;

    return 0;
}

/**
 * @brief Get file extension for format
 */
static const char* get_format_extension(const char *format) {
    if (!format) {
        return ".wav";
    }

    if (strcmp(format, "aiff") == 0) {
        return ".aiff";
    }
    if (strcmp(format, "ogg") == 0) {
        return ".ogg";
    }

    return ".wav";
}

/**
 * @brief Display help text
 */
/**
 * @brief Display short help message
 * Shown when no arguments are provided or on usage errors.
 */
static void show_help(const char *program_name) {
    printf("PiSSTVpp2 - SSTV Image to Audio Encoder\n\n");
    printf("Usage: %s -i <input> [options]\n\n", 
           program_name ? program_name : "pisstvpp2");

    printf("Required Arguments:\n");
    printf("  -i <file>        Input image file\n\n");

    printf("Common Options:\n");
    printf("  -o <file>        Output audio file (default: input.wav)\n");
    printf("  -p <protocol>    SSTV protocol: m1, m2, s1, s2, sdx, r36, r72 (default: m1)\n");
    printf("  -f <format>      Audio format: wav, aiff, ogg (default: wav)\n");
    printf("  -r <rate>        Sample rate in Hz 8000-48000 (default: 22050)\n");
    printf("  -T <spec>        Text overlay: \"text|size=20|color=white|pos=top\"\n");
    printf("  -C <callsign>    Amateur radio callsign for CW signature\n");
    printf("  -v               Verbose output\n");
    printf("  -h               Detailed help (all options and examples)\n\n");

    printf("Examples:\n");
    printf("  %s -i photo.jpg\n", program_name ? program_name : "pisstvpp2");
    printf("  %s -i photo.jpg -p s2 -C N0CALL\n", program_name ? program_name : "pisstvpp2");
    printf("  %s -i photo.jpg -T \"Callsign|size=16|pos=bottom\"\n\n", program_name ? program_name : "pisstvpp2");
    printf("For detailed help on all options and styling: %s -h\n",
           program_name ? program_name : "pisstvpp2");
}

/**
 * @brief Display detailed help message with all options and styling information
 * Shown when user explicitly requests -h or --help
 */
static void show_detailed_help(const char *program_name) {
    printf("════════════════════════════════════════════════════════════════\n");
    printf("PiSSTVpp2 - SSTV Image to Audio Encoder - DETAILED HELP\n");
    printf("════════════════════════════════════════════════════════════════\n\n");

    printf("REQUIRED ARGUMENTS:\n");
    printf("─────────────────────────────────────────────────────────────────\n");
    printf("  -i <file>        Input image file (PNG, JPEG, GIF, BMP, TIFF, WebP)\n\n");

    printf("AUDIO OUTPUT OPTIONS:\n");
    printf("─────────────────────────────────────────────────────────────────\n");
    printf("  -o <file>        Output audio file (default: input_basename.wav)\n");
    printf("  -p <protocol>    SSTV protocol to use (default: m1)\n");
    printf("                   m1   - Martin 1 (VIS 44)\n");
    printf("                   m2   - Martin 2 (VIS 40)\n");
    printf("                   s1   - Scottie 1 (VIS 60)\n");
    printf("                   s2   - Scottie 2 (VIS 56)\n");
    printf("                   sdx  - Scottie DX (VIS 76)\n");
    printf("                   r36  - Robot 36 Color (VIS 8)\n");
    printf("                   r72  - Robot 72 Color (VIS 12)\n");
    printf("  -f <format>      Audio format (default: wav)\n");
    printf("                   wav  - Lossless PCM WAV\n");
    printf("                   aiff - Audio Interchange File Format\n");
    printf("                   ogg  - Ogg Vorbis compressed\n");
    printf("  -r <rate>        Audio sample rate in Hz (default: 22050)\n");
    printf("                   Valid range: 8000-48000\n");
    printf("                   Higher rates increase file size but may improve quality\n");
    printf("  -a <mode>        Aspect ratio correction (default: center)\n");
    printf("                   center  - Center image with black bars\n");
    printf("                   pad     - Pad image (may distort)\n");
    printf("                   stretch - Stretch to fit (may distort)\n\n");

    printf("TEXT OVERLAY OPTIONS:\n");
    printf("─────────────────────────────────────────────────────────────────\n");
    printf("  -T <spec>        Add text overlay with embedded styling\n\n");
    printf("  Format: \"-T \\\"text|setting=value|setting=value...\\\"\"\n");
    printf("  Multiple overlays: Use -T multiple times\n\n");

    printf("  TEXT STYLING SETTINGS:\n");
    printf("    size=<1-72>      Font size in pixels (default: 16)\n");
    printf("    color=<color>    Text color by name or hex code (default: white)\n");
    printf("    bg=<color>       Background color for text box (default: none)\n");
    printf("    pad=<pixels>     Padding around text (default: 2)\n");
    printf("    border=<pixels>  Border around text box (default: 0)\n");
    printf("    v-align=<opt>    Vertical alignment: top, center, bottom (default: center)\n");
    printf("    align=<opt>      Horizontal alignment: left, center, right (default: left)\n");
    printf("    mode=<opt>       Overlay mode: blend or opaque (default: blend)\n\n");

    printf("  POSITIONING SETTINGS:\n");
    printf("    pos=<location>   Quick position: top, bottom, center, topleft, etc.\n");
    printf("    x=<pixels>       Absolute X coordinate from left\n");
    printf("    y=<pixels>       Absolute Y coordinate from top\n");
    printf("    pos=<x:y>        Coordinate pair: x:y format\n\n");

    printf("  COLOR OPTIONS:\n");
    printf("    Named colors: white, black, red, green, blue, yellow, cyan, magenta,\n");
    printf("                  orange, gray, darkgray, lightgray, navy, purple, etc.\n");
    printf("    Hex codes: #RRGGBB format (e.g., #FF0000 = red)\n\n");

    printf("  BACKGROUND BAR (for visibility on weak/degraded signals):\n");
    printf("    bgbar=<bool>     Enable/disable background bar: true, false, 1, 0, yes, enable\n");
    printf("                     Creates a solid-color bar behind text for maximum visibility\n");
    printf("                     Perfect for HF SSTV where signal degradation is common\n");
    printf("    bgbar-margin=N   Extra margin for background bar (0-30 pixels, default: 4)\n");
    printf("                     Margin extends beyond text padding for stronger visibility\n");
    printf("                     Recommended: 3-6 pixels for typical overlays\n");
    printf("    bg=<color>       Set background bar fill color (use with bgbar=true)\n");
    printf("                     Provides high contrast for text readability\n");
    printf("    pad=<pixels>     Interior padding between text and bar edge (default: 2)\n");
    printf("    border=<pixels>  Optional border width around bar (default: 0)\n\n");

    printf("  TEXT OVERLAY EXAMPLES:\n");
    printf("    -T \\\"N0CALL\\\"                          # Simple text, default styling\n");
    printf("    -T \\\"N0CALL|size=24|color=yellow\\\"    # Larger yellow text\n");
    printf("    -T \\\"N0CALL|size=20|pos=bottom|color=white|bg=blue|pad=4\\\"\n");
    printf("    # HIGH-CONTRAST BAR (HF weak signal visibility):\n");
    printf("    -T \\\"W5ABC|size=16|color=white|bg=black|bgbar=true|bgbar-margin=4|pos=top\\\"\n");
    printf("    # CONTEST FORMAT (multiple overlays with bars):\n");
    printf("    -T \\\"Call|size=16|color=white|bg=navy|bgbar=true|pos=top\\\"\\\n");
    printf("         -T \\\"Grid|size=14|color=yellow|bg=darkblue|bgbar=true|pos=bottom\\\"\n\n");

    printf("CW SIGNATURE (AMATEUR RADIO):\n");
    printf("─────────────────────────────────────────────────────────────────\n");
    printf("  -C <callsign>    Ham radio callsign for CW encoding (max 31 chars)\n");
    printf("                   This adds CW audio signature to the transmission\n");
    printf("  -W <wpm>         CW transmission speed in words/minute (default: 15)\n");
    printf("                   Valid range: 1-50\n");
    printf("  -Q <hz>          CW tone frequency in Hz (default: 800)\n");
    printf("                   Valid range: 400-2000\n");
    printf("                   Lower frequencies are easier to hear, especially on HF\n\n");

    printf("DEBUGGING AND ANALYSIS:\n");
    printf("─────────────────────────────────────────────────────────────────\n");
    printf("  -v               Verbose output: show processing details and timings\n");
    printf("  -Z               Add timestamps to verbose output for performance analysis\n");
    printf("                   (implies -v)\n");
    printf("  -K               Keep intermediate processed images for inspection\n");
    printf("                   Useful for diagnosing image processing issues\n");
    printf("  -N               Skip audio encoding (test mode)\n");
    printf("                   Useful for testing overlays without audio generation\n\n");

    printf("COMPLETE EXAMPLES:\n");
    printf("───────────────────────────────────────────────────────────────────\n");
    printf("  # Basic conversion to default format\n");
    printf("  %s -i photo.jpg\n\n", program_name ? program_name : "pisstvpp2");

    printf("  # Scottie 2 protocol with different audio format\n");
    printf("  %s -i photo.jpg -p s2 -f ogg\n\n", program_name ? program_name : "pisstvpp2");

    printf("  # With amateur radio CW signature\n");
    printf("  %s -i photo.jpg -C W5ABC -W 20 -Q 900\n\n", program_name ? program_name : "pisstvpp2");

    printf("  # Multiple text overlays with styling\n");
    printf("  %s -i photo.jpg -T \\\"W5ABC|size=20|pos=top|color=white|bg=blue|pad=4\\\"\\\n", 
           program_name ? program_name : "pisstvpp2");
    printf("                                  -T \\\"EM12ab|size=16|pos=bottom|color=yellow\\\"\n\n");

    printf("  # HF SSTV weak signal (high-contrast background bars)\n");
    printf("  %s -i photo.jpg -T \\\"W5ABC|size=16|color=white|bg=black|bgbar=true|bgbar-margin=4|pos=top\\\"\\\n",
           program_name ? program_name : "pisstvpp2");
    printf("                               -T \\\"EM12AB|size=14|color=white|bg=black|bgbar=true|bgbar-margin=4\\\"\\\n");
    printf("                               -T \\\"559|size=16|color=yellow|bg=darkblue|bgbar=true|pos=bottom\\\"\n\n");

    printf("  # Verbose mode with timestamp analysis\n");
    printf("  %s -i photo.jpg -v -Z > session.log\n\n", program_name ? program_name : "pisstvpp2");

    printf("  # Test overlay rendering without audio encoding\n");
    printf("  %s -i photo.jpg -T \\\"Test|size=30|color=red|bg=white\\\" -N -K\n\n", 
           program_name ? program_name : "pisstvpp2");

    printf("COMPATIBILITY NOTES:\n");
    printf("───────────────────────────────────────────────────────────────────\n");
    printf("  • Image formats: Supports PNG, JPEG, GIF, BMP, TIFF, WebP via libvips\n");
    printf("  • Audio quality: Higher sample rates and protocols increase file size\n");
    printf("  • CW signature: Adds ~2-5 seconds based on callsign length and WPM\n");
    printf("  • For HF transmission: Consider lower CW frequency (400-600 Hz)\n");
    printf("  • Background bars: Recommended for weak/degraded signals (HF SSTV QSOs)\n");
    printf("                     Use bgbar=true with high-contrast colors (white on black)\n\n");

    printf("════════════════════════════════════════════════════════════════\n");
}

/**
 * @brief Public interface to show detailed help
 * @param program_name Name of the program (typically argv[0])
 */
void pisstvpp_config_show_detailed_help(const char *program_name) {
    show_detailed_help(program_name);
}
