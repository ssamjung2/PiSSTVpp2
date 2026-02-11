/**
 * @file audio_encoder_ogg.c
 * @brief OGG Vorbis format audio encoder
 *
 * Implements OGG Vorbis audio encoding for PiSSTVpp2 using libvorbis.
 * Produces compressed OGG files with excellent quality and patent-free compression.
 *
 * Requires: libvorbis-dev, libogg-dev
 * Install: apt-get install libvorbis-dev libogg-dev
 */

#include "pisstvpp2_audio_encoder.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

/* Conditional compilation for OGG support */
#ifdef HAVE_OGG_SUPPORT

#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>

/**
 * @brief OGG Vorbis encoder state
 */
typedef struct {
    FILE *fp;                   /**< Output file pointer */
    vorbis_info vi;             /**< Vorbis format information */
    vorbis_comment vc;          /**< Vorbis comment metadata */
    vorbis_dsp_state vd;        /**< Vorbis DSP state */
    vorbis_block vb;            /**< Vorbis block */
    ogg_stream_state os;        /**< OGG stream state */
    uint16_t sample_rate;       /**< Sample rate in Hz */
    uint16_t bit_depth;         /**< Bits per sample */
    uint8_t channels;           /**< Number of channels */
    int eos;                    /**< End-of-stream flag */
} OggEncoderState;

/**
 * @brief Helper: Write OGG page to file
 */
static int write_ogg_page(OggEncoderState *state, ogg_page *page)
{
    if (!state || !state->fp || !page) {
        return PISSTVPP2_ERR_AUDIO_OGG;
    }

    size_t wrote = fwrite(page->header, 1, page->header_len, state->fp);
    if (wrote != (size_t)page->header_len) {
        error_log(PISSTVPP2_ERR_AUDIO_OGG, "OGG page header write", 
                 "Failed to write OGG page header (%zu/%u bytes)", wrote, page->header_len);
        return PISSTVPP2_ERR_AUDIO_OGG;
    }

    wrote = fwrite(page->body, 1, page->body_len, state->fp);
    if (wrote != (size_t)page->body_len) {
        error_log(PISSTVPP2_ERR_AUDIO_OGG, "OGG page body write",
                 "Failed to write OGG page body (%zu/%u bytes)", wrote, page->body_len);
        return PISSTVPP2_ERR_AUDIO_OGG;
    }

    return PISSTVPP2_OK;
}

/**
 * @brief Initialize OGG Vorbis encoder
 *
 * Sets up Vorbis encoder with audio parameters and opens output file.
 */
static int ogg_encoder_init(AudioEncoderState *self, uint16_t sample_rate,
                            uint16_t bit_depth, uint8_t channels,
                            const char *filename)
{
    OggEncoderState *state = (OggEncoderState *)self;

    if (!state || !filename) {
        return PISSTVPP2_ERR_ARG_VALUE_INVALID;
    }

    state->sample_rate = sample_rate;
    state->bit_depth = bit_depth;
    state->channels = channels;
    state->eos = 0;

    // Open output file for writing
    state->fp = fopen(filename, "wb");
    if (!state->fp) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "Failed to open OGG output file: %s", filename);
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    // Initialize Vorbis structures
    vorbis_info_init(&state->vi);
    
    // Configure encoder for SSTV audio
    // Quality 0.6 provides good compression (~160 kbps equivalent) suitable for SSTV
    int vorbis_ret = vorbis_encode_init_vbr(&state->vi, (long)channels, (long)sample_rate, 0.6f);
    if (vorbis_ret != 0) {
        error_log(PISSTVPP2_ERR_AUDIO_OGG, "Failed to initialize Vorbis encoder (error code: %d)", vorbis_ret);
        fclose(state->fp);
        state->fp = NULL;
        vorbis_info_clear(&state->vi);
        return PISSTVPP2_ERR_AUDIO_OGG;
    }

    // Initialize comment structure
    vorbis_comment_init(&state->vc);
    vorbis_comment_add_tag(&state->vc, "ENCODER", "PiSSTVpp2");
    vorbis_comment_add_tag(&state->vc, "DESCRIPTION", "SSTV Audio Stream");

    // Initialize DSP state
    if (vorbis_analysis_init(&state->vd, &state->vi) != 0) {
        error_log(PISSTVPP2_ERR_AUDIO_OGG, "Failed to initialize Vorbis DSP state");
        fclose(state->fp);
        state->fp = NULL;
        vorbis_comment_clear(&state->vc);
        vorbis_info_clear(&state->vi);
        return PISSTVPP2_ERR_AUDIO_OGG;
    }

    // Initialize block
    if (vorbis_block_init(&state->vd, &state->vb) != 0) {
        error_log(PISSTVPP2_ERR_AUDIO_OGG, "Failed to initialize Vorbis block");
        fclose(state->fp);
        state->fp = NULL;
        vorbis_block_clear(&state->vb);
        vorbis_dsp_clear(&state->vd);
        vorbis_comment_clear(&state->vc);
        vorbis_info_clear(&state->vi);
        return PISSTVPP2_ERR_AUDIO_OGG;
    }

    // Initialize OGG stream state with random serial number
    srand((unsigned int)time(NULL));
    int serial = rand();
    if (ogg_stream_init(&state->os, serial) != 0) {
        error_log(PISSTVPP2_ERR_AUDIO_OGG, "Failed to initialize OGG stream");
        fclose(state->fp);
        state->fp = NULL;
        vorbis_block_clear(&state->vb);
        vorbis_dsp_clear(&state->vd);
        vorbis_comment_clear(&state->vc);
        vorbis_info_clear(&state->vi);
        return PISSTVPP2_ERR_AUDIO_OGG;
    }

    // Write Vorbis headers
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;
    ogg_page page;

    if (vorbis_analysis_headerout(&state->vd, &state->vc, &header,
                                  &header_comm, &header_code) != 0) {
        error_log(PISSTVPP2_ERR_AUDIO_OGG, "Failed to generate Vorbis headers");
        return PISSTVPP2_ERR_AUDIO_OGG;
    }

    // Put headers in OGG stream
    ogg_stream_packetin(&state->os, &header);
    ogg_stream_packetin(&state->os, &header_comm);
    ogg_stream_packetin(&state->os, &header_code);

    // Flush header pages
    while (ogg_stream_flush(&state->os, &page) != 0) {
        if (write_ogg_page(state, &page) != 0) {
            error_log(PISSTVPP2_ERR_AUDIO_OGG, "Failed to write OGG header page");
            return PISSTVPP2_ERR_AUDIO_OGG;
        }
    }

    return PISSTVPP2_OK;
}

/**
 * @brief Encode audio samples to OGG Vorbis format
 *
 * Takes PCM audio samples and encodes them to OGG Vorbis frames.
 * Processes samples in chunks to avoid stack overflow with large buffers.
 */
static int ogg_encoder_encode(AudioEncoderState *self, const uint16_t *samples,
                              uint32_t sample_count)
{
    OggEncoderState *state = (OggEncoderState *)self;

    if (!state || !state->fp || !samples) {
        return PISSTVPP2_ERR_ARG_VALUE_INVALID;
    }

    if (sample_count == 0) {
        return PISSTVPP2_OK;
    }

    // Process samples in chunks to avoid stack overflow
    // Vorbis analysis_buffer allocates on stack, so large requests can crash
    const uint32_t CHUNK_SIZE = 4096;  // Process 4K samples at a time
    uint32_t offset = 0;

    while (offset < sample_count) {
        uint32_t chunk_samples = (sample_count - offset) > CHUNK_SIZE ? 
                                 CHUNK_SIZE : (sample_count - offset);

        // Get buffer for writing samples
        float **buffer = vorbis_analysis_buffer(&state->vd, chunk_samples);
        if (!buffer) {
            error_log(PISSTVPP2_ERR_AUDIO_OGG, "Failed to get Vorbis analysis buffer");
            return PISSTVPP2_ERR_AUDIO_OGG;
        }

        // Convert int16 samples to float for Vorbis
        for (uint32_t i = 0; i < chunk_samples; i++) {
            int16_t sample_int = (int16_t)samples[offset + i];
            buffer[0][i] = (float)sample_int / 32768.0f;
        }

        // Tell Vorbis how many samples we wrote
        vorbis_analysis_wrote(&state->vd, chunk_samples);

        // Extract and encode blocks
        ogg_packet packet;
        ogg_page page;

        while (vorbis_analysis_blockout(&state->vd, &state->vb) == 1) {
            vorbis_analysis(&state->vb, NULL);
            vorbis_bitrate_addblock(&state->vb);

            while (vorbis_bitrate_flushpacket(&state->vd, &packet)) {
                ogg_stream_packetin(&state->os, &packet);

                while (ogg_stream_pageout(&state->os, &page) != 0) {
                    if (write_ogg_page(state, &page) != 0) {
                        error_log(PISSTVPP2_ERR_AUDIO_OGG, "Failed to write OGG page");
                        return PISSTVPP2_ERR_AUDIO_OGG;
                    }

                    if (ogg_page_eos(&page)) {
                        state->eos = 1;
                    }
                }
            }
        }

        offset += chunk_samples;
    }

    return PISSTVPP2_OK;
}

/**
 * @brief Finalize OGG Vorbis encoding
 *
 * Flushes any remaining encoded data and closes the file.
 */
static int ogg_encoder_finish(AudioEncoderState *self)
{
    OggEncoderState *state = (OggEncoderState *)self;

    if (!state || !state->fp) {
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    // Tell Vorbis that we're done adding samples
    vorbis_analysis_wrote(&state->vd, 0);

    ogg_packet packet;
    ogg_page page;

    // Finish any remaining blocks
    while (vorbis_analysis_blockout(&state->vd, &state->vb) == 1) {
        vorbis_analysis(&state->vb, NULL);
        vorbis_bitrate_addblock(&state->vb);

        while (vorbis_bitrate_flushpacket(&state->vd, &packet)) {
            ogg_stream_packetin(&state->os, &packet);

            while (ogg_stream_pageout(&state->os, &page) != 0) {
                if (write_ogg_page(state, &page) != 0) {
                    error_log(PISSTVPP2_ERR_AUDIO_OGG, "Failed to write final OGG page");
                    return PISSTVPP2_ERR_AUDIO_OGG;
                }
            }
        }
    }

    // Flush any remaining pages
    while (ogg_stream_flush(&state->os, &page) != 0) {
        if (write_ogg_page(state, &page) != 0) {
            error_log(PISSTVPP2_ERR_AUDIO_OGG, "Failed to write final OGG pages");
            return PISSTVPP2_ERR_AUDIO_OGG;
        }
    }

    // Flush and close file
    if (fflush(state->fp) != 0 || ferror(state->fp)) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "Failed to flush OGG output file");
        fclose(state->fp);
        state->fp = NULL;
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    if (fclose(state->fp) != 0) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "Failed to close OGG output file");
        return PISSTVPP2_ERR_FILE_WRITE;
    }

    state->fp = NULL;
    return PISSTVPP2_OK;
}

/**
 * @brief Destroy OGG Vorbis encoder
 */
static void ogg_encoder_destroy(AudioEncoderState *self)
{
    OggEncoderState *state = (OggEncoderState *)self;

    if (state) {
        ogg_stream_clear(&state->os);
        vorbis_block_clear(&state->vb);
        vorbis_dsp_clear(&state->vd);
        vorbis_comment_clear(&state->vc);
        vorbis_info_clear(&state->vi);

        if (state->fp) {
            fclose(state->fp);
            state->fp = NULL;
        }

        free(state);
    }
}

/**
 * @brief Create OGG Vorbis encoder instance
 */
AudioEncoder* audio_encoder_ogg_create(void)
{
    AudioEncoder *encoder = malloc(sizeof(AudioEncoder));
    if (!encoder) {
        return NULL;
    }

    OggEncoderState *state = malloc(sizeof(OggEncoderState));
    if (!state) {
        free(encoder);
        return NULL;
    }

    memset(state, 0, sizeof(OggEncoderState));

    encoder->init = ogg_encoder_init;
    encoder->encode = ogg_encoder_encode;
    encoder->finish = ogg_encoder_finish;
    encoder->destroy = ogg_encoder_destroy;
    encoder->state = (AudioEncoderState *)state;

    return encoder;
}

#else  /* !HAVE_OGG_SUPPORT */

/**
 * @brief Stub: OGG encoder creation when libvorbis not available
 */
AudioEncoder* audio_encoder_ogg_create(void)
{
    fprintf(stderr, "[ERROR] OGG Vorbis support not compiled in. Install libvorbis-dev and rebuild.\n");
    return NULL;
}

#endif  /* HAVE_OGG_SUPPORT */
