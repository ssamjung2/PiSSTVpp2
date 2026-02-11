/**
 * @file error.h
 * @brief Unified error code definitions and messaging for platform-agnostic error handling
 *
 * ## Overview
 * This module provides a centralized error handling system for PiSSTVpp2. Instead of
 * scattered error codes (-1, 0, 1, 2) throughout the codebase, all error conditions
 * are defined as named constants with corresponding human-readable messages.
 *
 * ## Error Categories
 * - **SUCCESS (0):** Operation completed successfully
 * - **ARGUMENT ERRORS (100-199):** Command-line argument validation failures
 * - **IMAGE ERRORS (200-299):** Image loading, processing, and format issues
 * - **SSTV ERRORS (300-399):** SSTV protocol, mode, and encoding issues
 * - **AUDIO ERRORS (400-499):** Audio encoding and format issues
 * - **FILE ERRORS (500-599):** File I/O, path, and format issues
 * - **SYSTEM ERRORS (600-699):** Memory, system resource issues
 * - **MMSSTV ERRORS (700-799):** MMSSTV library detection and integration
 * - **TEXT OVERLAY ERRORS (800-899):** Text overlay and color bar issues
 * - **GENERIC ERRORS (1000+):** General/unknown errors
 *
 * ## Usage Examples
 * @code
 * // Check for error
 * if (result == PISSTVPP2_OK) {
 *     log_verbose(verbose, 0, "Success: %s\n", error_string(result));
 * } else {
 *     fprintf(stderr, "Error [%d]: %s\n", result, error_string(result));
 * }
 *
 * // Log with context
 * error_log(PISSTVPP2_ERR_IMAGE_LOAD, "Failed to load: %s", filename);
 *
 * // Use in error propagation
 * int ret = image_load(...);
 * if (ret != PISSTVPP2_OK) {
 *     error_log(ret, "Image loading failed in main");
 *     return ret;
 * }
 * @endcode
 *
 * ## Design Principles
 * 1. **Consistency:** All error returns use named constants, never raw integers
 * 2. **Clarity:** Error messages explain WHAT went wrong and WHY
 * 3. **Context:** error_log() captures local context for debugging
 * 4. **Platform-Agnostic:** Uses only standard C and portable functions
 * 5. **Backward Compatible:** Designed to integrate without breaking existing code
 *
 * @author PiSSTVpp2 Contributors
 * @date February 2026
 */

#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdarg.h>

// ===========================================================================
// ERROR CODE DEFINITIONS
// ===========================================================================

/** @defgroup ErrorCodes Error code constants
 *  @{
 */

/** Success - operation completed without errors */
#define PISSTVPP2_OK 0

// Argument/CLI Errors (100-199)
/** Generic argument error */
#define PISSTVPP2_ERR_ARG_INVALID 101
/** Missing required argument */
#define PISSTVPP2_ERR_ARG_MISSING 102
/** Unrecognized command-line option */
#define PISSTVPP2_ERR_ARG_UNKNOWN_OPTION 103
/** Invalid option value */
#define PISSTVPP2_ERR_ARG_VALUE_INVALID 104
/** Filename too long or invalid */
#define PISSTVPP2_ERR_ARG_FILENAME_INVALID 105
/** Invalid SSTV mode/protocol code */
#define PISSTVPP2_ERR_ARG_MODE_INVALID 106
/** Invalid aspect ratio correction mode */
#define PISSTVPP2_ERR_ARG_ASPECT_MODE_INVALID 107
/** Invalid audio format specified */
#define PISSTVPP2_ERR_ARG_FORMAT_INVALID 108
/** Invalid sample rate */
#define PISSTVPP2_ERR_ARG_SAMPLE_RATE_INVALID 109
/** CW signature option invalid */
#define PISSTVPP2_ERR_ARG_CW_INVALID 110
/** No input file specified (-i required) */
#define PISSTVPP2_ERR_NO_INPUT_FILE 111
/** Invalid SSTV protocol specified */
#define PISSTVPP2_ERR_ARG_INVALID_PROTOCOL 112
/** Invalid audio format specified */
#define PISSTVPP2_ERR_ARG_INVALID_FORMAT 113
/** Invalid sample rate specified */
#define PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE 114
/** Invalid aspect ratio mode specified */
#define PISSTVPP2_ERR_ARG_INVALID_ASPECT 115
/** Invalid or too-long callsign for CW signature */
#define PISSTVPP2_ERR_ARG_CALLSIGN_INVALID 116
/** Invalid CW words-per-minute value */
#define PISSTVPP2_ERR_ARG_CW_INVALID_WPM 117
/** Invalid CW tone frequency */
#define PISSTVPP2_ERR_ARG_CW_INVALID_TONE 118
/** CW parameters used without callsign (-C required) */
#define PISSTVPP2_ERR_ARG_CW_MISSING_CALLSIGN 119
/** Filename too long (exceeds max path length) */
#define PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG 120

// Image Errors (200-299)
/** Generic image loading error */
#define PISSTVPP2_ERR_IMAGE_LOAD 201
/** Image format not supported */
#define PISSTVPP2_ERR_IMAGE_FORMAT_UNSUPPORTED 202
/** Image dimensions out of range */
#define PISSTVPP2_ERR_IMAGE_DIMENSIONS_INVALID 203
/** Image processing/transformation failed */
#define PISSTVPP2_ERR_IMAGE_PROCESS 204
/** Aspect ratio correction failed */
#define PISSTVPP2_ERR_IMAGE_ASPECT_CORRECTION 205
/** Image memory allocation failed */
#define PISSTVPP2_ERR_IMAGE_MEMORY 206
/** Text overlay operation failed */
#define PISSTVPP2_ERR_IMAGE_TEXT_OVERLAY 207
/** Color bar operation failed */
#define PISSTVPP2_ERR_IMAGE_COLOR_BAR 208

// SSTV Errors (300-399)
/** Generic SSTV encoding error */
#define PISSTVPP2_ERR_SSTV_ENCODE 301
/** SSTV mode not found in registry */
#define PISSTVPP2_ERR_SSTV_MODE_NOT_FOUND 302
/** SSTV initialization failed */
#define PISSTVPP2_ERR_SSTV_INIT 303
/** SSTV mode registry error */
#define PISSTVPP2_ERR_SSTV_REGISTRY 304
/** Mode resolution mismatch */
#define PISSTVPP2_ERR_SSTV_MODE_RESOLUTION 305
/** VIS header encoding failed */
#define PISSTVPP2_ERR_SSTV_VIS 306
/** CW signature encoding failed */
#define PISSTVPP2_ERR_SSTV_CW 307

// Audio Errors (400-499)
/** Generic audio encoding error */
#define PISSTVPP2_ERR_AUDIO_ENCODE 401
/** Audio format not supported */
#define PISSTVPP2_ERR_AUDIO_FORMAT_UNSUPPORTED 402
/** Audio sample rate not supported */
#define PISSTVPP2_ERR_AUDIO_SAMPLE_RATE_UNSUPPORTED 403
/** Audio buffer allocation failed */
#define PISSTVPP2_ERR_AUDIO_MEMORY 404
/** WAV encoding failed */
#define PISSTVPP2_ERR_AUDIO_WAV 405
/** AIFF encoding failed */
#define PISSTVPP2_ERR_AUDIO_AIFF 406
/** OGG Vorbis encoding failed */
#define PISSTVPP2_ERR_AUDIO_OGG 407

// File I/O Errors (500-599)
/** Cannot open file */
#define PISSTVPP2_ERR_FILE_OPEN 501
/** Cannot read file */
#define PISSTVPP2_ERR_FILE_READ 502
/** Cannot write file */
#define PISSTVPP2_ERR_FILE_WRITE 503
/** File does not exist */
#define PISSTVPP2_ERR_FILE_NOT_FOUND 504
/** File path invalid */
#define PISSTVPP2_ERR_FILE_PATH_INVALID 505
/** File I/O permission denied */
#define PISSTVPP2_ERR_FILE_PERMISSION 506
/** Disk full or write error */
#define PISSTVPP2_ERR_FILE_DISK_FULL 507

// System/Memory Errors (600-699)
/** Memory allocation failed */
#define PISSTVPP2_ERR_MEMORY_ALLOC 601
/** System resource limit exceeded */
#define PISSTVPP2_ERR_SYSTEM_RESOURCE 602
/** System call failed */
#define PISSTVPP2_ERR_SYSTEM_CALL 603

// MMSSTV Library Errors (700-799)
/** MMSSTV library not found */
#define PISSTVPP2_ERR_MMSSTV_NOT_FOUND 701
/** MMSSTV library initialization failed */
#define PISSTVPP2_ERR_MMSSTV_INIT 702
/** MMSSTV mode not found */
#define PISSTVPP2_ERR_MMSSTV_MODE_NOT_FOUND 703
/** MMSSTV encoding failed */
#define PISSTVPP2_ERR_MMSSTV_ENCODE 704
/** MMSSTV library incompatible */
#define PISSTVPP2_ERR_MMSSTV_INCOMPATIBLE 705

// Text Overlay Errors (800-899)
/** Text overlay initialization failed */
#define PISSTVPP2_ERR_TEXT_OVERLAY_INIT 801
/** Invalid text overlay parameters */
#define PISSTVPP2_ERR_TEXT_OVERLAY_PARAMS 802
/** Text rendering failed */
#define PISSTVPP2_ERR_TEXT_RENDER 803
/** Color bar creation failed */
#define PISSTVPP2_ERR_COLOR_BAR_CREATE 804

// Generic/Unknown Errors
/** Generic unknown error */
#define PISSTVPP2_ERR_UNKNOWN 1000

/** @} */

// ===========================================================================
// FUNCTION DECLARATIONS
// ===========================================================================

/**
 * @brief Get human-readable error message for error code
 *
 * Returns a descriptive error message corresponding to the given error code.
 * For unknown error codes, returns a generic error message.
 *
 * **Thread Safety:** This function is thread-safe (reads only static data)
 *
 * **Examples:**
 * @code
 * int error_code = PISSTVPP2_ERR_IMAGE_LOAD;
 * printf("Error: %s\n", error_string(error_code));
 * // Output: "Error: Failed to load image from file"
 *
 * printf("Code %d: %s\n", PISSTVPP2_OK, error_string(PISSTVPP2_OK));
 * // Output: "Code 0: Operation completed successfully"
 * @endcode
 *
 * @param error_code The error code to look up (defined as PISSTVPP2_ERR_*)
 * @return Pointer to static string describing the error (never NULL)
 *
 * @see PISSTVPP2_OK, PISSTVPP2_ERR_IMAGE_LOAD, PISSTVPP2_ERR_SSTV_INIT
 */
const char* error_string(int error_code);

/**
 * @brief Log an error with contextual information
 *
 * Prints an error message to stderr with the error code, human-readable message,
 * and additional context provided by the caller. Useful for error propagation
 * through call stack with local context at each level.
 *
 * **Format:**
 * ```
 * [ERROR] Error code XXX: (human-readable message)
 *         Context: (additional information)
 * ```
 *
 * **Thread Safety:** Unsafe without external synchronization (writes to stderr)
 *
 * **Examples:**
 * @code
 * // Simple error logging
 * error_log(PISSTVPP2_ERR_IMAGE_LOAD, "Loading: %s", filename);
 * // Output: [ERROR] Error code 201: (Failed to load image from file)
 * //         Context: Loading: photo.jpg
 *
 * // Error with additional details
 * int result = load_image(...);
 * if (result != PISSTVPP2_OK) {
 *     error_log(result, "Image size: %d x %d pixels", width, height);
 * }
 *
 * // Propagating error with local context
 * int ret = process_image(img);
 * if (ret != PISSTVPP2_OK) {
 *     error_log(ret, "Failed in main() during image processing");
 *     return ret;
 * }
 * @endcode
 *
 * @param error_code The error code (defined as PISSTVPP2_ERR_*)
 * @param context_format Printf-style format string for additional context
 * @param ... Variable arguments matching context_format
 *
 * @see error_string(), PISSTVPP2_OK
 */
void error_log(int error_code, const char *context_format, ...);

/**
 * @brief Check if error code represents a fatal condition
 *
 * Some errors are recoverable (e.g., optional feature not available) while
 * others are fatal (e.g., cannot write output file). This function helps
 * decide whether to continue or abort.
 *
 * **What's fatal:**
 * - File I/O errors (cannot proceed without output)
 * - Memory allocation failures
 * - System resource exhaustion
 *
 * **What's recoverable:**
 * - MMSSTV library not found (falls back to native modes)
 * - Unknown SSTV mode (user made typo, can suggest alternatives)
 *
 * @param error_code The error code to evaluate
 * @return Non-zero if fatal (should abort), 0 if potentially recoverable
 *
 * @example
 * @code
 * int ret = process_image();
 * if (ret != PISSTVPP2_OK) {
 *     error_log(ret, "Processing failed");
 *     if (error_is_fatal(ret)) {
 *         fprintf(stderr, "Fatal error, aborting\n");
 *         return ret;
 *     } else {
 *         fprintf(stderr, "Continuing with defaults\n");
 *     }
 * }
 * @endcode
 */
int error_is_fatal(int error_code);

/**
 * @brief Print error and exit program
 *
 * Convenience function for fatal errors that should terminate the program.
 * Prints the error message and exits with the error code (or 1 if code is 0).
 *
 * **Examples:**
 * @code
 * // Exit with code 201 (image error)
 * error_fatal_exit(PISSTVPP2_ERR_IMAGE_LOAD, "Could not find input image");
 *
 * // Exit with code 1 (no specific error)
 * error_fatal_exit(PISSTVPP2_ERR_UNKNOWN, "Something went wrong");
 * @endcode
 *
 * @param error_code The error code (defines exit code)
 * @param context_format Printf-style format string for context
 * @param ... Variable arguments matching context_format
 * @noreturn This function does not return (calls exit())
 *
 * @note Exit code is min(error_code, 255) to stay within standard shell limits
 */
void error_fatal_exit(int error_code, const char *context_format, ...) __attribute__((noreturn));

#endif // ERROR_H
