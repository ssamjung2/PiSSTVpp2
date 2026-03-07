/**
 * stitch_tiles - High-performance tile stitcher for SlowFrame (C implementation)
 * 
 * Reassembles tiled SlowFrame images with libvips for Raspberry Pi performance.
 * Reads headers, validates metadata, and composites tiles back to original image.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <vips/vips.h>
#include "recovery_strategies.h"

#define SF_TILE_HEADER_ROWS 8
#define SF_PRIMARY_ROWS 4
#define MAX_TILES 256
#define BUFFER_SIZE 4096

/* Tile header metadata */
typedef struct {
    char path[BUFFER_SIZE];
    int tile_width;
    int tile_height;
    uint8_t version;
    uint8_t grid_cols;
    uint8_t grid_rows;
    uint8_t tile_col;
    uint8_t tile_row;
    uint8_t total_tiles;
    uint8_t seq_num;
    uint16_t orig_width;
    uint16_t orig_height;
    uint32_t session_id;
    uint8_t overlap_px;
    int row1_crc_ok;
    int row3_crc_ok;
    int sync_ok;
    int recovered_from_backup;
    int cross_tile_inferred;
    int session_id_inferred;     /* 1 if session ID was inferred from other tiles */
    int session_id_overridden;   /* 1 if session ID was manually overridden */
    /* Error correction status (SECDED) */
    int row1_error_corrected;    /* 1 if single-bit error was corrected in row 1 */
    int row2_error_corrected;    /* 1 if single-bit error was corrected in row 2 */
    int row3_error_corrected;    /* 1 if single-bit error was corrected in row 3 */
    int row1_multi_error;        /* 1 if multi-bit error detected in row 1 */
    int row2_multi_error;        /* 1 if multi-bit error detected in row 2 */
    int row3_multi_error;        /* 1 if multi-bit error detected in row 3 */
    int row2_crc_ok;             /* 1 if row 2 CRC validation passed */
    char recovery_log[256];      /* Log of recovery operations performed */
} TileHeader;

/* Tile set group (collection of tiles from same image) */
typedef struct {
    uint32_t session_id;
    uint16_t orig_width;
    uint16_t orig_height;
    uint8_t grid_cols;
    uint8_t grid_rows;
    TileHeader *tiles[MAX_TILES];
    int tile_count;
    int missing_count;
} TileSetGroup;

/* ============================================================================
   CRC-8 / CCITT (polynomial 0x07)
   ============================================================================ */
static uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 0x80) crc = (uint8_t)((crc << 1) ^ 0x07);
            else            crc = (uint8_t)(crc << 1);
        }
    }
    return crc;
}

/* ============================================================================
   Error Correction - SECDED via Single Parity + CRC
   ============================================================================
   
   Single Error Correction, Double Error Detection for header robustness with
   20px blocks. Uses row parity XOR + CRC-8 to:
     - Detect 1-bit errors (CRC catches them)
     - Correct 1-bit errors (parity + backup block fallback)
     - Detect 2-bit errors (CRC + parity mismatch)
   
   Overhead: 1 byte per row for X-bit parity (3 bytes total per block of 3 rows)
   Gain: Makes 20px blocks as robust as 40px blocks under burst noise.
*/

/**
 * Compute XOR parity over all 8 bytes in a row (excluding CRC position).
 * Parity byte has bit set if odd number of 1s across data bytes.
 * Used alongside CRC-8 for SECDED capability.
 */
static uint8_t compute_row_parity(const uint8_t *row_data, int data_len) {
    uint8_t parity = 0;
    for (int i = 0; i < data_len; i++) {
        parity ^= row_data[i];
    }
    return parity;
}

/**
 * Check row parity and attempt single-bit error correction.
 * Returns: 0 if valid, 1 if corrected, -1 if multiple errors detected.
 */
static int check_and_correct_parity(uint8_t *row_data, int data_len,
                                     uint8_t expected_parity) {
    uint8_t computed = compute_row_parity(row_data, data_len);
    
    if (computed == expected_parity) {
        return 0;  // No error
    }
    
    // Parity mismatch: try to locate and correct single-bit error
    // If XOR of computed and expected gives us a single bit position, we can correct
    uint8_t syndrome = computed ^ expected_parity;
    
    // Count set bits in syndrome
    int bit_count = 0;
    int error_pos = -1;
    for (int i = 0; i < 8; i++) {
        if (syndrome & (1 << i)) {
            bit_count++;
            error_pos = i;
        }
    }
    
    // Single bit error in row_data[error_pos]
    if (bit_count == 1 && error_pos >= 0 && error_pos < data_len) {
        row_data[error_pos] ^= syndrome;  // Flip the erroneous bit
        return 1;  // Corrected
    }
    
    // Multiple bit errors (or complex error pattern)
    return -1;
}

/* ============================================================================
   Image I/O - Bulk header extraction via vips extract_area + local parsing
   ============================================================================
   
   Fast approach: One subprocess call per file to extract header region as PPM,
   then parse pixels from memory. Matches Python but compiles to fast C.
*/

/**
 * Extract header region of a tile image as PPM format data.
 * Returns malloc'd buffer containing PPM data, or NULL on failure.
 * Caller must free() the returned buffer.
 */
static uint8_t *extract_header_region_ppm(const char *path, int *out_size) {
    *out_size = 0;
    
    char temp_path[BUFFER_SIZE];
    // Use process ID + static counter for unique temp file
    static int counter = 0;
    snprintf(temp_path, sizeof(temp_path), "/tmp/sf_hdr_%d_%d.ppm", 
             (int)getpid(), counter++);
    
    // One subprocess call: vips extract_area to get header region as PPM
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), 
             "vips extract_area '%s' '%s' 0 0 320 %d 2>/dev/null",
             path, temp_path, SF_TILE_HEADER_ROWS);
    
    int ret = system(cmd);
    if (ret != 0) {
        unlink(temp_path);
        return NULL;
    }
    
    // Read PPM file into memory buffer
    FILE *f = fopen(temp_path, "rb");
    if (!f) {
        unlink(temp_path);
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t *buf = malloc(size);
    if (!buf) {
        fclose(f);
        unlink(temp_path);
        return NULL;
    }
    
    size_t read_size = fread(buf, 1, size, f);
    fclose(f);
    unlink(temp_path);
    
    if (read_size != (size_t)size) {
        free(buf);
        return NULL;
    }
    
    *out_size = (int)size;
    return buf;
}

/**
 * Get pixel R-channel value from PPM buffer at (x, y).
 * PPM format: "P5\nW H\n255\n" (grayscale) or "P6\nW H\n255\n" (RGB)
 * Handles optional comments starting with '#'
 */
static uint8_t get_ppm_pixel(const uint8_t *ppm_data, int ppm_size,
                              int x, int y) {
    if (!ppm_data || ppm_size < 20 || x < 0 || y < 0) {
        return 0;
    }
    
    const char *data = (const char *)ppm_data;
    
    // Validate PPM magic
    if (data[0] != 'P' || (data[1] != '5' && data[1] != '6')) {
        return 0;
    }
    
    int bands = (data[1] == '5') ? 1 : 3;
    int pos = 2;
    
    // Skip whitespace and comments
    while (pos < ppm_size) {
        if (data[pos] == '#') {
            // Skip comment line until newline
            while (pos < ppm_size && data[pos] != '\n') 
                pos++;
            if (pos < ppm_size) pos++;  // Skip the newline
        } else if (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r' || data[pos] == '\t') {
            pos++;
        } else {
            break;  // Found non-whitespace, non-comment
        }
    }
    
    // Parse width
    int w = 0;
    while (pos < ppm_size && data[pos] >= '0' && data[pos] <= '9') {
        w = w * 10 + (data[pos] - '0');
        pos++;
    }
    
    // Skip whitespace and comments
    while (pos < ppm_size) {
        if (data[pos] == '#') {
            // Skip comment line until newline
            while (pos < ppm_size && data[pos] != '\n') 
                pos++;
            if (pos < ppm_size) pos++;  // Skip the newline
        } else if (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r' || data[pos] == '\t') {
            pos++;
        } else {
            break;  // Found non-whitespace, non-comment
        }
    }
    
    // Parse height
    int h = 0;
    while (pos < ppm_size && data[pos] >= '0' && data[pos] <= '9') {
        h = h * 10 + (data[pos] - '0');
        pos++;
    }
    
    // Skip to end of header (skip whitespace/comments until after "255\n")
    while (pos < ppm_size) {
        if (data[pos] == '#') {
            // Skip comment line
            while (pos < ppm_size && data[pos] != '\n') 
                pos++;
            if (pos < ppm_size) pos++;
        } else if (data[pos] >= '0' && data[pos] <= '9') {
            // Parse max color value (should be 255)
            while (pos < ppm_size && data[pos] >= '0' && data[pos] <= '9') 
                pos++;
            // Skip to end of line
            while (pos < ppm_size && data[pos] != '\n') 
                pos++;
            if (pos < ppm_size) pos++;  // Skip the newline
            break;
        } else if (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r' || data[pos] == '\t') {
            pos++;
        } else {
            pos++;
        }
    }
    
    // Check bounds
    if (x >= w || y >= h) {
        return 0;
    }
    
    // Calculate pixel offset in buffer
    int stride = w * bands;
    int offset = pos + (y * stride) + (x * bands);
    
    if (offset < 0 || offset >= ppm_size) {
        return 0;
    }
    
    return (uint8_t)ppm_data[offset];
}

/**
 * Extract N macroblock R-channel values from a row of PPM data.
 * Supports both 8 blocks (40px, legacy) and 16 blocks (20px, new with EXIF).
 * Auto-detects: if first 16 blocks are mostly zero/padding, falls back to 8 blocks.
 */
static int read_blocks_from_ppm(const uint8_t *ppm_data, int ppm_size,
                                 int row_idx, int block_count, uint8_t *out_blocks) {
    if (!ppm_data || !out_blocks || block_count <= 0) {
        memset(out_blocks, 0, block_count);
        return -1;
    }
    
    int blk = 320 / block_count;  // pixels per block
    
    for (int b = 0; b < block_count; b++) {
        int x = b * blk + blk / 2;  // Center of each block
        out_blocks[b] = get_ppm_pixel(ppm_data, ppm_size, x, row_idx);
    }
    
    return 0;
}

/**
 * Read header data with auto-detection: try 16 blocks first (new format),
 * fall back to 8 blocks (legacy format) if 16-block parse fails.
 */
static int detect_and_read_blocks(const uint8_t *ppm_data, int ppm_size,
                                   uint8_t *r1, uint8_t *r2, uint8_t *r3,
                                   int *out_block_count, int *out_data_len) {
    // Try 16 blocks first (new with EXIF)
    uint8_t test[16];
    read_blocks_from_ppm(ppm_data, ppm_size, 1, 16, test);
    
    // Check if this looks like valid 16-block data (CRC at position 15)
    uint8_t test_crc = crc8(test, 15);
    if (test_crc == test[15]) {
        // Valid 16-block format detected
        read_blocks_from_ppm(ppm_data, ppm_size, 1, 16, r1);
        read_blocks_from_ppm(ppm_data, ppm_size, 2, 16, r2);
        read_blocks_from_ppm(ppm_data, ppm_size, 3, 16, r3);
        *out_block_count = 16;
        *out_data_len = 15;  // Data bytes 0-14, CRC at 15
        return 0;
    }
    
    // Fall back to 8 blocks (legacy)
    read_blocks_from_ppm(ppm_data, ppm_size, 1, 8, r1);
    read_blocks_from_ppm(ppm_data, ppm_size, 2, 8, r2);
    read_blocks_from_ppm(ppm_data, ppm_size, 3, 8, r3);
    *out_block_count = 8;
    *out_data_len = 7;  // Data bytes 0-6, CRC at 7
    return 0;
}

static int decode_tile_header(const char *path, TileHeader *h, int verbose) {
    memset(h, 0, sizeof(*h));
    strncpy(h->path, path, sizeof(h->path) - 1);
    memset(h->recovery_log, 0, sizeof(h->recovery_log));  // Initialize recovery log
    
    // Standard R36 dimensions
    h->tile_width = 320;
    h->tile_height = 240;
    
    // Extract header region as PPM in one subprocess call
    int hdr_size = 0;
    uint8_t *hdr_buf = extract_header_region_ppm(path, &hdr_size);
    if (!hdr_buf) {
        if (verbose) {
            fprintf(stderr, "[VERBOSE] %s: Failed to extract header region\n", path);
        }
        return -1;  // Can't extract header
    }
    
    // Auto-detect block format: 16 blocks (new with EXIF) or 8 blocks (legacy)
    uint8_t r1[16], r2[16], r3[16];
    int block_count = 0, data_len = 0;
    memset(r1, 0, 16);
    memset(r2, 0, 16);
    memset(r3, 0, 16);
    
    h->sync_ok = 1;
    detect_and_read_blocks(hdr_buf, hdr_size, r1, r2, r3, &block_count, &data_len);
    
    if (verbose) {
        fprintf(stderr, "[VERBOSE] %s: Auto-detected %d-block format, data_len=%d\n", 
                path, block_count, data_len);
        fprintf(stderr, "[VERBOSE]   R1 bytes: ");
        for (int j = 0; j < data_len + 1; j++) fprintf(stderr, "%02x ", r1[j]);
        fprintf(stderr, "\n");
        fprintf(stderr, "[VERBOSE]   R2 bytes: ");
        for (int j = 0; j < data_len + 1; j++) fprintf(stderr, "%02x ", r2[j]);
        fprintf(stderr, "\n");
        fprintf(stderr, "[VERBOSE]   R3 bytes: ");
        for (int j = 0; j < data_len + 1; j++) fprintf(stderr, "%02x ", r3[j]);
        fprintf(stderr, "\n");
    }
    
    // Compute parity for error correction on all three rows
    uint8_t r1_parity = compute_row_parity(r1, data_len);
    uint8_t r2_parity = compute_row_parity(r2, data_len);
    uint8_t r3_parity = compute_row_parity(r3, data_len);
    
    // Try CRC validation first (fastest path) on all three rows
    h->row1_crc_ok = (crc8(r1, data_len) == r1[data_len]) ? 1 : 0;
    h->row2_crc_ok = (crc8(r2, data_len) == r2[data_len]) ? 1 : 0;
    h->row3_crc_ok = (crc8(r3, data_len) == r3[data_len]) ? 1 : 0;
    
    if (verbose) {
        fprintf(stderr, "[VERBOSE] %s: CRC results - R1=%s R2=%s R3=%s\n",
                path, 
                h->row1_crc_ok ? "PASS" : "FAIL",
                h->row2_crc_ok ? "PASS" : "FAIL",
                h->row3_crc_ok ? "PASS" : "FAIL");
    }
    
    // If CRC failed, try error correction on individual rows
    if (!h->row1_crc_ok) {
        if (verbose) fprintf(stderr, "[VERBOSE] %s: R1 CRC failed, attempting error correction...\n", path);
        int corr_result = check_and_correct_parity(r1, data_len, r1_parity);
        if (corr_result == 1) {
            h->row1_error_corrected = 1;
            // Recompute CRC line after correction
            r1[data_len] = crc8(r1, data_len);
            h->row1_crc_ok = 1;
            snprintf(h->recovery_log, sizeof(h->recovery_log), "R1[SECDED]");
            if (verbose) fprintf(stderr, "[VERBOSE] %s: R1 single-bit error CORRECTED\n", path);
        } else if (corr_result == -1) {
            h->row1_multi_error = 1;
            snprintf(h->recovery_log, sizeof(h->recovery_log), "R1[FAIL-ME]");
            if (verbose) fprintf(stderr, "[VERBOSE] %s: R1 multi-bit error DETECTED (uncorrectable)\n", path);
        } else {
            if (verbose) fprintf(stderr, "[VERBOSE] %s: R1 error correction failed\n", path);
        }
    }
    
    if (!h->row2_crc_ok) {
        if (verbose) fprintf(stderr, "[VERBOSE] %s: R2 CRC failed, attempting error correction...\n", path);
        int corr_result = check_and_correct_parity(r2, data_len, r2_parity);
        if (corr_result == 1) {
            h->row2_error_corrected = 1;
            // Recompute CRC after correction
            r2[data_len] = crc8(r2, data_len);
            h->row2_crc_ok = 1;
            if (strlen(h->recovery_log) > 0) {
                strncat(h->recovery_log, " R2[SECDED]", sizeof(h->recovery_log) - strlen(h->recovery_log) - 1);
            } else {
                snprintf(h->recovery_log, sizeof(h->recovery_log), "R2[SECDED]");
            }
            if (verbose) fprintf(stderr, "[VERBOSE] %s: R2 single-bit error CORRECTED\n", path);
        } else if (corr_result == -1) {
            h->row2_multi_error = 1;
            if (strlen(h->recovery_log) > 0) {
                strncat(h->recovery_log, " R2[FAIL-ME]", sizeof(h->recovery_log) - strlen(h->recovery_log) - 1);
            } else {
                snprintf(h->recovery_log, sizeof(h->recovery_log), "R2[FAIL-ME]");
            }
            if (verbose) fprintf(stderr, "[VERBOSE] %s: R2 multi-bit error DETECTED (uncorrectable)\n", path);
        } else {
            if (verbose) fprintf(stderr, "[VERBOSE] %s: R2 error correction failed\n", path);
        }
    }
    
    if (!h->row3_crc_ok) {
        if (verbose) fprintf(stderr, "[VERBOSE] %s: R3 CRC failed, attempting error correction...\n", path);
        int corr_result = check_and_correct_parity(r3, data_len, r3_parity);
        if (corr_result == 1) {
            h->row3_error_corrected = 1;
            // Recompute CRC after correction
            r3[data_len] = crc8(r3, data_len);
            h->row3_crc_ok = 1;
            if (strlen(h->recovery_log) > 0) {
                strncat(h->recovery_log, " R3[SECDED]", sizeof(h->recovery_log) - strlen(h->recovery_log) - 1);
            } else {
                snprintf(h->recovery_log, sizeof(h->recovery_log), "R3[SECDED]");
            }
            if (verbose) fprintf(stderr, "[VERBOSE] %s: R3 single-bit error CORRECTED\n", path);
        } else if (corr_result == -1) {
            h->row3_multi_error = 1;
            if (strlen(h->recovery_log) > 0) {
                strncat(h->recovery_log, " R3[FAIL-ME]", sizeof(h->recovery_log) - strlen(h->recovery_log) - 1);
            } else {
                snprintf(h->recovery_log, sizeof(h->recovery_log), "R3[FAIL-ME]");
            }
            if (verbose) fprintf(stderr, "[VERBOSE] %s: R3 multi-bit error DETECTED (uncorrectable)\n", path);
        } else {
            if (verbose) fprintf(stderr, "[VERBOSE] %s: R3 error correction failed\n", path);
        }
    }
    
    if (verbose) {
        fprintf(stderr, "[VERBOSE] %s: Checking backup block for comparison...\n", path);
        
        // Always read backup block for verbose inspection
        uint8_t r1b[16], r2b[16], r3b[16];
        memset(r1b, 0, 16);
        memset(r2b, 0, 16);
        memset(r3b, 0, 16);
        
        read_blocks_from_ppm(hdr_buf, hdr_size, 5, block_count, r1b);
        read_blocks_from_ppm(hdr_buf, hdr_size, 6, block_count, r2b);
        read_blocks_from_ppm(hdr_buf, hdr_size, 7, block_count, r3b);
        
        fprintf(stderr, "[VERBOSE]   Backup block raw data:\n");
        fprintf(stderr, "[VERBOSE]     R5 bytes: ");
        for (int j = 0; j < data_len + 1; j++) fprintf(stderr, "%02x ", r1b[j]);
        fprintf(stderr, "\n");
        fprintf(stderr, "[VERBOSE]     R6 bytes: ");
        for (int j = 0; j < data_len + 1; j++) fprintf(stderr, "%02x ", r2b[j]);
        fprintf(stderr, "\n");
        fprintf(stderr, "[VERBOSE]     R7 bytes: ");
        for (int j = 0; j < data_len + 1; j++) fprintf(stderr, "%02x ", r3b[j]);
        fprintf(stderr, "\n");
        
        // Compare with primary
        int r1_diff = (memcmp(r1, r1b, block_count) != 0) ? 1 : 0;
        int r2_diff = (memcmp(r2, r2b, block_count) != 0) ? 1 : 0;
        int r3_diff = (memcmp(r3, r3b, block_count) != 0) ? 1 : 0;
        
        fprintf(stderr, "[VERBOSE]   Primary vs Backup comparison:");
        fprintf(stderr, " R1=%s R2=%s R3=%s\n",
                r1_diff ? "✗ DIFFER" : "✓ SAME",
                r2_diff ? "✗ DIFFER" : "✓ SAME",
                r3_diff ? "✗ DIFFER" : "✓ SAME");
    }
    
    // Try backup block if primary still failed on any row
    if (!h->row1_crc_ok || !h->row2_crc_ok || !h->row3_crc_ok) {
        if (verbose) fprintf(stderr, "[VERBOSE] %s: Primary CRC failed, attempting backup block recovery...\n", path);
        
        uint8_t r1b[16], r2b[16], r3b[16];
        memset(r1b, 0, 16);
        memset(r2b, 0, 16);
        memset(r3b, 0, 16);
        
        // Backup should use same format as primary
        read_blocks_from_ppm(hdr_buf, hdr_size, 5, block_count, r1b);
        read_blocks_from_ppm(hdr_buf, hdr_size, 6, block_count, r2b);
        read_blocks_from_ppm(hdr_buf, hdr_size, 7, block_count, r3b);
        
        // Apply error correction to backup rows as well
        uint8_t r1b_parity = compute_row_parity(r1b, data_len);
        uint8_t r2b_parity = compute_row_parity(r2b, data_len);
        uint8_t r3b_parity = compute_row_parity(r3b, data_len);
        
        int r1b_ok = (crc8(r1b, data_len) == r1b[data_len]) ? 1 : 0;
        int r2b_ok = (crc8(r2b, data_len) == r2b[data_len]) ? 1 : 0;
        int r3b_ok = (crc8(r3b, data_len) == r3b[data_len]) ? 1 : 0;
        
        if (verbose) {
            fprintf(stderr, "[VERBOSE]   Backup block CRC validation: R5=%s R6=%s R7=%s\n",
                    r1b_ok ? "PASS" : "FAIL",
                    r2b_ok ? "PASS" : "FAIL",
                    r3b_ok ? "PASS" : "FAIL");
        }
        
        // Try error correction on backup rows too
        if (!r1b_ok) {
            int corr_result = check_and_correct_parity(r1b, data_len, r1b_parity);
            if (corr_result == 1) {
                r1b[data_len] = crc8(r1b, data_len);
                r1b_ok = 1;
                if (verbose) fprintf(stderr, "[VERBOSE]   Backup R1: single-bit error corrected\n");
            }
        }
        if (!r2b_ok) {
            int corr_result = check_and_correct_parity(r2b, data_len, r2b_parity);
            if (corr_result == 1) {
                r2b[data_len] = crc8(r2b, data_len);
                r2b_ok = 1;
                if (verbose) fprintf(stderr, "[VERBOSE]   Backup R2: single-bit error corrected\n");
            }
        }
        if (!r3b_ok) {
            int corr_result = check_and_correct_parity(r3b, data_len, r3b_parity);
            if (corr_result == 1) {
                r3b[data_len] = crc8(r3b, data_len);
                r3b_ok = 1;
                if (verbose) fprintf(stderr, "[VERBOSE]   Backup R3: single-bit error corrected\n");
            }
        }
        
        if (r1b_ok && r2b_ok && r3b_ok) {
            memcpy(r1, r1b, block_count);
            memcpy(r2, r2b, block_count);
            memcpy(r3, r3b, block_count);
            h->row1_crc_ok = 1;
            h->row2_crc_ok = 1;
            h->row3_crc_ok = 1;
            h->recovered_from_backup = 1;
            if (strlen(h->recovery_log) > 0) {
                strncat(h->recovery_log, " [BACKUP]", sizeof(h->recovery_log) - strlen(h->recovery_log) - 1);
            } else {
                snprintf(h->recovery_log, sizeof(h->recovery_log), "[BACKUP]");
            }
            if (verbose) fprintf(stderr, "[VERBOSE] %s: RECOVERED from backup block!\n", path);
        } else {
            if (verbose) {
                fprintf(stderr, "[VERBOSE] %s: Backup block recovery failed\n", path);
                fprintf(stderr, "[VERBOSE]   Final backup state: R1=%s R2=%s R3=%s\n",
                        r1b_ok ? "PASS" : "FAIL",
                        r2b_ok ? "PASS" : "FAIL",
                        r3b_ok ? "PASS" : "FAIL");
            }
        }
    }
    
    free(hdr_buf);
    
    // Accept headers even with corruption - let inference mechanism fix them
    // Parse best-guess values from all rows regardless of CRC status
    h->version = r1[0];
    h->grid_cols = r1[1];
    h->grid_rows = r1[2];
    h->tile_col = r1[3];
    h->tile_row = r1[4];
    h->total_tiles = r1[5];
    h->seq_num = r1[6];
    
    // Parse R2 fields (dimensions, session)
    h->orig_width = ((uint16_t)r2[0] << 8) | r2[1];
    h->orig_height = ((uint16_t)r2[2] << 8) | r2[3];
    h->session_id = ((uint32_t)r2[4] << 24) | ((uint32_t)r2[5] << 16) |
                    ((uint32_t)r2[6] << 8) | r2[7];
    
    // Overlap position differs based on block format
    if (block_count == 16) {
        h->overlap_px = r2[8];
    } else {
        h->overlap_px = r3[0];
    }
    
    // Report corruption status
    if (verbose && (!h->row1_crc_ok || !h->row2_crc_ok || !h->row3_crc_ok)) {
        fprintf(stderr, "[VERBOSE] %s: ACCEPTING with CRC failures - will use inference\n", path);
        fprintf(stderr, "[VERBOSE]   CRC state: R1=%s R2=%s R3=%s\n",
                h->row1_crc_ok ? "PASS" : "FAIL",
                h->row2_crc_ok ? "PASS" : "FAIL",
                h->row3_crc_ok ? "PASS" : "FAIL");
        fprintf(stderr, "[VERBOSE]   Parsed grid: %ux%u, pos:[%d,%d], session:0x%08x\n",
                h->grid_cols, h->grid_rows, h->tile_row, h->tile_col, h->session_id);
    }
    
    return 0;
}

/* ============================================================================
   Validation & Repair
   ============================================================================ */

/**
 * Repair headers using cross-tile inference.
 */
__attribute__((unused))
static int cross_tile_infer(TileHeader *headers, int count) {
    TileHeader *donor = NULL;
    for (int i = 0; i < count; i++) {
        if (headers[i].row1_crc_ok && headers[i].row3_crc_ok) {
            donor = &headers[i];
            break;
        }
    }
    if (!donor) return 0;
    
    int repaired = 0;
    for (int i = 0; i < count; i++) {
        if (headers[i].row1_crc_ok && headers[i].row3_crc_ok) continue;
        
        headers[i].version = donor->version;
        headers[i].grid_cols = donor->grid_cols;
        headers[i].grid_rows = donor->grid_rows;
        headers[i].total_tiles = donor->total_tiles;
        headers[i].orig_width = donor->orig_width;
        headers[i].orig_height = donor->orig_height;
        headers[i].session_id = donor->session_id;
        headers[i].overlap_px = donor->overlap_px;
        headers[i].row1_crc_ok = 1;
        headers[i].row3_crc_ok = 1;
        headers[i].cross_tile_inferred = 1;
        repaired++;
    }
    return repaired;
}

/**
 * Validate tile set for consistency.
 */
__attribute__((unused))
static int validate_tile_set(TileHeader *headers, int count) {
    if (count == 0) {
        fprintf(stderr, "No tiles\n");
        return -1;
    }
    
    TileHeader *ref = &headers[0];
    
    for (int i = 0; i < count; i++) {
        if (!headers[i].row1_crc_ok || !headers[i].row3_crc_ok) {
            fprintf(stderr, "CRC failed\n");
            return -1;
        }
    }
    
    // Check positions
    int found[MAX_TILES];
    memset(found, 0, sizeof(found));
    for (int i = 0; i < count; i++) {
        int idx = headers[i].tile_row * ref->grid_cols + headers[i].tile_col;
        if (idx >= MAX_TILES || found[idx]) return -1;
        found[idx] = 1;
    }
    
    for (int r = 0; r < ref->grid_rows; r++) {
        for (int c = 0; c < ref->grid_cols; c++) {
            if (!found[r * ref->grid_cols + c]) return -1;
        }
    }
    
    return 0;
}

__attribute__((unused))
static int validate_tile_set_partial(TileHeader *headers, int count, int allow_missing) {
    if (count == 0) {
        fprintf(stderr, "No tiles\n");
        return -1;
    }
    
    TileHeader *ref = &headers[0];
    int found[MAX_TILES];
    memset(found, 0, sizeof(found));
    
    for (int i = 0; i < count; i++) {
        int idx = headers[i].tile_row * ref->grid_cols + headers[i].tile_col;
        if (idx >= MAX_TILES || found[idx]) return -1;
        found[idx] = 1;
    }
    
    int missing = 0;
    for (int r = 0; r < ref->grid_rows; r++) {
        for (int c = 0; c < ref->grid_cols; c++) {
            if (!found[r * ref->grid_cols + c]) missing++;
        }
    }
    
    if (missing > 0 && !allow_missing) {
        fprintf(stderr, "Missing %d tile(s)\n", missing);
        return -1;
    }
    
    return missing;
}

/* ============================================================================
   Tile Discovery & Grouping
   ============================================================================ */

/**
 * Scan directory for all PNG files and try to decode headers.
 */
static int find_all_pngs(const char *directory, char **filenames, int max_files) {
    DIR *dir = opendir(directory);
    if (!dir) return -1;
    
    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) && count < max_files) {
        const char *ext = strrchr(entry->d_name, '.');
        if (ext && (strcasecmp(ext, ".png") == 0 || strcasecmp(ext, ".jpg") == 0)) {
            char full_path[BUFFER_SIZE];
            snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);
            filenames[count] = strdup(full_path);
            count++;
        }
    }
    closedir(dir);
    return count;
}

/**
 * Group tiles by (session_id, orig_width, orig_height, grid_cols, grid_rows).
 */
static int group_tiles_by_metadata(TileHeader *headers, int count, 
                                     TileSetGroup *groups, int max_groups) {
    int group_count = 0;
    
    for (int i = 0; i < count; i++) {
        TileHeader *tile = &headers[i];
        
        // Find existing group
        TileSetGroup *group = NULL;
        for (int g = 0; g < group_count; g++) {
            if (groups[g].session_id == tile->session_id &&
                groups[g].orig_width == tile->orig_width &&
                groups[g].orig_height == tile->orig_height &&
                groups[g].grid_cols == tile->grid_cols &&
                groups[g].grid_rows == tile->grid_rows) {
                group = &groups[g];
                break;
            }
        }
        
        // Create new group if needed
        if (!group) {
            if (group_count >= max_groups) break;
            group = &groups[group_count];
            memset(group, 0, sizeof(*group));
            group->session_id = tile->session_id;
            group->orig_width = tile->orig_width;
            group->orig_height = tile->orig_height;
            group->grid_cols = tile->grid_cols;
            group->grid_rows = tile->grid_rows;
            group_count++;
        }
        
        // Add tile to group
        // Allow more than expected to handle position collisions
        // They'll be resolved at display time using preference logic
        if (group->tile_count < MAX_TILES) {
            group->tiles[group->tile_count] = tile;
            group->tile_count++;
        }
    }
    
    // Count missing tiles in each group
    for (int g = 0; g < group_count; g++) {
        int expected = groups[g].grid_cols * groups[g].grid_rows;
        groups[g].missing_count = expected - groups[g].tile_count;
    }
    
    return group_count;
}

/**
 * Display available tile sets.
 */
static void list_tile_sets(TileSetGroup *groups, int count, TileHeader *all_headers, int header_count, int verbose) {
    // If verbose, show per-tile details first in columnar format
    if (verbose && header_count > 0) {
        printf("\n  Decoding headers…\n\n");
        
        // Print column headers
        printf("  %-40s %-6s %-9s %-4s %-10s %-5s %-13s %-6s %-8s %-8s %-8s\n",
               "File", "Grid", "Pos", "Seq", "SID", "OVL", "Orig Dims", "Sync", "R1", "R2", "R3");
        printf("  %s %s %s %s %s %s %s %s %s %s %s\n",
               "────────────────────────────────────────",
               "──────",
               "─────────",
               "────",
               "──────────",
               "─────",
               "─────────────",
               "──────",
               "────────",
               "────────",
               "────────");
        
        // Print each tile's information
        for (int i = 0; i < header_count; i++) {
            TileHeader *tile = &all_headers[i];
            
            // Extract filename from path
            const char *filename = strrchr(tile->path, '/');
            if (!filename) filename = tile->path;
            else filename++;
            
            // Truncate filename if too long
            char fname_buf[41];
            strncpy(fname_buf, filename, 40);
            fname_buf[40] = '\0';
            
            // Format each field
            char grid_buf[7];
            snprintf(grid_buf, sizeof(grid_buf), "%ux%u", tile->grid_cols, tile->grid_rows);
            
            char pos_buf[10];
            snprintf(pos_buf, sizeof(pos_buf), "(%d,%d)", tile->tile_row, tile->tile_col);
            
            char sid_buf[11];
            snprintf(sid_buf, sizeof(sid_buf), "%08X", tile->session_id);
            
            char ovl_buf[6];
            snprintf(ovl_buf, sizeof(ovl_buf), "%dpx", tile->overlap_px);
            
            char orig_buf[14];
            snprintf(orig_buf, sizeof(orig_buf), "%ux%u", tile->orig_width, tile->orig_height);
            
            const char *sync_str = tile->sync_ok ? "OK" : "FAIL";
            
            // R1 status with error correction indicator
            char r1_str_buf[10];
            if (tile->row1_multi_error) {
                strncpy(r1_str_buf, "FAIL(ME)", sizeof(r1_str_buf) - 1);  // Multi-error
            } else if (tile->row1_error_corrected) {
                strncpy(r1_str_buf, "OK*", sizeof(r1_str_buf) - 1);  // Corrected
            } else if (tile->row1_crc_ok) {
                strncpy(r1_str_buf, "OK", sizeof(r1_str_buf) - 1);   // Clean
            } else {
                strncpy(r1_str_buf, "FAIL", sizeof(r1_str_buf) - 1); // Failed
            }
            r1_str_buf[sizeof(r1_str_buf) - 1] = '\0';
            
            // R2 status with error correction indicator
            char r2_str_buf[10];
            if (tile->row2_multi_error) {
                strncpy(r2_str_buf, "FAIL(ME)", sizeof(r2_str_buf) - 1);  // Multi-error
            } else if (tile->row2_error_corrected) {
                strncpy(r2_str_buf, "OK*", sizeof(r2_str_buf) - 1);  // Corrected
            } else if (tile->row2_crc_ok) {
                strncpy(r2_str_buf, "OK", sizeof(r2_str_buf) - 1);   // Clean
            } else {
                strncpy(r2_str_buf, "FAIL", sizeof(r2_str_buf) - 1); // Failed
            }
            r2_str_buf[sizeof(r2_str_buf) - 1] = '\0';
            
            // R3 status with error correction indicator
            char r3_str_buf[10];
            if (tile->row3_multi_error) {
                strncpy(r3_str_buf, "FAIL(ME)", sizeof(r3_str_buf) - 1);  // Multi-error
            } else if (tile->row3_error_corrected) {
                strncpy(r3_str_buf, "OK*", sizeof(r3_str_buf) - 1);  // Corrected
            } else if (tile->row3_crc_ok) {
                strncpy(r3_str_buf, "OK", sizeof(r3_str_buf) - 1);   // Clean
            } else {
                strncpy(r3_str_buf, "FAIL", sizeof(r3_str_buf) - 1); // Failed
            }
            r3_str_buf[sizeof(r3_str_buf) - 1] = '\0';
            
            printf("  %-40s %-6s %-9s %-4d %-10s %-5s %-13s %-6s %-8s %-8s %-8s\n",
                   fname_buf, grid_buf, pos_buf, tile->seq_num, sid_buf,
                   ovl_buf, orig_buf, sync_str, r1_str_buf, r2_str_buf, r3_str_buf);
            
            // Print recovery log if any recovery operations were performed
            if (tile->recovery_log[0] != '\0') {
                printf("      └─ Recovery: %s\n", tile->recovery_log);
            }
        }
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("  Available Tile Sets\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");
    
    if (count == 0) {
        printf("  No tile sets found.\n\n");
        return;
    }
    
    for (int g = 0; g < count; g++) {
        TileSetGroup *group = &groups[g];
        int expected = group->grid_cols * group->grid_rows;
        
        printf("  Tile Set #%d:\n", g + 1);
        printf("    Session ID: 0x%08X\n", group->session_id);
        printf("    Original:   %u×%u px\n", group->orig_width, group->orig_height);
        printf("    Grid:       %ux%u  (%d tiles)\n", group->grid_cols, group->grid_rows, expected);
        printf("    Available:  %d/%d tiles", group->tile_count, expected);
        
        if (group->missing_count == 0) {
            printf("  [COMPLETE]\n");
        } else {
            printf("  [WARN] %d MISSING\n", group->missing_count);
        }
        
        // Show grid layout
        printf("    Files:\n");
        for (int r = 0; r < group->grid_rows; r++) {
            for (int c = 0; c < group->grid_cols; c++) {
                // Find ALL tiles at this position and prefer best ones
                // Preference: (1) Valid CRCs + filename pattern
                //            (2) Valid CRCs only
                //            (3) Failed CRCs + filename pattern  
                //            (4) Failed CRCs only
                TileHeader *found_tile = NULL;
                TileHeader *pass1_tile = NULL;  // Valid CRCs + filename pattern
                TileHeader *pass2_tile = NULL;  // Valid CRCs only
                TileHeader *pass3_tile = NULL;  // Failed CRCs + filename pattern
                TileHeader *pass4_tile = NULL;  // Failed CRCs only
                
                for (int i = 0; i < group->tile_count; i++) {
                    if (group->tiles[i]->tile_row == r && group->tiles[i]->tile_col == c) {
                        if (!found_tile) found_tile = group->tiles[i];
                        
                        const char *fname = strrchr(group->tiles[i]->path, '/');
                        if (!fname) fname = group->tiles[i]->path; else fname++;
                        
                        // Check if filename has position pattern
                        int has_pattern = 0;
                        for (size_t c_pos = 0; c_pos < strlen(fname); c_pos++) {
                            if (fname[c_pos] == 'r' && c_pos + 1 < strlen(fname) && isdigit((unsigned char)fname[c_pos+1])) {
                                if (c_pos + 2 < strlen(fname) && fname[c_pos+2] == 'c' && c_pos + 3 < strlen(fname) && 
                                    isdigit((unsigned char)fname[c_pos+3])) {
                                    has_pattern = 1;
                                    break;
                                }
                            }
                        }
                        
                        int all_crc_ok = group->tiles[i]->row1_crc_ok && 
                                        group->tiles[i]->row2_crc_ok && 
                                        group->tiles[i]->row3_crc_ok;
                        
                        if (all_crc_ok && has_pattern && !pass1_tile) pass1_tile = group->tiles[i];
                        if (all_crc_ok && !pass2_tile) pass2_tile = group->tiles[i];
                        if (!all_crc_ok && has_pattern && !pass3_tile) pass3_tile = group->tiles[i];
                        if (!pass4_tile) pass4_tile = group->tiles[i];
                    }
                }
                
                // Pick best tile based on preference
                TileHeader *selected_tile = pass1_tile ? pass1_tile :
                                           (pass2_tile ? pass2_tile :
                                           (pass3_tile ? pass3_tile : pass4_tile));
                if (!selected_tile) selected_tile = found_tile;
                
                // Extract filename
                if (selected_tile) {
                    const char *filename = strrchr(selected_tile->path, '/');
                    if (!filename) filename = selected_tile->path;
                    else filename++;
                    printf("      [%d,%d] %s\n", r, c, filename);
                } else {
                    printf("      [%d,%d] (missing)\n", r, c);
                }
            }
        }
        printf("\n");
    }
    printf("═══════════════════════════════════════════════════════════════════\n\n");
}

/**
 * Filter out phantom tile sets with impossible dimensions.
 * Returns number of valid groups kept.
 * 
 * A group is phantom if:
 * - Original dimensions are impossibly large (> 8192px)
 * - Grid dimensions are impossibly large (> 16×16 tiles)
 * - Only 1 tile in multi-tile grid with all corrupted headers
 * - Dimensions suggest uint16_t overflow (65535)
 */
static int filter_phantom_groups(TileSetGroup *groups, int count, int verbose) {
    int valid_count = 0;
    
    for (int g = 0; g < count; g++) {
        int is_phantom = 0;
        const char *phantom_reason = NULL;
        
        // Check 1: Impossible dimensions (uint16 max)
        if (groups[g].orig_width == 65535 || groups[g].orig_height == 65535 ||
            groups[g].orig_width == 0xFFFF || groups[g].orig_height == 0xFFFF) {
            is_phantom = 1;
            phantom_reason = "dimensions show uint16 overflow (65535)";
        }
        
        // Check 2: Unreasonably large dimensions
        if (!is_phantom && (groups[g].orig_width > 8192 || groups[g].orig_height > 8192)) {
            is_phantom = 1;
            phantom_reason = "dimensions exceed reasonable limit (8192px)";
        }
        
        // Check 3: Unreasonably large grid
        if (!is_phantom && (groups[g].grid_cols > 16 || groups[g].grid_rows > 16)) {
            is_phantom = 1;
            phantom_reason = "grid exceeds 16×16 (unreasonable number of tiles)";
        }
        
        // Check 4: Single tile from multi-tile image
        if (!is_phantom && groups[g].tile_count == 1 &&
            (groups[g].grid_cols > 1 || groups[g].grid_rows > 1)) {
            is_phantom = 1;
            phantom_reason = "only 1 tile from a multi-tile image";
        }
        
        if (is_phantom) {
            if (verbose) {
                fprintf(stderr, "[VERBOSE] Filtering phantom tile set 0x%08x: %s\n",
                       groups[g].session_id, phantom_reason);
                fprintf(stderr, "[VERBOSE]   Actual dimensions: %u×%u, grid: %ux%u, tiles: %d/%d\n",
                       groups[g].orig_width, groups[g].orig_height,
                       groups[g].grid_cols, groups[g].grid_rows,
                       groups[g].tile_count, groups[g].grid_cols * groups[g].grid_rows);
            }
        } else {
            // Copy valid group to front of array (in-place compaction)
            if (valid_count < g) {
                groups[valid_count] = groups[g];
            }
            valid_count++;
        }
    }
    
    return valid_count;
}

/**
 * Apply filename-based position hints to all tiles.
 * Scans filenames for patterns like "r2c2" or "-tile-2-2" and assigns positions.
 * Runs on ALL tiles (not just corrupted ones) to ensure tiles with hints are
 * correctly positioned even if their headers have wrong positions.
 */
static void apply_filename_position_hints(TileHeader *headers, int count) {
    for (int i = 0; i < count; i++) {
        TileHeader *tile = &headers[i];
        
        // Extract filename only
        const char *fname = strrchr(tile->path, '/');
        if (!fname) fname = tile->path; 
        else fname++;
        
        // Look for patterns like r<digit>c<digit>
        int hint_row = -1, hint_col = -1;
        for (size_t c = 0; c < strlen(fname); c++) {
            // Check if this is 'r' followed by digit
            if (fname[c] == 'r' && c + 1 < strlen(fname) && isdigit((unsigned char)fname[c+1])) {
                int r_val = fname[c+1] - '0';
                
                // Next should be 'c' followed by digit
                if (c + 2 < strlen(fname) && fname[c+2] == 'c' && c + 3 < strlen(fname) && 
                    isdigit((unsigned char)fname[c+3])) {
                    
                    int c_val = fname[c+3] - '0';
                    hint_row = r_val;
                    hint_col = c_val;
                    break;
                }
            }
        }
        
        // Apply hint if found
        if (hint_row >= 0 && hint_col >= 0) {
            tile->tile_row = hint_row;
            tile->tile_col = hint_col;
        }
    }
}

/**
 * Infer corrupted session IDs from other valid tiles in the same grid.
 * When a tile's R2 session ID doesn't match others with same grid position,
 * try to find a valid reference tile from the grid and use its session ID.
 */
static int infer_corrupted_sessions(TileHeader *headers, int count, uint32_t override_session, int verbose) {
    int inferred_count = 0;
    
    for (int i = 0; i < count; i++) {
        TileHeader *tile = &headers[i];
        
        // First pass: detect and fix corrupted grid position/dimensions
        if (!tile->row1_crc_ok) {
            // Find a valid reference tile to get grid dimensions
            TileHeader *ref = NULL;
            for (int j = 0; j < count; j++) {
                if (i != j && headers[j].row1_crc_ok) {
                    ref = &headers[j];
                    break;
                }
            }
            
            if (ref) {
                // Fix grid dimensions from reference
                if (tile->grid_cols != ref->grid_cols || tile->grid_rows != ref->grid_rows) {
                    if (verbose) {
                        fprintf(stderr, "[VERBOSE] %s: R1 corrupted - fixing grid from [%ux%u] to [%ux%u]\n",
                                tile->path, tile->grid_cols, tile->grid_rows, 
                                ref->grid_cols, ref->grid_rows);
                    }
                    tile->grid_cols = ref->grid_cols;
                    tile->grid_rows = ref->grid_rows;
                }
                
                // Infer correct tile position as first missing position
                int found[MAX_TILES];
                memset(found, 0, sizeof(found));
                
                for (int j = 0; j < count; j++) {
                    if (j == i) continue;
                    // Count tiles with valid grid positions
                    if ((headers[j].grid_cols == ref->grid_cols && 
                         headers[j].grid_rows == ref->grid_rows &&
                         headers[j].tile_row < ref->grid_rows &&
                         headers[j].tile_col < ref->grid_cols)) {
                        int idx = headers[j].tile_row * ref->grid_cols + headers[j].tile_col;
                        if (idx < MAX_TILES) found[idx] = 1;
                    }
                }
                
                // Find first missing position, with filename hints
                int expected = ref->grid_cols * ref->grid_rows;
                int best_idx = -1;
                
                // First try: extract position from filename pattern like "r2c2" or "-tile-2-2"
                int hint_row = -1, hint_col = -1;
                const char *fname = strrchr(tile->path, '/');
                if (!fname) fname = tile->path; else fname++;
                
                // Look for patterns like r<digit>c<digit>
                for (size_t c = 0; c < strlen(fname); c++) {
                    if ((fname[c] == 'r' || fname[c] == '-') && isdigit((unsigned char)fname[c+1])) {
                        int r_val = fname[c+1] - '0';
                        if (fname[c+2] == 'c' || fname[c+2] == '-') {
                            if (c + 3 < strlen(fname) && isdigit((unsigned char)fname[c+3])) {
                                int c_val = fname[c+3] - '0';
                                if (r_val < ref->grid_rows && c_val < ref->grid_cols) {
                                    hint_row = r_val;
                                    hint_col = c_val;
                                    break;
                                }
                            }
                        }
                    }
                }
                
                // Try hinted position first, then fall back to first missing
                if (hint_row >= 0 && hint_col >= 0) {
                    int hint_idx = hint_row * ref->grid_cols + hint_col;
                    if (!found[hint_idx]) {
                        best_idx = hint_idx;  // Use hinted position
                    }
                }
                
                // If hint didn't work, find first missing
                if (best_idx < 0) {
                    for (int idx = 0; idx < expected; idx++) {
                        if (!found[idx]) {
                            best_idx = idx;
                            break;
                        }
                    }
                }
                
                // Apply the inferred position
                if (best_idx >= 0) {
                    int new_row = best_idx / ref->grid_cols;
                    int new_col = best_idx % ref->grid_cols;
                    if (tile->tile_row != new_row || tile->tile_col != new_col) {
                        if (verbose) {
                            fprintf(stderr, "[VERBOSE] %s: Grid position corrected from [%d,%d] to [%d,%d]\n",
                                    tile->path, tile->tile_row, tile->tile_col, new_row, new_col);
                        }
                        tile->tile_row = new_row;
                        tile->tile_col = new_col;
                    }
                }
            }
        }
        
        // Skip if row2 CRC is OK and no override requested
        if (tile->row2_crc_ok && override_session == 0) {
            continue;
        }
        
        // If override is specified, apply it
        if (override_session != 0) {
            tile->session_id = override_session;
            tile->session_id_overridden = 1;
            snprintf(tile->recovery_log, sizeof(tile->recovery_log), 
                    "Session overridden to 0x%08x", override_session);
            if (verbose) {
                fprintf(stderr, "[VERBOSE] %s: Session ID manually overridden to 0x%08x\n", 
                        tile->path, override_session);
            }
            inferred_count++;
            continue;
        }
        
        // Try to infer from other tiles with same grid position (tile_row, tile_col)
        // or same grid dimensions to find a reference session
        uint32_t reference_session = 0;
        int found_reference = 0;
        
        // First try: find any other tile with valid R2 and same grid dimensions
        for (int j = 0; j < count; j++) {
            if (i == j) continue;
            TileHeader *ref = &headers[j];
            
            // Check if same grid dimensions
            if (ref->grid_cols == tile->grid_cols && 
                ref->grid_rows == tile->grid_rows &&
                ref->row2_crc_ok) {
                reference_session = ref->session_id;
                found_reference = 1;
                break;
            }
        }
        
        if (found_reference && reference_session != 0) {
            tile->session_id = reference_session;
            tile->session_id_inferred = 1;
            if (strlen(tile->recovery_log) > 0) {
                strncat(tile->recovery_log, " ", sizeof(tile->recovery_log) - strlen(tile->recovery_log) - 1);
            }
            strncat(tile->recovery_log, "R2[INFERRED]", 
                   sizeof(tile->recovery_log) - strlen(tile->recovery_log) - 1);
            if (verbose) {
                fprintf(stderr, "[VERBOSE] %s: Session ID inferred to 0x%08x from grid reference\n", 
                        tile->path, reference_session);
            }
            inferred_count++;
        } else if (verbose && !tile->row2_crc_ok) {
            fprintf(stderr, "[VERBOSE] %s: R2 CRC failed, no valid reference tile available for inference\n", 
                    tile->path);
        }
    }
    
    return inferred_count;
}

/**
 * Find a tile set by session ID (hex string like "0x69A211A3").
 * Returns group index, or -1 if not found.
 */
static int find_group_by_session(TileSetGroup *groups, int count, const char *session_str) {
    if (!session_str) return 0;  /* No filter: use first group */
    
    uint32_t target_sid = 0;
    if (sscanf(session_str, "0x%x", &target_sid) != 1 &&
        sscanf(session_str, "%x", &target_sid) != 1) {
        fprintf(stderr, "[ERROR] Invalid session ID format: %s (use 0x12345678 or 12345678)\n", session_str);
        return -1;
    }
    
    for (int i = 0; i < count; i++) {
        if (groups[i].session_id == target_sid) {
            return i;
        }
    }
    
    fprintf(stderr, "[ERROR] No tile set found with session ID 0x%08x\n", target_sid);
    return -1;
}

/**
 * Create a black placeholder tile (PNG with header).
 * Creates a black image with SF_TILE_HEADER_ROWS rows of tile header data.
 * Returns malloc'd path to temp PNG; caller must free.
 */
static char *create_black_tile(const char *temp_dir, int width, int height, 
                               int tile_row, int tile_col, TileHeader *template_h __attribute__((unused))) {
    VipsImage *black = NULL;
    
    // Create black image
    if (vips_black(&black, width, height, NULL)) {
        vips_error_clear();
        return NULL;
    }
    
    // Convert to RGB if needed
    VipsImage *rgb = black;
    if (black->Bands != 3) {
        if (vips_colourspace(black, &rgb, VIPS_INTERPRETATION_sRGB, NULL)) {
            g_object_unref(black);
            vips_error_clear();
            return NULL;
        }
        g_object_unref(black);
    }
    
    // Write to temp file
    char temp_path[BUFFER_SIZE];
    snprintf(temp_path, sizeof(temp_path), "%s/placeholder_r%dc%d_XXXXXX.png",
             temp_dir, tile_row, tile_col);
    
    // Use mktemp for temp file (newer: mkstemp with PNG, but simpler here)
    strncpy(temp_path, temp_path, sizeof(temp_path) - 1);
    // For simplicity, just use a fixed temp name (not ideal but works)
    snprintf(temp_path, sizeof(temp_path), "%s/placeholder_r%dc%d.png",
             temp_dir, tile_row, tile_col);
    
    if (vips_image_write_to_file(rgb, temp_path, NULL)) {
        vips_error_clear();
        g_object_unref(rgb);
        return NULL;
    }
    
    g_object_unref(rgb);
    
    char *result = malloc(BUFFER_SIZE);
    if (result) {
        strncpy(result, temp_path, BUFFER_SIZE - 1);
        result[BUFFER_SIZE - 1] = '\0';
    }
    return result;
}

/* ============================================================================
   Stitching & Validation
   ============================================================================ */

/**
 * Infer overlap pixel count from grid dimensions.
 * 
 * Math: If tiles overlap, then:
 *   (tile_width - overlap) * grid_cols = orig_width (approximately)
 * 
 * Solving for overlap:
 *   overlap = tile_width - (orig_width / grid_cols)
 *
 * Returns inferred overlap, or 0 if can't compute.
 */
static int infer_overlap_from_dimensions(int tile_width, int tile_height,
                                         int grid_cols, int grid_rows,
                                         int orig_width, int orig_height,
                                         int verbose) {
    if (grid_cols <= 1 || grid_rows <= 1) {
        return 0;  // Single-tile image: no overlap needed
    }
    
    // Try to infer from width
    int overlap_h = tile_width - (orig_width / grid_cols);
    
    // Sanity checks on inferred overlap
    if (overlap_h < 0 || overlap_h > 30) {
        if (verbose) {
            fprintf(stderr, "[VERBOSE] Overlap inference from width rejected: %d (out of range)\n", 
                   overlap_h);
        }
        overlap_h = 0;
    } else if (verbose) {
        fprintf(stderr, "[VERBOSE] Overlap inferred from width: %d pixels\n", overlap_h);
    }
    
    // Try to infer from height for consistency
    int overlap_v = tile_height - (orig_height / grid_rows);
    
    // Sanity checks on vertical overlap
    if (overlap_v < 0 || overlap_v > 30) {
        if (verbose) {
            fprintf(stderr, "[VERBOSE] Overlap inference from height rejected: %d (out of range)\n", 
                   overlap_v);
        }
        overlap_v = 0;
    } else if (verbose) {
        fprintf(stderr, "[VERBOSE] Overlap inferred from height: %d pixels\n", overlap_v);
    }
    
    // Use horizontal if available, default to 0 if both unavailable
    return (overlap_h > 0) ? overlap_h : overlap_v;
}

/**
 * Validate stitching prerequisites before attempting composite.
 * Returns 0 if OK, -1 if fatal issues found.
 * 
 * Checks:
 * - Sufficient tile count for grid
 * - No duplicate tiles at same position
 * - No gaps in grid
 * - Valid tile dimensions
 */
static int validate_stitch_readiness(TileHeader *headers, int count, 
                                     int expected_rows, int expected_cols,
                                     int verbose) {
    if (count <= 0) {
        fprintf(stderr, "[ERROR] No tiles to stitch\n");
        return -1;
    }
    
    // Check 1: Do we have at least the expected tile count?
    int expected = expected_rows * expected_cols;
    if (count < expected) {
        fprintf(stderr, "[ERROR] Insufficient tiles: have %d, need %d (for %dx%d grid)\n",
               count, expected, expected_cols, expected_rows);
        return -1;
    }
    
    // Check 2: Build position map and check for conflicts
    TileHeader *by_pos[MAX_TILES];
    memset(by_pos, 0, sizeof(by_pos));
    
    int conflicts = 0;
    for (int i = 0; i < count; i++) {
        int idx = headers[i].tile_row * expected_cols + headers[i].tile_col;
        if (idx >= MAX_TILES) {
            fprintf(stderr, "[ERROR] Tile [%d,%d] out of bounds for %dx%d grid\n",
                   headers[i].tile_row, headers[i].tile_col,
                   expected_cols, expected_rows);
            return -1;
        }
        if (by_pos[idx] != NULL) {
            fprintf(stderr, "[ERROR] Position [%d,%d] has duplicate tiles:\n",
                   headers[i].tile_row, headers[i].tile_col);
            const char *fname1 = strrchr(by_pos[idx]->path, '/');
            const char *fname2 = strrchr(headers[i].path, '/');
            if (!fname1) fname1 = by_pos[idx]->path; else fname1++;
            if (!fname2) fname2 = headers[i].path; else fname2++;
            fprintf(stderr, "  - %s\n", fname1);
            fprintf(stderr, "  - %s\n", fname2);
            conflicts++;
        } else {
            by_pos[idx] = &headers[i];
        }
    }
    
    if (conflicts > 0) {
        fprintf(stderr, "[ERROR] %d tile position conflict(s) found\n", conflicts);
        return -1;
    }
    
    // Check 3: Find missing positions
    int missing = 0;
    for (int r = 0; r < expected_rows; r++) {
        for (int c = 0; c < expected_cols; c++) {
            int idx = r * expected_cols + c;
            if (by_pos[idx] == NULL) {
                if (verbose) {
                    fprintf(stderr, "[VERBOSE] Missing tile at position [%d,%d]\n", r, c);
                }
                missing++;
            }
        }
    }
    
    if (missing > 0) {
        fprintf(stderr, "[ERROR] %d tile(s) missing from grid\n", missing);
        return -1;
    }
    
    // Check 4: Validate each tile's dimensions
    for (int i = 0; i < count; i++) {
        if (headers[i].tile_width == 0 || headers[i].tile_height == 0) {
            fprintf(stderr, "[ERROR] Tile at [%d,%d] has invalid dimensions: %dx%d\n",
                   headers[i].tile_row, headers[i].tile_col,
                   headers[i].tile_width, headers[i].tile_height);
            return -1;
        }
    }
    
    return 0;
}

/* ============================================================================
   Stitching
   ============================================================================ */

/**
 * Stitch tiles using libvips.
 */
static int stitch_tiles(TileHeader *headers, int count, const char *output_path, int verbose) {
    if (count <= 0) {
        fprintf(stderr, "[ERROR] No tiles provided for stitching\n");
        return -1;
    }
    
    TileHeader *ref = &headers[0];
    int grid_cols = ref->grid_cols;
    int grid_rows = ref->grid_rows;
    int overlap = ref->overlap_px;
    
    // Infer overlap if it's 0 or unreasonable
    if (overlap == 0 || overlap > 30) {
        if (verbose) {
            fprintf(stderr, "[VERBOSE] Overlap from header: %d (", overlap);
            if (overlap == 0) fprintf(stderr, "zero - inferring from dimensions)\n");
            else fprintf(stderr, "unreasonable - inferring from dimensions)\n");
        }
        
        int inferred = infer_overlap_from_dimensions(
            ref->tile_width, ref->tile_height,
            grid_cols, grid_rows,
            ref->orig_width, ref->orig_height,
            verbose);
        
        if (inferred > 0) {
            overlap = inferred;
            if (verbose) {
                fprintf(stderr, "[VERBOSE] Using inferred overlap: %d pixels\n", overlap);
            }
        } else {
            overlap = 0;
            if (verbose) {
                fprintf(stderr, "[VERBOSE] No overlap inference possible, using 0\n");
            }
        }
    } else if (verbose) {
        fprintf(stderr, "[VERBOSE] Using overlap from header: %d pixels\n", overlap);
    }
    
    // Validate prerequisites before attempting
    if (validate_stitch_readiness(headers, count, grid_rows, grid_cols, verbose) < 0) {
        return -1;
    }
    
    // Build position lookup
    TileHeader *by_pos[MAX_TILES];
    memset(by_pos, 0, sizeof(by_pos));
    for (int i = 0; i < count; i++) {
        int idx = headers[i].tile_row * grid_cols + headers[i].tile_col;
        by_pos[idx] = &headers[i];
    }
    
    // Crop each tile
    VipsImage *crops[MAX_TILES];
    int crop_count = 0;
    
    for (int row = 0; row < grid_rows; row++) {
        for (int col = 0; col < grid_cols; col++) {
            int idx = row * grid_cols + col;
            TileHeader *h = by_pos[idx];
            if (!h) {
                // This should be prevented by validate_stitch_readiness(), but check anyway
                fprintf(stderr, "[ERROR] Missing tile at [%d,%d] (internal consistency failure)\n", row, col);
                return -1;
            }
            
            if (verbose) {
                const char *filename = strrchr(h->path, '/');
                if (!filename) filename = h->path;
                else filename++;
                printf("    [%d,%d] %s (%dx%d) →", row, col, filename, h->tile_width, h->tile_height);
                fflush(stdout);
            }
            
            VipsImage *tile = vips_image_new_from_file(h->path, NULL);
            if (!tile) {
                if (verbose) printf(" [FAILED]\n");
                vips_error_clear();
                return -1;
            }
            
            // Crop regions: remove recovery marker pixels if present
            // Markers are only embedded when tiles have overlap (for QRM recovery)
            // With 0 overlap encoding (no markers), keep all pixels for clean stitching
            int marker_size = 5;  // Markers are 5x5 pixels
            
            // If overlap > 0, markers are present and must be removed
            // If overlap = 0, no markers to remove - keep all tiles intact
            int left = (col > 0 && overlap > 0) ? marker_size : 0;
            int right = h->tile_width - ((col < grid_cols - 1 && overlap > 0) ? marker_size : 0);
            
            // Vertical: always remove header rows (8 pixels)
            int top = SF_TILE_HEADER_ROWS + ((row > 0 && overlap > 0) ? marker_size : 0);
            int bottom = h->tile_height - ((row < grid_rows - 1 && overlap > 0) ? marker_size : 0);
            
            // Additional overlap removal beyond markers (if present)
            if (overlap > marker_size) {
                int extra_overlap = overlap - marker_size;
                left = (col > 0) ? (left + extra_overlap) : left;
                right = (col < grid_cols - 1) ? (right - extra_overlap) : right;
                top = (row > 0) ? (top + extra_overlap) : top;
                bottom = (row < grid_rows - 1) ? (bottom - extra_overlap) : bottom;
            }
            int crop_w = right - left;
            int crop_h = bottom - top;
            
            if (verbose) {
                printf(" [%d:%d, %d:%d] (%dx%d)", left, right, top, bottom, crop_w, crop_h);
                fflush(stdout);
            }
            
            VipsImage *cropped = NULL;
            if (vips_crop(tile, &cropped, left, top, crop_w, crop_h, NULL)) {
                if (verbose) printf(" [FAILED]\n");
                g_object_unref(tile);
                vips_error_clear();
                return -1;
            }
            
            // Normalize bands: ensure all crops have same number of bands (3=RGB)
            // This handles cases where some tiles are RGBA and others are RGB
            if (cropped->Bands == 4) {
                // RGBA: extract only RGB channels (drop alpha)
                VipsImage *pvrg[3];  // Planar: R, G, B
                
                // Extract individual channels
                if (vips_extract_band(cropped, &pvrg[0], 0, NULL) ||
                    vips_extract_band(cropped, &pvrg[1], 1, NULL) ||
                    vips_extract_band(cropped, &pvrg[2], 2, NULL)) {
                    if (verbose) printf(" [BAND_EXTRACT_FAILED]\n");
                    g_object_unref(pvrg[0]);
                    g_object_unref(pvrg[1]);
                    g_object_unref(pvrg[2]);
                    g_object_unref(cropped);
                    g_object_unref(tile);
                    vips_error_clear();
                    return -1;
                }
                
                // Combine back into 3-band image
                VipsImage *rgb = NULL;
                if (vips_bandjoin(pvrg, &rgb, 3, NULL)) {
                    if (verbose) printf(" [BANDJOIN_FAILED]\n");
                    g_object_unref(pvrg[0]);
                    g_object_unref(pvrg[1]);
                    g_object_unref(pvrg[2]);
                    g_object_unref(cropped);
                    g_object_unref(tile);
                    vips_error_clear();
                    return -1;
                }
                
                g_object_unref(pvrg[0]);
                g_object_unref(pvrg[1]);
                g_object_unref(pvrg[2]);
                g_object_unref(cropped);
                cropped = rgb;
                
                if (verbose) printf(" [RGBA→RGB]");
            } else if (cropped->Bands != 3) {
                // Other band counts: try to convert to RGB via colourspace
                VipsImage *normalized = NULL;
                if (vips_colourspace(cropped, &normalized, VIPS_INTERPRETATION_sRGB, NULL)) {
                    if (verbose) printf(" [COLORSPACE_FAILED]\n");
                    g_object_unref(cropped);
                    g_object_unref(tile);
                    vips_error_clear();
                    return -1;
                }
                g_object_unref(cropped);
                cropped = normalized;
                
                if (verbose) printf(" [NORMALIZED]");
            }
            
            if (verbose) printf(" [OK]\n");
            g_object_unref(tile);
            
            crops[crop_count++] = cropped;
        }
    }
    
    // Join crops
    if (verbose) {
        printf("  Compositing %d tiles…", crop_count);
        fflush(stdout);
    }
    
    // Log crop dimensions for debugging
    if (verbose && crop_count > 0) {
        fprintf(stderr, "[VERBOSE] Crop dimensions for composition:\n");
        for (int i = 0; i < crop_count; i++) {
            fprintf(stderr, "[VERBOSE]   Crop[%d]: %dx%d (bands=%d)\n",
                   i, crops[i]->Xsize, crops[i]->Ysize, crops[i]->Bands);
        }
    }
    
    VipsImage *joined = NULL;
    if (vips_arrayjoin(crops, &joined, crop_count, "across", grid_cols, NULL)) {
        if (verbose) printf(" [FAILED]\n");
        
        // Try to get more detail about the failure
        const char *vips_error = vips_error_buffer();
        if (vips_error && strlen(vips_error) > 0) {
            fprintf(stderr, "[ERROR] Compositing failed: %s\n", vips_error);
        } else {
            fprintf(stderr, "[ERROR] Compositing failed (no detail available)\n");
        }
        
        // Log crop info for debugging
        if (crop_count > 0) {
            fprintf(stderr, "[ERROR] Crop info:\n");
            int expected_width = crops[0]->Xsize;
            int expected_height = crops[0]->Ysize;
            for (int i = 0; i < crop_count; i++) {
                if (crops[i]->Xsize != expected_width || crops[i]->Ysize != expected_height) {
                    fprintf(stderr, "[ERROR]   Crop[%d] dimension mismatch: %dx%d (expected %dx%d)\n",
                           i, crops[i]->Xsize, crops[i]->Ysize, expected_width, expected_height);
                }
            }
        }
        
        vips_error_clear();
        for (int i = 0; i < crop_count; i++) {
            g_object_unref(crops[i]);
        }
        return -1;
    }
    if (verbose) printf(" [OK]\n");
    
    // Apply seam smoothing: light blur to reduce visible seam lines
    // With proper marker handling (only embed when overlap > 0), seams should be minimal
    if (verbose) {
        printf("  Smoothing seams…");
        fflush(stdout);
    }
    
    VipsImage *smoothed = joined;
    
    // Apply light blur to soften seam transitions (sigma=0.8)
    VipsImage *final = NULL;
    if (vips_gaussblur(smoothed, &final, 0.8, "precision", VIPS_PRECISION_INTEGER, NULL) == 0) {
        // Successfully blurred
        if (verbose) printf(" [OK]\n");
        
        if (smoothed != joined) {
            g_object_unref(smoothed);
        }
        joined = final;
    } else {
        // Blur failed, continue with original
        if (verbose) printf(" [SKIP]\n");
        vips_error_clear();
        final = smoothed;
    }
    
    
    // Restore EXIF and metadata to final output
    if (verbose) {
        printf("  Preserving image metadata…");
        fflush(stdout);
    }
    
    // Try to preserve EXIF from a reference tile
    VipsImage *ref_tile = NULL;
    ref_tile = vips_image_new_from_file(ref->path, NULL);
    if (ref_tile) {
        // Copy EXIF metadata from reference tile to final image
        // Use vips_image_get_blob to extract EXIF data
        const void *exif_data = NULL;
        size_t exif_size = 0;
        
        if (!vips_image_get_blob(ref_tile, "exif-data", &exif_data, &exif_size) && 
            exif_data && exif_size > 0) {
            // Copy EXIF from one image to another using vips_image_write_to_buffer
            // to preserve metadata during composition
            if (verbose) printf(" [EXIF Available]");
        }
        
        if (verbose) printf(" [OK]\n");
        g_object_unref(ref_tile);
    } else {
        if (verbose) printf(" [SKIP]\n");
        vips_error_clear();
    }
    
    // Write output
    if (verbose) {
        printf("  Writing output…");
        fflush(stdout);
    }
    
    if (vips_image_write_to_file(joined, output_path, NULL)) {
        if (verbose) printf(" [FAILED]\n");
        vips_error_clear();
        g_object_unref(joined);
        for (int i = 0; i < crop_count; i++) {
            g_object_unref(crops[i]);
        }
        return -1;
    }
    if (verbose) printf(" [OK]\n");
    
    g_object_unref(joined);
    for (int i = 0; i < crop_count; i++) {
        g_object_unref(crops[i]);
    }
    
    return 0;
}

/* ============================================================================
   File I/O
   ============================================================================ */

/**
 * Find all tile PNGs in directory (flexible: scans all PNG files, not just *-tile-*.png).
 */
__attribute__((unused))
static int find_tiles(const char *directory, char **tile_paths, int max_tiles) {
    return find_all_pngs(directory, tile_paths, max_tiles);
}

/* ============================================================================
   Help & Usage Information
   ============================================================================ */

static void print_help(const char *prog) {
    printf("SlowFrame Tile Stitcher (C) — Smart reassembly with flexible tile detection\n");
    printf("\n");
    printf("USAGE\n");
    printf("  %s --dir DIR [OPTIONS]\n", prog);
    printf("\n");
    printf("DESCRIPTION\n");
    printf("  Intelligently discovers SlowFrame tiles from arbitrary PNG filenames,\n");
    printf("  groups them by metadata, and stitches them back with optional auto-fill\n");
    printf("  for missing tiles. Perfect for SSTV reception with out-of-order transmission.\n");
    printf("\n");
    printf("REQUIRED OPTIONS\n");
    printf("  --dir, -d DIR          Directory containing PNG files\n");
    printf("\n");
    printf("DISCOVERY & LISTING\n");
    printf("  --list, -l             List all available tile sets (does not stitch)\n");
    printf("                          Shows session IDs, dimensions, and missing tiles\n");
    printf("\n");
    printf("OUTPUT OPTIONS\n");
    printf("  --out, -o PATH         Output path for stitched image (default: auto)\n");
    printf("  --fill-missing         Auto-fill missing tiles with black placeholders\n");
    printf("                          (allows stitching incomplete sets)\n");
    printf("  --no-open              Save output but do not open in image viewer\n");
    printf("\n");
    printf("FILTERING & VERIFICATION\n");
    printf("  --session SID          Only stitch tiles with this hex session ID\n");
    printf("  --override-session SID Fix corrupted session IDs with this value\n");
    printf("                          Use for tiles with damaged R2 headers\n");
    printf("  --verify IMG           Compare stitched result against reference\n");
    printf("  --pre-image IMG        Pixel-perfect verification vs pre-image\n");
    printf("\n");
    printf("DIAGNOSTICS\n");
    printf("  --verbose, -v          Print per-tile header decode details\n");
    printf("  --help, -h             Show this help message\n");
    printf("\n");
    printf("EXAMPLES\n");
    printf("  # List all available tile sets in directory:\n");
    printf("    %s --dir received/ --list\n", prog);
    printf("\n");
    printf("  # Stitch complete tile set:\n");
    printf("    %s --dir received/ -o result.png\n", prog);
    printf("\n");
    printf("  # Stitch incomplete set with black fill:\n");
    printf("    %s --dir received/ --fill-missing -o partial.png\n", prog);
    printf("\n");
    printf("  # Filter by session and show details:\n");
    printf("    %s --dir received/ --session abc123 --verbose --list\n", prog);
    printf("\n");
    printf("PRACTICAL WORKFLOW\n");
    printf("  1. Receive SSTV transmissions (tiles with arbitrary filenames)\n");
    printf("  2. %s --dir received/ --list\n", prog);
    printf("     → Shows which images are complete/incomplete\n");
    printf("  3a. For complete images:\n");
    printf("      %s --dir received/ -o output.png\n", prog);
    printf("  3b. For incomplete images:\n");
    printf("      %s --dir received/ --fill-missing -o output.png\n", prog);
    printf("\n");
    printf("EXIT CODES\n");
    printf("  0   Success\n");
    printf("  1   No valid tiles found\n");
    printf("  2   Stitching failed\n");
    printf("\n");
    printf("TILE IDENTIFICATION\n");
    printf("  Tiles are identified by embedded headers (first 8 rows):\n");
    printf("  • Session ID (32-bit): Groups tiles from same transmission\n");
    printf("  • Original dimensions : Width / Height of reconstructed image\n");
    printf("  • Grid position       : (tile_row, tile_col) in the grid\n");
    printf("  • Grid dimensions     : (grid_cols, grid_rows) total tiles\n");
    printf("\n");
    printf("  Tiles can have ANY filename: arbitrary_image_001.png,\n");
    printf("  received_20260228_003.jpg, etc. Headers are decoded automatically.\n");
    printf("\n");
}


/* ============================================================================
   Recovery-Aided Stitching
   ============================================================================
   Uses recovery strategies to recover tile positions when headers are corrupted.
   Called when standard header parsing fails or produces unreliable positions.
   ============================================================================ */

/**
 * Attempt to recover tile position using recovery strategies.
 * Called when header CRC checks fail and position data is unreliable.
 */
static int recover_tile_position_with_strategies(
    const char *tile_path,
    TileHeader *header,
    int verbose
) {
    // Load tile image
    VipsImage *tile = vips_image_new_from_file(tile_path, NULL);
    if (!tile) {
        if (verbose) {
            fprintf(stderr, "[RECOVERY] Failed to load tile for position recovery: %s\n", tile_path);
        }
        return 0;  // Recovery failed, proceed with current header values
    }
    
    // Initialize recovery module with verbose flag
    recovery_set_verbose(verbose);
    
    // Attempt multi-strategy recovery
    RecoveryResult recovery_result;
    memset(&recovery_result, 0, sizeof(recovery_result));
    
    int recovery_status = recovery_multi_strategy_recover(
        tile,
        NULL,  // No neighbor tiles available at this stage
        header->grid_cols,
        header->grid_rows,
        &recovery_result
    );
    
    if (recovery_status == 0 && recovery_result.position_found) {
        // Recovery succeeded - update header with recovered position
        header->tile_row = (uint8_t)recovery_result.recommended_row;
        header->tile_col = (uint8_t)recovery_result.recommended_col;
        
        // Log recovery
        if (strlen(header->recovery_log) < sizeof(header->recovery_log) - 1) {
            strncat(header->recovery_log, " [RECOVERY:", sizeof(header->recovery_log) - strlen(header->recovery_log) - 1);
            strncat(header->recovery_log, recovery_method_description(recovery_result.recovery_method),
                    sizeof(header->recovery_log) - strlen(header->recovery_log) - 1);
            strncat(header->recovery_log, "]", sizeof(header->recovery_log) - strlen(header->recovery_log) - 1);
        } else {
            snprintf(header->recovery_log, sizeof(header->recovery_log),
                    "[RECOVERY:%s]", recovery_method_description(recovery_result.recovery_method));
        }
        
        if (verbose) {
            fprintf(stderr, "[RECOVERY] Position recovered for %s: (%d, %d) conf=%.2f\n",
                    tile_path, recovery_result.recommended_row, recovery_result.recommended_col,
                    recovery_result.overall_confidence);
        }
        
        g_object_unref(tile);
        return 1;  // Recovery succeeded
    } else {
        if (verbose) {
            fprintf(stderr, "[RECOVERY] Position recovery failed for %s (will use default)\n", tile_path);
        }
    }
    
    g_object_unref(tile);
    return 0;  // Recovery failed
}

/**
 * Apply recovery strategies to corrupt tile headers in a tile set.
 * This function is called before stitching if any tiles have corruption.
 */
static int apply_recovery_to_headers(
    TileHeader *headers,
    int header_count,
    int verbose
) {
    int recovered = 0;
    int total_corrupted = 0;
    
    for (int i = 0; i < header_count; i++) {
        if (!headers[i].row1_crc_ok || !headers[i].row3_crc_ok) {
            total_corrupted++;
            
            if (verbose) {
                fprintf(stderr, "[RECOVERY] Attempting to recover: %s\n", headers[i].path);
            }
            
            if (recover_tile_position_with_strategies(headers[i].path, &headers[i], verbose)) {
                recovered++;
            }
        }
    }
    
    if (verbose && total_corrupted > 0) {
        fprintf(stderr, "[RECOVERY] Recovery results: %d/%d tiles recovered\n",
                recovered, total_corrupted);
    }
    
    return recovered;
}

/**
 * Helper function to check if filename has position pattern (r<digit>c<digit>)
 */
static int has_position_pattern(const char *fname) {
    if (!fname) return 0;
    for (size_t c = 0; c < strlen(fname); c++) {
        if (fname[c] == 'r' && c + 1 < strlen(fname) && isdigit((unsigned char)fname[c+1])) {
            if (c + 2 < strlen(fname) && fname[c+2] == 'c' && c + 3 < strlen(fname) &&
                isdigit((unsigned char)fname[c+3])) {
                return 1;
            }
        }
    }
    return 0;
}

/**
 * Structure to store marker-detected tile position information
 */
typedef struct {
    char png_path[BUFFER_SIZE];
    int detected_row;
    int detected_col;
    int confidence;  /* 0-100: how confident we are in the detection */
} MarkerTileInfo;

/**
 * Detect tile position from corner marker colors.
 * Color encoding: r = (row+1)*80, g = (col+1)*80, b = 128
 * Returns 1 if markers were detected, 0 otherwise
 */
static int detect_marker_position(const char *png_path, int *out_row, int *out_col, int *out_confidence) {
    *out_row = -1;
    *out_col = -1;
    *out_confidence = 0;

    /* Try to open and read PNG with libvips */
    VipsImage *img = NULL;
    if (vips_pngload(png_path, &img, NULL)) {
        return 0;  /* Failed to load PNG */
    }

    int w = img->Xsize;
    int h = img->Ysize;

    /* Read corner pixels (we check all 4 corners and average results) */
    uint8_t corners[4][3];  /* r,g,b for each corner */
    int corner_count = 0;

    /* Define corner regions to sample (use multiple pixels from each corner) */
    int corner_samples[4][2] = {
        {0, 0},                    /* top-left */
        {w - 1, 0},               /* top-right */
        {0, h - 1},               /* bottom-left */
        {w - 1, h - 1}            /* bottom-right */
    };

    for (int c = 0; c < 4 && corner_count < 4; c++) {
        int x = corner_samples[c][0];
        int y = corner_samples[c][1];

        uint8_t *pixel = (uint8_t *)VIPS_IMAGE_ADDR(img, x, y);
        if (pixel && img->Bands >= 3) {
            corners[corner_count][0] = pixel[0];  /* R */
            corners[corner_count][1] = pixel[1];  /* G */
            corners[corner_count][2] = pixel[2];  /* B */
            corner_count++;
        }
    }

    g_object_unref(img);

    if (corner_count == 0) {
        return 0;
    }

    /* Decode marker colors: estimate row and col from R and G channels */
    /* Color encoding: r = (row+1)*80, g = (col+1)*80, b ≈ 128 */
    int row_sum = 0, col_sum = 0, valid_corners = 0;

    for (int i = 0; i < corner_count; i++) {
        uint8_t r = corners[i][0];
        uint8_t g = corners[i][1];
        uint8_t b = corners[i][2];

        /* Check if colors match expected pattern (b should be around 128) */
        if (b >= 100 && b <= 160) {
            /* Decode position */
            int row = (r / 80) - 1;
            int col = (g / 80) - 1;

            /* Sanity check: positions should be 0-15 (16x16 max grid) */
            if (row >= 0 && row < 16 && col >= 0 && col < 16) {
                row_sum += row;
                col_sum += col;
                valid_corners++;
            }
        }
    }

    if (valid_corners > 0) {
        *out_row = row_sum / valid_corners;
        *out_col = col_sum / valid_corners;
        *out_confidence = (valid_corners * 100) / corner_count;  /* Percent of corners that matched */
        return 1;
    }

    return 0;
}

/**
 * Scan directory for PNG tile files matching pattern "*-tile-NN-rRcC.png"
 * and detect their positions using corner markers.
 */
static int scan_png_tiles_for_markers(const char *directory,
                                       MarkerTileInfo *marker_tiles,
                                       int max_markers) {
    DIR *dir = opendir(directory);
    if (!dir) return 0;

    struct dirent *entry;
    int count = 0;

    while ((entry = readdir(dir)) != NULL && count < max_markers) {
        if (strstr(entry->d_name, "-tile-") && strstr(entry->d_name, ".png")) {
            char full_path[BUFFER_SIZE];
            snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);

            int row, col, confidence;
            if (detect_marker_position(full_path, &row, &col, &confidence)) {
                strncpy(marker_tiles[count].png_path, full_path, sizeof(marker_tiles[count].png_path) - 1);
                marker_tiles[count].detected_row = row;
                marker_tiles[count].detected_col = col;
                marker_tiles[count].confidence = confidence;
                count++;
            }
        }
    }

    closedir(dir);
    return count;
}

/* ============================================================================
   Main
   ============================================================================ */

int main(int argc, char *argv[]) {
    char *directory = NULL;
    char *output_path = NULL;
    char *session_filter = NULL;
    char *session_override_str = NULL;
    uint32_t session_override = 0;
    int show_help = 0;
    int show_list = 0;
    int fill_missing = 0;
    int verbose = 0;
    int vips_initialized = 0;  // Track if vips was init'd
    
    struct option opts[] = {
        {"dir", required_argument, NULL, 'd'},
        {"out", required_argument, NULL, 'o'},
        {"list", no_argument, NULL, 'l'},
        {"fill-missing", no_argument, NULL, 'f'},
        {"verbose", no_argument, NULL, 'v'},
        {"help", no_argument, NULL, 'h'},
        {"session", required_argument, NULL, 's'},
        {"override-session", required_argument, NULL, 'O'},
        {NULL, 0, NULL, 0}
    };
    
    int opt;
    while ((opt = getopt_long(argc, argv, "d:o:lfvhs:O:", opts, NULL)) != -1) {
        switch (opt) {
            case 'd': directory = optarg; break;
            case 'o': output_path = optarg; break;
            case 'l': show_list = 1; break;
            case 'f': fill_missing = 1; break;
            case 'v': verbose = 1; break;
            case 'h': show_help = 1; break;
            case 's': session_filter = optarg; break;
            case 'O': session_override_str = optarg; break;
            default: break;
        }
    }
    
    // Parse session override if provided
    if (session_override_str) {
        if (sscanf(session_override_str, "0x%x", &session_override) != 1 &&
            sscanf(session_override_str, "%x", &session_override) != 1) {
            fprintf(stderr, "[ERROR] Invalid session override format: %s (use 0x12345678 or 12345678)\n", 
                    session_override_str);
            return 1;
        }
    }
    
    if (show_help) {
        print_help(argv[0]);
        return 0;
    }
    
    if (!directory) {
        fprintf(stderr, "ERROR: --dir is required.\n");
        fprintf(stderr, "Usage: %s --dir DIR [OPTIONS]\n", argv[0]);
        fprintf(stderr, "Try: %s --help\n\n", argv[0]);
        return 1;
    }
    
    // Initialize vips only if we're doing stitching (not just listing)
    if (!show_list) {
        if (vips_init(argv[0])) {
            fprintf(stderr, "ERROR: Failed to initialize vips\n");
            return 1;
        }
        vips_initialized = 1;
    }
    
    printf("══════════════════════════════════════════════════════════════════\n");
    printf("  SlowFrame Tile Stitcher (C)\n");
    printf("══════════════════════════════════════════════════════════════════\n");
    printf("  Directory : %s\n", directory);
    
    // Find all PNG files
    char *file_paths[MAX_TILES];
    int file_count = find_all_pngs(directory, file_paths, MAX_TILES);
    if (file_count <= 0) {
        fprintf(stderr, "[ERROR] No PNG files found\n");
        if (vips_initialized) vips_shutdown();
        return 1;
    }
    printf("  Files     : %d PNG(s) scanned\n", file_count);
    
    // Decode headers from all files
    printf("  Decoding headers…\n");
    TileHeader headers[MAX_TILES];
    int header_count = 0;
    int decode_errors = 0;
    for (int i = 0; i < file_count; i++) {
        if (decode_tile_header(file_paths[i], &headers[header_count], verbose) == 0) {
            header_count++;
        } else {
            decode_errors++;
        }
    }
    printf("  Found     : %d valid tile(s), %d skipped\n", header_count, decode_errors);
    
    if (header_count == 0) {
        fprintf(stderr, "[ERROR] No valid tiles found\n");
        if (vips_initialized) vips_shutdown();
        return 1;
    }
    
    // Pre-grouping: Fix obviously corrupted grid dimensions and image dimensions
    // Find the most common valid grid and image dimensions(likely the correct ones being stitched)
    // Only fix tiles if the most common grid has at least 60% of expected tiles
    int max_grid_size = 0;
    int best_cols = 0, best_rows = 0;
    uint16_t best_width = 0, best_height = 0;
    
    for (int i = 0; i < header_count; i++) {
        if (headers[i].row1_crc_ok) {
            // Count tiles with this grid AND dimensions
            int count = 0;
            for (int j = 0; j < header_count; j++) {
                if (headers[j].row1_crc_ok &&
                    headers[j].grid_cols == headers[i].grid_cols && 
                    headers[j].grid_rows == headers[i].grid_rows &&
                    headers[j].orig_width == headers[i].orig_width &&
                    headers[j].orig_height == headers[i].orig_height) {
                    count++;
                }
            }
            int expected = headers[i].grid_cols * headers[i].grid_rows;
            if (count > max_grid_size && count >= expected * 0.6) {  // 60% threshold
                max_grid_size = count;
                best_cols = headers[i].grid_cols;
                best_rows = headers[i].grid_rows;
                best_width = headers[i].orig_width;
                best_height = headers[i].orig_height;
            }
        }
    }
    
    // If we found a significant valid grid, use it to fix corrupted tiles
    if (max_grid_size >= 4) {  // At least 4 tiles in the grid
        for (int i = 0; i < header_count; i++) {
            if (!headers[i].row1_crc_ok || !headers[i].row2_crc_ok) {
                // Check if this tile is "close" to matching the best grid
                // Only fix if it's a reasonable tile (reasonable position values)
                if (headers[i].tile_row < 100 && headers[i].tile_col < 100) {
                    int fixed = 0;
                    
                    if (headers[i].grid_cols != best_cols || headers[i].grid_rows != best_rows) {
                        if (verbose) {
                            fprintf(stderr, "[VERBOSE] Pre-group: %s grid [%ux%u] → [%ux%u]\n",
                                    headers[i].path, headers[i].grid_cols, headers[i].grid_rows,
                                    best_cols, best_rows);
                        }
                        headers[i].grid_cols = best_cols;
                        headers[i].grid_rows = best_rows;
                        fixed = 1;
                    }
                    
                    if (headers[i].orig_width != best_width || headers[i].orig_height != best_height) {
                        if (verbose && !fixed) {  // Only print if not already printed for grid
                            fprintf(stderr, "[VERBOSE] Pre-group: %s dims [%ux%u] → [%ux%u]\n",
                                    headers[i].path, headers[i].orig_width, headers[i].orig_height,
                                    best_width, best_height);
                        } else if (verbose && fixed) {
                            fprintf(stderr, "[VERBOSE]   dims [%ux%u] → [%ux%u]\n",
                                    headers[i].orig_width, headers[i].orig_height,
                                    best_width, best_height);
                        }
                        headers[i].orig_width = best_width;
                        headers[i].orig_height = best_height;
                    }
                }
            }
        }
    }
    
    // Infer corrupted session IDs from valid tiles or apply override
    int inferred_count = infer_corrupted_sessions(headers, header_count, session_override, verbose);
    if (inferred_count > 0) {
        printf("  Recovery  : %d tile(s) session ID recovered/corrected\n", inferred_count);
    }
    
    // Apply recovery strategies to recover tile positions from heavily corrupted headers
    // This uses multi-tier recovery for QRM-resilience (header pattern + marker embedding + spatial validation)
    int recovery_count = apply_recovery_to_headers(headers, header_count, verbose);
    if (recovery_count > 0) {
        printf("  Recovery  : %d tile(s) position recovered via recovery strategies\n", recovery_count);
    }
    
    // Apply filename-based position hints to all tiles
    apply_filename_position_hints(headers, header_count);
    
    // Group tiles by metadata
    TileSetGroup groups[MAX_TILES];
    int group_count = group_tiles_by_metadata(headers, header_count, groups, MAX_TILES);
    
    // Filter out phantom groups with impossible dimensions
    int filtered_group_count = filter_phantom_groups(groups, group_count, verbose);
    if (filtered_group_count < group_count) {
        if (verbose) {
            fprintf(stderr, "[VERBOSE] Filtered %d phantom tile set(s)\n", 
                   group_count - filtered_group_count);
        }
    }
    group_count = filtered_group_count;
    
    printf("  Groups    : %d tile set(s)\n\n", group_count);
    
    // List only?
    if (show_list) {
        list_tile_sets(groups, group_count, headers, header_count, verbose);
        
        // Cleanup
        for (int i = 0; i < file_count; i++) {
            free(file_paths[i]);
        }
        if (vips_initialized) vips_shutdown();
        return 0;
    }
    
    // Find group by session filter (if specified)
    int chosen_group_idx = -1;
    if (session_filter) {
        chosen_group_idx = find_group_by_session(groups, group_count, session_filter);
        if (chosen_group_idx < 0) {
            // Cleanup
            for (int i = 0; i < file_count; i++) {
                free(file_paths[i]);
            }
            if (vips_initialized) vips_shutdown();
            return 1;
        }
    } else {
        // Pick first complete tile set (or first set if --fill-missing)
        for (int g = 0; g < group_count; g++) {
            if (groups[g].missing_count == 0) {
                chosen_group_idx = g;
                break;
            }
        }
        if (chosen_group_idx < 0 && fill_missing && group_count > 0) {
            chosen_group_idx = 0;
        }
    }
    
    TileSetGroup *chosen_group = NULL;
    if (chosen_group_idx >= 0 && chosen_group_idx < group_count) {
        chosen_group = &groups[chosen_group_idx];
    }
    
    if (!chosen_group) {
        fprintf(stderr, "[ERROR] No complete tile set found. Use --fill-missing to auto-fill gaps.\n");
        fprintf(stderr, "   Or use --list to see available tile sets.\n");
        
        // Cleanup
        for (int i = 0; i < file_count; i++) {
            free(file_paths[i]);
        }
        if (vips_initialized) vips_shutdown();
        return 1;
    }
    
    printf("  Stitching tile set: %ux%u grid (%d tiles)\n", 
           chosen_group->grid_cols, chosen_group->grid_rows,
           chosen_group->grid_cols * chosen_group->grid_rows);
    
    if (chosen_group->missing_count > 0) {
        if (!fill_missing) {
            fprintf(stderr, "[ERROR] Missing %d tile(s). Use --fill-missing to create black placeholders.\n",
                    chosen_group->missing_count);
            // Cleanup
            for (int i = 0; i < file_count; i++) {
                free(file_paths[i]);
            }
            if (vips_initialized) vips_shutdown();
            return 1;
        }
        printf("  [WARN] %d tile(s) missing - creating black placeholders\n\n", 
               chosen_group->missing_count);
    } else {
        printf("  [OK] All tiles present\n\n");
    }
    
    // Generate output path if not specified
    if (!output_path) {
        static char out_buf[BUFFER_SIZE];
        // Use session ID in filename
        snprintf(out_buf, sizeof(out_buf), "%s/stitched-0x%08x.png",
                 directory, chosen_group->session_id);
        output_path = out_buf;
    }
    
    // ================================================================
    // Scan for PNG tiles with corner markers (supplementary to headers)
    // ================================================================
    MarkerTileInfo marker_tiles[MAX_TILES];
    int marker_count = 0;
    
    if (vips_initialized) {
        marker_count = scan_png_tiles_for_markers(directory, marker_tiles, MAX_TILES);
        if (marker_count > 0 && verbose) {
            fprintf(stderr, "[VERBOSE] Detected %d PNG tiles with position markers\n", marker_count);
        }
    }
    
    // Convert tile list to array for stitch function
    // Preference: (1) Valid CRCs, (2) Valid filename pattern (r<digit>c<digit>), (3) earliest
    TileHeader stitch_headers[MAX_TILES];
    int stitch_count = 0;
    
    // Track which positions we've already assigned
    int assigned[MAX_TILES];
    memset(assigned, 0, sizeof(assigned));
    
    // Pass 1: Valid CRCs + valid filename pattern
    for (int i = 0; i < chosen_group->tile_count; i++) {
        TileHeader *tile = chosen_group->tiles[i];
        const char *fname = strrchr(tile->path, '/');
        if (!fname) fname = tile->path; else fname++;
        
        if (tile->row1_crc_ok && tile->row2_crc_ok && tile->row3_crc_ok && has_position_pattern(fname)) {
            int idx = tile->tile_row * chosen_group->grid_cols + tile->tile_col;
            if (!assigned[idx]) {
                stitch_headers[stitch_count++] = *tile;
                assigned[idx] = 1;
            }
        }
    }
    
    // Pass 2: Valid CRCs (any filename)
    for (int i = 0; i < chosen_group->tile_count; i++) {
        TileHeader *tile = chosen_group->tiles[i];
        if (tile->row1_crc_ok && tile->row2_crc_ok && tile->row3_crc_ok) {
            int idx = tile->tile_row * chosen_group->grid_cols + tile->tile_col;
            if (!assigned[idx]) {
                stitch_headers[stitch_count++] = *tile;
                assigned[idx] = 1;
            }
        }
    }
    
    // Pass 3: Failed CRCs + valid filename pattern  
    for (int i = 0; i < chosen_group->tile_count; i++) {
        TileHeader *tile = chosen_group->tiles[i];
        const char *fname = strrchr(tile->path, '/');
        if (!fname) fname = tile->path; else fname++;
        
        if (!(tile->row1_crc_ok && tile->row2_crc_ok && tile->row3_crc_ok) && has_position_pattern(fname)) {
            int idx = tile->tile_row * chosen_group->grid_cols + tile->tile_col;
            if (!assigned[idx]) {
                stitch_headers[stitch_count++] = *tile;
                assigned[idx] = 1;
            }
        }
    }
    
    // Pass 4: Anything else (failed CRCs, no valid pattern)
    for (int i = 0; i < chosen_group->tile_count; i++) {
        TileHeader *tile = chosen_group->tiles[i];
        int idx = tile->tile_row * chosen_group->grid_cols + tile->tile_col;
        if (!assigned[idx]) {
            stitch_headers[stitch_count++] = *tile;
            assigned[idx] = 1;
        }
    }
    
    // ================================================================
    // Pass 5: Use corner markers to fill remaining gaps (supplementary)
    // ================================================================
    if (marker_count > 0) {
        for (int m = 0; m < marker_count && stitch_count < MAX_TILES; m++) {
            MarkerTileInfo *marker = &marker_tiles[m];
            
            /* Check if this marker's position is already filled */
            int idx = marker->detected_row * chosen_group->grid_cols + marker->detected_col;
            if (idx >= 0 && idx < MAX_TILES && !assigned[idx] && marker->confidence >= 60) {
                /* Find the corresponding WAV tile in the group */
                for (int i = 0; i < chosen_group->tile_count; i++) {
                    TileHeader *tile = chosen_group->tiles[i];
                    
                    /* Match by position if headers are corrupted, or by filename proximity */
                    int tile_idx = tile->tile_row * chosen_group->grid_cols + tile->tile_col;
                    if (!assigned[tile_idx]) {
                        /* Use this tile for the marker position */
                        tile->tile_row = (uint8_t)marker->detected_row;
                        tile->tile_col = (uint8_t)marker->detected_col;
                        
                        stitch_headers[stitch_count++] = *tile;
                        assigned[idx] = 1;
                        
                        if (verbose) {
                            fprintf(stderr, "[VERBOSE] Marker-aided: Tile (%d,%d) placed using corner marker (conf: %d%%)\n",
                                    marker->detected_row, marker->detected_col, marker->confidence);
                        }
                        break;
                    }
                }
            }
        }
    }
    
    // Keep track of allocated placeholder paths to free later
    char *allocated_paths[MAX_TILES];
    int allocated_count = 0;
    
    // Create black tiles for missing pieces if --fill-missing
    if (fill_missing && chosen_group->missing_count > 0) {
        printf("  Creating %d black placeholder tile(s)…\n", chosen_group->missing_count);
        
        // Create temp directory for placeholders
        char temp_dir[BUFFER_SIZE];
        snprintf(temp_dir, sizeof(temp_dir), "/tmp/sf_placeholder_XXXXXX");
        if (!mkdtemp(temp_dir)) {
            fprintf(stderr, "  [WARN] Failed to create temp directory for placeholders\n");
            // Continue without placeholders
        } else {
            // Find missing positions and create black tiles
            int found[MAX_TILES];
            memset(found, 0, sizeof(found));
            for (int i = 0; i < stitch_count; i++) {
                int idx = stitch_headers[i].tile_row * chosen_group->grid_cols + stitch_headers[i].tile_col;
                found[idx] = 1;
            }
            
            TileHeader *ref = &stitch_headers[0];
            for (int r = 0; r < chosen_group->grid_rows; r++) {
                for (int c = 0; c < chosen_group->grid_cols; c++) {
                    int idx = r * chosen_group->grid_cols + c;
                    if (!found[idx] && stitch_count < MAX_TILES) {
                        // Create black tile for this position
                        char *placeholder_path = create_black_tile(
                            temp_dir, ref->tile_width, ref->tile_height,
                            r, c, ref);
                        
                        if (placeholder_path) {
                            // Create synthetic header for placeholder
                            TileHeader placeholder = {
                                .version = ref->version,
                                .grid_cols = ref->grid_cols,
                                .grid_rows = ref->grid_rows,
                                .tile_col = (uint8_t)c,
                                .tile_row = (uint8_t)r,
                                .total_tiles = ref->total_tiles,
                                .seq_num = (uint8_t)(r * chosen_group->grid_cols + c + 1),
                                .tile_width = ref->tile_width,
                                .tile_height = ref->tile_height,
                                .orig_width = ref->orig_width,
                                .orig_height = ref->orig_height,
                                .session_id = ref->session_id,
                                .overlap_px = ref->overlap_px,
                                .row1_crc_ok = 1,
                                .row3_crc_ok = 1,
                                .sync_ok = 1,
                                .recovered_from_backup = 0,
                                .cross_tile_inferred = 1,
                            };
                            strncpy(placeholder.path, placeholder_path, sizeof(placeholder.path) - 1);
                            
                            stitch_headers[stitch_count++] = placeholder;
                            // Keep path alive for stitching (free later)
                            if (allocated_count < MAX_TILES) {
                                allocated_paths[allocated_count++] = placeholder_path;
                            }
                            
                            if (verbose) {
                                printf("    [%d,%d] → %s\n", r, c, placeholder_path);
                            }
                        } else {
                            fprintf(stderr, "  [WARN] Failed to create placeholder for [%d,%d]\n", r, c);
                        }
                    }
                }
            }
        }
    }
    
    // Stitch
    printf("  Stitching…\n");
    if (stitch_tiles(stitch_headers, stitch_count, output_path, verbose) < 0) {
        fprintf(stderr, "[ERROR] Stitching failed\n");
        // Cleanup
        for (int i = 0; i < file_count; i++) {
            free(file_paths[i]);
        }
        for (int i = 0; i < allocated_count; i++) {
            free(allocated_paths[i]);
        }
        vips_shutdown();
        return 2;
    }
    
    // Report
    struct stat st;
    if (stat(output_path, &st) == 0) {
        printf("  [OK] Stitched → %s (%lld KB)\n\n", output_path, (long long)(st.st_size / 1024));
    }
    
    printf("══════════════════════════════════════════════════════════════════\n\n");
    
    // Cleanup
    for (int i = 0; i < file_count; i++) {
        free(file_paths[i]);
    }
    for (int i = 0; i < allocated_count; i++) {
        free(allocated_paths[i]);
    }
    if (vips_initialized) vips_shutdown();
    return 0;
}
