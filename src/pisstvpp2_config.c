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
static int is_valid_protocol(const char *protocol);
static const char* get_format_extension(const char *format);

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

    // Debug and logging defaults
    config->verbose = 0;
    config->timestamp_logging = 0;
    config->keep_intermediate = 0;

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
    while ((option = getopt(argc, argv, "i:o:p:f:r:vC:W:T:a:KZh")) != -1) {
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

            // CW Callsign
            case 'C':
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

            // CW Tone Frequency
            case 'T': {
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

            // Help
            case 'h':
                show_help(argv[0]);
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

    // =====================================================================
    // DERIVED SETTINGS
    // =====================================================================

    // Auto-enable intermediate files in verbose mode
    if (config->verbose) {
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
    // No dynamic allocations currently, but allows future extension
    if (!config) {
        return;
    }
    // Placeholder for future cleanup operations
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
static void show_help(const char *program_name) {
    printf("PiSSTVpp2 - SSTV Image to Audio Encoder\n\n");
    printf("Usage: %s -i <input> [options]\n\n", 
           program_name ? program_name : "pisstvpp2");

    printf("Required Arguments:\n");
    printf("  -i <file>        Input image file (JPEG, PNG, BMP, GIF, TIFF, WebP)\n\n");

    printf("Optional Arguments:\n");
    printf("  -o <file>        Output audio file (default: input.wav)\n");
    printf("  -p <protocol>    SSTV protocol (default: m1)\n");
    printf("                   Options: m1, m2, s1, s2, sdx, r36, r72\n");
    printf("  -f <format>      Audio format (default: wav)\n");
    printf("                   Options: wav, aiff, ogg\n");
    printf("  -r <rate>        Sample rate in Hz (default: 22050)\n");
    printf("                   Range: 8000-48000\n");
    printf("  -a <mode>        Aspect ratio correction (default: center)\n");
    printf("                   Options: center, pad, stretch\n");
    printf("  -C <callsign>    Amateur radio callsign for CW signature\n");
    printf("  -W <wpm>         CW transmission speed in words/minute (default: 15)\n");
    printf("                   Range: 1-50 (requires -C)\n");
    printf("  -T <hz>          CW tone frequency in Hz (default: 800)\n");
    printf("                   Range: 400-2000 (requires -C)\n");
    printf("  -v               Enable verbose output\n");
    printf("  -Z               Add timestamps to verbose output (implies -v)\n");
    printf("  -K               Keep intermediate processed images\n");
    printf("  -h               Show this help message\n\n");

    printf("Examples:\n");
    printf("  %s -i photo.jpg\n", program_name ? program_name : "pisstvpp2");
    printf("  %s -i image.png -p s2 -f ogg -C N0CALL\n", 
           program_name ? program_name : "pisstvpp2");
    printf("  %s -i photo.jpg -f aiff -r 11025 -v\n",
           program_name ? program_name : "pisstvpp2");
}
