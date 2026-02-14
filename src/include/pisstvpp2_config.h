/**
 * @file pisstvpp2_config.h
 * @brief PiSSTVpp2 Configuration Management Module
 *
 * This module provides centralized configuration management for pisstvpp2.
 * It handles:
 * - Command-line argument parsing and validation
 * - Configuration defaults and constraints
 * - Inter-parameter validation (dependencies)
 * - Platform-agnostic error handling using error codes
 *
 * ## Architecture
 * The config module uses a single `PisstvppConfig` structure to encapsulate
 * all application settings. This enables:
 * - Easy extension with new options
 * - Clean separation of parsing from application logic
 * - Consistent validation across all parameters
 * - Unified error reporting
 *
 * ## Usage Pattern
 * @code
 * PisstvppConfig config;
 * int result = pisstvpp_config_init(&config);
 * if (result != PISSTVPP2_OK) {
 *     error_log(result, "Failed to initialize config");
 *     return 1;
 * }
 *
 * result = pisstvpp_config_parse(&config, argc, argv);
 * if (result != PISSTVPP2_OK) {
 *     error_log(result, "Invalid arguments");
 *     pisstvpp_config_cleanup(&config);
 *     return 1;
 * }
 *
 * // Config now contains all validated parameters
 * process_image(config.input_file, config.protocol, config.sample_rate);
 *
 * pisstvpp_config_cleanup(&config);
 * @endcode
 *
 * ## Aspect Ratio Modes
 * - ASPECT_CENTER: Center-crop to fill target dimensions (default)
 * - ASPECT_PAD: Add black bars to preserve aspect ratio
 * - ASPECT_STRETCH: Distort image to fit exact dimensions
 *
 * ## Audio Format Support
 * - WAV: Always available (LCMXF format)
 * - AIFF: Always available (lossless)
 * - OGG: Optional (requires libvorbis + libogg)
 *
 * ## Error Codes (100-199 range)
 * - PISSTVPP2_ERR_NO_INPUT_FILE: No input file specified
 * - PISSTVPP2_ERR_ARG_INVALID_PROTOCOL: Unknown SSTV protocol
 * - PISSTVPP2_ERR_ARG_INVALID_FORMAT: Unsupported audio format
 * - PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE: Sample rate out of range
 * - PISSTVPP2_ERR_ARG_INVALID_ASPECT: Unknown aspect ratio mode
 * - PISSTVPP2_ERR_ARG_CALLSIGN_INVALID: Callsign too long or invalid
 * - PISSTVPP2_ERR_ARG_CW_INVALID_WPM: WPM out of valid range
 * - PISSTVPP2_ERR_ARG_CW_INVALID_TONE: Tone frequency out of range
 * - PISSTVPP2_ERR_ARG_CW_MISSING_CALLSIGN: -W/-T used without -C
 * - PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG: Path exceeds 254 characters
 *
 * @author PiSSTVpp2 Contributors
 * @version 2.1.0
 * @date February 2026
 */

#ifndef PISSTVPP2_CONFIG_H
#define PISSTVPP2_CONFIG_H

#include <stdint.h>
#include "pisstvpp2_image.h"
#include "overlay_spec.h"

// ===========================================================================
// CONSTANTS
// ===========================================================================

/** @defgroup ConfigConstants Configuration Constants
 *  @{
 */

/** Maximum length for file paths (input/output filenames) */
#define CONFIG_MAX_FILENAME 255

/** Maximum length for CW callsign */
#define CONFIG_MAX_CALLSIGN 31

/** Default audio sample rate (Hz) */
#define CONFIG_DEFAULT_SAMPLE_RATE 22050

/** Minimum audio sample rate (Hz) for adequate quality */
#define CONFIG_MIN_SAMPLE_RATE 8000

/** Maximum audio sample rate (Hz) for device compatibility */
#define CONFIG_MAX_SAMPLE_RATE 48000

/** Default SSTV protocol */
#define CONFIG_DEFAULT_PROTOCOL "m1"

/** Default audio format */
#define CONFIG_DEFAULT_FORMAT "wav"

/** Default CW transmission speed (words per minute) */
#define CONFIG_DEFAULT_CW_WPM 15

/** Minimum CW transmission speed */
#define CONFIG_MIN_CW_WPM 1

/** Maximum CW transmission speed */
#define CONFIG_MAX_CW_WPM 50

/** Default CW tone frequency (Hz) */
#define CONFIG_DEFAULT_CW_TONE 800

/** Minimum CW tone frequency (Hz) */
#define CONFIG_MIN_CW_TONE 400

/** Maximum CW tone frequency (Hz) */
#define CONFIG_MAX_CW_TONE 2000

/** Maximum length for station ID callsign */
#define CONFIG_MAX_STATION_CALLSIGN 31

/** @} */

// ===========================================================================
// ENUMERATIONS
// ===========================================================================
// Note: Using enumerations from overlay_spec.h for consistency

/**
 * @brief Complete application configuration
 *
 * This structure encapsulates all configuration parameters derived from
 * command-line arguments and defaults. It's designed to be:
 * - Thread-safe for read operations after initialization
 * - Easy to extend with new parameters
 * - Self-documenting with field names matching CLI options
 *
 * All string fields are nul-terminated C strings with bounds checking
 * applied during parsing.
 */
typedef struct {
    // File paths
    char input_file[CONFIG_MAX_FILENAME + 1];      /**< Input image file path */
    char output_file[CONFIG_MAX_FILENAME + 1];     /**< Output audio file path (optional) */

    // SSTV encoding parameters
    char protocol[16];                              /**< SSTV protocol (e.g., "m1", "s2", "r36") */
    char format[16];                                /**< Audio format (wav, aiff, ogg) */
    uint16_t sample_rate;                           /**< Audio sample rate in Hz */

    // Visual aspect ratio handling
    AspectMode aspect_mode;                         /**< How to handle aspect ratio mismatch */

    // CW (Morse code) signature parameters
    int cw_enabled;                                 /**< 1 if CW signature requested, 0 otherwise */
    char cw_callsign[CONFIG_MAX_CALLSIGN + 1];     /**< Amateur radio callsign */
    int cw_wpm;                                     /**< CW transmission speed (words/minute) */
    uint16_t cw_tone;                               /**< CW tone frequency (Hz) */

    // Text overlay specifications (flexible system)
    OverlaySpecList overlay_specs;                  /**< List of text overlays to apply */
    TextOverlaySpec *current_overlay;               /**< Pointer to overlay currently being configured via CLI flags */

    // Color bar specifications (visual separation of overlay areas)
    ColorBarList colorbar_specs;                    /**< List of color bars to apply */

    // Debug and logging options
    int verbose;                                    /**< Enable verbose output */
    int timestamp_logging;                          /**< Add timestamps to verbose output */
    int keep_intermediate;                          /**< Keep intermediate processed images */
    int skip_audio_encoding;                        /**< 1 to skip SSTV audio encoding (overlay testing only) */
    int text_only;                                  /**< 1 to skip aspect ratio and resizing (only applies with -N) */

} PisstvppConfig;

// ===========================================================================
// PUBLIC FUNCTION DECLARATIONS
// ===========================================================================

/**
 * @brief Initialize configuration with default values
 *
 * Sets all configuration parameters to their default values. Must be called
 * before any other config operation.
 *
 * @param config Pointer to PisstvppConfig structure to initialize
 * @return Error code (PISSTVPP2_OK on success, error code on failure)
 *
 * @note This function initializes the structure with safe defaults. It does
 *       not perform any validation - that happens during parse.
 */
int pisstvpp_config_init(PisstvppConfig *config);

/**
 * @brief Parse command-line arguments into configuration
 *
 * Parses argc/argv style command-line arguments using getopt and populates
 * the configuration structure with validated values. Each parameter is
 * validated against constraints (range, length, dependencies).
 *
 * **Supported Options:**
 * - `-i <file>` Input image file (REQUIRED)
 * - `-o <file>` Output audio file (optional, auto-generated if omitted)
 * - `-p <protocol>` SSTV protocol: m1, m2, s1, s2, sdx, r36, r72 (default: m1)
 * - `-f <format>` Audio format: wav, aiff, ogg (default: wav)
 * - `-r <rate>` Sample rate in Hz, 8000-48000 (default: 22050)
 * - `-a <mode>` Aspect correction: center, pad, stretch (default: center)
 * - `-C <callsign>` Amateur radio callsign for CW signature
 * - `-W <wpm>` CW speed in words/minute, 1-50 (requires -C)
 * - `-T <hz>` CW tone frequency in Hz, 400-2000 (requires -C)
 * - `-v` Enable verbose output
 * - `-Z` Add timestamps to verbose output (implies -v)
 * - `-K` Keep intermediate processed images (debugging)
 * - `-h` Show help text
 *
 * @param config Pointer to PisstvppConfig structure to populate
 * @param argc Command-line argument count
 * @param argv Command-line argument vector
 * @return Error code (PISSTVPP2_OK on success, error code on failure)
 *
 * @retval PISSTVPP2_OK All arguments parsed and validated successfully
 * @retval PISSTVPP2_ERR_NO_INPUT_FILE No -i option provided
 * @retval PISSTVPP2_ERR_ARG_INVALID_PROTOCOL Unknown protocol specified
 * @retval PISSTVPP2_ERR_ARG_INVALID_FORMAT Unsupported audio format
 * @retval PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE Sample rate out of range
 * @retval PISSTVPP2_ERR_ARG_INVALID_ASPECT Unknown aspect mode
 * @retval PISSTVPP2_ERR_ARG_CALLSIGN_INVALID Callsign too long
 * @retval PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG File path too long
 *
 * @note The function shows help text (-h) and returns success without
 *       further parsing. Check config->input_file to distinguish normal
 *       help display from actual parsing.
 */
int pisstvpp_config_parse(PisstvppConfig *config, int argc, char *argv[]);

/**
 * @brief Finalize current overlay and add to overlay list
 *
 * Called when moving from one overlay to the next (via -O flag) or when
 * CLI parsing completes. Moves the overlay from current_overlay pointer
 * to the overlay_specs list.
 *
 * @param config Pointer to PisstvppConfig with current_overlay set
 * @return Error code (PISSTVPP2_OK on success, error code on failure)
 *
 * @retval PISSTVPP2_OK Overlay successfully added to list
 * @retval PISSTVPP2_ERR_MEMORY_ALLOC Memory allocation failed
 * @retval PISSTVPP2_ERR_ARG_INVALID_PROTOCOL Invalid overlay specification
 *
 * @note Sets current_overlay to NULL after finalization
 * @note Safe to call with NULL current_overlay (no-op)
 */
int pisstvpp_config_finalize_current_overlay(PisstvppConfig *config);

/**
 * @brief Validate complete configuration for consistency
 *
 * Performs inter-parameter validation that cannot be done during parsing.
 * For example, verifies that CW WPM/tone parameters are only used with
 * a callsign.
 *
 * @param config Pointer to PisstvppConfig to validate
 * @return Error code (PISSTVPP2_OK if valid, error code if issues found)
 *
 * @retval PISSTVPP2_OK Configuration is internally consistent
 * @retval PISSTVPP2_ERR_ARG_CW_MISSING_CALLSIGN -W/-T used without -C
 *
 * @note This is automatically called by pisstvpp_config_parse(), but can
 *       be called independently if configuration is modified after parsing.
 */
int pisstvpp_config_validate(const PisstvppConfig *config);

/**
 * @brief Get human-readable protocol name
 *
 * Converts a protocol code (e.g., "m1") to its full display name
 * (e.g., "Martin 1").
 *
 * @param protocol Protocol code (e.g., "m1", "s2", "r36")
 * @return Pointer to static string with protocol name, or "Unknown" if not found
 *
 * @note The returned pointer points to static memory; do not free
 */
const char* pisstvpp_config_get_protocol_name(const char *protocol);

/**
 * @brief Print current configuration to output stream
 *
 * Outputs a human-readable summary of all configuration parameters.
 * Useful for debugging and verbose mode output.
 *
 * @param config Pointer to PisstvppConfig to display
 *
 * @note Outputs to stdout; uses the logging module for consistency
 */
void pisstvpp_config_print(const PisstvppConfig *config);

/**
 * @brief Check if audio format is locally supported
 *
 * Determines if the specified audio format can be produced on this
 * system (accounts for optional dependencies like OGG Vorbis).
 *
 * **Always available:**
 * - wav (standard WAV format)
 * - aiff (Audio Interchange File Format)
 *
 * **Optionally available (if compiled with -DHAVE_OGG_SUPPORT):**
 * - ogg (OGG Vorbis)
 *
 * @param format Format name to check (e.g., "wav", "ogg")
 * @return 1 if format is supported, 0 if not supported
 *
 * @note This replaces the separate audio_encoder_is_format_supported()
 *       function, centralizing format support queries to the config module
 */
int pisstvpp_config_is_format_supported(const char *format);

/**
 * @brief Check if protocol is valid
 *
 * Validates that the given protocol code maps to a known SSTV mode.
 *
 * **Valid protocols:**
 * - m1, m2: Martin modes
 * - s1, s2, sdx: Scottie modes
 * - r36, r72: Robot modes
 *
 * @param protocol Protocol code to validate (e.g., "m1", "s2")
 * @return 1 if valid, 0 if not valid
 */
int pisstvpp_config_is_protocol_valid(const char *protocol);

/**
 * @brief Clean up configuration resources
 *
 * Releases any dynamically allocated resources associated with the
 * configuration. For current implementation, this is mostly a noop
 * but allows future extension (e.g., if config includes strdup'd strings).
 *
 * @param config Pointer to PisstvppConfig to clean up
 *
 * @note Safe to call multiple times; subsequent calls are noops
 */
void pisstvpp_config_cleanup(PisstvppConfig *config);

/**
 * @brief Generate output filename from input
 *
 * If no output file is specified in config, generates one by replacing
 * the input extension with the appropriate format extension.
 *
 * **Examples:**
 * - input.jpg, format=wav → input.wav
 * - image.png, format=ogg → image.ogg
 *
 * @param config Configuration containing input filename and format
 * @return Non-zero if filename was auto-generated, 0 if user-specified
 *
 * @note Modifies config->output_file in place
 */
int pisstvpp_config_autogen_output_filename(PisstvppConfig *config);

/**
 * @brief Display detailed help message with all options, styling, and examples
 *
 * Shows comprehensive help including all text overlay styling options,
 * color choices, positioning information, and detailed usage examples.
 *
 * @param program_name The name of the program (typically argv[0])
 */
void pisstvpp_config_show_detailed_help(const char *program_name);

#endif // PISSTVPP2_CONFIG_H
