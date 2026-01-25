/**
 * @file pisstvpp2.c
 * @brief PiSSTVpp2 - Raspberry Pi SSTV plus plus 2 image to audio encoder
 *
 * This source implements conversion of still images into SSTV audio
 * suitable for transmission. It targets use with libvips for image
 * processing and supports WAV/AIFF output formats. The program is
 * intended for open-source distribution; please include an appropriate
 * LICENSE file in the repository.
 *
 * Build example (macOS, Homebrew libvips):
 * gcc -O3 -ffast-math -funroll-loops -I/opt/homebrew/include \
 *   -I/opt/homebrew/Cellar/glib/2.86.3/include/glib-2.0 \
 *   -I/opt/homebrew/Cellar/glib/2.86.3/lib/glib-2.0/include \
 *   pisstvpp2.c -o pisstvpp2 \
 *   -L/opt/homebrew/lib \
 *   -L/opt/homebrew/Cellar/glib/2.86.3/lib \
 *   -lvips -lglib-2.0 -lgobject-2.0 -lm
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
#include <math.h>
#include <tgmath.h>
#include <unistd.h>
#include <errno.h>

// ===========================================================================
// TYPE DEFINITIONS
// ===========================================================================

/**
 * @struct ImageBuffer
 * @brief In-memory image pixel data structure
 *
 * Stores the decoded image as contiguous RGB pixel data for fast access.
 * Used by pixel sampling functions (get_pixel_rgb) during encoding.
 */
typedef struct {
    uint8_t *data;          /**< Pixel data (3 bytes per pixel: RGB) */
    int width;              /**< Image width in pixels */
    int height;             /**< Image height in pixels */
    int rowstride;          /**< Bytes between row starts */
} ImageBuffer;

/**
 * @enum AspectMode
 * @brief Aspect ratio correction mode selection
 *
 * Controls how images are resized to match SSTV mode dimensions.
 * - MODE_4_3: Center-crop to aspect ratio, then resize
 * - MODE_FIT: Add black bars (letterbox/pillarbox), then resize
 * - MODE_STRETCH: Non-uniform scaling (stretches image)
 */
enum AspectMode {
    MODE_4_3 = 0,           /**< Crop mode: center-crop for aspect ratio */
    MODE_FIT = 1,           /**< Fit mode: black-bar padding for aspect ratio */
    MODE_STRETCH = 2        /**< Stretch mode: non-uniform direct resize */
};

// ===========================================================================
// CONFIGURATION CONSTANTS
// ===========================================================================

/**
 * @defgroup AudioConfig Audio Configuration Constants
 * Default values for audio generation
 * @{
 */
enum {
    RATE       = 11025,     /**< Default sample rate (Hz) */
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
#define MAXSAMPLES (300 * MAXRATE * 2)      /**< Max audio buffer: 5 min @ max rate x2 */
#define ASPECT_TOLERANCE 0.01               /**< Aspect ratio match tolerance (0.01 = 1%) */
/** @} */

/**
 * @defgroup ErrorHandling Error Handling Macro
 * @brief Centralized VIPS operation error handling
 *
 * Simplifies error checking for libvips operations by automatically handling
 * error clearing, logging, and cleanup.
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
/** @} */

// ===========================================================================
// HELPER FUNCTIONS: ASPECT MODE CONVERSION
// ===========================================================================

/**
 * @brief Convert AspectMode enum to human-readable string
 * @param mode Aspect mode enumeration value
 * @return Pointer to static string ("4:3", "fit", "stretch", or "unknown")
 *
 * Used for user-facing messages and verbose output.
 */
static inline const char* aspect_mode_to_string(enum AspectMode mode) {
    switch (mode) {
        case MODE_4_3: return "4:3";
        case MODE_FIT: return "fit";
        case MODE_STRETCH: return "stretch";
        default: return "unknown";
    }
}

// =========
// globals
// =========

// ===========================================================================
// GLOBAL VARIABLES: AUDIO GENERATION STATE
// ===========================================================================

/**
 * @defgroup AudioState Audio Generation State Variables
 * Global audio buffer and synthesis parameters maintained across function calls.
 * @{
 */

/** @brief Audio sample buffer - holds PCM samples until written to file */
uint16_t   g_audio[MAXSAMPLES];

/** @brief Sample scaling factor (amplitude control) */
uint32_t   g_scale;

/** @brief Current number of audio samples generated */
uint32_t   g_samples;

/** @brief Precomputed constant: 2π / sample_rate (for tone synthesis) */
double     g_twopioverrate;

/** @brief Precomputed constant: microseconds per sample */
double     g_uspersample;

/** @brief Current phase accumulator (for tone synthesis) */
double     g_theta;

/** @brief Frequency adjustment fudge factor for calibration */
double     g_fudge;

/** @} */

// ===========================================================================
// GLOBAL VARIABLES: PROTOCOL AND FILE I/O STATE
// ===========================================================================

/**
 * @defgroup ProtocolState Protocol and I/O State Variables
 * SSTV protocol and image/audio file I/O state.
 * @{
 */

/** @brief SSTV protocol VIS code (44=Martin1, 40=Martin2, 60=Scottie1, etc) */
uint8_t    g_protocol;

/** @brief Audio sampling rate in Hz (typically 11025, 22050, or 48000) */
uint16_t   g_rate;

/** @brief Output file pointer (WAV or AIFF format) */
FILE *     g_outfp;

/** @} */

// ===========================================================================
// GLOBAL VARIABLES: IMAGE PROCESSING STATE
// ===========================================================================

/**
 * @defgroup ImageState Image Processing State Variables
 * Image buffer and VipsImage references for encoding.
 * @{
 */

/** @brief Decoded image object from libvips (maintains reference count) */
VipsImage *g_imgp;

/** @brief Buffered image pixel data in memory for fast pixel access */
ImageBuffer *g_imgbuf;

/** @} */

/* ========
 * Function prototypes (documented)
 */

// ===========================================================================
// FUNCTION PROTOTYPES: INITIALIZATION AND HELP
// ===========================================================================

/**
 * @brief Display usage information and command-line options to the user.
 *
 * Called when user invokes -h flag or provides invalid arguments.
 */
static void show_help(void);

// ===========================================================================
// FUNCTION PROTOTYPES: AUDIO SYNTHESIS
// ===========================================================================

/**
 * @brief Generate and append an audio tone to the global sample buffer.
 * @param tonefreq Frequency in Hz (e.g., 1200, 1500 Hz for SSTV tones)
 * @param tonedur Duration in microseconds
 *
 * Uses global phase accumulator (g_theta) and sample rate (g_rate) to
 * generate continuous sinusoidal waveforms at specified frequency.
 * Respects g_scale amplitude setting to prevent clipping.
 */
static void playtone(uint16_t tonefreq, double tonedur);

/**
 * @brief Generate a tone with envelope shaping (fade-in/fade-out).
 * @param tonefreq Frequency in Hz (0 = silence)
 * @param tonedur Duration in microseconds
 *
 * Applies 20ms Hann window (raised cosine) fade-in and fade-out envelopes
 * to eliminate clicking artifacts at tone boundaries. The Hann window provides
 * a smooth, audibly-transparent envelope that's standard in audio DSP.
 * Designed for CW Morse code signatures where abrupt keying causes pops.
 */
static void playtone_envelope(uint16_t tonefreq, double tonedur);

/**
 * @brief Append CW Morse code signature to audio stream.
 * @param callsign Amateur radio callsign or identifier string
 * @param wpm Words per minute (typical range: 5-20 WPM)
 * @param tone_freq Frequency in Hz for CW tones (typical: 800-1000 Hz)
 *
 * Encodes callsign as Morse code at specified speed. Useful for
 * identifying transmitted SSTV images over-the-air.
 */
static void add_cw_signature(const char *callsign, int wpm, uint16_t tone_freq);

// ===========================================================================
// FUNCTION PROTOTYPES: SSTV PROTOCOL HEADERS AND TRAILERS
// ===========================================================================

/**
 * @brief Insert SSTV VIS (Vertical Interval Signal) header sequence.
 * @param verbose If non-zero, print progress to stdout
 *
 * Generates the characteristic leader tone, VIS code burst, and protocol
 * identification tones that precede the image data. Timing and frequencies
 * are per SSTV standard specifications.
 */
static void addvisheader(void);

/**
 * @brief Append SSTV VIS trailer/terminator tones.
 *
 * Marks the end of an SSTV transmission with standard terminator tones.
 * Some modes require 300ms of 1200 Hz; others use the VIS trailer code.
 */
static void addvistrailer(void);

// ===========================================================================
// FUNCTION PROTOTYPES: COLOR AND PIXEL ENCODING
// ===========================================================================

/**
 * @brief Map 8-bit RGB brightness value to SSTV audio tone frequency.
 * @param colorval Brightness value (0=black, 255=white)
 * @return Frequency in Hz
 *
 * Converts RGB color intensity to the corresponding audio frequency
 * used in RGB-based SSTV modes (Martin, Scottie).
 * Range typically 1200-2300 Hz; higher values = brighter pixels.
 */
static uint16_t toneval_rgb(uint8_t colorval);

/**
 * @brief Map 8-bit YUV/luma value to SSTV audio tone frequency.
 * @param colorval Luma/brightness value (0=black, 255=white)
 * @return Frequency in Hz
 *
 * Converts YUV luma (luminance) to audio frequency for YUV-based modes
 * (Robot 36, etc.). Similar frequency range to RGB but different encoding.
 */
static uint16_t toneval_yuv(uint8_t colorval);

// ===========================================================================
// FUNCTION PROTOTYPES: IMAGE ENCODING ENGINES
// ===========================================================================

/**
 * @brief Encode image as Martin-family SSTV mode (Martin 1 or 2).
 * @param pixeltime Per-pixel transmit timing in microseconds
 * @param verbose If non-zero, print progress per scan line
 *
 * Martin 1 (VIS 44): 320x256 pixels at 1.3 sec/line
 * Martin 2 (VIS 40): 320x256 pixels at 0.33 sec/line
 *
 * Uses RGB color space; samples image left-to-right, top-to-bottom,
 * emitting one tone per pixel.
 */
static void buildaudio_m(double pixeltime, int verbose);

/**
 * @brief Encode image as Scottie-family SSTV mode (Scottie 1, 2, or DX).
 * @param pixeltime Per-pixel transmit timing in microseconds
 * @param verbose If non-zero, print progress per scan line
 *
 * Scottie 1 (VIS 60): 320x256 pixels
 * Scottie 2 (VIS 56): 320x256 pixels
 * Scottie DX (VIS 76): 320x256 pixels (much slower)
 *
 * Uses RGB color space with interlaced R/G/B channel transmission.
 */
static void buildaudio_s(double pixeltime, int verbose);

/**
 * @brief Encode image as Robot 36 SSTV mode (YUV color space).
 * @param verbose If non-zero, print progress per scan line
 *
 * Robot 36 (VIS 8): 320x240 pixels, YUV color encoding, ~38 second transmission
 *
 * Uses YUV color space; encodes each line with specific timing for Y (luma),
 * U (blue-difference), and V (red-difference) channels.
 */
static void buildaudio_r36(int verbose);
static void buildaudio_r72(int verbose);

// ===========================================================================
// FUNCTION PROTOTYPES: IMAGE LOADING AND PROCESSING
// ===========================================================================

/**
 * @brief Load image file using libvips and buffer pixel data.
 * @param filename Path to image file (PNG, JPEG, GIF, BMP, TIFF, WebP, etc)
 * @param verbose If non-zero, print debug messages during loading
 * @return 0 on success, -1 on failure (error messages printed to stderr)
 *
 * Loads image using libvips library, converts to RGB color space,
 * and stores in-memory pixel buffer (g_imgbuf) for fast per-pixel access.
 * Supports any format that libvips can decode.
 */
static int load_image_with_vips(const char *filename, int verbose);

/**
 * @brief Extract RGB pixel values from buffered image.
 * @param img Unused (for API compatibility)
 * @param x X coordinate (0 = leftmost)
 * @param y Y coordinate (0 = topmost)
 * @param r Pointer to receive red channel value (0-255)
 * @param g Pointer to receive green channel value (0-255)
 * @param b Pointer to receive blue channel value (0-255)
 *
 * Fast direct buffer access for encoding functions.
 * Bounds checks: returns (0,0,0) for out-of-range coordinates.
 */
static void get_pixel_rgb(VipsImage *img, int x, int y, uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * @brief Release the in-memory image buffer.
 *
 * Frees pixel data and ImageBuffer structure allocated by load_image_with_vips.
 * Called during cleanup or when replacing with corrected image.
 */
static void free_image_buffer(void);

/**
 * @brief Create in-memory pixel buffer from VipsImage.
 * @param image VipsImage object to buffer
 * @param verbose If non-zero, print buffer info to stdout
 * @return 0 on success, -1 on failure
 *
 * Converts VipsImage into contiguous RGB pixel buffer for fast access.
 * Used after image correction to re-buffer the modified image.
 */
static int buffer_image(VipsImage *image, int verbose);

/**
 * @brief Correct image resolution and aspect ratio for SSTV mode.
 * @param required_width Target width in pixels (typically 320)
 * @param required_height Target height in pixels (256 or 240)
 * @param mode Aspect ratio correction strategy (4:3, fit, or stretch)
 * @param verbose If non-zero, print progress to stdout
 * @return 0 on success, -1 on failure (error_code set by caller)
 *
 * Modifies global g_imgp and g_imgbuf to match SSTV mode requirements.
 * Performs resize/crop/pad operations via libvips before encoding.
 */
static int correct_image_aspect(int required_width, int required_height, enum AspectMode mode, int verbose);

// ===========================================================================
// FUNCTION PROTOTYPES: FILE OUTPUT
// ===========================================================================

/**
 * @brief Write audio buffer as AIFF (Audio Interchange File Format).
 *
 * Writes global audio buffer (g_audio, g_samples) to g_outfp as AIFF file
 * with proper headers. AIFF is compatible with many radio transmission tools.
 */
static void writefile_aiff(void);

/**
 * @brief Write audio buffer as WAV file.
 */
static void writefile_wav(void);

/**
 * @brief Print concise SSTV mode details to stdout.
 * @param protocol VIS protocol code for the selected SSTV mode.
 *
 * The function prints a short summary for each supported mode including
 * mode name, nominal scanline resolution, horizontal sampling, and
 * transmission time (TX Time). This output is intended for the
 * configuration summary shown at program start.
 */
// ================
// Mode Details
// ================

static void print_mode_details(int protocol) {
    switch(protocol) {
        case 44: // Martin 1
            printf("  Mode name:     Martin 1 (M1)\n");
            printf("  Resolution:    256 scan lines, 4288 pixels/line, 8-bit RGB\n");
            printf("  TX Time:       114 seconds total\n");
            break;
        case 40: // Martin 2
            printf("  Mode name:     Martin 2 (M2)\n");
            printf("  Resolution:    256 scan lines, 4288 pixels/line, 8-bit RGB\n");
            printf("  TX Time:       58 seconds total\n");
            break;
        case 60: // Scottie 1
            printf("  Mode name:     Scottie 1 (S1)\n");
            printf("  Resolution:    256 scan lines, 4496 pixels/line, 8-bit RGB\n");
            printf("  TX Time:       110 seconds total\n");
            break;
        case 56: // Scottie 2
            printf("  Mode name:     Scottie 2 (S2)\n");
            printf("  Resolution:    256 scan lines, 4496 pixels/line, 8-bit RGB\n");
            printf("  TX Time:       71 seconds total\n");
            break;
        case 76: // Scottie DX
            printf("  Mode name:     Scottie DX (SDX)\n");
            printf("  Resolution:    256 scan lines, 4496 pixels/line, 8-bit RGB\n");
            printf("  TX Time:       269 seconds total\n");
            break;
        case 8: // Robot 36
            printf("  Mode name:     Robot 36 Color (R36)\n");
            printf("  Resolution:    120 scan lines, 320 pixels/line, YUV format\n");
            printf("  TX Time:       36 seconds total\n");
            break;
        case 12: // Robot 72
            printf("  Mode name:     Robot 72 Color (R72)\n");
            printf("  Resolution:    120 scan lines, 320 pixels/line, YUV format\n");
            printf("  TX Time:       72 seconds total\n");
            break;
    }
}

/**
 * @brief Display program usage and options to stdout.
 *
 * The help text documents required and optional flags and provides
 * several usage examples. Keep this user-facing text concise and
 * up-to-date with available command-line options.
 */
// ================
// help/version
// ================

static void show_help(void) {
    printf("Usage: ./pisstvpp2 -i <input_file> [OPTIONS]\n\n");
    printf("PiSSTVpp v2.1.0 (built Jan 23 2026)\n");
    printf("Convert an image (PNG/JPEG/GIF/BMP) to SSTV audio format.\n\n");
    printf("REQUIRED OPTIONS:\n");
    printf("  -i <file>       Input image file (PNG, JPEG, GIF, or BMP)\n\n");
    printf("OPTIONAL OPTIONS:\n");
    printf("  -a <mode>       Aspect ratio correction: 4:3, fit, or stretch (default: 4:3)\n");
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
    printf("  -f <fmt>        Output format: wav or aiff (default: wav)\n");
    printf("  -r <rate>       Audio sample rate in Hz (default: 22050, range: 8000-48000)\n");
    printf("  -v              Enable verbose output (progress details)\n");
    printf("  -h              Display this help message\n\n");
    printf("CW SIGNATURE OPTIONS (optional):\n");
    printf("  -C <callsign>   Add CW signature with callsign (max 31 characters).\n");
    printf("  -W <wpm>        Set CW signature speed in WPM, range 1-50 (default: 15)\n");
    printf("  -T <freq>       Set CW signature tone frequency in Hz, range 400-2000 (default: 800)\n\n");    
    printf("EXAMPLES:\n");
    printf("  ./pisstvpp2 -i image.jpg -o out.aiff\n");
    printf("  ./pisstvpp2 -i image.jpg -f wav -p s2 -r 11025 -v\n");
    printf("  ./pisstvpp2 -i image.png -o output.wav -p r36\n\n");
    printf("  ./pisstvpp2 -i image.jpg -C N0CALL\n");
    printf("  ./pisstvpp2 -i image.jpg -S 1 -C N0CALL -W 20 -T 700\n\n");
}

/**
 * @brief Main program entrypoint.
 * @param argc Argument count from the command line.
 * @param argv Argument vector from the command line.
 * @return Exit status (0 on success, non-zero on error).
 *
 * Responsibilities:
 *  - Parse command-line arguments
 *  - Initialize libvips
 *  - Load the input image
 *  - Build SSTV audio frames for the requested protocol
 *  - Write WAV/AIFF output
 *  - Print configuration summary and final report
 */
// ================
//   main
// ================

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
    uint16_t rate = 22050;
    int verbose = 0;
    int cw_enable = 0;
    char cw_callsign[16] = "";
    int cw_wpm = 15;
    uint16_t cw_tone = 800;
    int option;
    int w_flag = 0, t_flag = 0;
    enum AspectMode aspect_mode = MODE_4_3;
    // Argument parsing with getopt
    while ((option = getopt(argc, argv, "i:o:p:f:r:vC:W:T:a:h")) != -1) {
        switch (option) {
            case 'a':
                if (strcmp(optarg, "4:3") == 0) {
                    aspect_mode = MODE_4_3;
                } else if (strcmp(optarg, "fit") == 0) {
                    aspect_mode = MODE_FIT;
                } else if (strcmp(optarg, "stretch") == 0) {
                    aspect_mode = MODE_STRETCH;
                } else {
                    fprintf(stderr, "Error: Aspect mode must be '4:3', 'fit', or 'stretch'\n");
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
                if (strcmp(optarg, "wav") != 0 && strcmp(optarg, "aiff") != 0) {
                    fprintf(stderr, "Error: Format must be 'wav' or 'aiff'\n");
                    return 1;
                }
                format = optarg;
                break;
            case 'r': {
                char *endptr = NULL;
                errno = 0;
                long tmp = strtol(optarg, &endptr, 10);
                if (errno != 0 || endptr == optarg || *endptr != '\0') {
                    fprintf(stderr, "Error: Invalid sample rate '%s'\n", optarg);
                    return 1;
                }
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
                if (cw_tone == 0 || cw_tone > 2000 || cw_tone < 400) cw_tone = 800;  // limit tone frequency to reasonable range
                if (cw_tone <= 0 || cw_tone > 2000 || cw_tone < 400) {
                    fprintf(stderr, "Error: CW tone must be between 400 Hz and 2000 Hz.\n");
                    return 1;
                }                
                break;
            case 'h':
                show_help();
                return 0;
            case '?':
            default:
                fprintf(stderr, "Error: Invalid option\n");
                show_help();
                return 1;
        }
    }

    // Require -C if -W or -T are provided
    if ((w_flag || t_flag) && strlen(cw_callsign) == 0) {
        fprintf(stderr, "Error: -C <callsign> is required if -W or -T are provided.\n");
        return 1;
    }

    // Validate required input file
    if (strlen(inputfile) == 0) {
        fprintf(stderr, "Error: Input file (-i) is required\n\n");
        show_help();
        return 1;
    }

    g_rate = rate;

    // Set VIS codes based on protocol
    if (strcmp(protocol, "m1") == 0) {
        g_protocol = 44; // Martin 1
    }
    else if (strcmp(protocol, "m2") == 0) {
        g_protocol = 40; // Martin 2
    }
    else if (strcmp(protocol, "s1") == 0) {
        g_protocol = 60; // Scottie 1
    }
    else if (strcmp(protocol, "s2") == 0) {
        g_protocol = 56; // Scottie 2
    }
    else if (strcmp(protocol, "sdx") == 0) {
        g_protocol = 76; // Scottie DX
    }
    else if (strcmp(protocol, "r36") == 0) {
        g_protocol = 8; // Robot 36
    }
    else if (strcmp(protocol, "r72") == 0) {
        g_protocol = 12; // Robot 72
    }
    else {
        fprintf(stderr, "Error: Unrecognized protocol '%s'\n", protocol);
        show_help();
        return 1;
    }

    // locals
    uint32_t starttime = time(NULL);

    // Determine output filename if not specified
    if (strlen(outputfile) == 0) {
        strncpy(outputfile, inputfile, sizeof(outputfile) - 1);
        outputfile[sizeof(outputfile) - 1] = '\0';

        // Check remaining space before appending extension
        size_t output_len = strlen(outputfile);
        size_t ext_len = strlen(format) + 1; // +1 for the dot

        if (output_len + ext_len >= sizeof(outputfile)) {
            fprintf(stderr, "Error: Output filename too long after adding extension\n");
            return 1;
        }

        strncat(outputfile, ".", sizeof(outputfile) - strlen(outputfile) - 1);
        strncat(outputfile, format, sizeof(outputfile) - strlen(outputfile) - 1);
    }

    // assign values to globals
    double temp1;
    double temp2;
    double temp3;
    temp1 = (double)(1 << (BITS - 1));
    temp2 = VOLPCT / 100.0;
    temp3 = temp1 * temp2;
    g_scale = (uint32_t)temp3;

    g_twopioverrate = 2.0 * M_PI / g_rate;
    g_uspersample = 1000000.0 / (double)g_rate;

    g_theta = 0.0;
    g_samples = 0.0;
    g_fudge = 0.0;

    printf("--------------------------------------------------------------\n");
    printf("PiSSTVpp v2.1.0 - SSTV Audio Encoder\n");
    printf("--------------------------------------------------------------\n");
    printf("Configuration Summary:\n");
    printf("  Input image:      %s\n", inputfile);
    printf("  Output file:      %s\n", outputfile);
    printf("  Audio format:     %s at %d Hz\n", strcmp(format, "aiff") == 0 ? "AIFF" : "WAV", g_rate);
    printf("  SSTV protocol:    %s (VIS code %d)\n", protocol, g_protocol);
    printf("  Image dimensions: 320x256 pixels\n\n");
    printf("Mode Details:\n");
    print_mode_details(g_protocol);
    printf("--------------------------------------------------------------\n");

    // --- Pre-encoding buffer size check ---
    // Compute required samples for selected mode/sample rate
    uint64_t required_samples = 0;
    if (g_protocol == 76) { // SDX
        // SDX: 256 lines, 320 pixels/line, pixel time 1080us
        // Per line: sync + separator + green + separator + blue + sync + porch + red
        double SYNC_PULSE_US   = 9000.0;
        double SEPARATOR_US    = 1500.0;
        double SYNC_PORCH_US   = 1500.0;
        double PIXEL_US        = 1080.0;
        int LINES = 256;
        int PIXELS = 320;
        double line_us = SYNC_PULSE_US + SEPARATOR_US + PIXELS * PIXEL_US + SEPARATOR_US + PIXELS * PIXEL_US + SYNC_PULSE_US + SYNC_PORCH_US + PIXELS * PIXEL_US;
        double total_us = LINES * line_us;
        // Add header/trailer estimate (1 sec margin)
        double total_sec = (total_us / 1e6) + 1.0;
        required_samples = (uint64_t)(total_sec * g_rate);
    }
    // Add similar checks for other modes if desired

    if (required_samples > 0 && required_samples > MAXSAMPLES) {
        fprintf(stderr, "\n[ERROR] Buffer too small for SDX mode: required %" PRIu64 " samples, MAXSAMPLES is %d.\n", required_samples, MAXSAMPLES);
        fprintf(stderr, "Increase MAXSAMPLES to at least %" PRIu64 " and rebuild.\n\n", required_samples);
        return 3;
    }

    // Load image using libvips (auto-detects format)
    if (verbose) {
        printf("[1/4] Loading image...\n");
    }
    if (load_image_with_vips(inputfile, verbose) != 0) {
        fprintf(stderr, "\n[ERROR] Failed to load image '%s'\n", inputfile);
        fprintf(stderr, "   Supported formats (common): PNG, JPEG, GIF, BMP, TIFF, WebP, etc.\n");
        fprintf(stderr, "   Check file exists and is a valid image file\n");
        error_code = 2;
        goto cleanup;
    }

    // --- Aspect ratio correction and resizing ---
    int required_width = 320, required_height = 256;
    if (g_protocol == 8 || g_protocol == 12) { // Robot 36 or Robot 72
        required_height = 240;
    }
    
    if (correct_image_aspect(required_width, required_height, aspect_mode, verbose) != 0) {
        error_code = 2;
        goto cleanup;
    }
    
    if (verbose) {
        printf("   [OK] Image loaded successfully\n\n");
        printf("[2/4] Opening output file...\n");
    }
    g_outfp = fopen(outputfile, "wb");
    if (!g_outfp) {
        fprintf(stderr, "\n[ERROR] Cannot create output file '%s'\n", outputfile);
        fprintf(stderr, "   Check directory exists and is writable\n");
        error_code = 2;
        goto cleanup;
    }
    if (verbose) {
        printf("   [OK] Output file ready\n\n");
        printf("[3/4] Encoding image as SSTV audio...\n");
    }

    // go!
    addvisheader();
    if (verbose) {
        printf("   [OK] VIS header added\n");
        printf("   --> Processing pixels...\n");
    }
    fflush(stdout);
    switch (g_protocol) {
        case 44: // Martin 1
            buildaudio_m(457.6, verbose);
            break;
        case 40: // Martin 2
            buildaudio_m(228.8, verbose);
            break;
        case 60: // Scottie 1
            buildaudio_s(432.0, verbose);
            break;
        case 56: // Scottie 2
            buildaudio_s(275.2, verbose);
            break;
        case 76: // Scottie DX
            buildaudio_s(1080.0, verbose);
            break;
        case 8: // Robot 36
            buildaudio_r36(verbose);
            break;
        case 12: // Robot 72
            buildaudio_r72(verbose);
            break;
        default:
            fprintf(stderr, "\n\n[ERROR] Unknown protocol code %d\n", g_protocol);
            return 4;
    }

    if (verbose) { printf("   [OK] Image encoded\n");
}

    addvistrailer();

    if (cw_enable) {
        // Add 2 seconds of silence between VIS trailer and CW signature
        playtone(0, 2000000.0);  // 2,000,000 microseconds = 2 seconds
        
        if (verbose) printf("   --> Adding CW signature: '%s' (WPM: %d, Tone: %d Hz)\n", cw_callsign[0] ? cw_callsign : "NOCALL", cw_wpm, cw_tone);
        add_cw_signature(cw_callsign[0] ? cw_callsign : "NOCALL", cw_wpm, cw_tone);
        if (verbose) printf("   [OK] CW signature added\n");
    }

    if (verbose) {
        printf("   [OK] VIS trailer added\n\n");
        printf("[4/4] Writing WAV file...\n");
        printf("   --> Format: %s\n", strcmp(format, "aiff") == 0 ? "AIFF" : "WAV");
        printf("   --> Sample rate: %d Hz\n", g_rate);
    }

    // Basic safety checks before writing
    if (g_samples == 0) {
        fprintf(stderr, "\n[ERROR] No audio samples generated\n");
        error_code = 2;
        goto cleanup;
    }
    if ((uint64_t)g_samples >= (uint64_t)MAXSAMPLES) {
        fprintf(stderr, "\n[ERROR] audio sample count at or beyond capacity (%u)\n", g_samples);
        error_code = 2;
        goto cleanup;
    }

    // Write output file based on format
    if (strcmp(format, "aiff") == 0) {
        writefile_aiff();
    } else {
        writefile_wav();
    }

    // ensure data made it to disk
    if (fflush(g_outfp) != 0 || ferror(g_outfp)) {
        fprintf(stderr, "\n[ERROR] Failed writing output file '%s': %s\n", outputfile, strerror(errno));
        error_code = 2;
        goto cleanup;
    }

    // cleanup
    free_image_buffer();
    if (g_imgp) {
        g_object_unref(g_imgp);
    }
    fclose(g_outfp);
    vips_shutdown();

    // cleanup and report
    uint32_t endtime = time(NULL);
    uint32_t elapsed = endtime - starttime;
    if (verbose) { printf("   [OK] File written\n");
}
    printf("--------------------------------------------------------------\n");
    printf("[COMPLETE] ENCODING COMPLETE\n");
    printf("--------------------------------------------------------------\n");
    printf("Output file: %s\n", outputfile);
    printf("Audio samples: %u", (uint32_t)g_samples);
    printf(" (%.2f seconds at %d Hz)\n", g_samples / (double)g_rate, g_rate);
    printf("Encoding time: %u second%s\n\n", elapsed, elapsed == 1 ? "" : "s");
    
    return error_code;

cleanup:
    // Centralized error cleanup
    free_image_buffer();
    if (g_imgp) {
        g_object_unref(g_imgp);
    }
    if (g_outfp) {
        fclose(g_outfp);
    }
    vips_shutdown();
    return error_code;
}


// =====================
//  subs 
// =====================    

// load_image_with_vips -- Load an image file using libvips, convert to RGB if needed.
//                         Stores the image data in a buffer for efficient pixel access.
//                         Supports: PNG, JPEG, GIF, BMP, TIFF, WebP, and many more.
/**
 * @brief Load an image using libvips into an internal buffer.
 * @param filename Path to the input image file.
 * @param verbose If non-zero, emit progress messages to stdout.
 * @return 0 on success, negative on failure.
 *
 * This function uses libvips to open the image, convert it to RGB
 * if required, and copy pixel data into a contiguous in-memory buffer
 * used by the SSTV encoder routines.
 */
static int load_image_with_vips(const char *filename, int verbose) {
    VipsImage *image = NULL;
    VipsImage *rgb_image = NULL;
    
    // Load the image - vips auto-detects format
    image = vips_image_new_from_file(filename, NULL);
    if (!image) {
        fprintf(stderr, "   [ERROR] Failed to load '%s'\n", filename);
        fprintf(stderr, "   Details: %s\n", vips_error_buffer());
        vips_error_clear();
        return -1;
    }
    
    if (verbose) { printf("   --> Detected: %dx%d, %d-band image\n", image->Xsize, image->Ysize, image->Bands);
}
    
    // Ensure image is in RGB format (3 bands, explicit sRGB interpretation)
    // Always convert to ensure consistent RGB byte ordering
    if (vips_colourspace(image, &rgb_image, VIPS_INTERPRETATION_sRGB, NULL)) {
        fprintf(stderr, "   [ERROR] Colorspace conversion failed\n");
        fprintf(stderr, "   Details: %s\n", vips_error_buffer());
        vips_error_clear();
        g_object_unref(image);
        return -1;
    }
    g_object_unref(image);
    image = rgb_image;
    
    // Allocate buffer structure
    g_imgbuf = (ImageBuffer *)malloc(sizeof(ImageBuffer));
    if (!g_imgbuf) {
        fprintf(stderr, "   [ERROR] Memory allocation failed\n");
        fprintf(stderr, "   Details: Unable to allocate image buffer structure\n");
        g_object_unref(image);
        return -1;
    }
    
    // Get image metadata
    g_imgbuf->width = image->Xsize;
    g_imgbuf->height = image->Ysize;
    g_imgbuf->rowstride = VIPS_IMAGE_SIZEOF_LINE(image);
    
    // Allocate data buffer
    int data_size = g_imgbuf->height * g_imgbuf->rowstride;
    g_imgbuf->data = (uint8_t *)malloc(data_size);
    if (!g_imgbuf->data) {
        fprintf(stderr, "   [ERROR] Memory allocation failed\n");
        fprintf(stderr, "   Details: Unable to allocate %d bytes for image data\n", data_size);
        fprintf(stderr, "   Hint: Image is %dx%d (%d bytes/row)\n", 
                g_imgbuf->width, g_imgbuf->height, g_imgbuf->rowstride);
        free(g_imgbuf);
        g_imgbuf = NULL;
        g_object_unref(image);
        return -1;
    }
    
    // Copy image data to buffer using region
    if (verbose) { printf("   --> Buffering pixel data...\n");
}
    VipsRegion *region = vips_region_new(image);
    if (!region) {
        fprintf(stderr, "   [ERROR] Image buffering failed\n");
        fprintf(stderr, "   Details: Unable to create memory region\n");
        free(g_imgbuf->data);
        free(g_imgbuf);
        g_imgbuf = NULL;
        g_object_unref(image);
        return -1;
    }
    
    VipsRect rect = { 0, 0, image->Xsize, image->Ysize };
    if (vips_region_prepare(region, &rect)) {
        fprintf(stderr, "   [ERROR] Image buffering failed\n");
        fprintf(stderr, "   Details: Unable to prepare image region for access\n");
        g_object_unref(region);
        free(g_imgbuf->data);
        free(g_imgbuf);
        g_imgbuf = NULL;
        g_object_unref(image);
        return -1;
    }
    
    // Copy data from region to buffer
    const uint8_t *src = VIPS_REGION_ADDR(region, 0, 0);
    if (src) {
        memcpy(g_imgbuf->data, src, data_size);
    } else {
        fprintf(stderr, "   [ERROR] Image buffering failed\n");
        fprintf(stderr, "   Details: Unable to access pixel data from image\n");
        g_object_unref(region);
        free(g_imgbuf->data);
        free(g_imgbuf);
        g_imgbuf = NULL;
        g_object_unref(image);
        return -1;
    }
    
    g_object_unref(region);
    
    if (verbose) { printf("   [OK] Image buffer ready: %dx%d RGB (%d bytes)\n", 
           g_imgbuf->width, g_imgbuf->height, data_size);
}
    
    // Store in global for compatibility
    g_imgp = image;
    return 0;
}

// Buffer an existing VipsImage into g_imgbuf without file I/O.
static int buffer_image(VipsImage *image, int verbose) {
    if (!image) {
        fprintf(stderr, "   [ERROR] No image to buffer\n");
        return -1;
    }

    free_image_buffer();

    g_imgbuf = (ImageBuffer *)malloc(sizeof(ImageBuffer));
    if (!g_imgbuf) {
        fprintf(stderr, "   [ERROR] Memory allocation failed\n");
        fprintf(stderr, "   Details: Unable to allocate image buffer structure\n");
        return -1;
    }

    g_imgbuf->width = image->Xsize;
    g_imgbuf->height = image->Ysize;
    g_imgbuf->rowstride = VIPS_IMAGE_SIZEOF_LINE(image);

    int data_size = g_imgbuf->height * g_imgbuf->rowstride;
    g_imgbuf->data = (uint8_t *)malloc(data_size);
    if (!g_imgbuf->data) {
        fprintf(stderr, "   [ERROR] Memory allocation failed\n");
        fprintf(stderr, "   Details: Unable to allocate %d bytes for image data\n", data_size);
        free(g_imgbuf);
        g_imgbuf = NULL;
        return -1;
    }

    if (verbose) { printf("   --> Buffering pixel data...\n"); }
    VipsRegion *region = vips_region_new(image);
    if (!region) {
        fprintf(stderr, "   [ERROR] Image buffering failed\n");
        fprintf(stderr, "   Details: Unable to create memory region\n");
        free(g_imgbuf->data);
        free(g_imgbuf);
        g_imgbuf = NULL;
        return -1;
    }

    VipsRect rect = (VipsRect){ 0, 0, image->Xsize, image->Ysize };
    if (vips_region_prepare(region, &rect)) {
        fprintf(stderr, "   [ERROR] Image buffering failed\n");
        fprintf(stderr, "   Details: Unable to prepare image region for access\n");
        g_object_unref(region);
        free(g_imgbuf->data);
        free(g_imgbuf);
        g_imgbuf = NULL;
        return -1;
    }

    const uint8_t *src = VIPS_REGION_ADDR(region, 0, 0);
    if (src) {
        memcpy(g_imgbuf->data, src, data_size);
    } else {
        fprintf(stderr, "   [ERROR] Image buffering failed\n");
        fprintf(stderr, "   Details: Unable to access pixel data from image\n");
        g_object_unref(region);
        free(g_imgbuf->data);
        free(g_imgbuf);
        g_imgbuf = NULL;
        return -1;
    }

    g_object_unref(region);
    if (verbose) { printf("   [OK] Image buffer ready: %dx%d RGB (%d bytes)\n", g_imgbuf->width, g_imgbuf->height, data_size); }
    return 0;
}

// correct_image_aspect -- Correct image resolution and aspect ratio using selected mode
//                         Modifies global g_imgp and g_imgbuf with corrected image
//                         Returns 0 on success, -1 on failure (error_code set by caller)
static int correct_image_aspect(int required_width, int required_height, enum AspectMode mode, int verbose) {
    int img_width = g_imgbuf ? g_imgbuf->width : 0;
    int img_height = g_imgbuf ? g_imgbuf->height : 0;
    double img_aspect = img_width > 0 && img_height > 0 ? (double)img_width / img_height : 0.0;
    double required_aspect = (double)required_width / required_height;

    // Check if correction is needed
    int needs_size_correction = (img_width != required_width || img_height != required_height);
    int needs_aspect_correction = (fabs(img_aspect - required_aspect) > ASPECT_TOLERANCE);
    
    if (needs_size_correction) {
        printf("[INFO] Input image is %dx%d, but SSTV mode requires %dx%d.\n", img_width, img_height, required_width, required_height);
    }
    if (needs_aspect_correction) {
        printf("[INFO] Input image aspect ratio is %.3f, but SSTV mode requires %.3f.\n", img_aspect, required_aspect);
    }
    
    // Early exit: skip correction if image is already correct
    if (!needs_size_correction && !needs_aspect_correction) {
        if (verbose) printf("[INFO] Image is already correct size and aspect ratio - no correction needed.\n");
        return 0;
    }
    
    if (verbose) printf("[INFO] Correcting image to %dx%d (%s mode)\n", required_width, required_height, aspect_mode_to_string(mode));
    VipsImage *corrected = NULL;
    
    // Pre-calculate correction dimensions
    int corr_w = img_width, corr_h = img_height;
    double in_aspect = img_aspect, out_aspect = required_aspect;
    if (in_aspect > out_aspect) {
        corr_h = (int)(img_width / out_aspect + 0.5);
    } else {
        corr_w = (int)(img_height * out_aspect + 0.5);
    }
    int left = (img_width - corr_w) / 2;
    int top = (img_height - corr_h) / 2;
    
    if (mode == MODE_4_3) {
        // Crop mode: center-crop to aspect, then resize
        VipsImage *cropped = NULL;
        if (vips_crop(g_imgp, &cropped, left, top, corr_w, corr_h, NULL)) {
            fprintf(stderr, "[ERROR] vips_crop failed\n");
            vips_error_clear();
            return -1;
        }
        if (vips_resize(cropped, &corrected, (double)required_width / corr_w, "vscale", (double)required_height / corr_h, NULL)) {
            fprintf(stderr, "[ERROR] vips_resize failed\n");
            vips_error_clear();
            g_object_unref(cropped);
            return -1;
        }
        g_object_unref(cropped);
    } else if (mode == MODE_FIT) {
        // Fit mode: pad with black bars to aspect, then resize
        VipsImage *padded = NULL;
        if (vips_embed(g_imgp, &padded, left, top, corr_w, corr_h, "extend", VIPS_EXTEND_BLACK, NULL)) {
            fprintf(stderr, "[ERROR] vips_embed failed\n");
            vips_error_clear();
            return -1;
        }
        if (vips_resize(padded, &corrected, (double)required_width / corr_w, "vscale", (double)required_height / corr_h, NULL)) {
            fprintf(stderr, "[ERROR] vips_resize failed\n");
            vips_error_clear();
            g_object_unref(padded);
            return -1;
        }
        g_object_unref(padded);
    } else if (mode == MODE_STRETCH) {
        if (vips_resize(g_imgp, &corrected, (double)required_width / img_width, "vscale", (double)required_height / img_height, NULL)) {
            fprintf(stderr, "[ERROR] vips_resize failed\n");
            vips_error_clear();
            return -1;
        }
    }
    
    free_image_buffer();
    if (g_imgp) g_object_unref(g_imgp);
    g_imgp = corrected;
    if (buffer_image(g_imgp, verbose) != 0) {
        fprintf(stderr, "[ERROR] Failed to buffer corrected image.\n");
        return -1;
    }
    
    img_width = g_imgbuf ? g_imgbuf->width : 0;
    img_height = g_imgbuf ? g_imgbuf->height : 0;
    if (img_width != required_width || img_height != required_height) {
        fprintf(stderr, "[ERROR] Image correction failed: got %dx%d, expected %dx%d.\n", img_width, img_height, required_width, required_height);
        return -1;
    }
    
    if (verbose) printf("[OK] Image correction complete.\n");
    return 0;
}

// get_pixel_rgb -- Extract RGB values from buffered image data at (x, y)
//                  Direct buffer access - very fast and reliable
//                  Returns values as separate uint8_t pointers
/**
 * @brief Read RGB pixel values from the loaded VipsImage buffer.
 * @param img Unused in current implementation (keeps signature compatible).
 * @param x X coordinate (0..width-1).
 * @param y Y coordinate (0..height-1).
 * @param r Pointer to receive red channel (0-255).
 * @param g Pointer to receive green channel (0-255).
 * @param b Pointer to receive blue channel (0-255).
 *
 * The current implementation reads from the global image buffer prepared
 * by load_image_with_vips(). This helper abstracts pixel access for the
 * various encoder routines.
 */
static void get_pixel_rgb(VipsImage *img __attribute__((unused)), int x, int y, uint8_t *r, uint8_t *g, uint8_t *b) {
    const uint8_t *pixel_addr;
    int offset;
    
    // Bounds check
    if (!g_imgbuf || x < 0 || x >= g_imgbuf->width || y < 0 || y >= g_imgbuf->height) {
        *r = *g = *b = 0;
        return;
    }
    
    // Direct buffer access: y * rowstride + x * 3 (RGB format)
    offset = (y * g_imgbuf->rowstride) + (x * 3);
    pixel_addr = &g_imgbuf->data[offset];
    
    // Extract RGB values
    *r = pixel_addr[0];
    *g = pixel_addr[1];
    *b = pixel_addr[2];
}


// free_image_buffer -- Release buffered image data
/**
 * @brief Free the internal image buffer allocated by load_image_with_vips.
 */
static void free_image_buffer(void) {
    if (g_imgbuf) {
        if (g_imgbuf->data) {
            free(g_imgbuf->data);
        }
        free(g_imgbuf);
        g_imgbuf = NULL;
    }
}

/**
 * @brief Generate a CW (Morse code) signature at the end of the SSTV transmission.
 *
 * This function generates a Morse code message as an audio tone, defaulting to 800 Hz,
 * and the message 'SSTV de (callsign)'.
 *
 * @param callsign The callsign to include in the message (e.g., "N0CALL").
 * @param wpm Words per minute for Morse timing (default: 15 if 0).
 * @param tone_freq Frequency in Hz for the CW tone (default: 800 if 0).
 */
static void add_cw_signature(const char *callsign, int wpm, uint16_t tone_freq) {
    // Morse code table (A-Z, 0-9, space, and a few symbols)
    static const struct { char c; const char *morse; } morse_table[] = {
        {'A', ".-"},   {'B', "-..."}, {'C', "-.-."}, {'D', "-.."},  {'E', "."},
        {'F', "..-."}, {'G', "--."},  {'H', "...."}, {'I', ".."},   {'J', ".---"},
        {'K', "-.-"},  {'L', ".-.."}, {'M', "--"},   {'N', "-."},   {'O', "---"},
        {'P', ".--."}, {'Q', "--.-"}, {'R', ".-."},  {'S', "..."},  {'T', "-"},
        {'U', "..-"},  {'V', "...-"}, {'W', ".--"},  {'X', "-..-"}, {'Y', "-.--"},
        {'Z', "--.."},
        {'0', "-----"}, {'1', ".----"}, {'2', "..---"}, {'3', "...--"}, {'4', "....-"},
        {'5', "....."}, {'6', "-...."}, {'7', "--..."}, {'8', "---.."}, {'9', "----."},
        {' ', " "}, {'/', "-..-."}, {'?', "..--.."}, {'=', "-...-"},
    };
    const int morse_table_len = sizeof(morse_table) / sizeof(morse_table[0]);

    // Morse timing (in microseconds)
    if (wpm <= 0) wpm = 15; // default WPM
    if (tone_freq == 0) tone_freq = 800; // default tone
    double dot_us = 1200000.0 / wpm; // standard: dot = 1.2s / wpm
    double dash_us = 3 * dot_us;
    double intra_us = dot_us; // between dots/dashes in a letter
    double inter_us = 3 * dot_us; // between letters
    double word_us = 7 * dot_us; // between words

    // Compose message
    char msg[128];
    snprintf(msg, sizeof(msg), "SSTV de %s", callsign ? callsign : "NOCALL");

    // Encode and play
    for (const char *p = msg; *p; ++p) {
        char ch = *p;
        if (ch >= 'a' && ch <= 'z') ch -= 32; // to uppercase
        const char *morse = NULL;
        for (int i = 0; i < morse_table_len; ++i) {
            if (morse_table[i].c == ch) {
                morse = morse_table[i].morse;
                break;
            }
        }
        if (!morse) continue; // skip unknown chars
        if (ch == ' ') {
            playtone(0, word_us); // word gap (silence)
            continue;
        }
        for (const char *m = morse; *m; ++m) {
            if (*m == '.') {
                playtone_envelope(tone_freq, dot_us);
            } else if (*m == '-') {
                playtone_envelope(tone_freq, dash_us);
            }
            // Intra-character gap (silence), except after last element
            if (*(m + 1)) playtone(0, intra_us);
        }
        // Inter-character gap (silence)
        playtone(0, inter_us);
    }
}

// playtone -- Add waveform info to audio data. New waveform data is 
//             added in a phase-continuous manner according to the 
//             audio frequency and duration provided. Note that the
//             audio is still in a purely hypothetical state - the
//             format of the output file is not determined until 
//             the file is written, at the end of the process.
//             Also, yes, a nod to Tom Hanks.

/**
 * @brief Generate a tone with optional envelope shaping (for CW signatures).
 * @param tonefreq Frequency in Hz of the generated tone (0 = silence).
 * @param tonedur Duration in microseconds for the tone.
 * @param envelope If non-zero, apply 5ms fade-in/fade-out ramps.
 *
 * The tone is synthesized into the global `g_audio` buffer using the
 * configured sample rate `g_rate` and global scaling factors. Phase is
 * continuous across calls to maintain seamless waveforms.
 * 
 * With envelope=1: Applies linear fade-in over first 5ms and fade-out
 * over last 5ms to eliminate clicking artifacts at tone boundaries.
 * functions.
 */
static void playtone(uint16_t tonefreq, double tonedur) {
    uint16_t tonesamples, voltage, i ;
    double   deltatheta ;
     
    tonedur += g_fudge ;
    tonesamples = ( tonedur / g_uspersample ) + 0.5 ;
    deltatheta = g_twopioverrate * tonefreq ;
    
    for ( i=1 ; i<=tonesamples ; i++ ) {
        g_samples++ ;
        
        if ( tonefreq == 0 ) { g_audio[ g_samples ] = 32768 ; }
        else {
            voltage = 0 + (int)( sin( g_theta ) * g_scale ) ;
            g_audio[g_samples] = voltage ;
            g_theta += deltatheta ;
        }
    } // end for i        
    
    g_fudge = tonedur - ( tonesamples * g_uspersample ) ;
}  // end playtone

/**
 * @brief Generate CW tone with adaptive, soft envelope (for add_cw_signature()).
 *
 * Scope:
 * - CW only. Not used for SSTV image tones. SSTV uses `playtone()`.
 * - Called by `add_cw_signature()` for dots/dashes and CW gaps.
 *
 * Inputs:
 * - tonefreq: CW tone frequency in Hz (0 = silence)
 * - tonedur:  Element duration in microseconds (dot/dash/gap length)
 *
 * Behavior:
 * - Adaptive ramp length: envelope_time_us = min(max(0.35 * tonedur, 5ms), 50ms)
 *   • Short elements (dots) get shorter ramps; long ones (dashes) get longer ramps.
 * - Envelope shape: smootherstep easing for very soft edges:
 *   • Fade-in:  S(t) = 6t^5 − 15t^4 + 10t^3,  t ∈ [0,1]
 *   • Fade-out: 1 − S(t)
 *   • Zero slope at start/end eliminates edge clicks.
 * - Amplitude: scaled to 65% of `g_scale` for natural loudness.
 * - DC bias: samples centered around 32768 (unsigned 16-bit PCM midpoint).
 * - Phase continuity: uses global `g_theta` so tones join seamlessly.
 *
 * Edge cases and safeguards:
 * - tonefreq == 0: emit silence via `playtone()` (no envelope).
 * - Too-short tones: if tonedur < 2×envelope, emit full-amplitude tone (no ramps).
 * - Buffer protection: checks/truncates to avoid `g_audio` overflow.
 * - Timing fidelity: honors `g_fudge` to preserve aggregate duration.
 */
static void playtone_envelope(uint16_t tonefreq, double tonedur) {
    uint16_t tonesamples, voltage, i ;
    double   deltatheta ;
    uint32_t envelope_samples;
    double envelope_time_us;
    int32_t cw_scale; /* CW amplitude: 10% of SSTV for clean sound */

    tonedur += g_fudge;
    tonesamples = ( tonedur / g_uspersample ) + 0.5 ;
    if (tonesamples == 0) {
        g_fudge = tonedur;
        return;
    }

    /* For silence, use normal playtone */
    if (tonefreq == 0) {
        playtone(tonefreq, tonedur - g_fudge);
        return;
    }
    
    /* CW amplitude: 8% to minimize harmonic distortion */
    cw_scale = (int32_t)(g_scale * 0.08);
    
    /* Very gentle 5ms fade envelope - smooth the envelope edges */
    envelope_time_us = 5000.0;
    envelope_samples = (uint32_t)(envelope_time_us / g_uspersample);
    deltatheta = g_twopioverrate * tonefreq;
    
    /* If tone too short for any envelope, use full amplitude throughout */
    if (tonesamples < 2 * envelope_samples) {
        /* Generate tone without ramps */
        for (i = 1; i <= tonesamples; i++) {
            voltage = 0 + (int)(sin(g_theta) * cw_scale);
            g_audio[g_samples] = voltage;
            g_samples++;
            g_theta += deltatheta;
        }
        g_fudge = tonedur - ((double)tonesamples * g_uspersample);
        return;
    }

    /* Fade-in: smootherstep easing (minimizes harmonics) */
    for (i = 0; i < envelope_samples && i < tonesamples; i++) {
        double t = (double)i / (double)envelope_samples;
        /* smootherstep: 6t^5 - 15t^4 + 10t^3 (zero derivative at endpoints) */
        double envelope = ((6.0*t - 15.0)*t + 10.0) * t * t * t;
        int32_t sig = (int32_t)(sin(g_theta) * cw_scale * envelope);
        int32_t bounded = 32768 + sig;
        if (bounded < 0) bounded = 0;
        if (bounded > 65535) bounded = 65535;
        voltage = (uint16_t)bounded;
        g_audio[g_samples] = voltage;
        g_samples++;
        g_theta += deltatheta;
    }

    /* Full amplitude middle section */
    for (; i < tonesamples - envelope_samples; i++) {
        int32_t sig = (int32_t)(sin(g_theta) * cw_scale);
        int32_t bounded = 32768 + sig;
        if (bounded < 0) bounded = 0;
        if (bounded > 65535) bounded = 65535;
        voltage = (uint16_t)bounded;
        g_audio[g_samples] = voltage;
        g_samples++;
        g_theta += deltatheta;
    }

    /* Fade-out: smootherstep easing mirrored */
    for (uint32_t j = 0; i < tonesamples; i++, j++) {
        double t = (double)j / (double)envelope_samples;
        /* smootherstep mirrored: 1 - S(t) */
        double s = ((6.0*t - 15.0)*t + 10.0) * t * t * t;
        double envelope = 1.0 - s;
        int32_t sig = (int32_t)(sin(g_theta) * cw_scale * envelope);
        int32_t bounded = 32768 + sig;
        if (bounded < 0) bounded = 0;
        if (bounded > 65535) bounded = 65535;
        voltage = (uint16_t)bounded;
        g_audio[g_samples] = voltage;
        g_samples++;
        g_theta += deltatheta;
    }

    /* Explicit silence sample for clean gap */
    g_audio[g_samples++] = 0;

    g_fudge = tonedur - ((double)tonesamples * g_uspersample);
}

// Map 8-bit color value to frequency per SSTV spec: 1500 + (color * 3.1372549)
static uint16_t toneval_rgb(uint8_t colorval) {
    return (uint16_t)(1500.0 + (colorval * 3.1372549));
}

static uint16_t toneval_yuv ( uint8_t colorval ) {
    return ( (float)colorval * 3.1372549 ) + 1500.0 ;
}    

// addvisheader -- Add the specific audio tones that make up the 
//                 Martin 1 VIS header to the audio data. Basically,
//                 this just means lots of calls to playtone(). 

/**
 * @brief Emit the SSTV VIS header into the audio stream.
 * @param verbose If non-zero, print a single-line progress message.
 *
 * The VIS header consists of leader tones, a short break and the VIS
 * bitstream encoded as short tones. Receivers use the VIS information
 * to determine which SSTV mode follows the header.
 */
void addvisheader(void) {
    printf( "Adding VIS header to audio data.\n" ) ;
    
    // bit of silence
    playtone(    0 , 500000 ) ;   

    // attention tones
    playtone( 1900 , 100000 ) ; // you forgot this one
    playtone( 1500 , 100000 ) ;
    playtone( 1900 , 100000 ) ;
    playtone( 1500 , 100000 ) ;
    playtone( 2300 , 100000 ) ;
    playtone( 1500 , 100000 ) ;
    playtone( 2300 , 100000 ) ;
    playtone( 1500 , 100000 ) ;
                    
    // VIS lead, break, mid, start
    playtone( 1900 , 300000 ) ;
    playtone( 1200 ,  10000 ) ;
    playtone( 1900 , 300000 ) ;
    playtone( 1200 ,  30000 ) ;

    int i;
    int parity = 0;

    //write VIS data tones
    for(i = 1; i <= 64; i = i<<1) {
        if(i & g_protocol) { //digit is a 1
            playtone(1100, 30000);
            parity = !parity;
        }
        else {
            playtone(1300, 30000); //digit is a 0
        }
    }

    //parity bit
    if(parity) //odd parity
        playtone(1100, 30000);
    else       //even parity
        playtone(1300, 30000);

    // VIS stop
    playtone( 1200 ,  30000 ) ; 
    printf( "Done adding VIS header to audio data.\n" ) ;
        
} // end addvisheader

/**
 * @brief Build audio samples for Martin (M1/M2) SSTV modes.
 * @param pixeltime Microseconds per pixel sample for the selected mode.
 * @param verbose If non-zero, emit per-row progress messages.
 *
 * Martin modes transmit RGB channels line-by-line with horizontal
 * synchronization and short porches. This routine reads rows from the
 * global image buffer and synthesizes corresponding tone sequences.
 */
/**
 * @brief Build audio samples for Martin (M1/M2) SSTV modes with explicit timing.
 * @param pixeltime Microseconds per pixel sample for the selected mode.
 * @param verbose If non-zero, emit per-row progress messages.
 *
 * This implementation uses explicit timing constants for all Martin mode events,
 * matching the published spec for Martin 1 and Martin 2. All timings are in microseconds.
 *
 * Martin 1:  Sync pulse 4.862ms, porch 0.572ms, separator 0.572ms, pixel 0.4576ms
 * Martin 2:  Sync pulse 4.862ms, porch 0.572ms, separator 0.572ms, pixel 0.2288ms
 *
 * Color order: Green, Blue, Red. 256 lines, 320 pixels per line.
 */
static void buildaudio_m(double pixeltime, int verbose) {
    // Explicit Martin mode timing constants (microseconds)
    const double SYNC_PULSE_US   = 4862.0;  // 4.862 ms @ 1200 Hz
    const double PORCH_US        = 572.0;   // 0.572 ms @ 1500 Hz
    const double SEPARATOR_US    = 572.0;   // 0.572 ms @ 1500 Hz
    const uint16_t SYNC_FREQ     = 1200;
    const uint16_t PORCH_FREQ    = 1500;
    const uint16_t SEPARATOR_FREQ= 1500;
    const int LINES              = 256;
    const int PIXELS_PER_LINE    = 320;

    uint16_t x, y, k;
    uint8_t r_pixel, g_pixel, b_pixel;
    uint8_t r[PIXELS_PER_LINE], g[PIXELS_PER_LINE], b[PIXELS_PER_LINE];

    for (y = 0; y < LINES; y++) {
        // Progress indicator every 64 rows (25%)
        if (verbose && y > 0 && y % 64 == 0) {
            int progress = (y * 100) / LINES;
            printf("   --> Encoding row %d/%d (%d%%)\n", y, LINES, progress);
        }

        // Read image data for this line
        for (x = 0; x < PIXELS_PER_LINE; x++) {
            get_pixel_rgb(g_imgp, x, y, &r_pixel, &g_pixel, &b_pixel);
            r[x] = r_pixel;
            g[x] = g_pixel;
            b[x] = b_pixel;
        }

        // --- Martin line timing sequence ---
        // 1. Sync pulse
        playtone(SYNC_FREQ, SYNC_PULSE_US);
        // 2. Sync porch
        playtone(PORCH_FREQ, PORCH_US);

        // 3. Green scan (320 pixels)
        for (k = 0; k < PIXELS_PER_LINE; k++)
            playtone(toneval_rgb(g[k]), pixeltime);

        // 4. Separator pulse
        playtone(SEPARATOR_FREQ, SEPARATOR_US);

        // 5. Blue scan (320 pixels)
        for (k = 0; k < PIXELS_PER_LINE; k++)
            playtone(toneval_rgb(b[k]), pixeltime);

        // 6. Separator pulse
        playtone(SEPARATOR_FREQ, SEPARATOR_US);

        // 7. Red scan (320 pixels)
        for (k = 0; k < PIXELS_PER_LINE; k++)
            playtone(toneval_rgb(r[k]), pixeltime);

        // 8. Separator pulse (end of line)
        playtone(SEPARATOR_FREQ, SEPARATOR_US);
    }
}

/**
 * @brief Build audio samples for Scottie (S1/S2/SDX) SSTV modes.
 * @param pixeltime Microseconds per pixel sample for the selected mode.
 * @param verbose If non-zero, emit per-row progress messages.
 *
 * Scottie family modes have slightly different timing and ordering than
 * Martin modes; this function implements the Scottie per-line encoding
 * using the global image buffer.
 */
/**
 * @brief Build audio samples for Scottie (S1/S2/SDX) SSTV modes with explicit timing.
 * @param pixeltime Microseconds per pixel sample for the selected mode.
 * @param verbose If non-zero, emit per-row progress messages.
 *
 * This implementation uses explicit timing constants for all Scottie mode events,
 * matching the published spec for Scottie 1, Scottie 2, and Scottie DX. All timings are in microseconds.
 *
 * Scottie 1:  Sync pulse 9.0ms, separator 1.5ms, pixel 0.4320ms
 * Scottie 2:  Sync pulse 9.0ms, separator 1.5ms, pixel 0.2752ms
 * Scottie DX: Sync pulse 9.0ms, separator 1.5ms, pixel 1.0800ms
 *
 * Color order: Green, Blue, Red. 256 lines, 320 pixels per line.
 * Note: The first line starts with a sync pulse before the separator.
 */
static void buildaudio_s(double pixeltime, int verbose) {
    // Explicit Scottie mode timing constants (microseconds)
    const double SYNC_PULSE_US   = 9000.0;  // 9.0 ms @ 1200 Hz
    const double SEPARATOR_US    = 1500.0;  // 1.5 ms @ 1500 Hz
    const double SYNC_PORCH_US   = 1500.0;  // 1.5 ms @ 1500 Hz
    const uint16_t SYNC_FREQ     = 1200;
    const uint16_t SEPARATOR_FREQ= 1500;
    const int LINES              = 256;
    const int PIXELS_PER_LINE    = 320;

    uint16_t x, y, k;
    uint8_t r_pixel, g_pixel, b_pixel;
    uint8_t r[PIXELS_PER_LINE], g[PIXELS_PER_LINE], b[PIXELS_PER_LINE];

    // First line starts with sync pulse
    playtone(SYNC_FREQ, SYNC_PULSE_US);

    for (y = 0; y < LINES; y++) {
        // Progress indicator every 64 rows (25%)
        if (verbose && y > 0 && y % 64 == 0) {
            int progress = (y * 100) / LINES;
            printf("   --> Encoding row %d/%d (%d%%)\n", y, LINES, progress);
        }

        // Read image data for this line
        for (x = 0; x < PIXELS_PER_LINE; x++) {
            get_pixel_rgb(g_imgp, x, y, &r_pixel, &g_pixel, &b_pixel);
            r[x] = r_pixel;
            g[x] = g_pixel;
            b[x] = b_pixel;
        }

        // --- Scottie line timing sequence ---
        // 1. Separator pulse
        playtone(SEPARATOR_FREQ, SEPARATOR_US);

        // 2. Green scan (320 pixels)
        for (k = 0; k < PIXELS_PER_LINE; k++)
            playtone(toneval_rgb(g[k]), pixeltime);

        // 3. Separator pulse
        playtone(SEPARATOR_FREQ, SEPARATOR_US);

        // 4. Blue scan (320 pixels)
        for (k = 0; k < PIXELS_PER_LINE; k++)
            playtone(toneval_rgb(b[k]), pixeltime);

        // 5. Sync pulse
        playtone(SYNC_FREQ, SYNC_PULSE_US);

        // 6. Sync porch
        playtone(SEPARATOR_FREQ, SYNC_PORCH_US);

        // 7. Red scan (320 pixels)
        for (k = 0; k < PIXELS_PER_LINE; k++)
            playtone(toneval_rgb(r[k]), pixeltime);
    }
}


//Builds audio scan data for the Robot 36.
//Applicable to only Robot 36.
/**
 * @brief Build audio samples for Robot 36 (R36) mode.
 * @param verbose If non-zero, emit per-pair progress messages.
 *
 * Robot 36 uses YUV-style encoding with pairs of lines and different
 * chroma placement; this routine converts RGB pixels to luma/chroma and
 * emits the appropriate tone stream.
 */
static void buildaudio_r36(int verbose) {
    // Explicit Robot 36 timing and frequency constants (microseconds, Hz)
    const double SYNC_PULSE_US     = 9000.0;   // 9.0 ms @ 1200 Hz
    const double SYNC_FREQ         = 1200.0;
    const double PORCH_US          = 3000.0;   // 3.0 ms @ 1500 Hz
    const double PORCH_FREQ        = 1500.0;
    const double SEP_EVEN_US       = 4500.0;   // 4.5 ms @ 1500 Hz (even line separator)
    const double SEP_EVEN_FREQ     = 1500.0;
    const double SEP_ODD_US        = 4500.0;   // 4.5 ms @ 2300 Hz (odd line separator)
    const double SEP_ODD_FREQ      = 2300.0;
    const double CHROMA_PORCH_US   = 1500.0;   // 1.5 ms @ 1900 Hz (chroma porch)
    const double CHROMA_PORCH_FREQ = 1900.0;
    const double Y_PIXEL_US        = 275.0;    // 275 us per pixel
    const double CHROMA_PIXEL_US   = 137.5;    // 137.5 us per pixel
    const int LINES                = 240;

    uint16_t x, y, k;
    uint8_t r1, g1, b1, r2, g2, b2, avgr, avgg, avgb;
    uint8_t y1[320], y2[320], ry[320], by[320];

    for (y = 0; y < LINES; y += 2) {
        // Progress indicator every 32 pairs (25%)
        if (verbose && y > 0 && y % 32 == 0) {
            int progress = (y * 100) / LINES;
            printf("   --> Encoding row pair %d/%d (%d%%)\n", y, LINES, progress);
        }

        // Read image data for this pair of lines
        for (x = 0; x < 320; x++) {
            // Even line pixel
            get_pixel_rgb(g_imgp, x, y, &r1, &g1, &b1);
            // Odd line pixel
            get_pixel_rgb(g_imgp, x, y + 1, &r2, &g2, &b2);

            avgr = (uint8_t)(((uint16_t)r1 + (uint16_t)r2) / 2);
            avgg = (uint8_t)(((uint16_t)g1 + (uint16_t)g2) / 2);
            avgb = (uint8_t)(((uint16_t)b1 + (uint16_t)b2) / 2);

            // Y value of even line
            y1[x] = 16.0 + (0.003906 * ((65.738 * (float)r1) + (129.057 * (float)g1) + (25.064 * (float)b1)));
            // Y value of odd line
            y2[x] = 16.0 + (0.003906 * ((65.738 * (float)r2) + (129.057 * (float)g2) + (25.064 * (float)b2)));
            // R-Y value of the average of the two lines (even scan)
            ry[x] = 128.0 + (0.003906 * ((112.439 * (float)avgr) + (-94.154 * (float)avgg) + (-18.285 * (float)avgb)));
            // B-Y value of the average of the two lines (odd scan)
            by[x] = 128.0 + (0.003906 * ((-37.945 * (float)avgr) + (-74.494 * (float)avgg) + (112.439 * (float)avgb)));
        }

        // --- Even line (Y + R-Y) ---
        // 1. Sync pulse
        playtone(SYNC_FREQ, SYNC_PULSE_US);
        // 2. Porch
        playtone(PORCH_FREQ, PORCH_US);
        // 3. Y scan (even line)
        for (k = 0; k < 320; k++)
            playtone(toneval_yuv(y1[k]), Y_PIXEL_US);
        // 4. Even line separator
        playtone(SEP_EVEN_FREQ, SEP_EVEN_US);
        // 5. Chroma porch
        playtone(CHROMA_PORCH_FREQ, CHROMA_PORCH_US);
        // 6. R-Y scan (average of two lines)
        for (k = 0; k < 320; k++)
            playtone(toneval_yuv(ry[k]), CHROMA_PIXEL_US);

        // --- Odd line (Y + B-Y) ---
        // 1. Sync pulse
        playtone(SYNC_FREQ, SYNC_PULSE_US);
        // 2. Porch
        playtone(PORCH_FREQ, PORCH_US);
        // 3. Y scan (odd line)
        for (k = 0; k < 320; k++)
            playtone(toneval_yuv(y2[k]), Y_PIXEL_US);
        // 4. Odd line separator
        playtone(SEP_ODD_FREQ, SEP_ODD_US);
        // 5. Chroma porch
        playtone(CHROMA_PORCH_FREQ, CHROMA_PORCH_US);
        // 6. B-Y scan (average of two lines)
        for (k = 0; k < 320; k++)
            playtone(toneval_yuv(by[k]), CHROMA_PIXEL_US);
    }
} // end buildaudio_r36

/**
 * @brief Build audio samples for Robot 72 (R72) mode.
 * @param verbose If non-zero, emit per-pair progress messages.
 *
 * Robot 72 mirrors Robot 36’s YUV-style encoding but with longer pixel times,
 * yielding ~72 seconds total TX time. Timings are doubled for luma/chroma pixels
 * while sync, porch, separators remain the same.
 */
/**
 * @brief Build audio samples for Robot 72 (R72) mode.
 * @param verbose If non-zero, emit per-line progress messages.
 *
 * Robot 72 mirrors Robot 36's YUV encoding structure but with doubled pixel timing
 * (431.25 μs luma, 215.625 μs chroma). Like R36, it alternates which chroma channel
 * (R-Y or B-Y) is transmitted based on line parity:
 * - Even lines: Sync → Y → R-Y separator/porch/scan
 * - Odd lines:  Sync → Y → B-Y separator/porch/scan
 *
 * Separator frequencies match R36 (1500 Hz for R-Y, 2300 Hz for B-Y) to maintain
 * decoder compatibility. Timing is exactly 2x that of R36 (436.5 sec total → 72 sec).
 *
 * Timing reference: N7CXI's proposal for SSTV Mode Specifications, May 2000.
 */
static void buildaudio_r72(int verbose) {
    // Explicit Robot 72 timing and frequency constants (doubled from Robot 36)
    const double SYNC_PULSE_US     = 9000.0;   // 9.0 ms @ 1200 Hz
    const double SYNC_FREQ         = 1200.0;
    const double PORCH_US          = 3000.0;   // 3.0 ms @ 1500 Hz
    const double PORCH_FREQ        = 1500.0;
    const double SEP_EVEN_US       = 4500.0;   // 4.5 ms @ 1500 Hz (even line separator for R-Y)
    const double SEP_EVEN_FREQ     = 1500.0;
    const double SEP_ODD_US        = 4500.0;   // 4.5 ms @ 2300 Hz (odd line separator for B-Y)
    const double SEP_ODD_FREQ      = 2300.0;
    const double CHROMA_PORCH_US   = 1500.0;   // 1.5 ms @ 1900 Hz (chroma porch)
    const double CHROMA_PORCH_FREQ = 1900.0;
    const double Y_PIXEL_US        = 550.0;    // 550 μs per pixel (2x R36's 275 μs)
    const double CHROMA_PIXEL_US   = 275.0;    // 275 μs per pixel (2x R36's 137.5 μs)
    const int LINES                = 240;

    uint16_t x, y, k;
    uint8_t r1, g1, b1, r2, g2, b2, avgr, avgg, avgb;
    uint8_t y1[320], y2[320], ry[320], by[320];

    for (y = 0; y < LINES; y += 2) {
        // Progress indicator every 30 pairs
        if (verbose && y > 0 && y % 60 == 0) {
            int progress = (y * 100) / LINES;
            printf("   --> Encoding line pair %d/%d (%d%%) [R72]\n", y, LINES, progress);
        }

        // Read image data for this pair of lines
        for (x = 0; x < 320; x++) {
            // Even line pixel
            get_pixel_rgb(g_imgp, x, y, &r1, &g1, &b1);
            // Odd line pixel
            get_pixel_rgb(g_imgp, x, y + 1, &r2, &g2, &b2);

            avgr = (uint8_t)(((uint16_t)r1 + (uint16_t)r2) / 2);
            avgg = (uint8_t)(((uint16_t)g1 + (uint16_t)g2) / 2);
            avgb = (uint8_t)(((uint16_t)b1 + (uint16_t)b2) / 2);

            // Y value of even line
            y1[x] = 16.0 + (0.003906 * ((65.738 * (float)r1) + (129.057 * (float)g1) + (25.064 * (float)b1)));
            // Y value of odd line
            y2[x] = 16.0 + (0.003906 * ((65.738 * (float)r2) + (129.057 * (float)g2) + (25.064 * (float)b2)));
            // R-Y value of the average of the two lines (even scan)
            ry[x] = 128.0 + (0.003906 * ((112.439 * (float)avgr) + (-94.154 * (float)avgg) + (-18.285 * (float)avgb)));
            // B-Y value of the average of the two lines (odd scan)
            by[x] = 128.0 + (0.003906 * ((-37.945 * (float)avgr) + (-74.494 * (float)avgg) + (112.439 * (float)avgb)));
        }

        // --- Even line (Y + R-Y) ---
        // 1. Sync pulse
        playtone(SYNC_FREQ, SYNC_PULSE_US);
        // 2. Porch
        playtone(PORCH_FREQ, PORCH_US);
        // 3. Y scan (even line)
        for (k = 0; k < 320; k++)
            playtone(toneval_yuv(y1[k]), Y_PIXEL_US);
        // 4. Even line separator
        playtone(SEP_EVEN_FREQ, SEP_EVEN_US);
        // 5. Chroma porch
        playtone(CHROMA_PORCH_FREQ, CHROMA_PORCH_US);
        // 6. R-Y scan (average of two lines)
        for (k = 0; k < 320; k++)
            playtone(toneval_yuv(ry[k]), CHROMA_PIXEL_US);

        // --- Odd line (Y + B-Y) ---
        // 1. Sync pulse
        playtone(SYNC_FREQ, SYNC_PULSE_US);
        // 2. Porch
        playtone(PORCH_FREQ, PORCH_US);
        // 3. Y scan (odd line)
        for (k = 0; k < 320; k++)
            playtone(toneval_yuv(y2[k]), Y_PIXEL_US);
        // 4. Odd line separator
        playtone(SEP_ODD_FREQ, SEP_ODD_US);
        // 5. Chroma porch
        playtone(CHROMA_PORCH_FREQ, CHROMA_PORCH_US);
        // 6. B-Y scan (average of two lines)
        for (k = 0; k < 320; k++)
            playtone(toneval_yuv(by[k]), CHROMA_PIXEL_US);
    }
}

// addvistrailer -- Add tones for VIS trailer to audio stream.
//                  More calls to playtone(). 

/**
 * @brief Emit a simple VIS trailer or closing tones to terminate the frame.
 *
 * The trailer is typically a short silence or synchronization tone that
 * marks the end of the SSTV transmission frame.
 */
static void addvistrailer(void) {
    // --- SPEC: SSTV VIS Trailer / Terminator ---
    // Common closing tones after image transmission (times in microseconds):
    // 1) Trailer sequence: 2300 Hz @ 300 ms, 1200 Hz @ 10 ms,
    //                      2300 Hz @ 100 ms, 1200 Hz @ 30 ms
    // 2) Post-silence: 500 ms

    // Explicit constants for clarity and supportability
    const uint16_t tr1_freq = 2300;  const uint32_t tr1_us = 300000; // 300 ms
    const uint16_t tr2_freq = 1200;  const uint32_t tr2_us =  10000; // 10 ms
    const uint16_t tr3_freq = 2300;  const uint32_t tr3_us = 100000; // 100 ms
    const uint16_t tr4_freq = 1200;  const uint32_t tr4_us =  30000; // 30 ms
    const uint32_t post_silence_us  = 500000;                         // 500 ms

    // Emit trailer tones
    playtone(tr1_freq, tr1_us);
    playtone(tr2_freq, tr2_us);
    playtone(tr3_freq, tr3_us);
    playtone(tr4_freq, tr4_us);

    // Final post-silence to mark end of transmission
    playtone(0, post_silence_us);
}

// writefile_aiff -- Save audio data to an AIFF file. Playback for
//                   AIFF format files is tricky. This worked on
//                   ARM Linux:
//                     aplay -r 11025 -c 1 -f U16_BE file.aiff
//                   The WAV output is much easier and more portable,
//                   but who knows - this code might be useful for
//                   something.

/**
 * @brief Write the current global audio buffer to an AIFF file.
 *
 * The function assumes samples are stored in `g_audio` and writes a
 * properly formatted AIFF file to `g_outfp`.
 */
static void writefile_aiff(void) {
    uint32_t totalsize;
    uint64_t audiosize64;
    uint32_t audiosize32;

    audiosize64 = 8 + ((uint64_t)2 * (uint64_t)g_samples); /* header + 2bytes/samp */
    if (audiosize64 > UINT32_MAX) {
        fprintf(stderr, "[ERROR] AIFF audio size too large (%" PRIu64 " bytes)\n", audiosize64);
        return;
    }
    audiosize32 = (uint32_t)audiosize64;
    totalsize = 4 + 8 + 18 + 8 + audiosize32;
    
    // "form" chunk
    fputs( "FORM" , g_outfp ) ;
    fputc( (totalsize & 0xff000000) >> 24 , g_outfp ) ;
    fputc( (totalsize & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (totalsize & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (totalsize & 0x000000ff)       , g_outfp ) ;    
    fputs( "AIFF" , g_outfp ) ;
    
    // "common" chunk
    fputs( "COMM" , g_outfp ) ;
    fputc(    0 , g_outfp ) ;   // size
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(   18 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;   // channels = 1
    fputc(    1 , g_outfp ) ;
    fputc( (g_samples & 0xff000000) >> 24 , g_outfp ) ;   // size
    fputc( (g_samples & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (g_samples & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (g_samples & 0x000000ff)       , g_outfp ) ;    
    fputc(    0 , g_outfp ) ;  // bits/sample
    fputc(   16 , g_outfp ) ;
    fputc( 0x40 , g_outfp ) ;  // 10 byte sample rate (??)
    fputc( 0x0c , g_outfp ) ;  // <--- 11025
    fputc( 0xac , g_outfp ) ;
    fputc( 0x44 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    
    // audio data chunk
    fputs( "SSND" , g_outfp ) ;
    fputc( (audiosize32 & 0xff000000) >> 24 , g_outfp ) ;
    fputc( (audiosize32 & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (audiosize32 & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (audiosize32 & 0x000000ff)       , g_outfp ) ;    
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    
    // FINALLY, the audio data itself (write in checked chunks)
    {
        const size_t BYTES_PER_SAMPLE = 2;
        const size_t CHUNK_BYTES = 8192; /* must be multiple of 2 */
        uint8_t *buf = malloc(CHUNK_BYTES);
        size_t samples_remaining = g_samples;
        size_t sample_index = 0;

        if (!buf) {
            fprintf(stderr, "[ERROR] malloc failed allocating audio buffer\n");
            return;
        }

        while (samples_remaining > 0) {
            size_t samples_this = (CHUNK_BYTES / BYTES_PER_SAMPLE);
            if (samples_this > samples_remaining) { samples_this = samples_remaining;
}
            size_t out_index = 0;
            for (size_t j = 0; j < samples_this; ++j, ++sample_index) {
                uint16_t s = g_audio[sample_index];
                buf[out_index++] = (uint8_t)(s & 0x00ff);
                buf[out_index++] = (uint8_t)((s & 0xff00) >> 8);
            }
            size_t wrote = fwrite(buf, 1, out_index, g_outfp);
            if (wrote != out_index) {
                fprintf(stderr, "[ERROR] write failed while writing AIFF audio data\n");
                perror("fwrite");
                free(buf);
                return;
            }
            samples_remaining -= samples_this;
        }
        free(buf);
    }
}

// writefile_wav -- Write audio data to a WAV file. Once the file
//                  is written, any audio player in the world ought
//                  to be able to play the file without any funky
//                  command-line params.

/**
 * @brief Write the current global audio buffer to a WAV file.
 *
 * The WAV writer produces a standard 16-bit PCM WAV file using the
 * configured sample rate `g_rate` and writes to `g_outfp`.
 */
static void writefile_wav(void) {
    uint32_t totalsize, byterate, blockalign;
    uint64_t audiosize64;
    uint32_t audiosize32;

    audiosize64 = (uint64_t)g_samples * (uint64_t)CHANS * (uint64_t)(BITS / 8); /* bytes of audio */
    if (audiosize64 > UINT32_MAX) {
        fprintf(stderr, "[ERROR] WAV audio size too large (%" PRIu64 " bytes)\n", audiosize64);
        return;
    }
    audiosize32 = (uint32_t)audiosize64;
    totalsize  = 4 + (8 + 16) + (8 + audiosize32) ; /* audio + some headers */
    byterate   = g_rate * CHANS * BITS / 8 ;        /* audio bytes / sec */
    blockalign = CHANS * BITS / 8 ;               /* total bytes / sample */
    
    // RIFF header 
    fputs( "RIFF" , g_outfp ) ;
    
    // total size, audio plus some headers (LE!!)
    fputc( (totalsize & 0x000000ff)       , g_outfp ) ;    
    fputc( (totalsize & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (totalsize & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (totalsize & 0xff000000) >> 24 , g_outfp ) ;
    fputs( "WAVE" , g_outfp ) ;
    
    // sub chunk 1 (format spec)
    
    fputs( "fmt " , g_outfp ) ;  // with a space!
    
    fputc(   16 , g_outfp ) ;   // size of chunk (LE!!)
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    
    fputc(    1 , g_outfp ) ;   // format = 1 (PCM) (LE)
    fputc(    0 , g_outfp ) ;
    
    fputc(    1 , g_outfp ) ;   // channels = 1 (LE)
    fputc(    0 , g_outfp ) ;
    
    // samples / channel / sec (LE!!)
    fputc( (g_rate & 0x000000ff)       , g_outfp ) ;
    fputc( (g_rate & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (g_rate & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (g_rate & 0xff000000) >> 24 , g_outfp ) ;

    // bytes total / sec (LE!!)
    fputc( (byterate & 0x000000ff)       , g_outfp ) ;    
    fputc( (byterate & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (byterate & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (byterate & 0xff000000) >> 24 , g_outfp ) ;

    // block alignment (LE!!)
    fputc( (blockalign & 0x00ff)       , g_outfp ) ;    
    fputc( (blockalign & 0xff00) >>  8 , g_outfp ) ;    
    
    fputc( (BITS & 0x00ff)       , g_outfp ) ;   // bits/sample (LE)
    fputc( (BITS & 0xff00) >>  8 , g_outfp ) ; 

    // sub chunk 2
    // header
    fputs( "data" , g_outfp ) ;
    
    // audio bytes total (LE!!)
    fputc( (audiosize32 & 0x000000ff)       , g_outfp ) ;    
    fputc( (audiosize32 & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (audiosize32 & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (audiosize32 & 0xff000000) >> 24 , g_outfp ) ;
    
    /* FINALLY, the audio data itself (LE!!) - write in chunks and check */
    {
        const size_t BYTES_PER_SAMPLE = 2;
        const size_t CHUNK_BYTES = 8192; /* multiple of 2 */
        uint8_t *buf = malloc(CHUNK_BYTES);
        size_t samples_remaining = g_samples;
        size_t sample_index = 0;

        if (!buf) {
            fprintf(stderr, "[ERROR] malloc failed allocating audio buffer\n");
            return;
        }

        while (samples_remaining > 0) {
            size_t samples_this = (CHUNK_BYTES / BYTES_PER_SAMPLE);
            if (samples_this > samples_remaining) { samples_this = samples_remaining;
}
            size_t out_index = 0;
            for (size_t j = 0; j < samples_this; ++j, ++sample_index) {
                uint16_t s = g_audio[sample_index];
                buf[out_index++] = (uint8_t)(s & 0x00ff);
                buf[out_index++] = (uint8_t)((s & 0xff00) >> 8);
            }
            size_t wrote = fwrite(buf, 1, out_index, g_outfp);
            if (wrote != out_index) {
                fprintf(stderr, "[ERROR] write failed while writing WAV audio data\n");
                perror("fwrite");
                free(buf);
                return;
            }
            samples_remaining -= samples_this;
        }
        free(buf);
    }
}

        
// end
