/**
 * @file slowframe_config.c
 * @brief SlowFrame Configuration Management Module Implementation
 *
 * This module provides the implementation of the configuration management system.
 * It handles command-line parsing, validation, and configuration lifecycle.
 *
 * @author SlowFrame Contributors
 * @version 2.1.0
 * @date February 2026
 */

#include "slowframe_config.h"
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
// ===========================================================================
// PUBLIC FUNCTION IMPLEMENTATIONS
// ===========================================================================

/**
 * @brief Initialize configuration with default values
 */
int slowframe_config_init(SlowframeConfig *config) {
    if (!config) {
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;  // Generic error
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
    if (overlay_init_result != SLOWFRAME_OK) {
        error_log(overlay_init_result, "Failed to initialize overlay spec list");
        return overlay_init_result;
    }

    // Initialize color bar specification list (max 10 color bars)
    int colorbar_init_result = colorbar_list_init(&config->colorbar_specs, 10);
    if (colorbar_init_result != SLOWFRAME_OK) {
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
    config->list_modes = 0;
    config->mmsstv_status = 0;

    return SLOWFRAME_OK;
}

/**
 * @brief Parse command-line arguments into configuration
 */
int slowframe_config_parse(SlowframeConfig *config, int argc, char *argv[]) {
    if (!config || !argv) {
        error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL, "Invalid config or argv");
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }

    // Initialize config with defaults first
    int result = slowframe_config_init(config);
    if (result != SLOWFRAME_OK) {
        return result;
    }

    int option;
    int w_flag = 0;  // Track if -W used
    int t_flag = 0;  // Track if -T used

    // Parse all options
    // Overlay text: -T (unified text overlay with embedded styling)
    // Color bars: -R (color bar rows)
    // CW: -Q (CW tone frequency)
    // Text overlays: -T (unified text overlay specification)
    // Testing: -N (skip audio encoding for overlay testing)
    while ((option = getopt(argc, argv, "i:o:p:f:r:vC:W:Q:a:KZhNOR:T:LM")) != -1) {
        switch (option) {
            // Input file (REQUIRED)
            case 'i':
                if (strlen(optarg) >= sizeof(config->input_file)) {
                    error_log(SLOWFRAME_ERR_ARG_FILENAME_TOO_LONG,
                            "Input filename too long (max %d chars)",
                            CONFIG_MAX_FILENAME);
                    return SLOWFRAME_ERR_ARG_FILENAME_TOO_LONG;
                }
                strncpy(config->input_file, optarg, 
                        sizeof(config->input_file) - 1);
                break;

            // Output file (OPTIONAL)
            case 'o':
                if (strlen(optarg) == 0) {
                    error_log(SLOWFRAME_ERR_ARG_FILENAME_TOO_LONG,
                            "Output filename cannot be empty");
                    return SLOWFRAME_ERR_ARG_FILENAME_TOO_LONG;
                }
                if (strlen(optarg) >= sizeof(config->output_file)) {
                    error_log(SLOWFRAME_ERR_ARG_FILENAME_TOO_LONG,
                            "Output filename too long (max %d chars)",
                            CONFIG_MAX_FILENAME);
                    return SLOWFRAME_ERR_ARG_FILENAME_TOO_LONG;
                }
                // Validate output filename: reject dangerous characters
                for (int i = 0; optarg[i]; i++) {
                    char c = optarg[i];
                    // Reject shell metacharacters and control chars
                    if (c == '&' || c == '|' || c == ';' || c == '`' || c == '$' ||
                        c == '%' || c == '\n' || c == '\r' || c == '\t' ||
                        (unsigned char)c < 32) {
                        error_log(SLOWFRAME_ERR_ARG_FILENAME_TOO_LONG,
                                "Output filename contains invalid character (code %d)", (unsigned char)c);
                        return SLOWFRAME_ERR_ARG_FILENAME_TOO_LONG;
                    }
                }
                strncpy(config->output_file, optarg,
                        sizeof(config->output_file) - 1);
                break;

            // SSTV Protocol
            case 'p':
                // Store protocol string without validation
                // Validation happens later in main() against the full mode registry
                // (which includes both native and MMSSTV modes)
                strncpy(config->protocol, optarg, 
                        sizeof(config->protocol) - 1);
                break;

            // Audio Format
            case 'f':
                if (!slowframe_config_is_format_supported(optarg)) {
                    error_log(SLOWFRAME_ERR_ARG_INVALID_FORMAT,
                            "Unsupported format: %s", optarg);
                    return SLOWFRAME_ERR_ARG_INVALID_FORMAT;
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
                    error_log(SLOWFRAME_ERR_ARG_INVALID_SAMPLE_RATE,
                            "Invalid sample rate: %s", optarg);
                    return SLOWFRAME_ERR_ARG_INVALID_SAMPLE_RATE;
                }

                if (tmp < CONFIG_MIN_SAMPLE_RATE || tmp > CONFIG_MAX_SAMPLE_RATE) {
                    error_log(SLOWFRAME_ERR_ARG_INVALID_SAMPLE_RATE,
                            "Sample rate must be %d-%d Hz",
                            CONFIG_MIN_SAMPLE_RATE, CONFIG_MAX_SAMPLE_RATE);
                    return SLOWFRAME_ERR_ARG_INVALID_SAMPLE_RATE;
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
                    error_log(SLOWFRAME_ERR_ARG_INVALID_ASPECT,
                            "Aspect mode must be 'center', 'pad', or 'stretch'");
                    return SLOWFRAME_ERR_ARG_INVALID_ASPECT;
                }
                break;

            // CW Callsign OR Overlay Text Color (context-dependent)
            case 'C':
                // Check if we're in overlay styling context
                if (config->current_overlay && strlen(config->current_overlay->text) > 0) {
                    // We're styling current overlay - treat as text color
                    int color_result = overlay_parse_color(optarg, &config->current_overlay->text_color);
                    if (color_result != SLOWFRAME_OK) {
                        error_log(color_result, "Invalid overlay text color: %s", optarg);
                        return color_result;
                    }
                } else {
                    // Traditional CW callsign context
                    if (strlen(optarg) == 0) {
                        error_log(SLOWFRAME_ERR_ARG_CALLSIGN_INVALID,
                                "Callsign cannot be empty");
                        return SLOWFRAME_ERR_ARG_CALLSIGN_INVALID;
                    }
                    if (strlen(optarg) >= sizeof(config->cw_callsign)) {
                        error_log(SLOWFRAME_ERR_ARG_CALLSIGN_INVALID,
                                "Callsign too long (max %d chars)",
                                CONFIG_MAX_CALLSIGN);
                        return SLOWFRAME_ERR_ARG_CALLSIGN_INVALID;
                    }
                    // Validate callsign format: alphanumeric and slash only
                    for (int i = 0; optarg[i]; i++) {
                        char c = optarg[i];
                        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '/')) {
                            error_log(SLOWFRAME_ERR_ARG_CALLSIGN_INVALID,
                                    "Callsign contains invalid character: '%c' (use A-Z, 0-9, / only)", c);
                            return SLOWFRAME_ERR_ARG_CALLSIGN_INVALID;
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
                    error_log(SLOWFRAME_ERR_ARG_CW_INVALID_WPM,
                            "Invalid CW WPM: %s (no leading zeros allowed)", optarg);
                    return SLOWFRAME_ERR_ARG_CW_INVALID_WPM;
                }
                char *endptr = NULL;
                errno = 0;
                long tmp = strtol(optarg, &endptr, 10);

                // Check for invalid input: non-numeric, partial conversion, etc.
                if (errno != 0 || endptr == optarg || *endptr != '\0') {
                    error_log(SLOWFRAME_ERR_ARG_CW_INVALID_WPM,
                            "Invalid CW WPM: %s (must be decimal number)", optarg);
                    return SLOWFRAME_ERR_ARG_CW_INVALID_WPM;
                }

                if (tmp < CONFIG_MIN_CW_WPM || tmp > CONFIG_MAX_CW_WPM) {
                    error_log(SLOWFRAME_ERR_ARG_CW_INVALID_WPM,
                            "CW WPM must be %d-%d",
                            CONFIG_MIN_CW_WPM, CONFIG_MAX_CW_WPM);
                    return SLOWFRAME_ERR_ARG_CW_INVALID_WPM;
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
                    error_log(SLOWFRAME_ERR_ARG_CW_INVALID_TONE,
                            "Invalid CW tone: %s (no leading zeros allowed)", optarg);
                    return SLOWFRAME_ERR_ARG_CW_INVALID_TONE;
                }
                char *endptr = NULL;
                errno = 0;
                long tmp = strtol(optarg, &endptr, 10);

                // Check for invalid input: non-numeric, partial conversion, etc.
                if (errno != 0 || endptr == optarg || *endptr != '\0') {
                    error_log(SLOWFRAME_ERR_ARG_CW_INVALID_TONE,
                            "Invalid CW tone: %s (must be decimal number in Hz)", optarg);
                    return SLOWFRAME_ERR_ARG_CW_INVALID_TONE;
                }

                if (tmp < CONFIG_MIN_CW_TONE || tmp > CONFIG_MAX_CW_TONE) {
                    error_log(SLOWFRAME_ERR_ARG_CW_INVALID_TONE,
                            "CW tone must be %d-%d Hz",
                            CONFIG_MIN_CW_TONE, CONFIG_MAX_CW_TONE);
                    return SLOWFRAME_ERR_ARG_CW_INVALID_TONE;
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
                    error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL,
                            "Color bar format error: expected 'position:color1,color2,...', got '%s'", optarg);
                    return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
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
                    if (color_result != SLOWFRAME_OK) {
                        error_log(color_result, "Invalid color in bar specification: %s", color_token);
                        return color_result;
                    }
                    
                    color_count++;
                    color_token = strtok(NULL, ",");
                }
                
                if (color_count == 0) {
                    error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL,
                            "Color bar must have at least one color");
                    return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
                }
                
                bar.num_colors = color_count;
                
                // Add color bar to list
                result = colorbar_list_add(&config->colorbar_specs, &bar);
                if (result != SLOWFRAME_OK) {
                    error_log(result, "Failed to add color bar specification");
                    return result;
                }
                
                if (config->verbose) {
                    printf("[CLI] Added color bar: position=%s, %d colors\n", position_str, color_count);
                }
                break;
            }

            // Unified Text Overlay Specification (-T "text|size=20|align=center|...")
            // Primary interface for all text overlay functionality
            // Format: "text|key1=value1|key2=value2|..."
            case 'T': {
                if (strlen(optarg) >= OVERLAY_MAX_TEXT_LENGTH) {
                    error_log(SLOWFRAME_ERR_ARG_CALLSIGN_INVALID,
                            "Overlay specification too long (max %d chars)",
                            OVERLAY_MAX_TEXT_LENGTH);
                    return SLOWFRAME_ERR_ARG_CALLSIGN_INVALID;
                }
                
                // Finalize any previous overlay before creating new one
                result = slowframe_config_finalize_current_overlay(config);
                if (result != SLOWFRAME_OK) {
                    error_log(result, "Failed to finalize previous overlay");
                    return result;
                }
                
                // Allocate new overlay with malloc and parse unified specification
                config->current_overlay = (TextOverlaySpec *)malloc(sizeof(TextOverlaySpec));
                if (!config->current_overlay) {
                    error_log(SLOWFRAME_ERR_MEMORY_ALLOC, "Failed to allocate overlay spec");
                    return SLOWFRAME_ERR_MEMORY_ALLOC;
                }
                
                // Parse the unified specification format
                int parse_result = overlay_parse_unified_spec(optarg, config->current_overlay);
                if (parse_result != SLOWFRAME_OK) {
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
                return SLOWFRAME_OK;

            // List modes
            case 'L':
                config->list_modes = 1;
                return SLOWFRAME_OK;  // Early return - no further validation needed

            // MMSSTV status
            case 'M':
                config->mmsstv_status = 1;
                return SLOWFRAME_OK;  // Early return - no further validation needed

            // Unknown option
            case '?':
            default:
                error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL,
                        "Unknown option or missing argument");
                return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
        }
    }

    // =====================================================================
    // FINALIZE CURRENT OVERLAY (if any was being configured)
    // =====================================================================

    result = slowframe_config_finalize_current_overlay(config);
    if (result != SLOWFRAME_OK) {
        error_log(result, "Failed to finalize overlay during parsing");
        return result;
    }
    // =====================================================================
    // BACKWARD COMPATIBILITY
    // =====================================================================
    // Legacy overlay flags have been sunset in favor of unified -T option
    // Backward compatibility layer removed - all overlays now use -T flag

    // =====================================================================
    // VALIDATION: INTERDEPENDENCIES
    // =====================================================================

    // Check for required input file
    if (strlen(config->input_file) == 0) {
        error_log(SLOWFRAME_ERR_NO_INPUT_FILE, 
                "Input file (-i) is required");
        return SLOWFRAME_ERR_NO_INPUT_FILE;
    }

    // CW parameters require callsign
    if ((w_flag || t_flag) && strlen(config->cw_callsign) == 0) {
        error_log(SLOWFRAME_ERR_ARG_CW_MISSING_CALLSIGN,
                "-C <callsign> is required if -W or -Q are provided");
        return SLOWFRAME_ERR_ARG_CW_MISSING_CALLSIGN;
    }

    // Text-only mode (-O) requires test mode (-N)
    if (config->text_only && !config->skip_audio_encoding) {
        error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL,
                "-O (text-only overlay) can only be used with -N (test mode)");
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
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
        result = slowframe_config_autogen_output_filename(config);
        if (result != SLOWFRAME_OK) {
            error_log(result, "Failed to generate output filename");
            return result;
        }
    } else {
        // Auto-append extension if output file doesn't have one
        const char *output = config->output_file;
        int output_len = strlen(output);
        int has_extension = 0;
        
        // Check if filename has an extension (look for dot after last slash)
        for (int i = output_len - 1; i >= 0; i--) {
            if (output[i] == '.') {
                has_extension = 1;
                break;
            }
            if (output[i] == '/') {
                break;  // Reached directory separator, no extension
            }
        }
        
        // If no extension, append one based on format
        if (!has_extension) {
            const char *ext = get_format_extension(config->format);
            int new_len = output_len + strlen(ext);
            
            if (new_len >= (int)sizeof(config->output_file)) {
                error_log(SLOWFRAME_ERR_ARG_FILENAME_TOO_LONG,
                        "Output filename too long after adding extension");
                return SLOWFRAME_ERR_ARG_FILENAME_TOO_LONG;
            }
            
            strncat(config->output_file, ext, sizeof(config->output_file) - output_len - 1);
        }
    }

    return SLOWFRAME_OK;
}

/**
 * @brief Finalize current overlay and add to overlay list
 */
int slowframe_config_finalize_current_overlay(SlowframeConfig *config) {
    if (!config) {
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }

    // No-op if no current overlay
    if (!config->current_overlay) {
        return SLOWFRAME_OK;
    }

    // Only add to list if overlay has text content
    if (strlen(config->current_overlay->text) == 0) {
        free(config->current_overlay);
        config->current_overlay = NULL;
        return SLOWFRAME_OK;
    }

    // Add overlay to list
    int result = overlay_spec_list_add(&config->overlay_specs, config->current_overlay);
    if (result != SLOWFRAME_OK) {
        error_log(result, "Failed to add overlay spec to list");
        free(config->current_overlay);
        config->current_overlay = NULL;
        return result;
    }

    // Clear the pointer
    free(config->current_overlay);
    config->current_overlay = NULL;

    return SLOWFRAME_OK;
}

/**
 * @brief Validate complete configuration for consistency
 */
int slowframe_config_validate(const SlowframeConfig *config) {
    if (!config) {
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }

    // Input file required
    if (strlen(config->input_file) == 0) {
        error_log(SLOWFRAME_ERR_NO_INPUT_FILE,
                "Configuration missing required input file");
        return SLOWFRAME_ERR_NO_INPUT_FILE;
    }

    // Protocol validation is deferred to main() where the full mode registry
    // (including MMSSTV modes) is available. Just check it's not empty.
    if (strlen(config->protocol) == 0) {
        error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL,
                "Configuration missing required protocol");
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }

    // Format must be valid
    if (!slowframe_config_is_format_supported(config->format)) {
        error_log(SLOWFRAME_ERR_ARG_INVALID_FORMAT,
                "Configuration contains unsupported format: %s",
                config->format);
        return SLOWFRAME_ERR_ARG_INVALID_FORMAT;
    }

    // Sample rate in valid range
    if (config->sample_rate < CONFIG_MIN_SAMPLE_RATE || 
        config->sample_rate > CONFIG_MAX_SAMPLE_RATE) {
        error_log(SLOWFRAME_ERR_ARG_INVALID_SAMPLE_RATE,
                "Configuration has invalid sample rate: %d Hz",
                config->sample_rate);
        return SLOWFRAME_ERR_ARG_INVALID_SAMPLE_RATE;
    }

    // CW interdependencies
    if (config->cw_enabled) {
        if (strlen(config->cw_callsign) == 0) {
            error_log(SLOWFRAME_ERR_ARG_CALLSIGN_INVALID,
                    "CW enabled but callsign not set");
            return SLOWFRAME_ERR_ARG_CALLSIGN_INVALID;
        }
        if (config->cw_wpm < CONFIG_MIN_CW_WPM || 
            config->cw_wpm > CONFIG_MAX_CW_WPM) {
            error_log(SLOWFRAME_ERR_ARG_CW_INVALID_WPM,
                    "CW WPM out of range: %d", config->cw_wpm);
            return SLOWFRAME_ERR_ARG_CW_INVALID_WPM;
        }
        if (config->cw_tone < CONFIG_MIN_CW_TONE || 
            config->cw_tone > CONFIG_MAX_CW_TONE) {
            error_log(SLOWFRAME_ERR_ARG_CW_INVALID_TONE,
                    "CW tone out of range: %d Hz", config->cw_tone);
            return SLOWFRAME_ERR_ARG_CW_INVALID_TONE;
        }
    }

    return SLOWFRAME_OK;
}

/**
 * @brief Get human-readable protocol name
 */
const char* slowframe_config_get_protocol_name(const char *protocol) {
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
void slowframe_config_print(const SlowframeConfig *config) {
    if (!config) {
        return;
    }

    printf("=== Configuration Summary ===\n");
    printf("Input file:      %s\n", config->input_file);
    printf("Output file:     %s\n", 
                strlen(config->output_file) ? config->output_file : 
                "(auto-generated)");
    printf("Protocol:        %s (%s)\n", config->protocol,
                slowframe_config_get_protocol_name(config->protocol));
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
int slowframe_config_is_format_supported(const char *format) {
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
 * @brief Check if protocol is a built-in native mode
 * 
 * NOTE: This only checks NATIVE modes. For full validation including MMSSTV modes,
 * use the mode registry in the initialized context.
 * 
 * @deprecated Use mode_registry_lookup_by_code() instead for complete validation
 */
int slowframe_config_is_protocol_valid(const char *protocol) {
    return is_valid_protocol(protocol);
}

/**
 * @brief Clean up configuration
 */
void slowframe_config_cleanup(SlowframeConfig *config) {
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
int slowframe_config_autogen_output_filename(SlowframeConfig *config) {
    if (!config || strlen(config->input_file) == 0) {
        return SLOWFRAME_ERR_ARG_FILENAME_TOO_LONG;
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

    if (output_len >= (int)sizeof(config->output_file)) {
        error_log(SLOWFRAME_ERR_ARG_FILENAME_TOO_LONG,
                "Generated filename too long");
        return SLOWFRAME_ERR_ARG_FILENAME_TOO_LONG;
    }

    snprintf(config->output_file, sizeof(config->output_file),
             "%.*s%s", base_len, input, ext);

    return SLOWFRAME_OK;
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
 * @brief Display detailed help message with all options and styling information
 * Shown when user explicitly requests -h or --help
 */
static void show_detailed_help(const char *program_name) {
    printf("════════════════════════════════════════════════════════════════\n");
    printf("SlowFrame - SSTV Image to Audio Encoder - DETAILED HELP\n");
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

    printf("\n  ╔══════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("  ║                    QUICK REFERENCE: TEXT OVERLAY PARAMETERS                      ║\n");
    printf("  ╠═════════════════════╦═════════════════════╦═══════════════════╦══════════════════╣\n");
    printf("  ║ PARAMETER           ║ TYPE/VALUES         ║ RANGE/DEFAULT     ║ SSTV USE         ║\n");
    printf("  ╠═════════════════════╬═════════════════════╬═══════════════════╬══════════════════╣\n");
    printf("  ║ size                ║ pixels              ║ 8-72 (def:16)     ║ Readability      ║\n");
    printf("  ║ font                ║ family name         ║ sans/mono/serif   ║ Font style       ║\n");
    printf("  ║ color               ║ name or hex code    ║ see color list    ║ Text color       ║\n");
    printf("  ║ bg                  ║ name or hex code    ║ see color list    ║ Background       ║\n");
    printf("  ║ pad                 ║ pixels              ║ 0-30 (def:5)      ║ Text spacing     ║\n");
    printf("  ║ align               ║ left/center/right   ║ horizontal align  ║ H. alignment     ║\n");
    printf("  ║ v-align             ║ top/center/bottom   ║ vertical align    ║ V. alignment     ║\n");
    printf("  ║ pos                 ║ placement name      ║ 9 positions       ║ Position         ║\n");
    printf("  ║ offset-x, y         ║ pixels              ║ -100 to +100      ║ Fine tuning      ║\n");
    printf("  ║ border              ║ pixels              ║ 0-8 (def:0)       ║ Box outline      ║\n");
    printf("  ║ border-color        ║ name or hex code    ║ see color list    ║ Border color     ║\n");
    printf("  ║ mode                ║ opaque/semi/transp  ║ blend mode        ║ Opacity mode     ║\n");
    printf("  ║ opacity/alpha       ║ percentage          ║ 0-100 (def:50)    ║ Alpha level      ║\n");
    printf("  ║ bgbar               ║ true/false/yes/no   ║ color bar enabled ║ Solid color bar  ║\n");
    printf("  ║ bgbar-width         ║ full/half/auto/px   ║ bar width mode    ║ Bar coverage     ║\n");
    printf("  ║ bgbar-color         ║ name or hex code    ║ see color list    ║ Bar fill color   ║\n");
    printf("  ║ bgbar-margin        ║ pixels              ║ 0-15 (def:2)      ║ Bar spacing      ║\n");
    printf("  ║ width               ║ pixels              ║ 50-600 or 0       ║ Text wrap        ║\n");
    printf("  ║ wrap                ║ word/char/none      ║ wrap mode         ║ Line break       ║\n");
    printf("  ║ line-spacing        ║ multiplier          ║ 0.8-2.0 (def:1.0) ║ Line height      ║\n");
    printf("  ║ timestamp-fmt       ║ strftime format     ║ %%H:%%M:%%S...       ║ Dynamic time     ║\n");
    printf("  ╠═════════════════════╩═════════════════════╩═══════════════════╩══════════════════╣\n");
    printf("  ║  Default Placement: bottom-right  │  Default Font Size: 16px                     ║\n");
    printf("  ║  Default Color: white on black    │  Default Opacity: 50%% semi-transparent       ║\n");
    printf("  ╚══════════════════════════════════════════════════════════════════════════════════╝\n\n");

    printf("  SSTV-OPTIMIZED DEFAULTS:\n");
    printf("    • Position: bottom-right (standard callsign placement)\n");
    printf("    • Font size: 16px (fits 320x240 to 640x480 images)\n");
    printf("    • Text color: white (maximum contrast)\n");
    printf("    • Background: black with 50%% opacity (semi-transparent)\n");
    printf("    • Background bar: full-width mode enabled for signal protection\n");
    printf("    • Padding: 5px around text for readability\n");
    printf("    • Alignment: centered text (both horizontal and vertical)\n\n");

    printf("  COLOR OPTIONS (140+ named colors supported):\n");
    printf("    Basic: white, black, red, green, blue, yellow, cyan, magenta\n");
    printf("    Light: lightgray, lightred, lightgreen, lightblue, lightyellow\n");
    printf("    Dark: darkgray, darkred, darkgreen, darkblue, darkyellow\n");
    printf("    Web-safe: orange, purple, brown, pink, lime, navy, teal, olive, maroon\n");
    printf("    Hex codes: #RRGGBB format (e.g., #FF0000 = red, #FFFF00 = yellow)\n\n");

    printf("  PLACEMENT OPTIONS (9 positions):\n");
    printf("    Quick: top, bottom, left, right, center\n");
    printf("    Corners: top-left, top-right, bottom-left, bottom-right\n\n");

    printf("  TEXT OVERLAY EXAMPLES:\n");
    printf("    -T \\\"N0CALL\\\"                          # Simple text, default styling\n");
    printf("    -T \\\"N0CALL|size=20|color=yellow\\\"    # Larger yellow text\n");
    printf(" \n");
    printf("    -T \\\"N0CALL|size=16|pos=bottom|color=white|bg=black|pad=5\\\"\n");
    printf("    # HIGH-CONTRAST BAR (HF weak signal visibility - SSTV optimized):\n");
    printf(" \n");    
    printf("    -T \\\"W5ABC|size=16|color=white|bg=black|bgbar=true|pos=bottom\\\"\n");
    printf("    # MULTI-LINE TEXT:\n");
    printf(" \n");    
    printf("    -T \\\"W5ABC Grid|size=14|width=150|wrap=word|line-spacing=1.2\\\"\n");
    printf("    # DYNAMIC TIMESTAMP:\n");
    printf(" \n");    
    printf("    -T \\\"W5ABC %%H:%%M:%%S|size=12|timestamp-fmt=%%H:%%M:%%S|pos=top\\\"\n");
    printf("    # CONTEST FORMAT (multiple overlays with bars):\n");
    printf(" \n");    
    printf("    -T \\\"Call|size=14|color=white|bg=navy|bgbar=true|pos=top\\\"\\\n");
    printf("         -T \\\"Grid|size=12|color=yellow|bg=darkblue|bgbar=true|pos=bottom\\\"\n\n");

    printf("CW SIGNATURE (AMATEUR RADIO):\n");
    printf("─────────────────────────────────────────────────────────────────\n");
    printf("  -C <callsign>    Ham radio callsign for CW encoding (max 31 chars)\n");
    printf("                   This adds CW audio signature to the transmission\n");
    printf("  -W <wpm>         CW transmission speed in words/minute (default: 15)\n");
    printf("                   Valid range: 1-50\n");
    printf("  -Q <hz>          CW tone frequency in Hz (default: 800)\n");
    printf("                   Valid range: 400-2000\n");

    printf("DEBUGGING AND ANALYSIS:\n");
    printf("─────────────────────────────────────────────────────────────────\n");
    printf("  -v               Verbose output: show processing details and timings\n");
    printf("  -Z               Add timestamps to verbose output for performance analysis\n");
    printf("                   (implies -v)\n");
    printf("  -K               Keep intermediate processed images for inspection\n");
    printf("                   Useful for diagnosing image processing issues\n");
    printf("  -N               Skip audio encoding (test mode)\n");
    printf("                   Useful for testing overlays without audio generation\n");
    printf("  -L               List all available SSTV modes and exit\n");
    printf("  -M               Show MMSSTV library detection status and exit\n\n");

    printf("COMPLETE EXAMPLES:\n");
    printf("───────────────────────────────────────────────────────────────────\n");
    printf("  # Basic conversion to default format\n");
    printf("  %s -i photo.jpg\n\n", program_name ? program_name : "slowframe");

    printf("  # Scottie 2 protocol with different audio format\n");
    printf("  %s -i photo.jpg -p s2 -f ogg\n\n", program_name ? program_name : "slowframe");

    printf("  # With amateur radio CW signature\n");
    printf("  %s -i photo.jpg -C W5ABC -W 20 -Q 900\n\n", program_name ? program_name : "slowframe");

    printf("  # Multiple text overlays with styling\n");
    printf("  %s -i photo.jpg -T \\\"W5ABC|size=20|pos=top|color=white|bg=blue|pad=4\\\"\\\n", 
           program_name ? program_name : "slowframe");
    printf("                                  -T \\\"EM12ab|size=16|pos=bottom|color=yellow\\\"\n\n");

    printf("  # HF SSTV weak signal (high-contrast background bars)\n");
    printf("  %s -i photo.jpg -T \\\"W5ABC|size=16|color=white|bg=black|bgbar=true|bgbar-margin=4|pos=top\\\"\\\n",
           program_name ? program_name : "slowframe");
    printf("                               -T \\\"EM12AB|size=14|color=white|bg=black|bgbar=true|bgbar-margin=4\\\"\\\n");
    printf("                               -T \\\"559|size=16|color=yellow|bg=darkblue|bgbar=true|pos=bottom\\\"\n\n");

    printf("  # Verbose mode with timestamped logs\n");
    printf("  %s -i photo.jpg -v -Z > session.log\n\n", program_name ? program_name : "slowframe");

    printf("  # Test overlay rendering without audio encoding\n");
    printf("  %s -i photo.jpg -T \\\"Test|size=30|color=red|bg=white\\\" -N -K\n\n", 
           program_name ? program_name : "slowframe");

    printf("COMPATIBILITY NOTES:\n");
    printf("───────────────────────────────────────────────────────────────────\n");
    printf("  • Image formats: Supports PNG, JPEG, GIF, BMP, TIFF, WebP via libvips\n");
    printf("  • Audio quality: Higher sample rates and protocols increase file size\n");
    printf("  • CW signature: Adds ~2-5 seconds based on callsign length and WPM\n");
    printf("  • Background bars: Recommended for weak/degraded signals (HF SSTV QSOs)\n");
    printf("                     Use bgbar=true with high-contrast colors (white on black)\n\n");
    printf("════════════════════════════════════════════════════════════════\n");
}

/**
 * @brief Public interface to show detailed help
 * @param program_name Name of the program (typically argv[0])
 */
void slowframe_config_show_detailed_help(const char *program_name) {
    show_detailed_help(program_name);
}
