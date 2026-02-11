/**
 * @file audio_encoder_aiff.c
 * @brief AIFF format audio encoder
 *
 * Implements AIFF (Audio Interchange File Format) encoding for PiSSTVpp2.
 * Produces standard 16-bit PCM AIFF files compatible with macOS and Pro Audio tools.
 */

#include "pisstvpp2_audio_encoder.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <inttypes.h>

/**
 * @brief AIFF encoder state
 */
typedef struct {
    FILE *fp;              /**< Output file pointer */
    uint16_t sample_rate;  /**< Sample rate in Hz */
    uint16_t bit_depth;    /**< Bits per sample */
    uint8_t channels;      /**< Number of channels */
} AiffEncoderState;

/**
 * @brief Convert sample rate to AIFF 10-byte extended format
 *
 * AIFF uses IEEE 754 80-bit extended precision format:
 * - 2 bytes: exponent (biased by 16383, big-endian)
 * - 8 bytes: mantissa (normalized, big-endian)
 *
 * This function converts standard sample rates to AIFF extended format.
 * Reference: AIFF specification and IEEE 754 extended precision format.
 */
static void sample_rate_to_extended(uint32_t rate, uint8_t *buf)
{
    // AIFF extended format: 16-bit exponent + 64-bit mantissa
    uint16_t exponent = 0;
    uint64_t mantissa = 0;

    // Map common sample rates to their IEEE 754 80-bit extended format
    // Format: exponent is biased by 16383 (0x3FFF)
    // For integer N: exponent = 0x3FFF + floor(log2(N)), mantissa = N << (63 - floor(log2(N)))
    switch (rate) {
        case 8000:   // 8 kHz: 2^12.96... ≈ 8000
            exponent = 0x400B;  // 16383 + 12
            mantissa = 0xFA00000000000000ULL;
            break;
        case 11025:  // 11.025 kHz: 2^13.43... ≈ 11025
            exponent = 0x400C;  // 16383 + 13
            mantissa = 0xAC44000000000000ULL;
            break;
        case 16000:  // 16 kHz: 2^13.96... ≈ 16000
            exponent = 0x400C;  // 16383 + 13
            mantissa = 0xFA00000000000000ULL;
            break;
        case 22050:  // 22.05 kHz: 2^14.43... ≈ 22050
            exponent = 0x400D;  // 16383 + 14
            mantissa = 0xAC44000000000000ULL;
            break;
        case 44100:  // 44.1 kHz: 2^15.43... ≈ 44100
            exponent = 0x400E;  // 16383 + 15
            mantissa = 0xAC44000000000000ULL;
            break;
        case 48000:  // 48 kHz: 2^15.55... ≈ 48000
            exponent = 0x400E;  // 16383 + 15
            mantissa = 0xBB80000000000000ULL;
            break;
        default: {
            // Generic calculation for other rates
            // Find highest bit position (log2 approximation)
            int highest_bit = 0;
            uint32_t temp = rate;
            while (temp > 1) {
                temp >>= 1;
                highest_bit++;
            }
            exponent = 0x3FFF + highest_bit;
            // Normalize: shift rate to fill MSB of 64-bit mantissa
            mantissa = ((uint64_t)rate) << (63 - highest_bit);
            break;
        }
    }

    // Write 16-bit exponent in big-endian format
    buf[0] = (exponent >> 8) & 0xff;
    buf[1] = exponent & 0xff;
    
    // Write 64-bit mantissa in big-endian format
    buf[2] = (mantissa >> 56) & 0xff;
    buf[3] = (mantissa >> 48) & 0xff;
    buf[4] = (mantissa >> 40) & 0xff;
    buf[5] = (mantissa >> 32) & 0xff;
    buf[6] = (mantissa >> 24) & 0xff;
    buf[7] = (mantissa >> 16) & 0xff;
    buf[8] = (mantissa >> 8) & 0xff;
    buf[9] = mantissa & 0xff;
}

/**
 * @brief Initialize AIFF encoder
 */
static int aiff_encoder_init(AudioEncoderState *self, uint16_t sample_rate,
                             uint16_t bit_depth, uint8_t channels,
                             const char *filename)
{
    AiffEncoderState *state = (AiffEncoderState *)self;

    if (!state || !filename) {
        return PISSTVPP2_ERR_ARG_VALUE_INVALID;
    }

    state->sample_rate = sample_rate;
    state->bit_depth = bit_depth;
    state->channels = channels;

    // Open output file for writing
    state->fp = fopen(filename, "wb");
    if (!state->fp) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "Failed to open AIFF output file: %s", filename);
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    return PISSTVPP2_OK;
}

/**
 * @brief Write AIFF file header
 *
 * This is called after all audio data has been encoded.
 * We write headers with correct sizes.
 */
static int aiff_encoder_write_header(AiffEncoderState *state, uint32_t audio_sample_count)
{
    if (!state || !state->fp) {
        return PISSTVPP2_ERR_AUDIO_AIFF;
    }

    // Calculate sizes
    uint32_t bytes_per_sample = state->channels * (state->bit_depth / 8);
    uint32_t audio_data_size = audio_sample_count * bytes_per_sample;
    uint32_t form_size = 4 + 8 + 18 + 8 + audio_data_size;

    // Rewind to beginning to write headers
    if (fseek(state->fp, 0, SEEK_SET) != 0) {
        error_log(PISSTVPP2_ERR_AUDIO_AIFF, "AIFF header seek", "Failed to seek in output file");
        return PISSTVPP2_ERR_AUDIO_AIFF;
    }

    // FORM chunk
    if (fputs("FORM", state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;

    // Form size (big-endian)
    if (fputc((form_size >> 24) & 0xff, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc((form_size >> 16) & 0xff, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc((form_size >> 8) & 0xff, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc(form_size & 0xff, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;

    if (fputs("AIFF", state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;

    // COMM chunk (Common)
    if (fputs("COMM", state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;

    // COMM chunk size (always 18 for PCM)
    if (fputc(0, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc(0, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc(0, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc(18, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;

    // Number of channels (big-endian)
    if (fputc(0, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc(state->channels, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;

    // Number of sample frames (big-endian)
    if (fputc((audio_sample_count >> 24) & 0xff, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc((audio_sample_count >> 16) & 0xff, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc((audio_sample_count >> 8) & 0xff, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc(audio_sample_count & 0xff, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;

    // Bits per sample (big-endian)
    if (fputc(0, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc(state->bit_depth, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;

    // Sample rate (80-bit extended precision format)
    uint8_t ext_rate[10];
    sample_rate_to_extended(state->sample_rate, ext_rate);
    for (int i = 0; i < 10; i++) {
        if (fputc(ext_rate[i], state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    }

    // SSND chunk (Sound Data)
    if (fputs("SSND", state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;

    // SSND chunk size (big-endian)
    uint32_t ssnd_size = 8 + audio_data_size;
    if (fputc((ssnd_size >> 24) & 0xff, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc((ssnd_size >> 16) & 0xff, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc((ssnd_size >> 8) & 0xff, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc(ssnd_size & 0xff, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;

    // Offset (always 0 for uncompressed)
    if (fputc(0, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc(0, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc(0, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc(0, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;

    // Block size (always 0 for uncompressed)
    if (fputc(0, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc(0, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc(0, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;
    if (fputc(0, state->fp) == EOF) return PISSTVPP2_ERR_AUDIO_AIFF;

    return PISSTVPP2_OK;
}

/**
 * @brief Encode audio samples to AIFF format
 *
 * For AIFF format, writes raw PCM data in big-endian format.
 */
static int aiff_encoder_encode(AudioEncoderState *self, const uint16_t *samples,
                               uint32_t sample_count)
{
    AiffEncoderState *state = (AiffEncoderState *)self;

    if (!state || !state->fp || !samples) {
        return PISSTVPP2_ERR_ARG_VALUE_INVALID;
    }

    if (sample_count == 0) {
        return PISSTVPP2_OK;
    }

    // For AIFF, write raw PCM data (big-endian int16)
    const size_t BYTES_PER_SAMPLE = 2;
    const size_t CHUNK_BYTES = 8192;
    uint8_t *buf = malloc(CHUNK_BYTES);

    if (!buf) {
        error_log(PISSTVPP2_ERR_MEMORY_ALLOC, "Failed to allocate AIFF write buffer (%zu bytes)", CHUNK_BYTES);
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
            // AIFF uses big-endian byte order
            buf[out_index++] = (uint8_t)((s & 0xff00) >> 8);
            buf[out_index++] = (uint8_t)(s & 0x00ff);
        }

        size_t wrote = fwrite(buf, 1, out_index, state->fp);
        if (wrote != out_index) {
            error_log(PISSTVPP2_ERR_FILE_WRITE, "Failed to write AIFF audio data (wrote %zu/%zu bytes)", wrote, out_index);
            result = PISSTVPP2_ERR_FILE_WRITE;
        }

        samples_remaining -= samples_this;
    }

    free(buf);
    return result;
}

/**
 * @brief Finalize AIFF encoding
 *
 * Writes the final AIFF headers with correct sizes and closes the file.
 */
static int aiff_encoder_finish(AudioEncoderState *self)
{
    AiffEncoderState *state = (AiffEncoderState *)self;

    if (!state || !state->fp) {
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    // Get file size to calculate audio sample count
    if (fseek(state->fp, 0, SEEK_END) != 0) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "Failed to seek to end of AIFF file");
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    long file_size = ftell(state->fp);
    if (file_size < 0) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "Failed to get AIFF file size");
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    // Audio data starts at byte 54 in AIFF file (after headers)
    long audio_data_size = file_size - 54;
    if (audio_data_size < 0) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "Invalid AIFF file size: %ld", file_size);
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    // Calculate sample count
    uint32_t bytes_per_sample = state->channels * (state->bit_depth / 8);
    uint32_t sample_count = audio_data_size / bytes_per_sample;

    // Write correct header with size information
    if (aiff_encoder_write_header(state, sample_count) != 0) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "Failed to write AIFF header");
        return PISSTVPP2_ERR_AUDIO_AIFF;
    }

    // Flush and close file
    if (fflush(state->fp) != 0 || ferror(state->fp)) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "Failed to flush AIFF output file");
        fclose(state->fp);
        state->fp = NULL;
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    if (fclose(state->fp) != 0) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "Failed to close AIFF output file");
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    state->fp = NULL;
    return PISSTVPP2_OK;
}

/**
 * @brief Destroy AIFF encoder
 */
static void aiff_encoder_destroy(AudioEncoderState *self)
{
    AiffEncoderState *state = (AiffEncoderState *)self;

    if (state) {
        if (state->fp) {
            fclose(state->fp);
        }
        free(state);
    }
}

/**
 * @brief Create AIFF encoder instance
 */
AudioEncoder* audio_encoder_aiff_create(void)
{
    AudioEncoder *encoder = malloc(sizeof(AudioEncoder));
    if (!encoder) {
        return NULL;
    }

    AiffEncoderState *state = malloc(sizeof(AiffEncoderState));
    if (!state) {
        free(encoder);
        return NULL;
    }

    memset(state, 0, sizeof(AiffEncoderState));

    encoder->init = aiff_encoder_init;
    encoder->encode = aiff_encoder_encode;
    encoder->finish = aiff_encoder_finish;
    encoder->destroy = aiff_encoder_destroy;
    encoder->state = (AudioEncoderState *)state;

    return encoder;
}
