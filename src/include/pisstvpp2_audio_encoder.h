/**
 * @file pisstvpp2_audio_encoder.h
 * @brief Audio encoder abstraction layer for pluggable format writers
 *
 * This module provides a clean, pluggable interface for audio encoding
 * in various formats (WAV, AIFF, OGG Vorbis, etc.). The abstraction
 * allows format handlers to be easily added, removed, or modified without
 * changing the core application logic.
 *
 * Design Pattern: Factory pattern with abstract interface
 *
 * Usage:
 *   AudioEncoder *encoder = audio_encoder_create("ogg");
 *   audio_encoder_init(encoder, sample_rate, bits, channels, output_path);
 *   audio_encoder_encode(encoder, audio_samples, sample_count);
 *   audio_encoder_finish(encoder);
 *   audio_encoder_destroy(encoder);
 */

#ifndef PISSTVPP2_AUDIO_ENCODER_H
#define PISSTVPP2_AUDIO_ENCODER_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @defgroup AudioEncoderInterface Audio Encoder Interface
 * Abstract interface for audio format encoders
 * @{
 */

/**
 * @brief Opaque encoder state structure
 *
 * Each encoder implementation provides its own state structure
 * (e.g., vorbis_dsp_state for OGG).
 */
typedef struct AudioEncoderState AudioEncoderState;

/**
 * @brief Audio encoder instance (abstract interface)
 *
 * Virtual function table pattern for polymorphic encoder behavior.
 * Each format encoder (WAV, AIFF, OGG) implements these functions.
 */
typedef struct {
    /**
     * @brief Initialize encoder for audio encoding
     *
     * Sets up encoder with audio parameters and opens output file.
     *
     * @param self         Encoder instance (cast to impl-specific type)
     * @param sample_rate  Audio sample rate in Hz (e.g., 11025, 22050, 48000)
     * @param bit_depth    Bits per sample (typically 16)
     * @param channels     Number of channels (typically 1 for mono SSTV)
     * @param filename     Output filename to write encoded audio to
     *
     * @return 0 on success, non-zero on error
     */
    int (*init)(AudioEncoderState *self, uint16_t sample_rate, uint16_t bit_depth,
                uint8_t channels, const char *filename);

    /**
     * @brief Encode audio samples
     *
     * Process PCM audio samples and write encoded/formatted data to output file.
     * For non-compressed formats (WAV, AIFF), writes raw samples.
     * For compressed formats (MP3, OGG), encodes to target compression format.
     *
     * @param self          Encoder instance
     * @param samples       PCM audio samples (int16_t values)
     * @param sample_count  Number of samples to encode
     *
     * @return 0 on success, non-zero on error
     */
    int (*encode)(AudioEncoderState *self, const uint16_t *samples,
                  uint32_t sample_count);

    /**
     * @brief Finalize encoding and close file
     *
     * Flushes any remaining encoded data, writes file footers/metadata,
     * and closes the output file. Must be called before destroy().
     *
     * @param self Encoder instance
     *
     * @return 0 on success, non-zero on error
     */
    int (*finish)(AudioEncoderState *self);

    /**
     * @brief Free encoder resources and cleanup
     *
     * Deallocates memory and closes any open resources.
     * Must be called after finish().
     *
     * @param self Encoder instance to destroy
     */
    void (*destroy)(AudioEncoderState *self);

    /**
     * @brief Opaque state (implementation-specific)
     *
     * Points to encoder-specific state (e.g., lame_t for MP3).
     * Cast to appropriate type in implementing module.
     */
    AudioEncoderState *state;

} AudioEncoder;

/** @} */

/**
 * @defgroup EncoderFactory Encoder Factory and Helpers
 * @{
 */

/**
 * @brief Create an audio encoder for the specified format
 *
 * Factory function that instantiates the appropriate encoder
 * implementation based on format string.
 *
 * Supported formats:
 *   - "wav"   : WAV (RIFF) format
 *   - "aiff"  : AIFF (Audio IFF) format
 *   - "mp3"   : MP3 (requires libmp3lame)
 *   - "ogg"   : OGG Vorbis (requires libvorbis, libogg)
 *
 * @param format  Format string (case-insensitive recommended)
 *
 * @return Pointer to AudioEncoder, or NULL if format not supported
 *         or memory allocation failed.
 *
 * Caller must call audio_encoder_destroy() to free resources.
 */
AudioEncoder* audio_encoder_create(const char *format);

/**
 * @brief Determine output filename for given input file and format
 *
 * Helper to generate standard output filename with appropriate extension.
 * For example: "image.jpg" + "mp3" -> "image.jpg.mp3"
 *
 * @param input_filename  Input filename to base output on
 * @param format          Output format ("wav", "aiff", "mp3", "ogg")
 * @param output_buffer   Buffer to store generated filename
 * @param buffer_size     Size of output_buffer
 *
 * @return 0 on success, -1 if buffer too small
 */
int audio_encoder_get_output_filename(const char *input_filename,
                                      const char *format,
                                      char *output_buffer,
                                      size_t buffer_size);

/**
 * @brief Check if a format string is valid
 *
 * @param format  Format string to validate
 *
 * @return 1 (true) if format is supported, 0 (false) otherwise
 */
int audio_encoder_is_format_supported(const char *format);

/** @} */

/**
 * @defgroup EncoderConvenience Convenience Functions
 * @{
 */

/**
 * @brief Initialize encoder (convenience wrapper)
 *
 * @param encoder      Encoder instance
 * @param sample_rate  Sample rate in Hz
 * @param bit_depth    Bits per sample
 * @param channels     Number of channels
 * @param filename     Output filename
 *
 * @return 0 on success, non-zero on error
 */
static inline int audio_encoder_init(AudioEncoder *encoder, uint16_t sample_rate,
                                     uint16_t bit_depth, uint8_t channels,
                                     const char *filename)
{
    if (!encoder || !encoder->init) return -1;
    return encoder->init(encoder->state, sample_rate, bit_depth, channels, filename);
}

/**
 * @brief Encode samples (convenience wrapper)
 *
 * @param encoder       Encoder instance
 * @param samples       PCM audio samples
 * @param sample_count  Number of samples
 *
 * @return 0 on success, non-zero on error
 */
static inline int audio_encoder_encode(AudioEncoder *encoder,
                                      const uint16_t *samples,
                                      uint32_t sample_count)
{
    if (!encoder || !encoder->encode) return -1;
    return encoder->encode(encoder->state, samples, sample_count);
}

/**
 * @brief Finalize encoding (convenience wrapper)
 *
 * @param encoder Encoder instance
 *
 * @return 0 on success, non-zero on error
 */
static inline int audio_encoder_finish(AudioEncoder *encoder)
{
    if (!encoder || !encoder->finish) return -1;
    return encoder->finish(encoder->state);
}

/**
 * @brief Destroy encoder (convenience wrapper)
 *
 * @param encoder Encoder instance to destroy
 */
static inline void audio_encoder_destroy(AudioEncoder *encoder)
{
    if (!encoder) return;
    if (encoder->destroy) {
        encoder->destroy(encoder->state);
    }
    free(encoder);
}

/** @} */

#endif /* PISSTVPP2_AUDIO_ENCODER_H */
