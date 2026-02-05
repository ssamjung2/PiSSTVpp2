/**
 * @file pisstvpp2.c
 * @brief PiSSTVpp2 - Raspberry Pi SSTV Image to Audio Encoder (Main Entry Point)
 *
 * ## Overview
 * This is the main program entry point for PiSSTVpp2, a high-performance SSTV
 * (Slow Scan Television) image-to-audio encoder designed for Raspberry Pi and
 * other embedded systems. The program converts still images into audio signals
 * compatible with amateur radio SSTV transmission protocols.
 *
 * ## Architecture
 * PiSSTVpp2 uses a modular architecture with three main subsystems:
 * - **Image Processing** (pisstvpp2_image.h): libvips-based image loading,
 *   scaling, aspect correction, and pixel access
 * - **SSTV Encoding** (pisstvpp2_sstv.h): Audio synthesis, protocol encoding
 *   (Martin, Scottie, Robot), VIS headers, and optional CW signatures
 * - **Audio Encoding** (pisstvpp2_audio_encoder.h): Pluggable format encoders
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
 *   pisstvpp2.c pisstvpp2_image.c pisstvpp2_sstv.c pisstvpp2_audio_encoder.c \
 *   -o pisstvpp2 \
 *   -L/opt/homebrew/lib -L/opt/homebrew/Cellar/glib/2.86.3/lib \
 *   -lvips -lglib-2.0 -lgobject-2.0 -lvorbis -logg -lm
 * @endcode
 *
 * ## Usage Examples
 * @code
 * # Basic: Convert image to WAV using default Martin 1 mode
 * ./pisstvpp2 -i photo.jpg
 *
 * # Specify output format and protocol
 * ./pisstvpp2 -i photo.png -f ogg -p s1 -o output.ogg
 *
 * # Verbose output with custom sample rate
 * ./pisstvpp2 -i image.jpg -f wav -r 11025 -v
 *
 * # Add CW signature
 * ./pisstvpp2 -i photo.jpg -C N0CALL -W 20 -T 700
 *
 * # Robot 36 mode with aspect correction
 * ./pisstvpp2 -i photo.jpg -p r36 -a pad
 * @endcode
 *
 * ## License
 * Open source software - see LICENSE file in repository
 *
 * @author PiSSTVpp2 Contributors
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
#include "pisstvpp2_image.h"
#include "pisstvpp2_sstv.h"
#include "pisstvpp2_audio_encoder.h"
#include "logging.h"

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
 * @brief Macros and patterns for consistent error management
 *
 * The VIPS_CALL macro provides centralized error handling for all libvips
 * operations. It reduces boilerplate and ensures consistent error cleanup.
 *
 * **Usage Pattern:**
 * @code
 * VIPS_CALL(vips_resize, image, &resized, scale, NULL);
 * // Automatically checks return value, logs errors, cleans up, and jumps to cleanup
 * @endcode
 *
 * **Error Handling Flow:**
 * 1. Execute libvips function with provided arguments
 * 2. If return value != 0 (error), print function name
 * 3. Clear libvips error buffer (prevents memory leaks)
 * 4. Set error_code = 2 (runtime error)
 * 5. Jump to cleanup: label for resource cleanup
 *
 * @note Requires 'cleanup' label and 'error_code' variable in calling function
 * @{
 */
#define VIPS_CALL(func, ...) do { \
    if (func(__VA_ARGS__)) { \
        fprintf(stderr, "[ERROR] %s failed\n", #func); \
        vips_error_clear(); \
        error_code = 2; \
        goto cleanup; \
    } \
} while(0)
/** @} */ // End of ErrorHandling group

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
    printf("Usage: ./pisstvpp2 -i <input_file> [OPTIONS]\n\n");
    printf("PiSSTVpp v2.1.0 (built Jan 23 2026)\n");
    printf("Convert an image (PNG/JPEG/GIF/BMP) to SSTV audio format.\n\n");
    printf("REQUIRED OPTIONS:\n");
    printf("  -i <file>       Input image file (PNG, JPEG, GIF, or BMP)\n\n");
    printf("OPTIONAL OPTIONS:\n");
    printf("  -a <mode>       Aspect ratio correction: center, pad, or stretch (default: center)\n");
    printf("  -o <file>       Output audio file (default: input_file.wav)\n");
    printf("  -p <protocol>   SSTV protocol to use (default: m1)\n");
    printf("                  Available protocols:\n");
    printf("                    m1     - Martin 1 (VIS 44)\n");
    printf("                    m2     - Martin 2 (VIS 40)\n");
    printf("                    s1     - Scottie 1 (VIS 60)\n");
    printf("                    s2     - Scottie 2 (VIS 56)\n");
    printf("                    sdx    - Scottie DX (VIS 76)\n");
    printf("                    r36    - Robot 36 Color (VIS 8)\n");
    printf("                    r72    - Robot 72 Color (VIS 12)\n");
    printf("  -f <fmt>        Output format: wav, aiff, or ogg (default: wav)\n");
    printf("  -r <rate>       Audio sample rate in Hz (default: 22050, range: 8000-48000)\n");
    printf("  -v              Enable verbose output (progress details)\n");
    printf("  -K              Keep intermediate processed image (for debugging)\n");
    printf("  -Z              Add timestamps to verbose logging (auto-enables -v, for log analysis)\n");
    printf("  -h              Display this help message\n\n");
    printf("CW SIGNATURE OPTIONS (optional):\n");
    printf("  -C <callsign>   Add CW signature with callsign (max 31 characters).\n");
    printf("  -W <wpm>        Set CW signature speed in WPM, range 1-50 (default: 15)\n");
    printf("  -T <freq>       Set CW signature tone frequency in Hz, range 400-2000 (default: 800)\n\n");    
    printf("EXAMPLES:\n");
    printf("  ./pisstvpp2 -i image.jpg -o out.aiff\n");
    printf("  ./pisstvpp2 -i image.jpg -f wav -p s2 -r 11025 -v\n");
    printf("  ./pisstvpp2 -i image.png -o output.wav -p r36\n\n");
    printf("  ./pisstvpp2 -i image.jpg -v -Z                          # Verbose with timestamps\n");
    printf("  ./pisstvpp2 -i image.jpg -C N0CALL -K                   # Keep intermediate, add CW\n");
    printf("  ./pisstvpp2 -i image.jpg -v -Z > processing.log         # Log with timestamps\n\n");
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

    // Initialize libvips
    if (VIPS_INIT(argv[0])) {
        vips_error_exit(NULL);
    }

    int error_code = 0;  // For centralized error cleanup
    char inputfile[255] = {0};
    char outputfile[255] = {0};
    const char *protocol = "m1";
    const char *format = "wav";
    uint16_t rate = RATE;
    int verbose = 0;
    int cw_enable = 0;
    char cw_callsign[16] = "";
    int cw_wpm = 15;
    uint16_t cw_tone = 800;
    int option;
    int w_flag = 0, t_flag = 0;  // Track if WPM/tone flags provided (require -C)
    AspectMode aspect_mode = ASPECT_CENTER;
    int keep_intermediate = 0;   // Keep intermediate processed image (-K flag)
    int timestamp_logging = 0;   // Add timestamps to verbose output (-Z flag)
    
    // ======================================================================
    // ARGUMENT PARSING
    // ======================================================================
    // Parse command-line options using getopt. Each option validates input
    // and sets configuration variables. Invalid values cause immediate exit
    // with error message.
    while ((option = getopt(argc, argv, "i:o:p:f:r:vC:W:T:a:KZh")) != -1) {
        switch (option) {
            // Aspect ratio correction mode
            case 'a':
                // Parse aspect correction: center-crop, black-pad, or stretch
                if (strcmp(optarg, "center") == 0) {
                    aspect_mode = ASPECT_CENTER;  // Center-crop to fill (default)
                } else if (strcmp(optarg, "pad") == 0) {
                    aspect_mode = ASPECT_PAD;  // Add black bars to preserve aspect
                } else if (strcmp(optarg, "stretch") == 0) {
                    aspect_mode = ASPECT_STRETCH;  // Distort to fit exact dimensions
                } else {
                    fprintf(stderr, "Error: Aspect mode must be 'center', 'pad', or 'stretch'\n");
                    return 1;
                }
                break;
            case 'i':
                if (strlen(optarg) >= sizeof(inputfile)) {
                    fprintf(stderr, "Error: Input filename too long (max 254 characters)\n");
                    return 1;
                }
                strncpy(inputfile, optarg, sizeof(inputfile) - 1);
                break;
            case 'o':
                if (strlen(optarg) >= sizeof(outputfile)) {
                    fprintf(stderr, "Error: Output filename too long (max 254 characters)\n");
                    return 1;
                }
                strncpy(outputfile, optarg, sizeof(outputfile) - 1);
                break;
            case 'p':
                protocol = optarg;
                break;
            case 'f':
                if (!audio_encoder_is_format_supported(optarg)) {
                    fprintf(stderr, "Error: Format must be 'wav', 'aiff', or 'ogg'\n");
                    return 1;
                }
                format = optarg;
                break;
            // Sample rate validation (robust string-to-integer conversion)
            case 'r': {
                char *endptr = NULL;
                errno = 0;  // Clear errno before strtol (best practice)
                long tmp = strtol(optarg, &endptr, 10);
                
                // Validate conversion: check for errors, empty string, trailing chars
                if (errno != 0 || endptr == optarg || *endptr != '\0') {
                    fprintf(stderr, "Error: Invalid sample rate '%s'\n", optarg);
                    return 1;
                }
                
                // Validate range: 8kHz minimum for clarity, 48kHz maximum for compatibility
                if (tmp < 8000 || tmp > 48000) {
                    fprintf(stderr, "Error: Sample rate must be between 8000 and 48000 Hz.\n");
                    return 1;
                }
                rate = (uint16_t)tmp;
                break;
            }
            case 'v':
                verbose = 1;
                break;
            case 'C':
                if (optarg && strlen(optarg) < sizeof(cw_callsign)) {
                    strncpy(cw_callsign, optarg, sizeof(cw_callsign) - 1);
                    cw_enable = 1;
                } else {
                    fprintf(stderr, "Error: Callsign too long (max 31 characters)\n");
                    return 1;
                }
                break;
            case 'W':
                cw_wpm = atoi(optarg);
                w_flag = 1;
                if (cw_wpm <= 0 || cw_wpm > 50) {
                    fprintf(stderr, "Error: Words per minute rate must be between 1 and 50.\n");
                    return 1;
                }
                break;
            case 'T':
                cw_tone = (uint16_t)atoi(optarg);
                t_flag = 1;
                if (cw_tone <= 0 || cw_tone > 2000 || cw_tone < 400) {
                    fprintf(stderr, "Error: CW tone must be between 400 Hz and 2000 Hz.\n");
                    return 1;
                }                
                break;
            case 'h':
                show_help();
                return 0;
            // Keep intermediate processed image (useful for debugging)
            case 'K':
                keep_intermediate = 1;
                break;
            // Add timestamps to verbose output (for log file analysis)
            // Note: Also auto-enables verbose mode for convenience
            case 'Z':
                timestamp_logging = 1;
                verbose = 1;  // Auto-enable verbose when timestamps requested
                break;
            case '?':
            default:
                fprintf(stderr, "Error: Invalid option\n");
                show_help();
                return 1;
        }
    }

    // ======================================================================
    // PARAMETER VALIDATION: INTERDEPENDENCIES
    // ======================================================================
    // CW parameters require callsign: WPM/tone flags meaningless without it
    if ((w_flag || t_flag) && strlen(cw_callsign) == 0) {
        fprintf(stderr, "Error: -C <callsign> is required if -W or -T are provided.\n");
        return 1;
    }

    // ======================================================================
    // FEATURE INTERACTIONS
    // ======================================================================
    // When verbose mode is enabled (-v or -Z), automatically enable intermediate
    // image output for debugging and timeline analysis purposes
    if (verbose) {
        keep_intermediate = 1;  // Auto-enable intermediate image in verbose mode
    }

    // Validate required input file
    if (strlen(inputfile) == 0) {
        fprintf(stderr, "Error: Input file (-i) is required\n\n");
        show_help();
        return 1;
    }

    // Store validated sample rate locally (passed to SSTV module)

    // ======================================================================
    // PROTOCOL MAPPING: STRING TO VIS CODE
    // ======================================================================
    // Map user-friendly protocol strings to SSTV VIS (Vertical Interval
    // Signaling) codes defined in SSTV specification. VIS codes tell the
    // decoder which mode to use.
    uint8_t protocol_code = 44;  // Default: Martin 1
    if (strcmp(protocol, "m1") == 0) {
        protocol_code = 44; // Martin 1
    }
    else if (strcmp(protocol, "m2") == 0) {
        protocol_code = 40; // Martin 2
    }
    else if (strcmp(protocol, "s1") == 0) {
        protocol_code = 60; // Scottie 1
    }
    else if (strcmp(protocol, "s2") == 0) {
        protocol_code = 56; // Scottie 2
    }
    else if (strcmp(protocol, "sdx") == 0) {
        protocol_code = 76; // Scottie DX
    }
    else if (strcmp(protocol, "r36") == 0) {
        protocol_code = 8; // Robot 36
    }
    else if (strcmp(protocol, "r72") == 0) {
        protocol_code = 12; // Robot 72
    }
    else {
        fprintf(stderr, "Error: Unrecognized protocol '%s'\n", protocol);
        show_help();
        return 1;
    }

    // Note: SSTV module initialization is deferred to main encoding phase
    sstv_set_protocol(protocol_code);

    // locals - use gettimeofday for millisecond precision
    struct timeval start_tv, end_tv;
    gettimeofday(&start_tv, NULL);

    // ======================================================================
    // OUTPUT FILENAME GENERATION
    // ======================================================================
    // If no output file specified: derive from input filename + audio format
    // Example: photo.jpg with -f wav → photo.jpg.wav
    if (strlen(outputfile) == 0) {
        // Copy input filename as base (safe: null-terminated)
        strncpy(outputfile, inputfile, sizeof(outputfile) - 1);
        outputfile[sizeof(outputfile) - 1] = '\0';

        // Verify sufficient space for extension (buffer overflow protection)
        size_t output_len = strlen(outputfile);
        size_t ext_len = strlen(format) + 1; // +1 for the dot

        if (output_len + ext_len >= sizeof(outputfile)) {
            fprintf(stderr, "Error: Output filename too long after adding extension\n");
            return 1;
        }

        // Append .{format} extension safely
        strncat(outputfile, ".", sizeof(outputfile) - strlen(outputfile) - 1);
        strncat(outputfile, format, sizeof(outputfile) - strlen(outputfile) - 1);
    } else {
        // If output file has no extension, append the selected format
        const char *slash = strrchr(outputfile, '/');
        const char *dot = strrchr(outputfile, '.');
        if (!dot || (slash && dot < slash)) {
            size_t output_len = strlen(outputfile);
            size_t ext_len = strlen(format) + 1; // +1 for the dot

            if (output_len + ext_len >= sizeof(outputfile)) {
                fprintf(stderr, "Error: Output filename too long after adding extension\n");
                return 1;
            }

            strncat(outputfile, ".", sizeof(outputfile) - strlen(outputfile) - 1);
            strncat(outputfile, format, sizeof(outputfile) - strlen(outputfile) - 1);
        }
    }

    // Initialize SSTV encoding module
    if (sstv_init(rate, verbose, timestamp_logging) != 0) {
        fprintf(stderr, "[ERROR] Failed to initialize SSTV module\n");
        error_code = 1;
        goto cleanup;
    }

    // Print configuration summary (with or without timestamps based on mode)
    if (verbose) {
        verbose_print(verbose, timestamp_logging, "--------------------------------------------------------------\n");
        verbose_print(verbose, timestamp_logging, "PiSSTVpp v2.1.0 - SSTV Audio Encoder\n");
        verbose_print(verbose, timestamp_logging, "--------------------------------------------------------------\n");
        verbose_print(verbose, timestamp_logging, "Configuration Summary:\n");
        verbose_print(verbose, timestamp_logging, "  Input image:      %s\n", inputfile);
        verbose_print(verbose, timestamp_logging, "  Output file:      %s\n", outputfile);
        const char *format_display = "WAV";
        if (strcmp(format, "aiff") == 0) format_display = "AIFF";
        else if (strcmp(format, "ogg") == 0 || strcmp(format, "vorbis") == 0) format_display = "OGG Vorbis";
        verbose_print(verbose, timestamp_logging, "  Audio format:     %s at %d Hz\n", format_display, rate);
        verbose_print(verbose, timestamp_logging, "  SSTV protocol:    %s (VIS code %d)\n", protocol, protocol_code);
        verbose_print(verbose, timestamp_logging, "  Image dimensions: 320x256 pixels\n");
        verbose_print(verbose, timestamp_logging, "Mode Details:\n");
        sstv_get_mode_details(protocol_code, verbose, timestamp_logging);
        verbose_print(verbose, timestamp_logging, "--------------------------------------------------------------\n");
    } else {
        printf("--------------------------------------------------------------\n");
        printf("PiSSTVpp v2.1.0 - SSTV Audio Encoder\n");
        printf("--------------------------------------------------------------\n");
        printf("Configuration Summary:\n");
        printf("  Input image:      %s\n", inputfile);
        printf("  Output file:      %s\n", outputfile);
        const char *format_display = "WAV";
        if (strcmp(format, "aiff") == 0) format_display = "AIFF";
        else if (strcmp(format, "ogg") == 0 || strcmp(format, "vorbis") == 0) format_display = "OGG Vorbis";
        printf("  Audio format:     %s at %d Hz\n", format_display, rate);
        printf("  SSTV protocol:    %s (VIS code %d)\n", protocol, protocol_code);
        printf("  Image dimensions: 320x256 pixels\n");
        printf("Mode Details:\n");
        sstv_get_mode_details(protocol_code, 0, 0);
        printf("--------------------------------------------------------------\n");
    }

    // Load image using new image module (auto-detects format)
    verbose_print(verbose, timestamp_logging, "[1/4] Loading image...\n");
    if (image_load_from_file(inputfile, verbose, timestamp_logging, NULL) != 0) {
        fprintf(stderr, "\n[ERROR] Failed to load image '%s'\n", inputfile);
        fprintf(stderr, "   Supported formats (common): PNG, JPEG, GIF, BMP, TIFF, WebP, etc.\n");
        fprintf(stderr, "   Check file exists and is a valid image file\n");
        error_code = 2;
        goto cleanup;
    }

    // ======================================================================
    // IMAGE DIMENSION REQUIREMENTS
    // ======================================================================
    // SSTV modes have fixed pixel dimensions per specification:
    // - Martin/Scottie: 320x256 (4:3.2 aspect ~1.25:1)
    // - Robot: 320x240 (standard 4:3 aspect)
    int required_width = 320, required_height = 256;
    if (protocol_code == 8 || protocol_code == 12) { // Robot 36 or Robot 72
        required_height = 240;  // Robot modes use 4:3 aspect ratio
    }
    
    // ======================================================================
    // INTERMEDIATE IMAGE PATH CONSTRUCTION
    // ======================================================================
    // Build path for intermediate resized image preserving original format.
    // Structure: {output_dir}/{output_base}{original_extension}
    // Example: output.wav in /tmp with input.png → /tmp/output.png
    char intermediate_image[1024];
    {
        // Extract directory component from output path
        char out_dir[1024] = {0};
        const char *last_slash = strrchr(outputfile, '/');
        if (last_slash) {
            int dir_len = last_slash - outputfile;
            strncpy(out_dir, outputfile, dir_len);
            out_dir[dir_len] = '\0';
        } else {
            strcpy(out_dir, ".");
        }
        
        // Get base name without extension
        char out_base[256];
        const char *base_start = last_slash ? last_slash + 1 : outputfile;
        const char *dot = strrchr(base_start, '.');
        if (dot) {
            int base_len = dot - base_start;
            strncpy(out_base, base_start, base_len);
            out_base[base_len] = '\0';
        } else {
            strcpy(out_base, base_start);
        }
        
        // Get original image extension
        const char *orig_ext = image_get_original_extension();
        
        // Build intermediate path: {dir}/{base}{original_ext}
        snprintf(intermediate_image, sizeof(intermediate_image), "%s/%s%s", 
                 out_dir, out_base, orig_ext);
    }
    
    if (image_correct_aspect_and_resize(required_width, required_height, aspect_mode, verbose, timestamp_logging,
                                        keep_intermediate ? intermediate_image : NULL) != 0) {
        error_code = 2;
        goto cleanup;
    }
    
    verbose_print(verbose, timestamp_logging, "[2/4] Encoding image as SSTV audio...\n");

    // Encode the image
    verbose_print(verbose, timestamp_logging, "   --> Processing pixels...\n");
    fflush(stdout);
    
    if (sstv_encode_frame(verbose, timestamp_logging) != 0) {
        fprintf(stderr, "\n[ERROR] Failed to encode SSTV frame\n");
        error_code = 2;
        goto cleanup;
    }

    verbose_print(verbose, timestamp_logging, "   [OK] Image encoded\n");

    if (cw_enable) {
        verbose_print(verbose, timestamp_logging, "   --> Adding CW signature: '%s' (WPM: %d, Tone: %d Hz)\n", cw_callsign[0] ? cw_callsign : "NOCALL", cw_wpm, cw_tone);
        sstv_add_cw_signature(cw_callsign[0] ? cw_callsign : "NOCALL", cw_wpm, cw_tone);
        verbose_print(verbose, timestamp_logging, "   [OK] CW signature added\n");
    }

    verbose_print(verbose, timestamp_logging, "   [OK] adding VIS footer to audio\n");
    verbose_print(verbose, timestamp_logging, "[3/4] Writing audio file...\n");
    verbose_print(verbose, timestamp_logging, "   --> Format: %s\n", format);
    verbose_print(verbose, timestamp_logging, "   --> Sample rate: %d Hz\n", rate);

    // Get samples from SSTV module
    uint32_t sample_count = 0;
    const uint16_t *audio_samples = sstv_get_samples(&sample_count);
    
    // Basic safety checks before writing
    if (sample_count == 0) {
        fprintf(stderr, "\n[ERROR] No audio samples generated\n");
        error_code = 2;
        goto cleanup;
    }
    if ((uint64_t)sample_count >= (uint64_t)MAXSAMPLES) {
        fprintf(stderr, "\n[ERROR] audio sample count at or beyond capacity (%u)\n", sample_count);
        error_code = 2;
        goto cleanup;
    }

    // ======================================================================
    // AUDIO ENCODING: FACTORY PATTERN
    // ======================================================================
    // Create format-specific encoder using factory pattern. This allows
    // adding new formats (FLAC, Opus, etc.) without modifying main logic.
    // Encoders implement common interface: init → encode → finish → destroy
    
    AudioEncoder *encoder = audio_encoder_create(format);
    if (!encoder) {
        // Factory returns NULL if format unsupported (should never happen
        // due to earlier validation, but defensive check)
        fprintf(stderr, "\n[ERROR] Unsupported format: '%s'\n", format);
        error_code = 2;
        goto cleanup;
    }

    // Initialize encoder: opens file, writes headers
    if (audio_encoder_init(encoder, rate, BITS, CHANS, outputfile) != 0) {
        fprintf(stderr, "\n[ERROR] Failed to initialize %s encoder\n", format);
        audio_encoder_destroy(encoder);
        error_code = 2;
        goto cleanup;
    }

    // Write audio samples (encoding happens here for OGG)
    if (audio_encoder_encode(encoder, audio_samples, sample_count) != 0) {
        fprintf(stderr, "\n[ERROR] Failed to encode audio\n");
        audio_encoder_destroy(encoder);
        error_code = 2;
        goto cleanup;
    }

    // Finalize file: update headers with final sizes, flush buffers, close
    if (audio_encoder_finish(encoder) != 0) {
        fprintf(stderr, "\n[ERROR] Failed to finalize encoding\n");
        audio_encoder_destroy(encoder);
        error_code = 2;
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
    verbose_print(verbose, timestamp_logging, "[4/4] File written! Done.\n");
    
    if (verbose) {
        verbose_print(verbose, timestamp_logging, "--------------------------------------------------------------\n");
        verbose_print(verbose, timestamp_logging, "[COMPLETE] ENCODING COMPLETE\n");
        verbose_print(verbose, timestamp_logging, "--------------------------------------------------------------\n");
        verbose_print(verbose, timestamp_logging, "Output file: %s\n", outputfile);
        verbose_print(verbose, timestamp_logging, "Audio samples: %u (%.2f seconds at %d Hz)\n", sample_count, sample_count / (double)rate, rate);
        verbose_print(verbose, timestamp_logging, "Encoding time: %u millisecond%s\n", elapsed_ms, elapsed_ms == 1 ? "" : "s");
    } else {
        printf("--------------------------------------------------------------\n");
        printf("[COMPLETE] ENCODING COMPLETE\n");
        printf("--------------------------------------------------------------\n");
        printf("Output file: %s\n", outputfile);
        printf("Audio samples: %u (%.2f seconds at %d Hz)\n", sample_count, sample_count / (double)rate, rate);
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
    image_free();       // Free libvips image resources
    sstv_cleanup();     // Free SSTV audio buffer
    vips_shutdown();    // Shutdown libvips (releases all resources)
    return error_code;  // Propagate error code to shell (0 = success, >0 = error)
}

// end
