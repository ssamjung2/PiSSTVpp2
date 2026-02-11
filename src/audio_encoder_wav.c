/**
 * @file audio_encoder_wav.c
 * @brief WAV format audio encoder
 *
 * Implements WAV (RIFF) format audio encoding for PiSSTVpp2.
 * Produces standard 16-bit PCM WAV files compatible with all audio players.
 */

#include "pisstvpp2_audio_encoder.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <inttypes.h>

/**
 * @brief WAV encoder state
 */
typedef struct {
    FILE *fp;              /**< Output file pointer */
    uint16_t sample_rate;  /**< Sample rate in Hz */
    uint16_t bit_depth;    /**< Bits per sample */
    uint8_t channels;      /**< Number of channels */
} WavEncoderState;

/**
 * @brief Initialize WAV encoder
 */
static int wav_encoder_init(AudioEncoderState *self, uint16_t sample_rate,
                            uint16_t bit_depth, uint8_t channels,
                            const char *filename)
{
    WavEncoderState *state = (WavEncoderState *)self;

    if (!state || !filename) {
        return PISSTVPP2_ERR_ARG_VALUE_INVALID;
    }

    state->sample_rate = sample_rate;
    state->bit_depth = bit_depth;
    state->channels = channels;

    // Open output file for writing
    state->fp = fopen(filename, "wb");
    if (!state->fp) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "Failed to open WAV output file: %s", filename);
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    return PISSTVPP2_OK;
}

/**
 * @brief Write WAV file header
 *
 * This is called after all audio data has been encoded.
 * We write a placeholder header that will be overwritten with correct sizes.
 */
static int wav_encoder_write_header(WavEncoderState *state, uint32_t audio_sample_count)
{
    if (!state || !state->fp) {
        return PISSTVPP2_ERR_AUDIO_WAV;
    }

    // Calculate sizes
    uint32_t bytes_per_sample = state->channels * (state->bit_depth / 8);
    uint32_t audio_data_size = audio_sample_count * bytes_per_sample;
    uint32_t file_size = 36 + audio_data_size;  // 36 = RIFF header + fmt chunk
    uint32_t byte_rate = state->sample_rate * state->channels * (state->bit_depth / 8);
    uint16_t block_align = state->channels * (state->bit_depth / 8);

    // Rewind to beginning to write headers
    if (fseek(state->fp, 0, SEEK_SET) != 0) {
        error_log(PISSTVPP2_ERR_AUDIO_WAV, "WAV header seek", "Failed to seek in output file");
        return PISSTVPP2_ERR_AUDIO_WAV;
    }

    // RIFF header
    if (fputs("RIFF", state->fp) == EOF) {
        error_log(PISSTVPP2_ERR_AUDIO_WAV, "WAV write", "Failed to write RIFF header");
        return PISSTVPP2_ERR_AUDIO_WAV;
    }

    // File size (little-endian)
    if (fputc((file_size & 0x000000ff), state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc((file_size & 0x0000ff00) >> 8, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc((file_size & 0x00ff0000) >> 16, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc((file_size & 0xff000000) >> 24, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;

    if (fputs("WAVE", state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;

    // fmt sub-chunk
    if (fputs("fmt ", state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;

    // Chunk size (16 for PCM)
    if (fputc(16, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc(0, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc(0, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc(0, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;

    // Audio format (1 = PCM)
    if (fputc(1, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc(0, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;

    // Channels
    if (fputc(state->channels, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc(0, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;

    // Sample rate (little-endian)
    if (fputc((state->sample_rate & 0x000000ff), state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc((state->sample_rate & 0x0000ff00) >> 8, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc((state->sample_rate & 0x00ff0000) >> 16, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc((state->sample_rate & 0xff000000) >> 24, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;

    // Byte rate (little-endian)
    if (fputc((byte_rate & 0x000000ff), state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc((byte_rate & 0x0000ff00) >> 8, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc((byte_rate & 0x00ff0000) >> 16, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc((byte_rate & 0xff000000) >> 24, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;

    // Block align (little-endian)
    if (fputc((block_align & 0x00ff), state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc((block_align & 0xff00) >> 8, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;

    // Bits per sample (little-endian)
    if (fputc((state->bit_depth & 0x00ff), state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc((state->bit_depth & 0xff00) >> 8, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;

    // data sub-chunk
    if (fputs("data", state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;

    // Data size (little-endian)
    if (fputc((audio_data_size & 0x000000ff), state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc((audio_data_size & 0x0000ff00) >> 8, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc((audio_data_size & 0x00ff0000) >> 16, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;
    if (fputc((audio_data_size & 0xff000000) >> 24, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_WAV;

    return PISSTVPP2_OK;
}

/**
 * @brief Encode audio samples to WAV format
 *
 * For WAV format, simply writes raw PCM data in little-endian format.
 */
static int wav_encoder_encode(AudioEncoderState *self, const uint16_t *samples,
                              uint32_t sample_count)
{
    WavEncoderState *state = (WavEncoderState *)self;

    if (!state || !state->fp || !samples) {
        return PISSTVPP2_ERR_ARG_VALUE_INVALID;
    }

    if (sample_count == 0) {
        return PISSTVPP2_OK;
    }

    // For WAV, just write raw PCM data (little-endian int16)
    const size_t BYTES_PER_SAMPLE = 2;
    const size_t CHUNK_BYTES = 8192;
    uint8_t *buf = malloc(CHUNK_BYTES);

    if (!buf) {
        error_log(PISSTVPP2_ERR_MEMORY_ALLOC, "Failed to allocate WAV write buffer (%zu bytes)", CHUNK_BYTES);
        return PISSTVPP2_ERR_MEMORY_ALLOC;
    }

    size_t samples_remaining = sample_count;
    size_t sample_index = 0;
    int result = 0;

    while (samples_remaining > 0 && result == 0) {
        size_t samples_this = CHUNK_BYTES / BYTES_PER_SAMPLE;
        if (samples_this > samples_remaining) {
            samples_this = samples_remaining;
        }

        size_t out_index = 0;
        for (size_t j = 0; j < samples_this; ++j, ++sample_index) {
            uint16_t s = samples[sample_index];
            buf[out_index++] = (uint8_t)(s & 0x00ff);
            buf[out_index++] = (uint8_t)((s & 0xff00) >> 8);
        }

        size_t wrote = fwrite(buf, 1, out_index, state->fp);
        if (wrote != out_index) {
            error_log(PISSTVPP2_ERR_FILE_WRITE, "Failed to write WAV audio data (wrote %zu/%zu bytes)", wrote, out_index);
            result = PISSTVPP2_ERR_FILE_WRITE;
        }

        samples_remaining -= samples_this;
    }

    free(buf);
    return result;
}

/**
 * @brief Finalize WAV encoding
 *
 * Writes the final WAV headers with correct sizes and closes the file.
 */
static int wav_encoder_finish(AudioEncoderState *self)
{
    WavEncoderState *state = (WavEncoderState *)self;

    if (!state || !state->fp) {
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    // Get file size to calculate audio sample count
    if (fseek(state->fp, 0, SEEK_END) != 0) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "Failed to seek to end of WAV file");
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    long file_size = ftell(state->fp);
    if (file_size < 0) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "Failed to get WAV file size");
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    // Audio data starts at byte 44 in WAV file (after headers)
    long audio_data_size = file_size - 44;
    if (audio_data_size < 0) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "Invalid WAV file size: %ld", file_size);
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    // Calculate sample count
    uint32_t bytes_per_sample = state->channels * (state->bit_depth / 8);
    uint32_t sample_count = audio_data_size / bytes_per_sample;

    // Write correct header with size information
    if (wav_encoder_write_header(state, sample_count) != 0) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "Failed to write WAV header");
        return PISSTVPP2_ERR_AUDIO_WAV;
    }

    // Flush and close file
    if (fflush(state->fp) != 0 || ferror(state->fp)) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "Failed to flush WAV output file");
        fclose(state->fp);
        state->fp = NULL;
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    if (fclose(state->fp) != 0) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "Failed to close WAV output file");
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    state->fp = NULL;
    return PISSTVPP2_OK;
}

/**
 * @brief Destroy WAV encoder
 */
static void wav_encoder_destroy(AudioEncoderState *self)
{
    WavEncoderState *state = (WavEncoderState *)self;

    if (state) {
        if (state->fp) {
            fclose(state->fp);
        }
        free(state);
    }
}

/**
 * @brief Create WAV encoder instance
 */
AudioEncoder* audio_encoder_wav_create(void)
{
    AudioEncoder *encoder = malloc(sizeof(AudioEncoder));
    if (!encoder) {
        return NULL;
    }

    WavEncoderState *state = malloc(sizeof(WavEncoderState));
    if (!state) {
        free(encoder);
        return NULL;
    }

    memset(state, 0, sizeof(WavEncoderState));

    encoder->init = wav_encoder_init;
    encoder->encode = wav_encoder_encode;
    encoder->finish = wav_encoder_finish;
    encoder->destroy = wav_encoder_destroy;
    encoder->state = (AudioEncoderState *)state;

    return encoder;
}
