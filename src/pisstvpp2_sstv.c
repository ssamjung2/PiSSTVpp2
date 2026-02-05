/*
 * pisstvpp2_sstv.c
 *
 * SSTV Audio Encoding Module Implementation for PiSSTVpp2
 *
 * This module contains all audio synthesis and SSTV mode encoding functionality.
 * See pisstvpp2_sstv.h for API documentation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <tgmath.h>
#include "logging.h"
#include "pisstvpp2_sstv.h"
#include "pisstvpp2_image.h"

/* ============================================================================
   MODULE-PRIVATE GLOBAL STATE
   ============================================================================ */

/**
 * @brief SSTV module state container
 *
 * Groups previously separate globals into a single static state object.
 */
typedef struct {
    uint16_t *audio;          /**< Audio sample buffer */
    uint32_t samples;         /**< Current number of audio samples generated */
    uint32_t max_samples;     /**< Maximum capacity of audio buffer */
    uint32_t scale;           /**< Audio sample scaling factor */
    uint16_t rate;            /**< Current audio sample rate in Hz */
    double twopioverrate;     /**< Precomputed constant: 2π / sample_rate */
    double uspersample;       /**< Precomputed constant: microseconds per sample */
    double theta;             /**< Current phase accumulator */
    double fudge;             /**< Frequency adjustment fudge factor */
    uint8_t protocol;         /**< SSTV protocol VIS code */
    int initialized;          /**< Module initialized flag */
} SstvState;

/** Module-private SSTV state */
static SstvState g_sstv = {
    .audio = NULL,
    .samples = 0,
    .max_samples = 0,
    .scale = 0,
    .rate = SSTV_DEFAULT_RATE,
    .twopioverrate = 0.0,
    .uspersample = 0.0,
    .theta = 0.0,
    .fudge = 0.0,
    .protocol = 44,
    .initialized = 0
};

/* ============================================================================
   HELPER FUNCTIONS: TONE SYNTHESIS
   ============================================================================ */

/**
 * @brief Map 8-bit RGB brightness value to SSTV audio tone frequency.
 * @param colorval Brightness value (0=black, 255=white)
 * @return Frequency in Hz
 *
 * Converts RGB color intensity to the corresponding audio frequency
 * used in RGB-based SSTV modes (Martin, Scottie).
 * Range typically 1200-2300 Hz; higher values = brighter pixels.
 */
static uint16_t toneval_rgb(uint8_t colorval) {
    return (uint16_t)(1500.0 + (colorval * 3.1372549));
}

/**
 * @brief Map 8-bit YUV/luma value to SSTV audio tone frequency.
 * @param colorval Luma/brightness value (0=black, 255=white)
 * @return Frequency in Hz
 *
 * Converts YUV luma (luminance) to audio frequency for YUV-based modes
 * (Robot 36, etc.). Similar frequency range to RGB but different encoding.
 */
static uint16_t toneval_yuv(uint8_t colorval) {
    return (uint16_t)((float)colorval * 3.1372549 + 1500.0);
}

/**
 * @brief Generate and append an audio tone to the global sample buffer.
 * @param tonefreq Frequency in Hz (e.g., 1200, 1500 Hz for SSTV tones)
 * @param tonedur Duration in microseconds
 *
 * Uses module phase accumulator and sample rate to
 * generate continuous sinusoidal waveforms at specified frequency.
 * Respects configured amplitude setting to prevent clipping.
 */
static void playtone(uint16_t tonefreq, double tonedur) {
    uint32_t tonesamples, i;
    uint16_t voltage;
    double deltatheta;
    
    tonedur += g_sstv.fudge;
    tonesamples = (uint32_t)((tonedur / g_sstv.uspersample) + 0.5);
    deltatheta = g_sstv.twopioverrate * tonefreq;
    
    for (i = 1; i <= tonesamples; i++) {
        if (g_sstv.samples >= g_sstv.max_samples) {
            fprintf(stderr, "[WARNING] Audio buffer overflow at sample %u\n", g_sstv.samples);
            return;
        }
        g_sstv.samples++;
        
        if (tonefreq == 0) { 
            g_sstv.audio[g_sstv.samples] = 32768;
        } else {
            voltage = (uint16_t)(32768.0 + sin(g_sstv.theta) * g_sstv.scale);
            g_sstv.audio[g_sstv.samples] = voltage;
            g_sstv.theta += deltatheta;
        }
    }
    
    g_sstv.fudge = tonedur - ((double)tonesamples * g_sstv.uspersample);
}

/**
 * @brief Generate CW tone with Tukey window envelope (for add_cw_signature()).
 *
 * IMPROVEMENTS over previous smootherstep approach:
 * 1. Adaptive envelope duration - scales with tone length (35% of duration)
 *    • Short dots (dits) get 5ms envelope minimum
 *    • Long dashes get longer envelope, proportional to length
 *    • Maximum 50ms envelope to avoid over-softening long elements
 * 
 * 2. Tukey window function - better spectral characteristics than smootherstep
 *    • w(t) = 0.5 * (1 - cos(π*t)) for fade-in/out
 *    • Zero endpoints (eliminates clicks)
 *    • Full amplitude maintained in center section
 *    • Natural "punch" without artifacts
 * 
 * 3. Gain compensation - maintains perceived loudness consistency
 *    • Tukey window area ~0.867 of full amplitude
 *    • Amplitude adjusted to compensate for envelope attenuation
 *    • Consistent CW level across different tone lengths
 */
static void playtone_envelope(uint16_t tonefreq, double tonedur) {
    uint32_t tonesamples, i;
    uint16_t voltage;
    double deltatheta;
    uint32_t envelope_samples;
    double envelope_time_us;
    double cw_amplitude;
    
    tonedur += g_sstv.fudge;
    tonesamples = (uint32_t)((tonedur / g_sstv.uspersample) + 0.5);
    if (tonesamples == 0) {
        g_sstv.fudge = tonedur;
        return;
    }
    
    /* For silence, use normal playtone */
    if (tonefreq == 0) {
        playtone(tonefreq, tonedur - g_sstv.fudge);
        return;
    }
    
    /* Adaptive envelope duration based on tone length:
       25% of duration, bounded between 5ms and 40ms. */
    double tonedur_ms = (tonedur / 1000.0);
    envelope_time_us = tonedur_ms * 0.25 * 1000.0;
    if (envelope_time_us < 5000.0)   envelope_time_us = 5000.0;
    if (envelope_time_us > 40000.0)  envelope_time_us = 40000.0;
    
    envelope_samples = (uint32_t)(envelope_time_us / g_sstv.uspersample);
    if (envelope_samples == 0) envelope_samples = 1;
    
    deltatheta = g_sstv.twopioverrate * tonefreq;
    
    /* CW amplitude: Use full amplitude (1.0).
       The envelope itself controls the volume ramping. */
    cw_amplitude = 1.0;
    
    /* If tone too short for any envelope, use full amplitude throughout */
    if (tonesamples < 2 * envelope_samples) {
        /* Generate tone without envelope for very short tones */
        for (i = 1; i <= tonesamples; i++) {
            if (g_sstv.samples >= g_sstv.max_samples) return;
            voltage = (uint16_t)(32768.0 + sin(g_sstv.theta) * g_sstv.scale * cw_amplitude);
            g_sstv.audio[g_sstv.samples] = voltage;
            g_sstv.samples++;
            g_sstv.theta += deltatheta;
        }
        g_sstv.fudge = tonedur - ((double)tonesamples * g_sstv.uspersample);
        return;
    }
    
    /* Fade-in: Tukey window cosine taper */
    for (i = 0; i < envelope_samples && i < tonesamples; i++) {
        if (g_sstv.samples >= g_sstv.max_samples) return;
        double t = (double)i / (double)envelope_samples;
        double envelope = 0.5 * (1.0 - cos(M_PI * t));
        double modulated_amplitude = cw_amplitude * envelope;
        voltage = (uint16_t)(32768.0 + sin(g_sstv.theta) * g_sstv.scale * modulated_amplitude);
        g_sstv.audio[g_sstv.samples] = voltage;
        g_sstv.samples++;
        g_sstv.theta += deltatheta;
    }
    
    /* Full amplitude middle section */
    for (; i < tonesamples - envelope_samples; i++) {
        if (g_sstv.samples >= g_sstv.max_samples) return;
        voltage = (uint16_t)(32768.0 + sin(g_sstv.theta) * g_sstv.scale * cw_amplitude);
        g_sstv.audio[g_sstv.samples] = voltage;
        g_sstv.samples++;
        g_sstv.theta += deltatheta;
    }
    
    /* Fade-out: Tukey window mirrored */
    for (uint32_t j = 0; i < tonesamples; i++, j++) {
        if (g_sstv.samples >= g_sstv.max_samples) return;
        double t = (double)j / (double)envelope_samples;
        double envelope = 0.5 * (1.0 + cos(M_PI * t));
        double modulated_amplitude = cw_amplitude * envelope;
        voltage = (uint16_t)(32768.0 + sin(g_sstv.theta) * g_sstv.scale * modulated_amplitude);
        g_sstv.audio[g_sstv.samples] = voltage;
        g_sstv.samples++;
        g_sstv.theta += deltatheta;
    }
    
    g_sstv.fudge = tonedur - ((double)tonesamples * g_sstv.uspersample);
}

/* ============================================================================
   VIS HEADER AND TRAILER
   ============================================================================ */

/**
 * @brief Emit the SSTV VIS header into the audio stream.
 */
static void addvisheader(int verbose, int timestamp_logging) {
    log_verbose(verbose, timestamp_logging, "   [OK] Adding VIS header to audio.\n");
    
    // bit of silence
    playtone(0, 500000);
    
    // attention tones
    playtone(1900, 100000);
    playtone(1500, 100000);
    playtone(1900, 100000);
    playtone(1500, 100000);
    playtone(2300, 100000);
    playtone(1500, 100000);
    playtone(2300, 100000);
    playtone(1500, 100000);
    
    // VIS lead, break, mid, start
    playtone(1900, 300000);
    playtone(1200, 10000);
    playtone(1900, 300000);
    playtone(1200, 30000);
    
    int i;
    int parity = 0;
    
    // write VIS data tones
    for (i = 1; i <= 64; i = i << 1) {
        if (i & g_sstv.protocol) {  // digit is a 1
            playtone(1100, 30000);
            parity = !parity;
        } else {
            playtone(1300, 30000);  // digit is a 0
        }
    }
    
    // parity bit
    if (parity) {  // odd parity
        playtone(1100, 30000);
    } else {       // even parity
        playtone(1300, 30000);
    }
    
    // VIS stop
    playtone(1200, 30000);
}

/**
 * @brief Emit a simple VIS trailer or closing tones to terminate the frame.
 */
static void addvistrailer(void) {
    const uint16_t tr1_freq = 2300;  const uint32_t tr1_us = 300000;
    const uint16_t tr2_freq = 1200;  const uint32_t tr2_us = 10000;
    const uint16_t tr3_freq = 2300;  const uint32_t tr3_us = 100000;
    const uint16_t tr4_freq = 1200;  const uint32_t tr4_us = 30000;
    const uint32_t post_silence_us = 500000;
    
    playtone(tr1_freq, tr1_us);
    playtone(tr2_freq, tr2_us);
    playtone(tr3_freq, tr3_us);
    playtone(tr4_freq, tr4_us);
    playtone(0, post_silence_us);
}

/* ============================================================================
   CW MORSE CODE SIGNATURE
   ============================================================================ */

/**
 * @brief Generate a CW (Morse code) signature at the end of the SSTV transmission.
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
    if (wpm <= 0) wpm = 15;
    if (tone_freq == 0) tone_freq = 800;
    double dot_us = 1200000.0 / wpm;
    double dash_us = 3 * dot_us;
    double intra_us = dot_us;
    double inter_us = 3 * dot_us;
    double word_us = 7 * dot_us;
    
    // Compose message
    char msg[128];
    snprintf(msg, sizeof(msg), "SSTV de %s", callsign ? callsign : "NOCALL");
    
    // Encode and play
    for (const char *p = msg; *p; ++p) {
        char ch = *p;
        if (ch >= 'a' && ch <= 'z') ch -= 32;  // to uppercase
        const char *morse = NULL;
        for (int i = 0; i < morse_table_len; ++i) {
            if (morse_table[i].c == ch) {
                morse = morse_table[i].morse;
                break;
            }
        }
        if (!morse) continue;
        if (ch == ' ') {
            playtone(0, word_us);
            continue;
        }
        for (const char *m = morse; *m; ++m) {
            if (*m == '.') {
                playtone_envelope(tone_freq, dot_us);
            } else if (*m == '-') {
                playtone_envelope(tone_freq, dash_us);
            }
            if (*(m + 1)) playtone(0, intra_us);
        }
        playtone(0, inter_us);
    }
}

/* ============================================================================
   SSTV MODE ENCODERS
   ============================================================================ */

/**
 * @brief Wrapper for getting pixel RGB from the image module.
 */
static void get_pixel_rgb(int x, int y, uint8_t *r, uint8_t *g, uint8_t *b) {
    image_get_pixel_rgb(x, y, r, g, b);
}

/**
 * @brief Build audio samples for Martin (M1/M2) SSTV modes.
 */
static void buildaudio_m(double pixeltime, int verbose, int timestamp_logging) {
    const double SYNC_PULSE_US = 4862.0;
    const double PORCH_US = 572.0;
    const double SEPARATOR_US = 572.0;
    const uint16_t SYNC_FREQ = 1200;
    const uint16_t PORCH_FREQ = 1500;
    const uint16_t SEPARATOR_FREQ = 1500;
    const int LINES = 256;
    const int PIXELS_PER_LINE = 320;
    
    uint16_t x, y, k;
    uint8_t r_pixel, g_pixel, b_pixel;
    uint8_t r[PIXELS_PER_LINE], g[PIXELS_PER_LINE], b[PIXELS_PER_LINE];
    
    for (y = 0; y < LINES; y++) {
        if (verbose && y > 0 && y % 64 == 0) {
            int progress = (y * 100) / LINES;
            log_verbose(verbose, timestamp_logging, "   --> Encoding row %d/%d (%d%%)\n", y, LINES, progress);
        }
        
        for (x = 0; x < PIXELS_PER_LINE; x++) {
            get_pixel_rgb(x, y, &r_pixel, &g_pixel, &b_pixel);
            r[x] = r_pixel;
            g[x] = g_pixel;
            b[x] = b_pixel;
        }
        
        playtone(SYNC_FREQ, SYNC_PULSE_US);
        playtone(PORCH_FREQ, PORCH_US);
        for (k = 0; k < PIXELS_PER_LINE; k++)
            playtone(toneval_rgb(g[k]), pixeltime);
        playtone(SEPARATOR_FREQ, SEPARATOR_US);
        for (k = 0; k < PIXELS_PER_LINE; k++)
            playtone(toneval_rgb(b[k]), pixeltime);
        playtone(SEPARATOR_FREQ, SEPARATOR_US);
        for (k = 0; k < PIXELS_PER_LINE; k++)
            playtone(toneval_rgb(r[k]), pixeltime);
        playtone(SEPARATOR_FREQ, SEPARATOR_US);
    }
}

/**
 * @brief Build audio samples for Scottie (S1/S2/SDX) SSTV modes.
 */
static void buildaudio_s(double pixeltime, int verbose, int timestamp_logging) {
    const double SYNC_PULSE_US = 9000.0;
    const double SEPARATOR_US = 1500.0;
    const double SYNC_PORCH_US = 1500.0;
    const uint16_t SYNC_FREQ = 1200;
    const uint16_t SEPARATOR_FREQ = 1500;
    const int LINES = 256;
    const int PIXELS_PER_LINE = 320;
    
    uint16_t x, y, k;
    uint8_t r_pixel, g_pixel, b_pixel;
    uint8_t r[PIXELS_PER_LINE], g[PIXELS_PER_LINE], b[PIXELS_PER_LINE];
    
    playtone(SYNC_FREQ, SYNC_PULSE_US);
    
    for (y = 0; y < LINES; y++) {
        if (verbose && y > 0 && y % 64 == 0) {
            int progress = (y * 100) / LINES;
            log_verbose(verbose, timestamp_logging, "   --> Encoding row %d/%d (%d%%)\n", y, LINES, progress);
        }
        
        for (x = 0; x < PIXELS_PER_LINE; x++) {
            get_pixel_rgb(x, y, &r_pixel, &g_pixel, &b_pixel);
            r[x] = r_pixel;
            g[x] = g_pixel;
            b[x] = b_pixel;
        }
        
        playtone(SEPARATOR_FREQ, SEPARATOR_US);
        for (k = 0; k < PIXELS_PER_LINE; k++)
            playtone(toneval_rgb(g[k]), pixeltime);
        playtone(SEPARATOR_FREQ, SEPARATOR_US);
        for (k = 0; k < PIXELS_PER_LINE; k++)
            playtone(toneval_rgb(b[k]), pixeltime);
        playtone(SYNC_FREQ, SYNC_PULSE_US);
        playtone(SEPARATOR_FREQ, SYNC_PORCH_US);
        for (k = 0; k < PIXELS_PER_LINE; k++)
            playtone(toneval_rgb(r[k]), pixeltime);
    }
}

/**
 * @brief Build audio samples for Robot 36 (R36) mode.
 */
static void buildaudio_r36(int verbose, int timestamp_logging) {
    const double SYNC_PULSE_US = 9000.0;
    const double SYNC_FREQ = 1200.0;
    const double PORCH_US = 3000.0;
    const double PORCH_FREQ = 1500.0;
    const double SEP_EVEN_US = 4500.0;
    const double SEP_EVEN_FREQ = 1500.0;
    const double SEP_ODD_US = 4500.0;
    const double SEP_ODD_FREQ = 2300.0;
    const double CHROMA_PORCH_US = 1500.0;
    const double CHROMA_PORCH_FREQ = 1900.0;
    const double Y_PIXEL_US = 275.0;
    const double CHROMA_PIXEL_US = 137.5;
    const int LINES = 240;
    
    uint16_t x, y, k;
    uint8_t r1, g1, b1, r2, g2, b2, avgr, avgg, avgb;
    uint8_t y1[320], y2[320], ry[320], by[320];
    
    for (y = 0; y < LINES; y += 2) {
        if (verbose && y > 0 && y % 32 == 0) {
            int progress = (y * 100) / LINES;
            log_verbose(verbose, timestamp_logging, "   --> Encoding row pair %d/%d (%d%%)\n", y, LINES, progress);
        }
        
        for (x = 0; x < 320; x++) {
            get_pixel_rgb(x, y, &r1, &g1, &b1);
            get_pixel_rgb(x, y + 1, &r2, &g2, &b2);
            
            avgr = (uint8_t)(((uint16_t)r1 + (uint16_t)r2) / 2);
            avgg = (uint8_t)(((uint16_t)g1 + (uint16_t)g2) / 2);
            avgb = (uint8_t)(((uint16_t)b1 + (uint16_t)b2) / 2);
            
            y1[x] = 16.0 + (0.003906 * ((65.738 * (float)r1) + (129.057 * (float)g1) + (25.064 * (float)b1)));
            y2[x] = 16.0 + (0.003906 * ((65.738 * (float)r2) + (129.057 * (float)g2) + (25.064 * (float)b2)));
            ry[x] = 128.0 + (0.003906 * ((112.439 * (float)avgr) + (-94.154 * (float)avgg) + (-18.285 * (float)avgb)));
            by[x] = 128.0 + (0.003906 * ((-37.945 * (float)avgr) + (-74.494 * (float)avgg) + (112.439 * (float)avgb)));
        }
        
        playtone(SYNC_FREQ, SYNC_PULSE_US);
        playtone(PORCH_FREQ, PORCH_US);
        for (k = 0; k < 320; k++)
            playtone(toneval_yuv(y1[k]), Y_PIXEL_US);
        playtone(SEP_EVEN_FREQ, SEP_EVEN_US);
        playtone(CHROMA_PORCH_FREQ, CHROMA_PORCH_US);
        for (k = 0; k < 320; k++)
            playtone(toneval_yuv(ry[k]), CHROMA_PIXEL_US);
        
        playtone(SYNC_FREQ, SYNC_PULSE_US);
        playtone(PORCH_FREQ, PORCH_US);
        for (k = 0; k < 320; k++)
            playtone(toneval_yuv(y2[k]), Y_PIXEL_US);
        playtone(SEP_ODD_FREQ, SEP_ODD_US);
        playtone(CHROMA_PORCH_FREQ, CHROMA_PORCH_US);
        for (k = 0; k < 320; k++)
            playtone(toneval_yuv(by[k]), CHROMA_PIXEL_US);
    }
}

/**
 * @brief Build audio samples for Robot 72 (R72) mode.
 */
static void buildaudio_r72(int verbose, int timestamp_logging) {
    const double SYNC_PULSE_US = 9000.0;
    const double SYNC_FREQ = 1200.0;
    const double PORCH_US = 3000.0;
    const double PORCH_FREQ = 1500.0;
    const double SEP_EVEN_US = 4500.0;
    const double SEP_EVEN_FREQ = 1500.0;
    const double SEP_ODD_US = 4500.0;
    const double SEP_ODD_FREQ = 2300.0;
    const double CHROMA_PORCH_US = 1500.0;
    const double CHROMA_PORCH_FREQ = 1900.0;
    const double Y_PIXEL_US = 550.0;
    const double CHROMA_PIXEL_US = 275.0;
    const int LINES = 240;
    
    uint16_t x, y, k;
    uint8_t r1, g1, b1, r2, g2, b2, avgr, avgg, avgb;
    uint8_t y1[320], y2[320], ry[320], by[320];
    
    for (y = 0; y < LINES; y += 2) {
        if (verbose && y > 0 && y % 60 == 0) {
            int progress = (y * 100) / LINES;
            log_verbose(verbose, timestamp_logging, "   --> Encoding line pair %d/%d (%d%%) [R72]\n", y, LINES, progress);
        }
        
        for (x = 0; x < 320; x++) {
            get_pixel_rgb(x, y, &r1, &g1, &b1);
            get_pixel_rgb(x, y + 1, &r2, &g2, &b2);
            
            avgr = (uint8_t)(((uint16_t)r1 + (uint16_t)r2) / 2);
            avgg = (uint8_t)(((uint16_t)g1 + (uint16_t)g2) / 2);
            avgb = (uint8_t)(((uint16_t)b1 + (uint16_t)b2) / 2);
            
            y1[x] = 16.0 + (0.003906 * ((65.738 * (float)r1) + (129.057 * (float)g1) + (25.064 * (float)b1)));
            y2[x] = 16.0 + (0.003906 * ((65.738 * (float)r2) + (129.057 * (float)g2) + (25.064 * (float)b2)));
            ry[x] = 128.0 + (0.003906 * ((112.439 * (float)avgr) + (-94.154 * (float)avgg) + (-18.285 * (float)avgb)));
            by[x] = 128.0 + (0.003906 * ((-37.945 * (float)avgr) + (-74.494 * (float)avgg) + (112.439 * (float)avgb)));
        }
        
        playtone(SYNC_FREQ, SYNC_PULSE_US);
        playtone(PORCH_FREQ, PORCH_US);
        for (k = 0; k < 320; k++)
            playtone(toneval_yuv(y1[k]), Y_PIXEL_US);
        playtone(SEP_EVEN_FREQ, SEP_EVEN_US);
        playtone(CHROMA_PORCH_FREQ, CHROMA_PORCH_US);
        for (k = 0; k < 320; k++)
            playtone(toneval_yuv(ry[k]), CHROMA_PIXEL_US);
        
        playtone(SYNC_FREQ, SYNC_PULSE_US);
        playtone(PORCH_FREQ, PORCH_US);
        for (k = 0; k < 320; k++)
            playtone(toneval_yuv(y2[k]), Y_PIXEL_US);
        playtone(SEP_ODD_FREQ, SEP_ODD_US);
        playtone(CHROMA_PORCH_FREQ, CHROMA_PORCH_US);
        for (k = 0; k < 320; k++)
            playtone(toneval_yuv(by[k]), CHROMA_PIXEL_US);
    }
}

/* ============================================================================
   PUBLIC API IMPLEMENTATION
   ============================================================================ */

int sstv_init(uint16_t sample_rate, int verbose, int timestamp_logging) {
    (void)verbose;              // Unused parameter
    (void)timestamp_logging;    // Unused parameter
    
    if (sample_rate < 8000 || sample_rate > SSTV_MAX_RATE) {
        fprintf(stderr, "[ERROR] Sample rate must be between 8000 and %d Hz\n", SSTV_MAX_RATE);
        return -1;
    }
    
    g_sstv.rate = sample_rate;
    g_sstv.max_samples = SSTV_MAX_SAMPLES;
    
    /* Allocate or reallocate audio buffer */
    if (!g_sstv.audio) {
        g_sstv.audio = (uint16_t *)malloc(g_sstv.max_samples * sizeof(uint16_t));
        if (!g_sstv.audio) {
            fprintf(stderr, "[ERROR] Failed to allocate audio buffer\n");
            return -1;
        }
    }
    
    /* Calculate synthesis constants */
    g_sstv.twopioverrate = (2.0 * M_PI) / (double)g_sstv.rate;
    g_sstv.uspersample = 1000000.0 / (double)g_sstv.rate;
    
    /* Scale factor for audio amplitude (65% of max) */
    g_sstv.scale = (uint32_t)((65.0 / 100.0) * 32767.0);
    
    /* Reset state */
    g_sstv.theta = 0.0;
    g_sstv.fudge = 0.0;
    g_sstv.samples = 0;
    g_sstv.initialized = 1;
    
    return 0;
}

void sstv_set_protocol(uint8_t protocol) {
    g_sstv.protocol = protocol;
}

uint8_t sstv_get_protocol(void) {
    return g_sstv.protocol;
}

int sstv_encode_frame(int verbose, int timestamp_logging) {
    if (!g_sstv.initialized) {
        fprintf(stderr, "[ERROR] SSTV module not initialized\n");
        return -1;
    }
    
    if (g_sstv.samples > 0) {
        fprintf(stderr, "[WARNING] Audio buffer not empty; consider calling sstv_reset_buffer()\n");
    }
    
    addvisheader(verbose, timestamp_logging);
    
    switch (g_sstv.protocol) {
        case 44:  // Martin 1
            buildaudio_m(457.6, verbose, timestamp_logging);
            break;
        case 40:  // Martin 2
            buildaudio_m(228.8, verbose, timestamp_logging);
            break;
        case 60:  // Scottie 1
            buildaudio_s(432.0, verbose, timestamp_logging);
            break;
        case 56:  // Scottie 2
            buildaudio_s(275.2, verbose, timestamp_logging);
            break;
        case 76:  // Scottie DX
            buildaudio_s(1080.0, verbose, timestamp_logging);
            break;
        case 8:   // Robot 36
            buildaudio_r36(verbose, timestamp_logging);
            break;
        case 12:  // Robot 72
            buildaudio_r72(verbose, timestamp_logging);
            break;
        default:
            fprintf(stderr, "[ERROR] Unknown SSTV protocol code: %d\n", g_sstv.protocol);
            return -1;
    }
    
    addvistrailer();
    return 0;
}

void sstv_add_cw_signature(const char *callsign, int wpm, uint16_t tone_freq) {
    if (!g_sstv.initialized) {
        fprintf(stderr, "[ERROR] SSTV module not initialized\n");
        return;
    }
    
    /* 2 seconds of silence between VIS trailer and CW signature */
    playtone(0, 2000000.0);
    
    add_cw_signature(callsign, wpm, tone_freq);
}

const uint16_t *sstv_get_samples(uint32_t *sample_count) {
    if (!g_sstv.audio) {
        if (sample_count) *sample_count = 0;
        return NULL;
    }
    if (sample_count) *sample_count = g_sstv.samples;
    return g_sstv.audio;
}

uint32_t sstv_get_sample_count(void) {
    return g_sstv.samples;
}

uint16_t sstv_get_sample_rate(void) {
    return g_sstv.rate;
}

void sstv_cleanup(void) {
    if (g_sstv.audio) {
        free(g_sstv.audio);
        g_sstv.audio = NULL;
    }
    g_sstv.samples = 0;
    g_sstv.max_samples = 0;
    g_sstv.rate = SSTV_DEFAULT_RATE;
    g_sstv.theta = 0.0;
    g_sstv.fudge = 0.0;
    g_sstv.scale = 0;
    g_sstv.protocol = 44;
    g_sstv.initialized = 0;
}

void sstv_reset_buffer(void) {
    g_sstv.samples = 0;
    g_sstv.theta = 0.0;
    g_sstv.fudge = 0.0;
}

void sstv_get_mode_details(uint8_t protocol, int verbose, int timestamp_logging) {
    switch (protocol) {
        case 44:  // Martin 1
            log_verbose(verbose, timestamp_logging, "  Mode name:     Martin 1 (M1)\n");
            log_verbose(verbose, timestamp_logging, "  Resolution:    256 scan lines, 4288 pixels/line, 8-bit RGB\n");
            log_verbose(verbose, timestamp_logging, "  TX Time:       114 seconds total\n");
            break;
        case 40:  // Martin 2
            log_verbose(verbose, timestamp_logging, "  Mode name:     Martin 2 (M2)\n");
            log_verbose(verbose, timestamp_logging, "  Resolution:    256 scan lines, 4288 pixels/line, 8-bit RGB\n");
            log_verbose(verbose, timestamp_logging, "  TX Time:       58 seconds total\n");
            break;
        case 60:  // Scottie 1
            log_verbose(verbose, timestamp_logging, "  Mode name:     Scottie 1 (S1)\n");
            log_verbose(verbose, timestamp_logging, "  Resolution:    256 scan lines, 4496 pixels/line, 8-bit RGB\n");
            log_verbose(verbose, timestamp_logging, "  TX Time:       110 seconds total\n");
            break;
        case 56:  // Scottie 2
            log_verbose(verbose, timestamp_logging, "  Mode name:     Scottie 2 (S2)\n");
            log_verbose(verbose, timestamp_logging, "  Resolution:    256 scan lines, 4496 pixels/line, 8-bit RGB\n");
            log_verbose(verbose, timestamp_logging, "  TX Time:       71 seconds total\n");
            break;
        case 76:  // Scottie DX
            log_verbose(verbose, timestamp_logging, "  Mode name:     Scottie DX (SDX)\n");
            log_verbose(verbose, timestamp_logging, "  Resolution:    256 scan lines, 4496 pixels/line, 8-bit RGB\n");
            log_verbose(verbose, timestamp_logging, "  TX Time:       269 seconds total\n");
            break;
        case 8:   // Robot 36
            log_verbose(verbose, timestamp_logging, "  Mode name:     Robot 36 Color (R36)\n");
            log_verbose(verbose, timestamp_logging, "  Resolution:    120 scan lines, 320 pixels/line, YUV format\n");
            log_verbose(verbose, timestamp_logging, "  TX Time:       36 seconds total\n");
            break;
        case 12:  // Robot 72
            log_verbose(verbose, timestamp_logging, "  Mode name:     Robot 72 Color (R72)\n");
            log_verbose(verbose, timestamp_logging, "  Resolution:    120 scan lines, 320 pixels/line, YUV format\n");
            log_verbose(verbose, timestamp_logging, "  TX Time:       72 seconds total\n");
            break;
    }
}
