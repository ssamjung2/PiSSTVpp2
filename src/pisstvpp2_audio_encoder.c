/**
 * @file pisstvpp2_audio_encoder.c
 * @brief Audio encoder factory and initialization
 *
 * Implements the factory pattern for audio format encoders.
 * Provides pluggable format writers (WAV, AIFF, OGG Vorbis).
 */

#include "pisstvpp2_audio_encoder.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// Forward declarations for encoder implementations
extern AudioEncoder* audio_encoder_wav_create(void);
extern AudioEncoder* audio_encoder_aiff_create(void);
extern AudioEncoder* audio_encoder_ogg_create(void);

/**
 * @brief Convert format string to lowercase
 */
static char* strtolower(const char *src, char *dst, size_t dst_size)
{
    if (!src || !dst || dst_size == 0) return NULL;
    
    size_t i = 0;
    while (i < dst_size - 1 && src[i] != '\0') {
        dst[i] = tolower((unsigned char)src[i]);
        i++;
    }
    dst[i] = '\0';
    return dst;
}

/**
 * @brief Create an audio encoder for the specified format
 *
 * Factory function that instantiates the appropriate encoder
 * implementation based on format string.
 *
 * @param format  Format string ("wav", "aiff", "ogg", etc.)
 *
 * @return Pointer to AudioEncoder, or NULL if format not supported
 */
AudioEncoder* audio_encoder_create(const char *format)
{
    if (!format) return NULL;

    // Normalize format string to lowercase
    char fmt_lower[16];
    if (!strtolower(format, fmt_lower, sizeof(fmt_lower))) {
        return NULL;
    }

    // Dispatch to appropriate encoder implementation
    if (strcmp(fmt_lower, "wav") == 0) {
        return audio_encoder_wav_create();
    } else if (strcmp(fmt_lower, "aiff") == 0) {
        return audio_encoder_aiff_create();
    } else if (strcmp(fmt_lower, "ogg") == 0 || strcmp(fmt_lower, "vorbis") == 0) {
        return audio_encoder_ogg_create();
    }

    // Format not supported
    fprintf(stderr, "[ERROR] Unsupported audio format: '%s'\n", format);
    return NULL;
}

/**
 * @brief Check if a format string is valid and supported
 *
 * @param format  Format string to validate
 *
 * @return 1 (true) if format is supported, 0 (false) otherwise
 */
int audio_encoder_is_format_supported(const char *format)
{
    if (!format) return 0;

    char fmt_lower[16];
    if (!strtolower(format, fmt_lower, sizeof(fmt_lower))) {
        return 0;
    }

    return (strcmp(fmt_lower, "wav") == 0 ||
            strcmp(fmt_lower, "aiff") == 0 ||
            strcmp(fmt_lower, "ogg") == 0 ||
            strcmp(fmt_lower, "vorbis") == 0);
}

/**
 * @brief Determine output filename based on format
 *
 * Generates standard output filename with appropriate extension.
 *
 * @param input_filename  Input filename (e.g., "image.jpg")
 * @param format          Output format (e.g., "wav")
 * @param output_buffer   Buffer to store generated filename
 * @param buffer_size     Size of output_buffer
 *
 * @return 0 on success, -1 if buffer too small
 */
int audio_encoder_get_output_filename(const char *input_filename,
                                      const char *format,
                                      char *output_buffer,
                                      size_t buffer_size)
{
    if (!input_filename || !format || !output_buffer || buffer_size == 0) {
        return -1;
    }

    // Get the file extension for this format
    const char *extension = NULL;
    
    char fmt_lower[16];
    if (!strtolower(format, fmt_lower, sizeof(fmt_lower))) {
        return -1;
    }

    if (strcmp(fmt_lower, "wav") == 0) {
        extension = "wav";
    } else if (strcmp(fmt_lower, "aiff") == 0) {
        extension = "aiff";
    } else if (strcmp(fmt_lower, "ogg") == 0 || strcmp(fmt_lower, "vorbis") == 0) {
        extension = "ogg";
    } else {
        return -1;  // Unknown format
    }

    // Build output filename: input_filename.extension
    int needed = snprintf(output_buffer, buffer_size, "%s.%s", 
                         input_filename, extension);
    
    if (needed < 0 || (size_t)needed >= buffer_size) {
        return -1;  // Buffer too small
    }

    return 0;
}
