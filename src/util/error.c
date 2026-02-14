/**
 * @file error.c
 * @brief Implementation of unified error code system for SlowFrame
 *
 * Provides platform-agnostic error handling with consistent messaging,
 * context logging, and fatality evaluation.
 *
 * @author SlowFrame Contributors
 * @date February 2026
 */

#include "error.h"
#include <string.h>
#include <stdlib.h>

// ===========================================================================
// ERROR MESSAGE STRINGS
// ===========================================================================

/**
 * @brief Array of error messages indexed by error code category
 *
 * Messages are organized by error code ranges (100s, 200s, etc.) for easy
 * lookup and extension. Each message clearly states WHAT went wrong and
 * contextual information for debugging.
 */
static const struct {
    int code;
    const char *message;
} error_messages[] = {
    // Success
    {SLOWFRAME_OK, "Operation completed successfully"},
    
    // Argument/CLI Errors (100-199)
    {SLOWFRAME_ERR_ARG_INVALID, "Invalid argument provided"},
    {SLOWFRAME_ERR_ARG_MISSING, "Required argument missing"},
    {SLOWFRAME_ERR_ARG_UNKNOWN_OPTION, "Unknown command-line option"},
    {SLOWFRAME_ERR_ARG_VALUE_INVALID, "Invalid value for argument"},
    {SLOWFRAME_ERR_ARG_FILENAME_INVALID, "Filename invalid or too long (max 254 characters)"},
    {SLOWFRAME_ERR_ARG_MODE_INVALID, "Invalid SSTV mode code (use --list-modes to see available)"},
    {SLOWFRAME_ERR_ARG_ASPECT_MODE_INVALID, "Invalid aspect mode (must be 'center', 'pad', or 'stretch')"},
    {SLOWFRAME_ERR_ARG_FORMAT_INVALID, "Invalid audio format (must be 'wav', 'aiff', or 'ogg')"},
    {SLOWFRAME_ERR_ARG_SAMPLE_RATE_INVALID, "Invalid sample rate (must be 8000-48000 Hz)"},
    {SLOWFRAME_ERR_ARG_CW_INVALID, "Invalid CW signature option (check callsign, WPM, or tone frequency)"},
    {SLOWFRAME_ERR_NO_INPUT_FILE, "No input file specified (use -i <filename>)"},
    {SLOWFRAME_ERR_ARG_INVALID_PROTOCOL, "Invalid SSTV protocol (use m1, m2, s1, s2, sdx, r36, or r72)"},
    {SLOWFRAME_ERR_ARG_INVALID_FORMAT, "Invalid audio format (must be 'wav', 'aiff', or 'ogg')"},
    {SLOWFRAME_ERR_ARG_INVALID_SAMPLE_RATE, "Sample rate out of valid range (8000-48000 Hz)"},
    {SLOWFRAME_ERR_ARG_INVALID_ASPECT, "Invalid aspect ratio mode (use 'center', 'pad', or 'stretch')"},
    {SLOWFRAME_ERR_ARG_CALLSIGN_INVALID, "Callsign invalid or too long (max 31 characters)"},
    {SLOWFRAME_ERR_ARG_CW_INVALID_WPM, "CW words-per-minute out of range (1-50)"},
    {SLOWFRAME_ERR_ARG_CW_INVALID_TONE, "CW tone frequency out of range (400-2000 Hz)"},
    {SLOWFRAME_ERR_ARG_CW_MISSING_CALLSIGN, "CW parameters require -C <callsign> option"},
    {SLOWFRAME_ERR_ARG_FILENAME_TOO_LONG, "Filename too long (maximum 254 characters)"},
    
    // Image Errors (200-299)
    {SLOWFRAME_ERR_IMAGE_LOAD, "Failed to load image from file"},
    {SLOWFRAME_ERR_IMAGE_FORMAT_UNSUPPORTED, "Image format not supported (try PNG, JPEG, GIF, BMP, TIFF, or WebP)"},
    {SLOWFRAME_ERR_IMAGE_DIMENSIONS_INVALID, "Image dimensions out of valid range for SSTV"},
    {SLOWFRAME_ERR_IMAGE_PROCESS, "Image processing operation failed"},
    {SLOWFRAME_ERR_IMAGE_ASPECT_CORRECTION, "Aspect ratio correction failed"},
    {SLOWFRAME_ERR_IMAGE_MEMORY, "Insufficient memory for image processing"},
    {SLOWFRAME_ERR_IMAGE_TEXT_OVERLAY, "Text overlay operation failed"},
    {SLOWFRAME_ERR_IMAGE_COLOR_BAR, "Color bar creation failed"},
    
    // SSTV Errors (300-399)
    {SLOWFRAME_ERR_SSTV_ENCODE, "SSTV encoding operation failed"},
    {SLOWFRAME_ERR_SSTV_MODE_NOT_FOUND, "SSTV mode not found in registry"},
    {SLOWFRAME_ERR_SSTV_INIT, "SSTV module initialization failed"},
    {SLOWFRAME_ERR_SSTV_REGISTRY, "Error accessing SSTV mode registry"},
    {SLOWFRAME_ERR_SSTV_MODE_RESOLUTION, "Image resolution incompatible with selected SSTV mode"},
    {SLOWFRAME_ERR_SSTV_VIS, "VIS header encoding failed"},
    {SLOWFRAME_ERR_SSTV_CW, "CW signature encoding failed"},
    
    // Audio Errors (400-499)
    {SLOWFRAME_ERR_AUDIO_ENCODE, "Audio encoding operation failed"},
    {SLOWFRAME_ERR_AUDIO_FORMAT_UNSUPPORTED, "Audio format not supported on this system"},
    {SLOWFRAME_ERR_AUDIO_SAMPLE_RATE_UNSUPPORTED, "Sample rate not supported by audio encoder"},
    {SLOWFRAME_ERR_AUDIO_MEMORY, "Insufficient memory for audio buffer"},
    {SLOWFRAME_ERR_AUDIO_WAV, "WAV file encoding failed"},
    {SLOWFRAME_ERR_AUDIO_AIFF, "AIFF file encoding failed"},
    {SLOWFRAME_ERR_AUDIO_OGG, "OGG Vorbis encoding failed (library may not be installed)"},
    
    // File I/O Errors (500-599)
    {SLOWFRAME_ERR_FILE_OPEN, "Cannot open file"},
    {SLOWFRAME_ERR_FILE_READ, "Cannot read from file"},
    {SLOWFRAME_ERR_FILE_WRITE, "Cannot write to file"},
    {SLOWFRAME_ERR_FILE_NOT_FOUND, "File does not exist"},
    {SLOWFRAME_ERR_FILE_PATH_INVALID, "File path is invalid or contains unsupported characters"},
    {SLOWFRAME_ERR_FILE_PERMISSION, "Permission denied when accessing file"},
    {SLOWFRAME_ERR_FILE_DISK_FULL, "Disk full or write error"},
    
    // System/Memory Errors (600-699)
    {SLOWFRAME_ERR_MEMORY_ALLOC, "Memory allocation failed (system out of memory)"},
    {SLOWFRAME_ERR_SYSTEM_RESOURCE, "System resource limit exceeded"},
    {SLOWFRAME_ERR_SYSTEM_CALL, "System call failed"},
    
    // MMSSTV Library Errors (700-799)
    {SLOWFRAME_ERR_MMSSTV_NOT_FOUND, "MMSSTV library not found (operating in native mode only)"},
    {SLOWFRAME_ERR_MMSSTV_INIT, "MMSSTV library initialization failed"},
    {SLOWFRAME_ERR_MMSSTV_MODE_NOT_FOUND, "Mode not found in MMSSTV library"},
    {SLOWFRAME_ERR_MMSSTV_ENCODE, "MMSSTV library encoding failed"},
    {SLOWFRAME_ERR_MMSSTV_INCOMPATIBLE, "MMSSTV library version incompatible with this application"},
    
    // Text Overlay Errors (800-899)
    {SLOWFRAME_ERR_TEXT_OVERLAY_INIT, "Text overlay module initialization failed"},
    {SLOWFRAME_ERR_TEXT_OVERLAY_PARAMS, "Invalid text overlay parameters"},
    {SLOWFRAME_ERR_TEXT_RENDER, "Failed to render text on image"},
    {SLOWFRAME_ERR_COLOR_BAR_CREATE, "Failed to create color bar"},
};

#define ERROR_MESSAGE_COUNT (sizeof(error_messages) / sizeof(error_messages[0]))

// ===========================================================================
// FUNCTION IMPLEMENTATIONS
// ===========================================================================

const char* error_string(int error_code) {
    // Search for exact match
    for (size_t i = 0; i < ERROR_MESSAGE_COUNT; i++) {
        if (error_messages[i].code == error_code) {
            return error_messages[i].message;
        }
    }
    
    // Unknown error code
    return "Unknown error (please check error code)";
}

void error_log(int error_code, const char *context_format, ...) {
    const char *message = error_string(error_code);
    
    fprintf(stderr, "[ERROR] Error code %d: (%s)\n", error_code, message);
    
    if (context_format != NULL) {
        va_list args;
        va_start(args, context_format);
        fprintf(stderr, "        Context: ");
        vfprintf(stderr, context_format, args);
        fprintf(stderr, "\n");
        va_end(args);
    }
}

int error_is_fatal(int error_code) {
    // Categories that are fatal (cannot continue)
    if (error_code >= 500 && error_code < 600) {
        // File I/O errors are always fatal
        return 1;
    }
    
    if (error_code >= 600 && error_code < 700) {
        // System/memory errors are always fatal
        return 1;
    }
    
    // Specific fatal errors from other categories
    switch (error_code) {
        // Memory allocation failures are fatal
        case SLOWFRAME_ERR_IMAGE_MEMORY:
        case SLOWFRAME_ERR_AUDIO_MEMORY:
        case SLOWFRAME_ERR_SYSTEM_RESOURCE:
            return 1;
        
        // Recoverable errors (MMSSTV not found = use native modes)
        case SLOWFRAME_ERR_MMSSTV_NOT_FOUND:
            return 0;
        
        // Argument errors are fatal in main() but handled by validation
        case SLOWFRAME_ERR_ARG_INVALID:
        case SLOWFRAME_ERR_ARG_MISSING:
        case SLOWFRAME_ERR_ARG_UNKNOWN_OPTION:
        case SLOWFRAME_ERR_ARG_VALUE_INVALID:
            return 1;
        
        // Default: most errors should stop processing
        default:
            return 1;
    }
}

void error_fatal_exit(int error_code, const char *context_format, ...) {
    const char *message = error_string(error_code);
    
    fprintf(stderr, "[FATAL] Error code %d: (%s)\n", error_code, message);
    
    if (context_format != NULL) {
        va_list args;
        va_start(args, context_format);
        fprintf(stderr, "        Context: ");
        vfprintf(stderr, context_format, args);
        fprintf(stderr, "\n");
        va_end(args);
    }
    
    // Convert error code to exit code (0-255 range for shell)
    int exit_code = (error_code > 0) ? (error_code % 256) : 1;
    exit(exit_code);
}

