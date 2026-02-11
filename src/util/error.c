/**
 * @file error.c
 * @brief Implementation of unified error code system for PiSSTVpp2
 *
 * Provides platform-agnostic error handling with consistent messaging,
 * context logging, and fatality evaluation.
 *
 * @author PiSSTVpp2 Contributors
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
    {PISSTVPP2_OK, "Operation completed successfully"},
    
    // Argument/CLI Errors (100-199)
    {PISSTVPP2_ERR_ARG_INVALID, "Invalid argument provided"},
    {PISSTVPP2_ERR_ARG_MISSING, "Required argument missing"},
    {PISSTVPP2_ERR_ARG_UNKNOWN_OPTION, "Unknown command-line option"},
    {PISSTVPP2_ERR_ARG_VALUE_INVALID, "Invalid value for argument"},
    {PISSTVPP2_ERR_ARG_FILENAME_INVALID, "Filename invalid or too long (max 254 characters)"},
    {PISSTVPP2_ERR_ARG_MODE_INVALID, "Invalid SSTV mode code (use --list-modes to see available)"},
    {PISSTVPP2_ERR_ARG_ASPECT_MODE_INVALID, "Invalid aspect mode (must be 'center', 'pad', or 'stretch')"},
    {PISSTVPP2_ERR_ARG_FORMAT_INVALID, "Invalid audio format (must be 'wav', 'aiff', or 'ogg')"},
    {PISSTVPP2_ERR_ARG_SAMPLE_RATE_INVALID, "Invalid sample rate (must be 8000-48000 Hz)"},
    {PISSTVPP2_ERR_ARG_CW_INVALID, "Invalid CW signature option (check callsign, WPM, or tone frequency)"},
    {PISSTVPP2_ERR_NO_INPUT_FILE, "No input file specified (use -i <filename>)"},
    {PISSTVPP2_ERR_ARG_INVALID_PROTOCOL, "Invalid SSTV protocol (use m1, m2, s1, s2, sdx, r36, or r72)"},
    {PISSTVPP2_ERR_ARG_INVALID_FORMAT, "Invalid audio format (must be 'wav', 'aiff', or 'ogg')"},
    {PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE, "Sample rate out of valid range (8000-48000 Hz)"},
    {PISSTVPP2_ERR_ARG_INVALID_ASPECT, "Invalid aspect ratio mode (use 'center', 'pad', or 'stretch')"},
    {PISSTVPP2_ERR_ARG_CALLSIGN_INVALID, "Callsign invalid or too long (max 31 characters)"},
    {PISSTVPP2_ERR_ARG_CW_INVALID_WPM, "CW words-per-minute out of range (1-50)"},
    {PISSTVPP2_ERR_ARG_CW_INVALID_TONE, "CW tone frequency out of range (400-2000 Hz)"},
    {PISSTVPP2_ERR_ARG_CW_MISSING_CALLSIGN, "CW parameters require -C <callsign> option"},
    {PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG, "Filename too long (maximum 254 characters)"},
    
    // Image Errors (200-299)
    {PISSTVPP2_ERR_IMAGE_LOAD, "Failed to load image from file"},
    {PISSTVPP2_ERR_IMAGE_FORMAT_UNSUPPORTED, "Image format not supported (try PNG, JPEG, GIF, BMP, TIFF, or WebP)"},
    {PISSTVPP2_ERR_IMAGE_DIMENSIONS_INVALID, "Image dimensions out of valid range for SSTV"},
    {PISSTVPP2_ERR_IMAGE_PROCESS, "Image processing operation failed"},
    {PISSTVPP2_ERR_IMAGE_ASPECT_CORRECTION, "Aspect ratio correction failed"},
    {PISSTVPP2_ERR_IMAGE_MEMORY, "Insufficient memory for image processing"},
    {PISSTVPP2_ERR_IMAGE_TEXT_OVERLAY, "Text overlay operation failed"},
    {PISSTVPP2_ERR_IMAGE_COLOR_BAR, "Color bar creation failed"},
    
    // SSTV Errors (300-399)
    {PISSTVPP2_ERR_SSTV_ENCODE, "SSTV encoding operation failed"},
    {PISSTVPP2_ERR_SSTV_MODE_NOT_FOUND, "SSTV mode not found in registry"},
    {PISSTVPP2_ERR_SSTV_INIT, "SSTV module initialization failed"},
    {PISSTVPP2_ERR_SSTV_REGISTRY, "Error accessing SSTV mode registry"},
    {PISSTVPP2_ERR_SSTV_MODE_RESOLUTION, "Image resolution incompatible with selected SSTV mode"},
    {PISSTVPP2_ERR_SSTV_VIS, "VIS header encoding failed"},
    {PISSTVPP2_ERR_SSTV_CW, "CW signature encoding failed"},
    
    // Audio Errors (400-499)
    {PISSTVPP2_ERR_AUDIO_ENCODE, "Audio encoding operation failed"},
    {PISSTVPP2_ERR_AUDIO_FORMAT_UNSUPPORTED, "Audio format not supported on this system"},
    {PISSTVPP2_ERR_AUDIO_SAMPLE_RATE_UNSUPPORTED, "Sample rate not supported by audio encoder"},
    {PISSTVPP2_ERR_AUDIO_MEMORY, "Insufficient memory for audio buffer"},
    {PISSTVPP2_ERR_AUDIO_WAV, "WAV file encoding failed"},
    {PISSTVPP2_ERR_AUDIO_AIFF, "AIFF file encoding failed"},
    {PISSTVPP2_ERR_AUDIO_OGG, "OGG Vorbis encoding failed (library may not be installed)"},
    
    // File I/O Errors (500-599)
    {PISSTVPP2_ERR_FILE_OPEN, "Cannot open file"},
    {PISSTVPP2_ERR_FILE_READ, "Cannot read from file"},
    {PISSTVPP2_ERR_FILE_WRITE, "Cannot write to file"},
    {PISSTVPP2_ERR_FILE_NOT_FOUND, "File does not exist"},
    {PISSTVPP2_ERR_FILE_PATH_INVALID, "File path is invalid or contains unsupported characters"},
    {PISSTVPP2_ERR_FILE_PERMISSION, "Permission denied when accessing file"},
    {PISSTVPP2_ERR_FILE_DISK_FULL, "Disk full or write error"},
    
    // System/Memory Errors (600-699)
    {PISSTVPP2_ERR_MEMORY_ALLOC, "Memory allocation failed (system out of memory)"},
    {PISSTVPP2_ERR_SYSTEM_RESOURCE, "System resource limit exceeded"},
    {PISSTVPP2_ERR_SYSTEM_CALL, "System call failed"},
    
    // MMSSTV Library Errors (700-799)
    {PISSTVPP2_ERR_MMSSTV_NOT_FOUND, "MMSSTV library not found (operating in native mode only)"},
    {PISSTVPP2_ERR_MMSSTV_INIT, "MMSSTV library initialization failed"},
    {PISSTVPP2_ERR_MMSSTV_MODE_NOT_FOUND, "Mode not found in MMSSTV library"},
    {PISSTVPP2_ERR_MMSSTV_ENCODE, "MMSSTV library encoding failed"},
    {PISSTVPP2_ERR_MMSSTV_INCOMPATIBLE, "MMSSTV library version incompatible with this application"},
    
    // Text Overlay Errors (800-899)
    {PISSTVPP2_ERR_TEXT_OVERLAY_INIT, "Text overlay module initialization failed"},
    {PISSTVPP2_ERR_TEXT_OVERLAY_PARAMS, "Invalid text overlay parameters"},
    {PISSTVPP2_ERR_TEXT_RENDER, "Failed to render text on image"},
    {PISSTVPP2_ERR_COLOR_BAR_CREATE, "Failed to create color bar"},
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
        case PISSTVPP2_ERR_IMAGE_MEMORY:
        case PISSTVPP2_ERR_AUDIO_MEMORY:
        case PISSTVPP2_ERR_SYSTEM_RESOURCE:
            return 1;
        
        // Recoverable errors (MMSSTV not found = use native modes)
        case PISSTVPP2_ERR_MMSSTV_NOT_FOUND:
            return 0;
        
        // Argument errors are fatal in main() but handled by validation
        case PISSTVPP2_ERR_ARG_INVALID:
        case PISSTVPP2_ERR_ARG_MISSING:
        case PISSTVPP2_ERR_ARG_UNKNOWN_OPTION:
        case PISSTVPP2_ERR_ARG_VALUE_INVALID:
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

// ===========================================================================
// UTILITY FUNCTION: Print all error codes and messages
// ===========================================================================

/**
 * @brief Print table of all defined error codes (for debugging/reference)
 *
 * This function is not exported but can be useful during development.
 * It prints a table of all error codes and their messages.
 *
 * Usage in code:
 * @code
 * #ifdef DEBUG
 * error_print_all_codes();
 * #endif
 * @endcode
 */
static void error_print_all_codes(void) {
    fprintf(stderr, "\n=== PiSSTVpp2 Error Code Reference ===\n");
    fprintf(stderr, "Code | Category                  | Message\n");
    fprintf(stderr, "-----+---------------------------+---\n");
    
    for (size_t i = 0; i < ERROR_MESSAGE_COUNT; i++) {
        int code = error_messages[i].code;
        const char *category = "Unknown";
        
        if (code == 0) category = "Success";
        else if (code >= 100 && code < 200) category = "Argument/CLI";
        else if (code >= 200 && code < 300) category = "Image";
        else if (code >= 300 && code < 400) category = "SSTV";
        else if (code >= 400 && code < 500) category = "Audio";
        else if (code >= 500 && code < 600) category = "File I/O";
        else if (code >= 600 && code < 700) category = "System";
        else if (code >= 700 && code < 800) category = "MMSSTV";
        else if (code >= 800 && code < 900) category = "Text Overlay";
        
        fprintf(stderr, "%4d | %-25s | %s\n", code, category, error_messages[i].message);
    }
    fprintf(stderr, "\n");
}
