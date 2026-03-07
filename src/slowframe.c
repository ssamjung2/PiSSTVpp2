/**
 * @file slowframe.c
 * @brief SlowFrame - SSTV Image to Audio Encoder (Main Entry Point)
 *
 * ## Overview
 * This is the main program entry point for SlowFrame, a high-performance SSTV
 * (Slow Scan Television) image-to-audio encoder designed for Raspberry Pi and
 * other embedded systems. The program converts still images into audio signals
 * compatible with amateur radio SSTV transmission protocols.
 *
 * ## Architecture
 * SlowFrame uses a modular architecture with three main subsystems:
 * - **Image Processing** (slowframe_image.h): libvips-based image loading,
 *   scaling, aspect correction, and pixel access
 * - **SSTV Encoding** (slowframe_sstv.h): Audio synthesis, protocol encoding
 *   (Martin, Scottie, Robot), VIS headers, and optional CW signatures
 * - **Audio Encoding** (slowframe_audio_encoder.h): Pluggable format encoders
 *   supporting WAV, AIFF, and OGG Vorbis (optional)
 *
 * ## Features
 * - Multi-format support: PNG, JPEG, GIF, BMP, TIFF, WebP (via libvips)
 * - SSTV protocols: Martin 1/2, Scottie 1/2/DX, Robot 36/72
 * - Audio formats: WAV, AIFF (always), OGG (if libraries available)
 * - Aspect ratio correction: center-crop, pad-black, stretch
 * - Optional CW (Morse) signature with configurable WPM and tone
 * - Sample rates: 8000-48000 Hz (configurable)
 * - Verbose progress reporting
 *
 * ## Dependencies
 * **Required:**
 * - libvips >= 8.0 (image processing)
 * - glib-2.0 (required by libvips)
 *
 * **Optional (for additional audio formats):**
 * - libvorbis + libogg (OGG Vorbis encoding)
 *
 * ## Building
 * **Recommended (uses Makefile with auto-detection):**
 * @code
 * make clean && make all
 * @endcode
 *
 * **Manual compilation (requires all optional codecs):**
 * @code
 * gcc -O3 -ffast-math -funroll-loops -DHAVE_OGG_SUPPORT \
 *   -I/opt/homebrew/include \
 *   -I/opt/homebrew/Cellar/glib/2.86.3/include/glib-2.0 \
 *   -I/opt/homebrew/Cellar/glib/2.86.3/lib/glib-2.0/include \
 *   slowframe.c slowframe_image.c slowframe_sstv.c slowframe_audio_encoder.c \
 *   -o slowframe \
 *   -L/opt/homebrew/lib -L/opt/homebrew/Cellar/glib/2.86.3/lib \
 *   -lvips -lglib-2.0 -lgobject-2.0 -lvorbis -logg -lm
 * @endcode
 *
 * ## Usage Examples
 * @code
 * # Basic: Convert image to WAV using default Martin 1 mode
 * ./slowframe -i photo.jpg
 *
 * # Specify output format and protocol
 * ./slowframe -i photo.png -f ogg -p s1 -o output.ogg
 *
 * # Verbose output with custom sample rate
 * ./slowframe -i image.jpg -f wav -r 11025 -v
 *
 * # Add CW signature
 * ./slowframe -i photo.jpg -C N0CALL -W 20 -T 700
 *
 * # Robot 36 mode with aspect correction
 * ./slowframe -i photo.jpg -p r36 -a pad
 * @endcode
 *
 * ## License
 * Open source software - see LICENSE file in repository
 *
 * @author SlowFrame Contributors
 * @version 2.1.0
 * @date January 2026
 */

// ===========================================================================
// INCLUDES
// ===========================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <vips/vips.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <math.h>
#include <tgmath.h>
#include <unistd.h>
#include <errno.h>
#include "slowframe_image.h"
#include "slowframe_sstv.h"
#include "slowframe_audio_encoder.h"
#include "slowframe_config.h"
#include "slowframe_context.h"
#include "recovery_strategies.h"
#include "sstv/mode_registry.h"
#include "sstv/modes_martin.h"
#include "sstv/modes_scottie.h"
#include "sstv/modes_robot.h"
#include "mmsstv/mmsstv_loader.h"
#include "mmsstv/mmsstv_adapter.h"
#include "logging.h"
#include "error.h"

// ===========================================================================
// TYPE DEFINITIONS
// ===========================================================================

/**
 * @defgroup AudioConfig Audio Configuration Constants
 * Default values for audio generation
 * @{
 */
enum {
    RATE       = 22050,     /**< Default sample rate (Hz) */
    MAXRATE    = 48000,     /**< Maximum sample rate (Hz) */
    BITS       = 16,        /**< Audio bit depth per sample */
    CHANS      = 1,         /**< Audio channel count (mono) */
    VOLPCT     = 65         /**< Volume as % of max (65 = appropriate level for SSTV) */
};
/** @} */

/**
 * @defgroup PerformanceConfig Performance and Tuning Constants
 * @{
 */
#define MAXSAMPLES SSTV_MAX_SAMPLES         /**< Max audio buffer size (see SSTV_MAX_SAMPLES) */
/** @} */

/**
 * @defgroup ErrorHandling Error Handling Utilities
 * @brief Centralized error code system for consistent error management
 *
 * All error handling uses the unified error code system (error.h) with
 * SLOWFRAME_OK for success and SLOWFRAME_ERR_* codes for failures.
 *
 * **Error Handling Pattern:**
 * @code
 * int result = some_operation();
 * if (result != SLOWFRAME_OK) {
 *     error_log(result, "Operation context", "Additional details");
 *     return result;  // or set error_code = result; goto cleanup;
 * }
 * @endcode
 *
 * **Error Categories:**
 * - Arguments (100-199): Command-line parsing, validation
 * - Image (200-299): Image loading, processing, format issues  
 * - SSTV (300-399): SSTV encoding, mode issues
 * - Audio (400-499): Audio encoding, format issues
 * - File I/O (500-599): File operations, permissions
 * - System (600-699): Memory, resource limits
 * - MMSSTV (700-799): Dynamic library integration
 * - Text Overlay (800-899): Text overlay and color bars
 *
 * @see error.h For complete error code definitions
 * @{
 */

// ===========================================================================
// FUNCTION PROTOTYPES
// ===========================================================================

/**
 * @brief Display comprehensive usage information to the user.
 *
 * Outputs detailed help text including:
 * - Required and optional command-line arguments
 * - All supported SSTV protocols with VIS codes
 * - Audio format options (WAV, AIFF, OGG)
 * - Aspect ratio correction modes
 * - CW signature parameters
 * - Real-world usage examples
 *
 * This function is called when:
 * - User provides -h flag (help request)
 * - Required arguments are missing
 * - Invalid protocol or format is specified
 *
 * @note Always exits normally after display (does not return error)
 */
static void show_help(void);

/**
 * @brief Display all available SSTV modes from the registry.
 *
 * Lists all registered SSTV modes with their metadata:
 * - Mode code (e.g., m1, s1, r36)
 * - Full mode name
 * - VIS code (for decoder recognition)
 * - Image dimensions
 * - Encoding duration
 * - Color/grayscale indicator
 * - Source (native or MMSSTV)
 *
 * @note Requires context initialization to access mode registry
 */
static void list_available_modes(void);

/**
 * @brief Display MMSSTV library detection status and diagnostic information.
 *
 * Initializes a context to load the MMSSTV library and displays:
 * - Library detection status (DETECTED / NOT DETECTED)
 * - Library version (if available)
 * - Library path (if available)
 * - Number of MMSSTV modes available
 * - Troubleshooting help (if library not found)
 *
 * @note Requires context initialization to load MMSSTV adapter
 */
static void show_mmsstv_status(void);

// ===========================================================================
// VERBOSE OUTPUT HELPER
// ===========================================================================

/**
 * @brief Print verbose output with optional timestamp prefix.
 *
 * This helper function centralizes verbose logging and allows consistent
 * timestamp formatting across all verbose output. Timestamps are useful when
 * redirecting output to log files for post-analysis of encoding timeline.
 *
 * **Timestamp Format:** [HH:MM:SS.mmm] when enabled (mmm = milliseconds)
 *
 * @param verbose_enabled Non-zero to print, zero to suppress
 * @param add_timestamp Non-zero to prepend timestamp, zero for plain output
 * @param format Printf-style format string
 * @param ... Variable arguments matching format string
 *
 * @note Safe to call with verbose_enabled=0 (does nothing, no performance impact)
 */
static void verbose_print(int verbose_enabled, int add_timestamp, const char *format, ...) {
    if (!verbose_enabled) return;
    
    if (add_timestamp) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        struct tm *local_time = localtime(&tv.tv_sec);
        int milliseconds = tv.tv_usec / 1000;
        printf("[%02d:%02d:%02d.%03d] ", 
               local_time->tm_hour, 
               local_time->tm_min, 
               local_time->tm_sec,
               milliseconds);
    }
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

// ===========================================================================
// HELP TEXT DISPLAY
// ===========================================================================

/**
 * @brief Display comprehensive help text with usage, options, and examples.
 *
 * Provides user-facing documentation including:
 * - Command syntax
 * - Required arguments (-i input file)
 * - Optional flags (format, protocol, sample rate, aspect correction, etc.)
 * - Protocol descriptions with VIS codes
 * - CW signature configuration
 * - Real-world usage examples
 *
 * This function writes to stdout and is safe to call multiple times.
 *
 * @note Does NOT exit the program - caller decides whether to continue
 */
static void show_help(void) {
    printf("SlowFrame - SSTV Image to Audio Encoder v2.1.0\n\n");
    printf("Usage: ./slowframe -i <input_file> [OPTIONS]\n\n");
    printf("REQUIRED ARGUMENTS:\n");
    printf("  -i <file>                 Input image file (PNG, JPEG, GIF, BMP, TIFF, WebP)\n\n");

    printf("AUDIO OUTPUT OPTIONS:\n");
    printf("  -o <file>                 Output audio file (default: input_basename.wav)\n");
    printf("  -p <protocol>             SSTV protocol (default: m1)\n");
    printf("                            m1=Martin 1, m2=Martin 2, s1=Scottie 1, s2=Scottie 2,\n");
    printf("                            sdx=Scottie DX, r36=Robot 36, r72=Robot 72\n");
    printf("  -f <format>               Audio format: wav, aiff, ogg (default: wav)\n");
    printf("  -r <rate>                 Sample rate in Hz: 8000-48000 (default: 22050)\n");
    printf("  -a <mode>                 Aspect ratio: center, pad, stretch (default: center)\n\n");

    printf("TEXT OVERLAY:\n");
    printf("  -T <spec>                 Text overlay spec (e.g., \"N0CALL|size=20|pos=bottom\")\n");
    printf("                            Use -h (in slowframe_config) for full styling options\n\n");

    printf("CW SIGNATURE:\n");
    printf("  -C <callsign>             Ham radio callsign for CW signature (max 31 chars)\n");
    printf("  -W <wpm>                  CW speed: 1-50 words/minute (default: 15)\n");
    printf("  -Q <hz>                   CW tone: 400-2000 Hz (default: 800)\n\n");

    printf("DEBUGGING & ANALYSIS:\n");
    printf("  -v                        Verbose output with processing details\n");
    printf("  -Z                        Add timestamps to verbose logging (implies -v)\n");
    printf("  -K[file]                  Keep intermediate processed image\n");
    printf("                            No arg: auto-name as <input_base>-<mode>.<ext>\n");
    printf("                            With arg (adjacent, no space): save to that file\n");
    printf("                              e.g.  -Kmy_preview.jpg\n");
    printf("  -N                        Skip audio encoding (test mode)\n");
    printf("  -O                        Text-only overlay (no resizing, requires -N)\n\n");

    printf("IMAGE OPTIONS:\n");
    printf("  -D <degrees>              Rotate input image before encoding (+/- 0-360)\n");
    printf("                            Positive = clockwise, negative = counterclockwise\n\n");

    printf("HELP:\n");
    printf("  -h                        Show detailed help with all options and examples\n");
    printf("  -L                        List all available SSTV modes\n\n");

    printf("QUICK EXAMPLES:\n");
    printf("  ./slowframe -i photo.jpg\n");
    printf("  ./slowframe -i photo.jpg -p s2 -C N0CALL -W 18\n");
    printf("  ./slowframe -i photo.jpg -T \"N0CALL|size=20|pos=bottom\"\n");
    printf("  ./slowframe -i photo.jpg -T \"W5ABC|size=16|pos=top\" -T \"EM12AB|size=14|pos=bottom\"\n");
    printf("  ./slowframe -i photo.jpg -v -Z > encoder.log\n");
    printf("  ./slowframe -i photo.jpg -T \"Test|size=32|color=yellow|bg=black\" -N\n\n");
    printf("For detailed help: Check documentation or use slowframe_config -h for advanced options.\n");
}

/**
 * @brief Display all available SSTV modes from the mode registry.
 */
static void list_available_modes(void) {
    printf("════════════════════════════════════════════════════════════════\n");
    printf("SlowFrame v2.1.0 - Available SSTV Modes\n");
    printf("════════════════════════════════════════════════════════════════\n\n");

    // Initialize minimal context to access registry
    SlowframeContext ctx;
    SlowframeConfig dummy_config;
    memset(&dummy_config, 0, sizeof(SlowframeConfig));
    
    // Set minimal config to pass validation
    strncpy(dummy_config.input_file, "dummy.png", sizeof(dummy_config.input_file) - 1);
    strncpy(dummy_config.protocol, "m1", sizeof(dummy_config.protocol) - 1);
    dummy_config.sample_rate = 22050;
    
    int result = slowframe_context_init(&ctx, &dummy_config);
    if (result != SLOWFRAME_OK) {
        fprintf(stderr, "Error: Failed to initialize context for mode listing\n");
        return;
    }

    // Get registry and adapter from context
    mode_registry_t *registry = slowframe_context_get_mode_registry(&ctx);
    if (!registry) {
        fprintf(stderr, "Error: Mode registry not available\n");
        slowframe_context_cleanup(&ctx);
        return;
    }

    mmsstv_adapter_t *adapter = slowframe_context_get_mmsstv_adapter(&ctx);
    
    // Show MMSSTV library status
    if (mmsstv_adapter_is_available(adapter)) {
        size_t mmsstv_count = mmsstv_adapter_get_mode_count(adapter);
        printf("MMSSTV Library: ✓ LOADED (%zu additional modes available)\n\n", mmsstv_count);
    } else {
        printf("MMSSTV Library: ✗ NOT LOADED (use -M for details)\n\n");
    }

    // Get all registered modes
    mode_list_t modes = mode_registry_list_all(registry);
    
    if (modes.count == 0) {
        printf("No modes registered.\n");
    } else {
        // Count modes by source
        size_t native_count = 0;
        size_t mmsstv_count = 0;
        for (size_t i = 0; i < modes.count; i++) {
            if (strcmp(modes.modes[i].source, "native") == 0) {
                native_count++;
            } else if (strcmp(modes.modes[i].source, "mmsstv") == 0) {
                mmsstv_count++;
            }
        }
        
        // Print native modes
        if (native_count > 0) {
            printf("NATIVE MODES (%zu):\n", native_count);
            printf("%-12s %-24s %-10s %-14s %-12s %-8s\n",
                   "Code", "Mode Name", "VIS", "Resolution", "Duration", "Color");
            printf("%-12s %-24s %-10s %-14s %-12s %-8s\n",
                   "────────────", "────────────────────────", "──────────", "──────────────", "────────────", "────────");
            
            for (size_t i = 0; i < modes.count; i++) {
                const mode_definition_t *mode = &modes.modes[i];
                if (strcmp(mode->source, "native") == 0) {
                    char resolution[15];
                    char duration[13];
                    snprintf(resolution, sizeof(resolution), "%ux%u", mode->width, mode->height);
                    snprintf(duration, sizeof(duration), "%.1fs", mode->duration_sec);
                    printf("%-12s %-24s 0x%02X       %-14s %-12s %-8s\n",
                           mode->code,
                           mode->name,
                           mode->vis_code,
                           resolution,
                           duration,
                           mode->is_color ? "color" : "mono");
                }
            }
            printf("\n");
        }
        
        // Print MMSSTV modes
        if (mmsstv_count > 0) {
            printf("MMSSTV MODES (%zu):\n", mmsstv_count);
            printf("%-12s %-24s %-10s %-14s %-12s %-8s\n",
                   "Code", "Mode Name", "VIS", "Resolution", "Duration", "Color");
            printf("%-12s %-24s %-10s %-14s %-12s %-8s\n",
                   "────────────", "────────────────────────", "──────────", "──────────────", "────────────", "────────");
            
            for (size_t i = 0; i < modes.count; i++) {
                const mode_definition_t *mode = &modes.modes[i];
                if (strcmp(mode->source, "mmsstv") == 0) {
                    char resolution[15];
                    char duration[13];
                    snprintf(resolution, sizeof(resolution), "%ux%u", mode->width, mode->height);
                    snprintf(duration, sizeof(duration), "%.1fs", mode->duration_sec);
                    printf("%-12s %-24s 0x%02X       %-14s %-12s %-8s\n",
                           mode->code,
                           mode->name,
                           mode->vis_code,
                           resolution,
                           duration,
                           mode->is_color ? "color" : "mono");
                }
            }
            printf("\n");
        }
    }
    
    printf("Total modes: %zu\n", modes.count);
    printf("\nUsage: ./slowframe -i <image> -p <code> [options]\n");
    printf("Example: ./slowframe -i photo.jpg -p s1\n\n");

    // Cleanup
    slowframe_context_cleanup(&ctx);
}

static void show_mmsstv_status(void) {
    printf("════════════════════════════════════════════════════════════════\n");
    printf("SlowFrame v2.1.0 - MMSSTV Library Status\n");
    printf("════════════════════════════════════════════════════════════════\n\n");

    // Initialize minimal context to load MMSSTV adapter
    SlowframeContext ctx;
    SlowframeConfig dummy_config;
    memset(&dummy_config, 0, sizeof(SlowframeConfig));
    
    // Set minimal config to pass validation
    strncpy(dummy_config.input_file, "dummy.png", sizeof(dummy_config.input_file) - 1);
    strncpy(dummy_config.protocol, "m1", sizeof(dummy_config.protocol) - 1);
    dummy_config.sample_rate = 22050;
    
    int result = slowframe_context_init(&ctx, &dummy_config);
    if (result != SLOWFRAME_OK) {
        fprintf(stderr, "Error: Failed to initialize context for MMSSTV status check\n");
        return;
    }

    // Get MMSSTV adapter from context
    mmsstv_adapter_t *adapter = slowframe_context_get_mmsstv_adapter(&ctx);
    
    if (mmsstv_adapter_is_available(adapter)) {
        printf("Library Status:      ✓ DETECTED\n");
        
        const char *version = mmsstv_adapter_get_version(adapter);
        if (version) {
            printf("Library Version:     %s\n", version);
        }
        
        const char *path = mmsstv_adapter_get_library_path(adapter);
        if (path) {
            printf("Library Path:        %s\n", path);
        }
        
        size_t mode_count = mmsstv_adapter_get_mode_count(adapter);
        printf("MMSSTV Modes:        %zu\n", mode_count);
        
        printf("\n");
        printf("MMSSTV library is loaded and operational.\n");
        printf("Additional modes are available. Use -L to list all modes.\n");
        
    } else {
        printf("Library Status:      ✗ NOT DETECTED\n");
        printf("MMSSTV Modes:        0\n");

        /* Show the specific reason from the loader (e.g. bad MMSSTV_LIB_PATH) */
        char status_buf[512];
        if (mmsstv_adapter_get_status(adapter, status_buf, (int)sizeof(status_buf)) > 0) {
            printf("Reason:              %s\n", status_buf);
        }

        printf("\n");
        printf("The MMSSTV library was not found on this system.\n");
        printf("SlowFrame will work with %d built-in modes only.\n", 7);
        printf("\n");
        printf("To enable additional MMSSTV modes:\n");
        printf("  1. Install the mmsstv-portable library package\n");
        printf("  2. Or set the MMSSTV_LIB_PATH environment variable:\n");
#ifdef __APPLE__
        printf("     export MMSSTV_LIB_PATH=/path/to/libsstv_encoder.1.0.0.dylib\n");
#else
        printf("     export MMSSTV_LIB_PATH=/path/to/libsstv_encoder.so.1.0.0\n");
#endif
        printf("\n");
        printf("Detection attempted:\n");
        printf("  • $MMSSTV_LIB_PATH environment variable\n");
        printf("  • pkg-config --variable=libdir mmsstv-portable\n");
        printf("  • Standard library paths: /usr/local/lib, /usr/lib, /opt/mmsstv/lib\n");
    }
    
    printf("\n");

    // Cleanup
    slowframe_context_cleanup(&ctx);
}

/**
 * @brief Main program entry point and workflow orchestration.
 *
 * This function coordinates the complete SSTV encoding pipeline:
 *
 * **Phase 1: Initialization**
 * - Parse and validate command-line arguments (getopt)
 * - Initialize libvips image processing library
 * - Map protocol string to VIS code
 * - Initialize SSTV encoding module
 *
 * **Phase 2: Image Processing**
 * - Load source image (supports: PNG, JPEG, GIF, BMP, TIFF, WebP, etc.)
 * - Apply aspect ratio correction (center/pad/stretch)
 * - Resize to required SSTV dimensions (320x256 or 320x240)
 * - Save intermediate processed image
 *
 * **Phase 3: Audio Synthesis**
 * - Encode image pixels as SSTV audio signal
 * - Add VIS code header and trailer
 * - Optionally append CW (Morse) signature
 *
 * **Phase 4: Audio Encoding**
 * - Instantiate format-specific encoder (factory pattern)
 * - Write audio data in selected format (WAV/AIFF/OGG)
 * - Finalize file (headers, checksums, cleanup)
 *
 * **Phase 5: Reporting**
 * - Display configuration summary
 * - Report encoding statistics (samples, duration, time)
 * - Clean up all resources
 *
 * @param argc Argument count from shell
 * @param argv Argument vector (array of C strings)
 *
 * @return Exit status code:
 *         - 0: Success (audio file written)
 *         - 1: Usage error (invalid arguments, missing required options)
 *         - 2: Runtime error (file I/O, encoding, libvips failures)
 *
 * @note Uses centralized error handling with 'goto cleanup' pattern to ensure
 *       proper resource cleanup on all error paths.
 *
 * @see show_help() For complete usage documentation
 * @see image_load_from_file() Image loading implementation
 * @see sstv_encode_frame() SSTV encoding implementation
 * @see audio_encoder_create() Encoder factory implementation
 */
int main(int argc, char *argv[]) {

    // Show short help if no arguments provided
    if (argc == 1) {
        show_help();
        fprintf(stderr, "\n[ERROR] Error code %d: (%s)\n", 
                SLOWFRAME_ERR_NO_INPUT_FILE, 
                error_string(SLOWFRAME_ERR_NO_INPUT_FILE));
        fprintf(stderr, "        Context: Input file (-i) is required\n");
        return SLOWFRAME_ERR_NO_INPUT_FILE;
    }

    // Initialize libvips
    if (VIPS_INIT(argv[0])) {
        error_log(SLOWFRAME_ERR_SSTV_INIT, "libvips initialization", 
                 "Failed to initialize libvips: %s", vips_error_buffer());
        vips_error_clear();
        return SLOWFRAME_ERR_SSTV_INIT;
    }

    int error_code = 0;  // For centralized error cleanup
    SlowframeContext ctx = {0};  // Application context
    int context_initialized = 0;  // Track context lifecycle

    // ======================================================================
    // CONFIGURATION INITIALIZATION
    // ======================================================================
    // Initialize configuration structure with defaults, then parse command-line
    // arguments. The config module handles all validation and error reporting.
    SlowframeConfig config;
    int config_result = slowframe_config_init(&config);
    if (config_result != SLOWFRAME_OK) {
        error_log(config_result, "Failed to initialize configuration");
        return config_result;
    }

    // ======================================================================
    // ARGUMENT PARSING
    // ======================================================================
    // Parse and validate all command-line arguments using configuration module.
    // This replaces inline getopt logic with centralized, tested code.
    config_result = slowframe_config_parse(&config, argc, argv);
    if (config_result != SLOWFRAME_OK) {
        // Error already logged by slowframe_config_parse()
        return config_result;
    }

    // ======================================================================
    // LIST MODES (if requested, then exit)
    // ======================================================================
    if (config.list_modes) {
        list_available_modes();
        return SLOWFRAME_OK;
    }

    // ======================================================================
    // MMSSTV STATUS: DISPLAY LIBRARY DETECTION INFORMATION
    // ======================================================================
    if (config.mmsstv_status) {
        show_mmsstv_status();
        return SLOWFRAME_OK;
    }

    // ======================================================================
    // CONTEXT INITIALIZATION
    // ======================================================================
    // Initialize the application context (this loads MMSSTV adapter and builds
    // the complete mode registry with both native and MMSSTV modes)
    int result = slowframe_context_init(&ctx, &config);
    if (result != SLOWFRAME_OK) {
        // Error already logged by slowframe_context_init()
        error_code = result;
        goto cleanup;
    }
    context_initialized = 1;

    // ======================================================================
    // PROTOCOL VALIDATION: LOOKUP IN REGISTRY
    // ======================================================================
    // Validate protocol against registered modes (native + MMSSTV) and map to VIS code.
    mode_registry_t *registry = slowframe_context_get_mode_registry(&ctx);
    if (!registry) {
        error_log(SLOWFRAME_ERR_IMAGE_LOAD, "Mode registry not available");
        error_code = SLOWFRAME_ERR_IMAGE_LOAD;
        goto cleanup;
    }

    const mode_definition_t *selected_mode =
        mode_registry_lookup_by_code(registry, config.protocol);
    if (!selected_mode) {
        // Check if MMSSTV modes are available to provide better error message
        mmsstv_adapter_t *adapter = slowframe_context_get_mmsstv_adapter(&ctx);
        int has_mmsstv = mmsstv_adapter_is_available(adapter);
        
        if (has_mmsstv) {
            error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL,
                      "Mode '%s' not recognized. Use -L to list all available modes.",
                      config.protocol);
        } else {
            error_log(SLOWFRAME_ERR_ARG_INVALID_PROTOCOL,
                      "Mode '%s' not recognized.\n"
                      "• Use -L to list available modes\n"
                      "• Use -M to check MMSSTV library status (for additional modes)",
                      config.protocol);
        }
        
        error_code = SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
        goto cleanup;
    }

    // Set protocol VIS code from selected mode definition
    uint8_t protocol_code = selected_mode->vis_code;
    sstv_set_protocol(protocol_code);

    // locals - use gettimeofday for millisecond precision
    struct timeval start_tv, end_tv;
    gettimeofday(&start_tv, NULL);

    // Initialize SSTV encoding module
    int sstv_result = sstv_init(config.sample_rate, config.verbose, config.timestamp_logging);
    if (sstv_result != SLOWFRAME_OK) {
        // Error already logged by sstv_init()
        error_code = sstv_result;
        goto cleanup;
    }

    // Print configuration summary (with or without timestamps based on mode)
    if (config.verbose) {
        verbose_print(config.verbose, config.timestamp_logging, "--------------------------------------------------------------\n");
        verbose_print(config.verbose, config.timestamp_logging, "PiSSTVpp v2.1.0 - SSTV Audio Encoder\n");
        verbose_print(config.verbose, config.timestamp_logging, "--------------------------------------------------------------\n");
        verbose_print(config.verbose, config.timestamp_logging, "Configuration Summary:\n");
        verbose_print(config.verbose, config.timestamp_logging, "  Input image:      %s\n", config.input_file);
        
        // Show output based on mode
        if (config.skip_audio_encoding) {
            // -N flag: show overlay image output instead of audio
            char overlay_image[1024];
            char out_dir[1024] = {0};
            const char *last_slash = strrchr(config.output_file, '/');
            if (last_slash) {
                int dir_len = last_slash - config.output_file;
                strncpy(out_dir, config.output_file, dir_len);
                out_dir[dir_len] = '\0';
            } else {
                strcpy(out_dir, ".");
            }
            char out_base[256];
            const char *base_start = last_slash ? last_slash + 1 : config.output_file;
            const char *dot = strrchr(base_start, '.');
            if (dot) {
                int base_len = dot - base_start;
                strncpy(out_base, base_start, base_len);
                out_base[base_len] = '\0';
            } else {
                strcpy(out_base, base_start);
            }
            // Extract extension from input file instead of relying on original_extension
            // (which may not be set yet before image loading)
            const char *input_ext = "";
            const char *dot_input = strrchr(config.input_file, '.');
            if (dot_input && dot_input != config.input_file) {
                input_ext = dot_input;
            }
            if (config.keep_intermediate_explicit && strlen(config.intermediate_file) > 0) {
                strncpy(overlay_image, config.intermediate_file, sizeof(overlay_image) - 1);
                overlay_image[sizeof(overlay_image) - 1] = '\0';
            } else {
                snprintf(overlay_image, sizeof(overlay_image), "%s/%s_overlay%s", out_dir, out_base, input_ext);
            }
            verbose_print(config.verbose, config.timestamp_logging, "  Output image:     %s (test mode - overlay only)\n", overlay_image);
            verbose_print(config.verbose, config.timestamp_logging, "  MODE:             TEST/DEBUG (audio encoding disabled)\n");
        } else {
            verbose_print(config.verbose, config.timestamp_logging, "  Output file:      %s\n", config.output_file);
        }
        
        const char *format_display = "WAV";
        if (strcmp(config.format, "aiff") == 0) format_display = "AIFF";
        else if (strcmp(config.format, "ogg") == 0 || strcmp(config.format, "vorbis") == 0) format_display = "OGG Vorbis";
        
        if (!config.skip_audio_encoding) {
            verbose_print(config.verbose, config.timestamp_logging, "  Audio format:     %s at %d Hz\n", format_display, config.sample_rate);
            verbose_print(config.verbose, config.timestamp_logging, "  SSTV protocol:    %s (VIS code %d)\n", config.protocol, protocol_code);
            verbose_print(config.verbose, config.timestamp_logging, "  Image dimensions: 320x256 pixels\n");
            verbose_print(config.verbose, config.timestamp_logging, "Mode Details:\n");
            sstv_get_mode_details(protocol_code, config.verbose, config.timestamp_logging);
        }
        verbose_print(config.verbose, config.timestamp_logging, "--------------------------------------------------------------\n");
    } else {
        printf("--------------------------------------------------------------\n");
        printf("PiSSTVpp v2.1.0 - SSTV Audio Encoder\n");
        printf("--------------------------------------------------------------\n");
        printf("Configuration Summary:\n");
        printf("  Input image:      %s\n", config.input_file);
        
        // Show output based on mode
        if (config.skip_audio_encoding) {
            // -N flag: show overlay image output instead of audio
            char overlay_image[1024];
            char out_dir[1024] = {0};
            const char *last_slash = strrchr(config.output_file, '/');
            if (last_slash) {
                int dir_len = last_slash - config.output_file;
                strncpy(out_dir, config.output_file, dir_len);
                out_dir[dir_len] = '\0';
            } else {
                strcpy(out_dir, ".");
            }
            char out_base[256];
            const char *base_start = last_slash ? last_slash + 1 : config.output_file;
            const char *dot = strrchr(base_start, '.');
            if (dot) {
                int base_len = dot - base_start;
                strncpy(out_base, base_start, base_len);
                out_base[base_len] = '\0';
            } else {
                strcpy(out_base, base_start);
            }
            // Extract extension from input file instead of relying on original_extension
            // (which may not be set yet before image loading)
            const char *input_ext = "";
            const char *dot_input = strrchr(config.input_file, '.');
            if (dot_input && dot_input != config.input_file) {
                input_ext = dot_input;
            }
            if (config.keep_intermediate_explicit && strlen(config.intermediate_file) > 0) {
                strncpy(overlay_image, config.intermediate_file, sizeof(overlay_image) - 1);
                overlay_image[sizeof(overlay_image) - 1] = '\0';
            } else {
                snprintf(overlay_image, sizeof(overlay_image), "%s/%s_overlay%s", out_dir, out_base, input_ext);
            }
            printf("  Output image:     %s (test mode - overlay only)\n", overlay_image);
            printf("  MODE:             TEST/DEBUG (audio encoding disabled)\n");
        } else {
            printf("  Output file:      %s\n", config.output_file);
        }
        
        const char *format_display = "WAV";
        if (strcmp(config.format, "aiff") == 0) format_display = "AIFF";
        else if (strcmp(config.format, "ogg") == 0 || strcmp(config.format, "vorbis") == 0) format_display = "OGG Vorbis";
        
        if (!config.skip_audio_encoding) {
            printf("  Audio format:     %s at %d Hz\n", format_display, config.sample_rate);
            printf("  SSTV protocol:    %s (VIS code %d)\n", config.protocol, protocol_code);
            printf("  Image dimensions: %ux%u pixels\n", selected_mode->width, selected_mode->height);
            printf("Mode Details:\n");
            sstv_get_mode_details(protocol_code, 0, 0);
        }
        printf("--------------------------------------------------------------\n");
    }

    // ======================================================================
    // TILING MODE (if -X flag provided)
    // ======================================================================
    // When a tile grid is requested, the entire single-image pipeline is
    // replaced by a loop that: loads the original, crops one tile per iteration,
    // applies the 4-row structured header (Layer 1), aspect-corrects, encodes
    // SSTV audio, and writes a numbered audio file per tile.
    // ======================================================================
    if (config.tile_cols > 0) {

        int required_width  = (int)selected_mode->width;
        int required_height = (int)selected_mode->height;
        int tile_total      = config.tile_cols * config.tile_rows;

        /* Auto-enable keep_intermediate for tiling (saves pre-header tile PNGs) */
        if (tile_total > 1) {
            config.keep_intermediate = 1;
        }

        // ----------------------------------------------------------------
        // Derive audio output base: strip extension from config.output_file
        // Tile audio:  <base>-tile-NN-rRcC.<audio_ext>
        // ----------------------------------------------------------------
        char tile_audio_base[CONFIG_MAX_FILENAME + 1];
        const char *audio_ext_str;
        {
            const char *odot = strrchr(config.output_file, '.');
            if (odot) {
                size_t blen = (size_t)(odot - config.output_file);
                if (blen >= sizeof(tile_audio_base)) blen = sizeof(tile_audio_base) - 1;
                strncpy(tile_audio_base, config.output_file, blen);
                tile_audio_base[blen] = '\0';
                audio_ext_str = odot;          /* includes the dot, e.g. ".wav" */
            } else {
                strncpy(tile_audio_base, config.output_file, sizeof(tile_audio_base) - 1);
                tile_audio_base[sizeof(tile_audio_base) - 1] = '\0';
                audio_ext_str = ".wav";
            }
        }

        // ----------------------------------------------------------------
        // Derive intermediate image base (used when -K is active)
        // Tile intermediate:  <inter_base>-tile-NN-rRcC.png
        // Always PNG — the structured header rows require lossless storage;
        // JPEG compression would corrupt the CRC-protected pixel values.
        // ----------------------------------------------------------------
        char tile_inter_base[1024] = {0};
        // Always build the base name from input file stem + protocol
        // (used for pre-image and tile files, regardless of -K flag)
        {
            const char *inp = config.input_file;
            const char *base_start = strrchr(inp, '/');
            base_start = base_start ? base_start + 1 : inp;
            const char *idot = strrchr(base_start, '.');
            int stem_len = idot ? (int)(idot - base_start) : (int)strlen(base_start);
            snprintf(tile_inter_base, sizeof(tile_inter_base),
                     "%.*s-%s", stem_len, base_start, config.protocol);
        }

        // ----------------------------------------------------------------
        // Load image once to determine original (post-rotation) dimensions
        // ----------------------------------------------------------------
        verbose_print(config.verbose, config.timestamp_logging,
                     "[1/4] Loading image to determine original dimensions...\n");
        {
            int lr = image_load_from_file(config.input_file,
                                          config.verbose, config.timestamp_logging, NULL);
            if (lr != SLOWFRAME_OK) { error_code = lr; goto cleanup; }
        }
        if (config.rotation_degrees != 0) {
            int rr = image_rotate(config.rotation_degrees,
                                  config.verbose, config.timestamp_logging);
            if (rr != SLOWFRAME_OK) { error_code = rr; goto cleanup; }
        }
        int orig_w = 0, orig_h = 0;
        image_get_dimensions(&orig_w, &orig_h);

        if (config.verbose) {
            verbose_print(config.verbose, config.timestamp_logging,
                         "   --> Original dimensions: %dx%d\n", orig_w, orig_h);
            verbose_print(config.verbose, config.timestamp_logging,
                         "   --> Grid: %dx%d (%d tiles)\n",
                         config.tile_cols, config.tile_rows, tile_total);
            if (config.tile_overlap > 0)
                verbose_print(config.verbose, config.timestamp_logging,
                             "   --> Overlap: %d px per shared edge\n", config.tile_overlap);
        }

        // Session ID: timestamp XOR PID — unique per run, no extra deps
        uint32_t session_id = (uint32_t)(time(NULL)) ^ (uint32_t)(getpid());

        /* Aspect ratio check — warn if tile crop shape doesn't match mode.
         * For a CxR grid, each tile covers (orig_w/C) x (orig_h/R) of the
         * source.  If that ratio doesn't match required_width/required_height
         * the image_correct_aspect_and_resize step will stretch or letterbox.
         * Formula: distortion % = |tile_ar - mode_ar| / mode_ar * 100        */
        {
            double tile_ar = (double)orig_w / config.tile_cols
                           / ((double)orig_h / config.tile_rows);
            double mode_ar = (double)required_width / required_height;
            double distortion_pct = fabs(tile_ar - mode_ar) / mode_ar * 100.0;
            if (distortion_pct > 1.0) {
                /* Compute a suggestion: square grid that would be undistorted */
                fprintf(stderr,
                    "\n[WARNING] Tile aspect ratio mismatch!\n"
                    "  Grid %dx%d creates %dx%d tile crops (aspect %.4f)\n"
                    "  Mode %-5s needs aspect %.4f  →  %.1f%% distortion\n"
                    "  Undistorted grids for this source+mode: ",
                    config.tile_cols, config.tile_rows,
                    orig_w / config.tile_cols, orig_h / config.tile_rows,
                    tile_ar,
                    config.protocol, mode_ar,
                    distortion_pct);
                /* Print valid grid suggestions (distortion < 1%) */
                int found = 0;
                for (int c = 1; c <= 16; c++) {
                    for (int r = 1; r <= 16; r++) {
                        if (c == 1 && r == 1) continue; /* skip trivial */
                        double ta = (double)orig_w / c / ((double)orig_h / r);
                        if (fabs(ta - mode_ar) / mode_ar * 100.0 < 1.0) {
                            fprintf(stderr, "%dx%d ", c, r);
                            if (++found >= 6) { fprintf(stderr, "..."); goto done_suggest; }
                        }
                    }
                }
                done_suggest:
                fprintf(stderr, "\n\n");
            }
        }

        // ----------------------------------------------------------------
        // Scale source ONCE to the full SSTV grid content resolution.
        //
        // All tiles are then cropped from this single pre-scaled image.
        // This guarantees that every tile's content comes from one
        // consistent resize operation — eliminating per-tile scaling
        // discrepancies and seam discontinuities.
        //
        // Grid content size:
        //   grid_w = required_width  * tile_cols
        //   grid_h = content_h       * tile_rows   (content_h = mode_h - 8)
        //
        // The pre-image is always written to disk (the tile loop loads it).
        // It is deleted after encoding if -K was not requested.
        // ----------------------------------------------------------------
        int content_h = required_height - (int)SF_TILE_HEADER_ROWS;
        int grid_w    = required_width  * config.tile_cols;
        int grid_h    = content_h       * config.tile_rows;

        char pre_path[1024];
        snprintf(pre_path, sizeof(pre_path), "%s-tile-pre.png", tile_inter_base);

        printf("[PRE] Scaling source to grid resolution %dx%d...\n", grid_w, grid_h);
        {
            int lp = image_load_from_file(config.input_file,
                                          config.verbose,
                                          config.timestamp_logging, NULL);
            if (lp != SLOWFRAME_OK) { error_code = lp; goto cleanup; }
        }
        if (config.rotation_degrees != 0) {
            int rp = image_rotate(config.rotation_degrees,
                                  config.verbose, config.timestamp_logging);
            if (rp != SLOWFRAME_OK) { error_code = rp; goto cleanup; }
        }
        {
            int ap = image_correct_aspect_and_resize(
                         grid_w, grid_h,
                         config.aspect_mode,
                         config.verbose, config.timestamp_logging, NULL);
            if (ap != SLOWFRAME_OK) { error_code = ap; goto cleanup; }
        }
        {
            int sp = image_save_to_file(pre_path, config.verbose);
            if (sp != SLOWFRAME_OK) { error_code = sp; goto cleanup; }
        }
        printf("[PRE] Saved: %s\n", pre_path);

        int seq = 1;
        for (int tile_row = 0; tile_row < config.tile_rows; tile_row++) {
            for (int tile_col = 0; tile_col < config.tile_cols; tile_col++) {

                printf("--------------------------------------------------------------\n");
                printf("[TILE %d/%d]  row=%d  col=%d\n",
                       seq, tile_total, tile_row, tile_col);
                printf("--------------------------------------------------------------\n");

                // ---- Crop tile from the globally-scaled pre-image ----
                //
                // Pre-image coordinates (all in SSTV pixels):
                //   x = tile_col * required_width
                //   y = tile_row * content_h
                //
                // For tile_row == 0: crop content_h rows; pad_top adds the
                //   8 header rows as black, which the header overwrites.
                //
                // For tile_row > 0: start the crop SF_TILE_HEADER_ROWS rows
                //   above the tier boundary — those rows are a duplicate of
                //   the preceding tier's bottom, which the header overwrites.
                //   This gives exactly required_height rows with no pad needed.
                //
                // User tile_overlap extends the crop on all sides (left, right, top, bottom)
                // in pre-image pixel coordinates so the stitcher can trim them as usual.
                // This preserves recovery marker data in corners.
                int px_x = tile_col * required_width;
                int px_y = tile_row * content_h;

                int olap_left   = (tile_col > 0)                  ? config.tile_overlap : 0;
                int olap_right  = (tile_col < config.tile_cols-1) ? config.tile_overlap : 0;
                int olap_top    = (tile_row > 0)                  ? config.tile_overlap : 0;
                int olap_bottom = (tile_row < config.tile_rows-1) ? config.tile_overlap : 0;

                // For interior rows: borrow SF_TILE_HEADER_ROWS rows upward
                int top_borrow = (tile_row > 0) ? (int)SF_TILE_HEADER_ROWS : 0;

                int crop_x      = px_x - olap_left;
                int crop_y      = px_y - top_borrow - olap_top;
                int crop_w_tile = required_width  + olap_left + olap_right;
                int crop_h_tile = content_h + top_borrow + olap_bottom;  /* required_height when row>0 */

                /* Clamp to pre-image bounds */
                if (crop_x < 0) { crop_w_tile += crop_x; crop_x = 0; }
                if (crop_y < 0) { crop_h_tile += crop_y; crop_y = 0; }
                if (crop_x + crop_w_tile > grid_w) crop_w_tile = grid_w - crop_x;
                if (crop_y + crop_h_tile > grid_h) crop_h_tile = grid_h - crop_y;

                verbose_print(config.verbose, config.timestamp_logging,
                             "   --> Pre-image crop: (%d,%d) %dx%d\n",
                             crop_x, crop_y, crop_w_tile, crop_h_tile);

                // ---- Load pre-image and crop tile region ----
                {
                    int lr2 = image_load_from_file(pre_path,
                                                   config.verbose,
                                                   config.timestamp_logging, NULL);
                    if (lr2 != SLOWFRAME_OK) { error_code = lr2; goto cleanup; }
                }
                {
                    int cr = image_crop_region(crop_x, crop_y,
                                               crop_w_tile, crop_h_tile,
                                               config.verbose, config.timestamp_logging);
                    if (cr != SLOWFRAME_OK) { error_code = cr; goto cleanup; }
                }

                // ---- Pad/position header rows ----
                if (tile_row == 0) {
                    // No preceding tier: prepend SF_TILE_HEADER_ROWS black rows.
                    int pr = image_pad_top((int)SF_TILE_HEADER_ROWS,
                                          config.verbose, config.timestamp_logging);
                    if (pr != SLOWFRAME_OK) { error_code = pr; goto cleanup; }
                }
                // For tile_row > 0: the top SF_TILE_HEADER_ROWS rows are real
                // content borrowed from the previous tier's bottom. The header
                // will overwrite them — no pad_top needed.

                // ---- Apply text overlays (if any) ----
                if (overlay_spec_list_count(&config.overlay_specs) > 0) {
                    int ovr = image_apply_overlay_list(&config.overlay_specs,
                                                       config.verbose,
                                                       config.timestamp_logging);
                    if (ovr != SLOWFRAME_OK) { error_code = ovr; goto cleanup; }
                }

                // ---- Apply Layer 1: structured color-bar header rows ----
                {
                    ExifMetadata exif_data;
                    if (image_get_exif_data(&exif_data) != SLOWFRAME_OK) {
                        // Use defaults if EXIF extraction failed
                        memset(&exif_data, 0, sizeof(ExifMetadata));
                        exif_data.f_stop = 28;
                        exif_data.metering_mode = 1;
                        exif_data.exposure_program = 2;
                        exif_data.exposure_time_log = 0;
                        exif_data.focal_length = 50;
                        exif_data.brightness_ev = 10;
                        exif_data.iso_speed = 400;
                        exif_data.white_balance = 0;
                        exif_data.color_space = 0;
                        exif_data.color_profile_id = 0;
                        exif_data.device_make_id = 0;
                        exif_data.device_model_id = 0;
                    }

                    SFTileInfo ti = {
                        .version        = 2,  /* Version 2: 16-block EXIF format */
                        .grid_cols      = (uint8_t)config.tile_cols,
                        .grid_rows      = (uint8_t)config.tile_rows,
                        .tile_col       = (uint8_t)tile_col,
                        .tile_row       = (uint8_t)tile_row,
                        .total_tiles    = (uint8_t)tile_total,
                        .seq_num        = (uint8_t)seq,
                        .orig_width     = (uint16_t)orig_w,
                        .orig_height    = (uint16_t)orig_h,
                        .overlap_px     = (uint8_t)config.tile_overlap,
                        .session_id     = session_id,
                        /* Use extracted EXIF data from source image */
                        .exif_version   = 0x23,  /* EXIF 2.3 */
                        .f_stop         = exif_data.f_stop,
                        .metering_mode  = exif_data.metering_mode,
                        .exposure_program = exif_data.exposure_program,
                        .exposure_time_log = exif_data.exposure_time_log,
                        .focal_length   = exif_data.focal_length,
                        .brightness_ev  = exif_data.brightness_ev,
                        .iso_speed      = exif_data.iso_speed,
                        .white_balance  = exif_data.white_balance,
                        .color_space    = exif_data.color_space,
                        .color_profile_id = exif_data.color_profile_id,
                        .device_make_id = exif_data.device_make_id,
                        .device_model_id = exif_data.device_model_id,
                        .date_year      = exif_data.date_year,
                        .date_month     = exif_data.date_month,
                        .date_day       = exif_data.date_day,
                        .date_hour      = exif_data.date_hour,
                        .date_minute    = exif_data.date_minute,
                        .date_second    = exif_data.date_second,
                    };
                    int hr = image_apply_tile_header(&ti,
                                                     config.verbose,
                                                     config.timestamp_logging);
                    if (hr != SLOWFRAME_OK) { error_code = hr; goto cleanup; }
                }

                // ---- Embed recovery markers (for QRM resilience) ----
                // Only embed markers when we have overlap, since markers overwrite image data
                // With 0 overlap, markers just create stitching artifacts without providing recovery benefit
                if (config.recovery_enabled && config.recovery_embed_markers && config.tile_overlap > 0) {
                    int mr = image_embed_recovery_markers(tile_row, tile_col);
                    if (mr == SLOWFRAME_OK && config.verbose) {
                        verbose_print(config.verbose, config.timestamp_logging,
                                    "   --> Embedded recovery markers at (%d, %d)\n",
                                    tile_row, tile_col);
                    }
                }

                // ---- Save intermediate tile image (if -K) ----
                if (config.keep_intermediate) {
                    char tile_inter[1024];
                    snprintf(tile_inter, sizeof(tile_inter),
                             "%s-tile-%02d-r%dc%d.png",
                             tile_inter_base, seq, tile_row, tile_col);
                    int sr = image_save_to_file(tile_inter, config.verbose);
                    if (sr != SLOWFRAME_OK) { error_code = sr; goto cleanup; }
                    verbose_print(config.verbose, config.timestamp_logging,
                                 "   --> Saved tile intermediate: %s\n", tile_inter);
                }

                // ---- SSTV encode ----
                verbose_print(config.verbose, config.timestamp_logging,
                             "[2/4] Encoding tile %d/%d as SSTV audio...\n",
                             seq, tile_total);
                {
                    int enc = sstv_encode_frame_with_mode(selected_mode,
                                                          config.verbose,
                                                          config.timestamp_logging);
                    if (enc != SLOWFRAME_OK) { error_code = enc; goto cleanup; }
                }

                // ---- Write audio ----
                char tile_audio_path[CONFIG_MAX_FILENAME + 64];
                snprintf(tile_audio_path, sizeof(tile_audio_path),
                         "%s-tile-%02d-r%dc%d%s",
                         tile_audio_base, seq, tile_row, tile_col,
                         audio_ext_str);

                verbose_print(config.verbose, config.timestamp_logging,
                             "[3/4] Writing tile audio: %s\n", tile_audio_path);

                {
                    uint32_t sc = 0;
                    const uint16_t *samp = sstv_get_samples(&sc);
                    if (sc == 0) {
                        error_log(SLOWFRAME_ERR_SSTV_ENCODE, "No audio samples for tile %d", seq);
                        error_code = SLOWFRAME_ERR_SSTV_ENCODE;
                        goto cleanup;
                    }

                    AudioEncoder *enc2 = audio_encoder_create(config.format);
                    if (!enc2) {
                        error_code = SLOWFRAME_ERR_ARG_INVALID_FORMAT;
                        goto cleanup;
                    }
                    int ir = audio_encoder_init(enc2, config.sample_rate, BITS, CHANS,
                                               tile_audio_path);
                    if (ir != SLOWFRAME_OK) {
                        audio_encoder_destroy(enc2);
                        error_code = ir;
                        goto cleanup;
                    }
                    int er2 = audio_encoder_encode(enc2, samp, sc);
                    if (er2 != SLOWFRAME_OK) {
                        audio_encoder_destroy(enc2);
                        error_code = er2;
                        goto cleanup;
                    }
                    int fr = audio_encoder_finish(enc2);
                    audio_encoder_destroy(enc2);
                    if (fr != SLOWFRAME_OK) { error_code = fr; goto cleanup; }

                    printf("   [OK] %s  (%.2f s at %d Hz)\n",
                           tile_audio_path,
                           sc / (double)config.sample_rate,
                           config.sample_rate);
                }

                /* Reset SSTV audio buffer so next tile starts clean */
                sstv_reset_buffer();

                seq++;
            } /* tile_col */
        } /* tile_row */

        /* Remove the pre-image temp file if -K was not requested */
        if (!config.keep_intermediate) {
            remove(pre_path);
        }

        /* Final summary */
        gettimeofday(&end_tv, NULL);
        uint32_t elapsed_tile_ms = (uint32_t)(
            (end_tv.tv_sec  - start_tv.tv_sec)  * 1000 +
            (end_tv.tv_usec - start_tv.tv_usec) / 1000);

        printf("--------------------------------------------------------------\n");
        printf("[COMPLETE] TILING COMPLETE\n");
        printf("--------------------------------------------------------------\n");
        printf("Tiles encoded: %d  (%dx%d grid)\n",
               tile_total, config.tile_cols, config.tile_rows);
        printf("Session ID:    0x%08X\n", (unsigned int)session_id);
        printf("Encoding time: %u ms\n", elapsed_tile_ms);

        image_free();
        sstv_cleanup();
        vips_shutdown();
        slowframe_context_cleanup(&ctx);
        slowframe_config_cleanup(&config);
        return SLOWFRAME_OK;
    }
    /* ---- end tiling-mode block ---- */

    // Load image using new image module (auto-detects format)
    verbose_print(config.verbose, config.timestamp_logging, "[1/4] Loading image...\n");
    int image_result = image_load_from_file(config.input_file, config.verbose, config.timestamp_logging, NULL);
    if (image_result != SLOWFRAME_OK) {
        // Error already logged by image_load_from_file(), propagate error code
        error_code = image_result;
        goto cleanup;
    }

    // ======================================================================
    // IMAGE ROTATION (if requested via -D flag)
    // ======================================================================
    if (config.rotation_degrees != 0) {
        verbose_print(config.verbose, config.timestamp_logging,
                     "[1a/4] Rotating image %+d degrees...\n", config.rotation_degrees);
        int rotate_result = image_rotate(config.rotation_degrees,
                                         config.verbose, config.timestamp_logging);
        if (rotate_result != SLOWFRAME_OK) {
            error_log(rotate_result, "Image rotation failed");
            error_code = rotate_result;
            goto cleanup;
        }
        verbose_print(config.verbose, config.timestamp_logging,
                     "   [OK] Image rotation complete\n");
    }

    // ======================================================================
    // IMAGE DIMENSION REQUIREMENTS
    // ======================================================================
    // Get required dimensions from selected mode definition
    int required_width = (int)selected_mode->width;
    int required_height = (int)selected_mode->height;
    // ======================================================================
    // Build path for intermediate resized image preserving original format.
    // Structure: {output_dir}/{output_base}{suffix}{original_extension}
    // Example: output.wav in /tmp with input.png → /tmp/output.png
    // When -N (skip audio), add "_overlay" suffix to prevent collision with source
    char intermediate_image[1024];
    {

        if (config.keep_intermediate_explicit && strlen(config.intermediate_file) > 0) {
            // User explicitly requested -K with a filename (or auto-generated from input+protocol)
            strncpy(intermediate_image, config.intermediate_file, sizeof(intermediate_image) - 1);
            intermediate_image[sizeof(intermediate_image) - 1] = '\0';
        } else {
            // Legacy: build from output audio filename (auto-enable cases: verbose, overlays, -N)
            char out_dir[1024] = {0};
            const char *last_slash = strrchr(config.output_file, '/');
            if (last_slash) {
                int dir_len = last_slash - config.output_file;
                strncpy(out_dir, config.output_file, dir_len);
                out_dir[dir_len] = '\0';
            } else {
                strcpy(out_dir, ".");
            }

            char out_base[256];
            const char *base_start = last_slash ? last_slash + 1 : config.output_file;
            const char *dot = strrchr(base_start, '.');
            if (dot) {
                int base_len = dot - base_start;
                strncpy(out_base, base_start, base_len);
                out_base[base_len] = '\0';
            } else {
                strcpy(out_base, base_start);
            }

            const char *orig_ext = image_get_original_extension();

            if (config.text_only) {
                snprintf(intermediate_image, sizeof(intermediate_image), "%s/%s_textonly%s",
                         out_dir, out_base, orig_ext);
            } else if (config.skip_audio_encoding) {
                snprintf(intermediate_image, sizeof(intermediate_image), "%s/%s_overlay%s",
                         out_dir, out_base, orig_ext);
            } else {
                // Append _processed suffix to avoid accidentally overwriting input
                snprintf(intermediate_image, sizeof(intermediate_image), "%s/%s_processed%s",
                         out_dir, out_base, orig_ext);
            }
        }
    }
    
    // Skip aspect ratio correction if text-only mode is enabled
    if (!config.text_only) {
        int aspect_result = image_correct_aspect_and_resize(required_width, required_height, config.aspect_mode, 
                                                             config.verbose, config.timestamp_logging,
                                                             NULL);
        if (aspect_result != SLOWFRAME_OK) {
            // Error already logged by image_correct_aspect_and_resize()
            error_code = aspect_result;
            goto cleanup;
        }
    } else {
        verbose_print(config.verbose, config.timestamp_logging,
                     "[1a/4] Text-only mode: skipping aspect ratio correction and resizing\n");
    }

    // ======================================================================
    // APPLY TEXT OVERLAY (if enabled)
    // ======================================================================
    // Text overlays are applied with blue text and white background
    // Configured via -T (text overlay) option
    
    if (overlay_spec_list_count(&config.overlay_specs) > 0) {
        verbose_print(config.verbose, config.timestamp_logging, 
                     "[1b/4] Applying %zu text overlay(s)...\n",
                     overlay_spec_list_count(&config.overlay_specs));
        
        int overlay_result = image_apply_overlay_list(&config.overlay_specs, 
                                                      config.verbose, 
                                                      config.timestamp_logging);
        if (overlay_result != SLOWFRAME_OK) {
            error_log(overlay_result, "Text overlay application failed");
            error_code = overlay_result;
            goto cleanup;
        }
        
        verbose_print(config.verbose, config.timestamp_logging, 
                     "   [OK] Overlays applied to image\n");
    }

    // ======================================================================
    // SAVE INTERMEDIATE IMAGE (if requested via -K flag)
    // ======================================================================
    // Save intermediate image after all processing (aspect correction + overlays)
    // but before audio encoding. This ensures the intermediate includes overlays.
    
    if (config.keep_intermediate) {
        int save_result = image_save_to_file(intermediate_image, config.verbose);
        if (save_result != SLOWFRAME_OK) {
            error_log(save_result, "Failed to save intermediate image");
            error_code = save_result;
            goto cleanup;
        }
        verbose_print(config.verbose, config.timestamp_logging, 
                     "   --> Saved intermediate image (with overlays): %s\n", intermediate_image);
    }
    
    // ======================================================================
    // CHECK FOR TEST MODE (skip audio encoding)
    // ======================================================================
    // If -N flag was used, skip SSTV encoding and exit after saving intermediate
    // This allows users to test overlay functionality without encoding overhead
    
    if (config.skip_audio_encoding) {
        verbose_print(config.verbose, config.timestamp_logging, 
                     "[2/4] Skipping SSTV audio encoding (test mode)...\n");
        
        // Processed image was saved above due to auto-enabled keep_intermediate
        if (!config.keep_intermediate) {
            // Safety check: should never happen due to auto-enable, but be defensive
            int save_result = image_save_to_file(intermediate_image, config.verbose);
            if (save_result != SLOWFRAME_OK) {
                error_log(save_result, "Failed to save test output image");
                error_code = save_result;
                goto cleanup;
            }
        }
        
        verbose_print(config.verbose, config.timestamp_logging, 
                     "[OK] Test mode complete\n");
        verbose_print(config.verbose, config.timestamp_logging, 
                     "     Overlay result saved to: %s\n", intermediate_image);
        verbose_print(config.verbose, config.timestamp_logging, 
                     "     (Audio encoding skipped in test mode)\n");
        
        // Jump to cleanup without encoding
        error_code = SLOWFRAME_OK;
        goto cleanup;
    }
    
    verbose_print(config.verbose, config.timestamp_logging, "[2/4] Encoding image as SSTV audio...\n");

    // Encode the image
    verbose_print(config.verbose, config.timestamp_logging, "   --> Processing pixels...\n");
    fflush(stdout);
    
    int encode_result = sstv_encode_frame_with_mode(selected_mode,
                                                    config.verbose,
                                                    config.timestamp_logging);
    if (encode_result != SLOWFRAME_OK) {
        // Error already logged by sstv_encode_frame()
        error_code = encode_result;
        goto cleanup;
    }

    verbose_print(config.verbose, config.timestamp_logging, "   [OK] Image encoded\n");

    if (config.cw_enabled) {
        verbose_print(config.verbose, config.timestamp_logging, "   --> Adding CW signature: '%s' (WPM: %d, Tone: %d Hz)\n", config.cw_callsign[0] ? config.cw_callsign : "NOCALL", config.cw_wpm, config.cw_tone);
        sstv_add_cw_signature(config.cw_callsign[0] ? config.cw_callsign : "NOCALL", config.cw_wpm, config.cw_tone);
        verbose_print(config.verbose, config.timestamp_logging, "   [OK] CW signature added\n");
    }

    verbose_print(config.verbose, config.timestamp_logging, "   [OK] adding VIS footer to audio\n");
    verbose_print(config.verbose, config.timestamp_logging, "[3/4] Writing audio file...\n");
    verbose_print(config.verbose, config.timestamp_logging, "   --> Format: %s\n", config.format);
    verbose_print(config.verbose, config.timestamp_logging, "   --> Sample rate: %d Hz\n", config.sample_rate);

    // Get samples from SSTV module
    uint32_t sample_count = 0;
    const uint16_t *audio_samples = sstv_get_samples(&sample_count);
    
    // Basic safety checks before writing
    if (sample_count == 0) {
        error_log(SLOWFRAME_ERR_SSTV_ENCODE, "Audio synthesis", "No audio samples generated from SSTV encoding");
        error_code = SLOWFRAME_ERR_SSTV_ENCODE;
        goto cleanup;
    }
    if ((uint64_t)sample_count >= (uint64_t)MAXSAMPLES) {
        error_log(SLOWFRAME_ERR_SYSTEM_RESOURCE, "Audio buffer", "Audio sample count exceeds capacity: %u >= %llu", sample_count, (unsigned long long)MAXSAMPLES);
        error_code = SLOWFRAME_ERR_SYSTEM_RESOURCE;
        goto cleanup;
    }

    // ======================================================================
    // AUDIO ENCODING: FACTORY PATTERN
    // ======================================================================
    // Create format-specific encoder using factory pattern. This allows
    // adding new formats (FLAC, Opus, etc.) without modifying main logic.
    // Encoders implement common interface: init → encode → finish → destroy
    
    AudioEncoder *encoder = audio_encoder_create(config.format);
    if (!encoder) {
        // Factory returns NULL if format unsupported (should never happen
        // due to earlier validation, but defensive check)
        error_log(SLOWFRAME_ERR_ARG_INVALID_FORMAT, "Audio encoder factory", "Unsupported format: %s", config.format);
        error_code = SLOWFRAME_ERR_ARG_INVALID_FORMAT;
        goto cleanup;
    }

    // Initialize encoder: opens file, writes headers
    int encoder_init_result = audio_encoder_init(encoder, config.sample_rate, BITS, CHANS, config.output_file);
    if (encoder_init_result != SLOWFRAME_OK) {
        error_log(encoder_init_result, "Audio encoder initialization", 
                 "Failed to initialize %s encoder for output file: %s", config.format, config.output_file);
        audio_encoder_destroy(encoder);
        error_code = encoder_init_result;
        goto cleanup;
    }

    // Write audio samples (encoding happens here for OGG)
    int encoder_encode_result = audio_encoder_encode(encoder, audio_samples, sample_count);
    if (encoder_encode_result != SLOWFRAME_OK) {
        error_log(encoder_encode_result, "Audio sample encoding", 
                 "Failed to encode %u audio samples to %s format", sample_count, config.format);
        audio_encoder_destroy(encoder);
        error_code = encoder_encode_result;
        goto cleanup;
    }

    // Finalize file: update headers with final sizes, flush buffers, close
    int encoder_finish_result = audio_encoder_finish(encoder);
    if (encoder_finish_result != SLOWFRAME_OK) {
        error_log(encoder_finish_result, "Audio file finalization", 
                 "Failed to finalize %s audio file: %s", config.format, config.output_file);
        audio_encoder_destroy(encoder);
        error_code = encoder_finish_result;
        goto cleanup;
    }

    // Clean up encoder resources (frees memory, ensures file closed)
    audio_encoder_destroy(encoder);

    // Cleanup and report
    image_free();
    sstv_cleanup();
    vips_shutdown();

    gettimeofday(&end_tv, NULL);
    uint32_t elapsed_ms = (end_tv.tv_sec - start_tv.tv_sec) * 1000 + 
                          (end_tv.tv_usec - start_tv.tv_usec) / 1000;
    verbose_print(config.verbose, config.timestamp_logging, "[4/4] File written! Done.\n");
    
    if (config.verbose) {
        verbose_print(config.verbose, config.timestamp_logging, "--------------------------------------------------------------\n");
        verbose_print(config.verbose, config.timestamp_logging, "[COMPLETE] ENCODING COMPLETE\n");
        verbose_print(config.verbose, config.timestamp_logging, "--------------------------------------------------------------\n");
        verbose_print(config.verbose, config.timestamp_logging, "Output file: %s\n", config.output_file);
        verbose_print(config.verbose, config.timestamp_logging, "Audio samples: %u (%.2f seconds at %d Hz)\n", sample_count, sample_count / (double)config.sample_rate, config.sample_rate);
        verbose_print(config.verbose, config.timestamp_logging, "Encoding time: %u millisecond%s\n", elapsed_ms, elapsed_ms == 1 ? "" : "s");
    } else {
        printf("--------------------------------------------------------------\n");
        printf("[COMPLETE] ENCODING COMPLETE\n");
        printf("--------------------------------------------------------------\n");
        printf("Output file: %s\n", config.output_file);
        printf("Audio samples: %u (%.2f seconds at %d Hz)\n", sample_count, sample_count / (double)config.sample_rate, config.sample_rate);
        printf("Encoding time: %u millisecond%s\n", elapsed_ms, elapsed_ms == 1 ? "" : "s");
    }
    
    return error_code;

cleanup:
    // ======================================================================
    // CENTRALIZED ERROR CLEANUP
    // ======================================================================
    // This label is the target of all error path 'goto' statements. It ensures
    // proper cleanup regardless of where failure occurs. Each cleanup function
    // is safe to call multiple times (idempotent) and handles NULL/uninitialized
    // state gracefully.
    if (context_initialized) {
        slowframe_context_cleanup(&ctx);  // Free context (includes MMSSTV adapter)
    }
    image_free();                       // Free libvips image resources
    sstv_cleanup();                     // Free SSTV audio buffer
    slowframe_config_cleanup(&config);   // Free config resources
    vips_shutdown();                    // Shutdown libvips (releases all resources)
    return error_code;  // Propagate error code to shell (0 = success, >0 = error)
}

// end
