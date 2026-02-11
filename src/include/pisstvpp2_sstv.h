/*
 * pisstvpp2_sstv.h
 *
 * SSTV Audio Encoding Module for PiSSTVpp2
 *
 * This module handles all SSTV (Slow Scan Television) encoding functionality:
 * - Audio tone synthesis and modulation
 * - SSTV mode encoding (Martin 1/2, Scottie 1/2/DX, Robot 36/72)
 * - VIS header/trailer generation
 * - CW Morse code signature embedding
 *
 * Architecture:
 * - sstv_init(): Initialize audio buffer and synthesis parameters
 * - sstv_set_mode(): Configure SSTV protocol and audio parameters
 * - sstv_encode_frame(): Generate SSTV audio for the loaded image
 * - sstv_add_cw_signature(): Append CW signature to audio stream
 * - sstv_get_samples(): Retrieve generated audio samples
 * - sstv_cleanup(): Release resources
 *
 * Global State:
 * - Audio buffer (g_audio): PCM sample buffer
 * - Sample counter (g_samples): Current number of samples generated
 * - Synthesis state (g_theta, g_fudge): Phase continuity and timing
 *
 * Dependencies:
 * - pisstvpp2_image.h: For image pixel data access
 * - error.h: For unified error code system
 */

#ifndef PISSTVPP2_SSTV_H
#define PISSTVPP2_SSTV_H

#include <stdint.h>
#include <stdio.h>
#include "error.h"

/* ============================================================================
   SSTV AUDIO CONSTANTS
   ============================================================================ */

/**
 * @defgroup AudioConfig Audio Configuration Constants
 * Default values for audio generation
 * @{
 */
#define SSTV_DEFAULT_RATE       22050   /**< Default sample rate (Hz) */
#define SSTV_MAX_RATE           48000   /**< Maximum sample rate (Hz) */
#define SSTV_BITS               16      /**< Audio bit depth per sample */
#define SSTV_CHANS              1       /**< Audio channel count (mono) */
#define SSTV_VOLPCT             65      /**< Volume as % of max (65 = appropriate for SSTV) */
/** @} */

/**
 * @defgroup AudioBuffering Audio Buffer Sizing
 * @{
 */
#define SSTV_MAX_SAMPLES        (600 * SSTV_MAX_RATE * 2)  /**< Max samples: 10 min @ 48kHz x2 */
/** @} */

/* ============================================================================
   SSTV PROTOCOL IDENTIFIERS
   ============================================================================ */

/**
 * VIS (Vertical Interval Signal) codes for supported SSTV modes.
 * These are the 6-bit protocol identifiers transmitted in the VIS header.
 */
enum {
    SSTV_MARTIN_1   = 44,   /**< Martin 1 */
    SSTV_MARTIN_2   = 40,   /**< Martin 2 */
    SSTV_SCOTTIE_1  = 60,   /**< Scottie 1 */
    SSTV_SCOTTIE_2  = 56,   /**< Scottie 2 */
    SSTV_SCOTTIE_DX = 76,   /**< Scottie DX */
    SSTV_ROBOT_36   = 8,    /**< Robot 36 Color */
    SSTV_ROBOT_72   = 12    /**< Robot 72 Color */
};

/* ============================================================================
   MODULE INITIALIZATION AND CONFIGURATION
   ============================================================================ */

/**
 * sstv_init
 *
 * Initialize the SSTV encoding module.
 *
 * @param sample_rate Audio sample rate in Hz (8000-48000)
 * @param verbose If non-zero, print initialization messages
 * @param timestamp_logging If non-zero, add millisecond-precision timestamps to verbose output
 *
 * @return Error code: PISSTVPP2_OK on success, or:
 *   - PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE: Sample rate out of valid range
 *   - PISSTVPP2_ERR_MEMORY_ALLOC: Failed to allocate audio buffer
 *   - PISSTVPP2_ERR_SSTV_INIT: Initialization failed
 *
 * Effects:
 * - Allocates audio buffer if needed
 * - Initializes synthesis parameters (scaling, phase accumulator)
 * - Validates sample rate
 * - Clears any previous audio data
 */
int sstv_init(uint16_t sample_rate, int verbose, int timestamp_logging);

/**
 * sstv_set_protocol
 *
 * Set the SSTV protocol/mode to use for encoding.
 *
 * @param protocol VIS code (SSTV_MARTIN_1, SSTV_ROBOT_36, etc.)
 *
 * Effects:
 * - Stores protocol for use in addvisheader()
 * - Validates protocol code
 */
void sstv_set_protocol(uint8_t protocol);

/**
 * sstv_get_protocol
 *
 * Get the currently configured SSTV protocol.
 *
 * @return VIS protocol code
 */
uint8_t sstv_get_protocol(void);

/* ============================================================================
   SSTV FRAME ENCODING
   ============================================================================ */

/**
 * sstv_encode_frame
 *
 * Encode a complete SSTV transmission frame from the loaded image.
 *
 * Responsibilities:
 * - Add VIS header to audio stream
 * - Select and invoke appropriate mode encoder
 * - Add VIS trailer
 *
 * @param verbose If non-zero, print progress per scan line
 * @param timestamp_logging If non-zero, add millisecond-precision timestamps to verbose output
 *
 * @return Error code: PISSTVPP2_OK on success, or:
 *   - PISSTVPP2_ERR_SSTV_INIT: Module not initialized
 *   - PISSTVPP2_ERR_SSTV_MODE_NOT_FOUND: SSTV mode not recognized
 *   - PISSTVPP2_ERR_SSTV_ENCODE: Audio encoding failed
 *   - PISSTVPP2_ERR_IMAGE_LOAD: No image loaded
 *
 * Effects:
 * - Appends audio samples to global buffer
 * - Updates g_samples counter
 *
 * Notes:
 * - Requires a valid image to be loaded (via pisstvpp2_image module)
 * - Requires SSTV protocol to be set (via sstv_set_protocol)
 */
int sstv_encode_frame(int verbose, int timestamp_logging);

/**
 * sstv_add_cw_signature
 *
 * Append a CW Morse code signature to the audio stream.
 *
 * @param callsign Amateur radio callsign (e.g., "N0CALL")
 * @param wpm Words per minute (typical: 5-20, default: 15)
 * @param tone_freq CW tone frequency in Hz (typical: 400-2000, default: 800)
 *
 * Effects:
 * - Appends silence then Morse code tones to audio buffer
 * - Message format: "SSTV de <callsign>"
 * - Uses envelope shaping to avoid clicks
 */
void sstv_add_cw_signature(const char *callsign, int wpm, uint16_t tone_freq);

/* ============================================================================
   AUDIO BUFFER ACCESS
   ============================================================================ */

/**
 * sstv_get_samples
 *
 * Retrieve pointer to audio sample buffer.
 *
 * @param sample_count Output: pointer to receive number of valid samples
 *
 * @return Pointer to audio buffer (uint16_t array), or NULL if not initialized
 *
 * Notes:
 * - Buffer contains 16-bit PCM samples
 * - Buffer is not copied; pointer is to internal state
 * - Do not modify buffer contents
 */
const uint16_t *sstv_get_samples(uint32_t *sample_count);

/**
 * sstv_get_sample_count
 *
 * Get the number of audio samples currently in the buffer.
 *
 * @return Number of valid samples
 */
uint32_t sstv_get_sample_count(void);

/**
 * sstv_get_sample_rate
 *
 * Get the configured audio sample rate.
 *
 * @return Sample rate in Hz
 */
uint16_t sstv_get_sample_rate(void);

/* ============================================================================
   CLEANUP AND DIAGNOSTICS
   ============================================================================ */

/**
 * sstv_cleanup
 *
 * Release all resources allocated by the SSTV module.
 *
 * Effects:
 * - Frees audio buffer
 * - Resets all state variables
 * - Safe to call multiple times
 */
void sstv_cleanup(void);

/**
 * sstv_reset_buffer
 *
 * Clear the audio buffer without deallocating memory.
 *
 * Useful for re-encoding with the same sample rate.
 *
 * Effects:
 * - Sets g_samples to 0
 * - Resets phase accumulator and fudge factor
 */
void sstv_reset_buffer(void);

/**
 * sstv_get_mode_details
 *
 * Print details about an SSTV mode to stdout.
 *
 * @param protocol VIS code (SSTV_MARTIN_1, etc.)
 *
 * Prints:
 * - Mode name and abbreviation
 * - Resolution (lines and pixels per line)
 * - TX time estimate
 */
void sstv_get_mode_details(uint8_t protocol, int verbose, int timestamp_logging);

#endif /* PISSTVPP2_SSTV_H */
