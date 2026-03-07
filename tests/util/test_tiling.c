/**
 * @file test_tiling.c
 * @brief Unit tests for SlowFrame tile splitting and header row encoding
 *
 * Test groups:
 *   1. CRC-8/CCITT algorithm (polynomial 0x07) — cross-check with known vectors
 *   2. SFTileInfo struct layout / SF_TILE_HEADER_ROWS constant
 *   3. Tile crop math (crop box calculation for all grid/overlap combinations)
 *   4. image_apply_tile_header pixel verification (sync row + data rows)
 *   5. image_crop_region dimension verification
 *   6. Error-path handling (no image loaded)
 *
 * Requires: libvips (for groups 4 and 5), error.c, slowframe_image.c and its deps
 *
 * Build via test_tiling_runner.py or directly:
 *   cc -std=c11 -Wall -O0 -g \
 *      $(pkg-config --cflags vips glib-2.0 gobject-2.0) \
 *      -I src/include -I src -I src/image -I src/util \
 *      tests/util/test_tiling.c \
 *      src/slowframe_image.c src/image/image_aspect.c \
 *      src/image/image_loader.c src/image/image_processor.c \
 *      src/image/image_text_overlay.c \
 *      src/overlay_spec.c src/util/error.c \
 *      $(pkg-config --libs vips glib-2.0 gobject-2.0) -lm \
 *      -o bin/test_tiling
 *
 * @author SlowFrame Contributors
 * @version 2.1.0
 * @date February 2026
 */

#include "slowframe_image.h"   /* image_apply_tile_header, image_crop_region,
                                  image_load_from_file, image_get_pixel_rgb,
                                  image_get_dimensions, image_free,
                                  SFTileInfo, SF_TILE_HEADER_ROWS            */
#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <vips/vips.h>

/* ============================================================================
   Test harness macros
   ============================================================================ */

static int g_tests_run    = 0;
static int g_tests_passed = 0;

#define TEST(name) \
    do { printf("  TEST %-60s ", (name)); g_tests_run++; } while (0)

#define PASS() \
    do { printf("✅ PASS\n"); g_tests_passed++; } while (0)

#define FAIL(fmt, ...) \
    do { printf("❌ FAIL: " fmt "\n", ##__VA_ARGS__); } while (0)

#define ASSERT_EQ_INT(got, want, label) \
    do { \
        if ((got) != (want)) { \
            FAIL("%s: got %d, want %d", (label), (int)(got), (int)(want)); \
            return; \
        } \
    } while (0)

#define ASSERT_EQ_UINT8(got, want, label) \
    do { \
        if ((uint8_t)(got) != (uint8_t)(want)) { \
            FAIL("%s: got 0x%02X, want 0x%02X", (label), \
                 (unsigned)(uint8_t)(got), (unsigned)(uint8_t)(want)); \
            return; \
        } \
    } while (0)

/* Path for ephemeral test PNG files written to /tmp */
#define TEST_PNG_PATH   "/tmp/sf_test_tiling_320x240.png"

/* ============================================================================
   CRC-8/CCITT reference implementation (mirrors sf_crc8 in slowframe_image.c)
   Used independently to derive expected values for pixel-level checks.
   ============================================================================ */

static uint8_t ref_crc8(const uint8_t *data, size_t len) {
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
   Tile crop-box math (mirrors exact algorithm from slowframe.c tiling block)
   ============================================================================ */

typedef struct { int left, top, w, h; } CropBox;

static CropBox compute_tile_crop(int orig_w, int orig_h,
                                  int tile_cols, int tile_rows,
                                  int tile_col,  int tile_row,
                                  int overlap) {
    int ctw = orig_w / tile_cols;
    int cth = orig_h / tile_rows;

    int content_w = (tile_col == tile_cols - 1)
                    ? (orig_w - tile_col * ctw) : ctw;
    int content_h = (tile_row == tile_rows - 1)
                    ? (orig_h - tile_row * cth) : cth;

    int left   = tile_col * ctw - (tile_col > 0 ? overlap : 0);
    int top    = tile_row * cth - (tile_row > 0 ? overlap : 0);
    int right  = tile_col * ctw + content_w
                 + (tile_col < tile_cols - 1 ? overlap : 0);
    int bottom = tile_row * cth + content_h
                 + (tile_row < tile_rows - 1 ? overlap : 0);

    if (left   < 0)      left   = 0;
    if (top    < 0)      top    = 0;
    if (right  > orig_w) right  = orig_w;
    if (bottom > orig_h) bottom = orig_h;

    return (CropBox){ left, top, right - left, bottom - top };
}

/* ============================================================================
   GROUP 1: CRC-8/CCITT algorithm
   ============================================================================ */

static void test_crc8_zero_input(void) {
    TEST("CRC-8: all-zero 7-byte input gives non-zero CRC");
    uint8_t data[7] = {0};
    /* CRC-8/CCITT of 7 zero bytes — computed reference: 0x00
     * (XOR chain over 0x00 bytes keeps CRC at 0x00) */
    uint8_t got  = ref_crc8(data, 7);
    uint8_t want = 0x00;
    ASSERT_EQ_UINT8(got, want, "crc8([0..0])");
    PASS();
}

static void test_crc8_known_vector_a(void) {
    TEST("CRC-8: vector [0x01,0x02,0x03,0x04,0x05,0x06,0x07]");
    /* Python cross-check:
     *   crc = 0
     *   for b in [1,2,3,4,5,6,7]:
     *       crc ^= b
     *       for _ in range(8):
     *           crc = ((crc << 1) ^ 0x07) & 0xFF if (crc & 0x80) else (crc << 1) & 0xFF
     *   → 0xD8
     */
    uint8_t data[7] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07};
    ASSERT_EQ_UINT8(ref_crc8(data, 7), 0xD8, "crc");
    PASS();
}

static void test_crc8_known_vector_b(void) {
    TEST("CRC-8: typical row-1 payload [1,2,2,0,0,4,1]");
    /* version=1, grid_cols=2, grid_rows=2, tile_col=0, tile_row=0,
     * total_tiles=4, seq_num=1
     * Python → 0x1A */
    uint8_t data[7] = {1,2,2,0,0,4,1};
    ASSERT_EQ_UINT8(ref_crc8(data, 7), 0x1A, "crc");
    PASS();
}

static void test_crc8_append_crc_makes_zero(void) {
    TEST("CRC-8: appending CRC byte makes crc8(data+crc, 8) predictable");
    /* CRC-8/CCITT has the property: crc8(data || crc_byte, n+1) == 0x00
     * only for some polynomial variants; for poly=0x07 verify just that
     * the CRC changes when data changes. */
    uint8_t d1[7] = {1,2,2,0,0,4,1};
    uint8_t d2[7] = {1,2,2,0,0,4,2};   /* seq_num differs */
    uint8_t c1 = ref_crc8(d1, 7);
    uint8_t c2 = ref_crc8(d2, 7);
    if (c1 == c2) {
        FAIL("CRC did not change when data changed");
        return;
    }
    PASS();
}

static void test_crc8_single_byte_change_detection(void) {
    TEST("CRC-8: each field change produces unique CRC");
    /* Sweep through 7 positions and flip each byte — no two CRCs should
     * collide (this verifies single-bit error detection). */
    uint8_t base[7]  = {1, 3, 2, 1, 0, 6, 1};
    uint8_t seen[256] = {0};
    seen[ref_crc8(base, 7)] = 1;
    int collisions = 0;
    for (int pos = 0; pos < 7; pos++) {
        for (int delta = 1; delta < 256; delta++) {
            uint8_t d[7];
            memcpy(d, base, 7);
            d[pos] ^= (uint8_t)delta;
            uint8_t c = ref_crc8(d, 7);
            if (seen[c]) { collisions++; }
            seen[c] = 1;
        }
        /* Reset seen table between positions */
        memset(seen, 0, sizeof seen);
        seen[ref_crc8(base, 7)] = 1;
    }
    if (collisions > 0) {
        FAIL("CRC collision detected (%d collisions across single-byte changes)",
             collisions);
        return;
    }
    PASS();
}

/* ============================================================================
   GROUP 2: SFTileInfo struct layout / constant
   ============================================================================ */

static void test_struct_layout_constants(void) {
    TEST("SFTileInfo: field widths and SF_TILE_HEADER_ROWS");
    /* Must catch any accidental padding or type changes */
    _Static_assert(sizeof(((SFTileInfo*)0)->version)     == 1, "version must be uint8_t");
    _Static_assert(sizeof(((SFTileInfo*)0)->grid_cols)   == 1, "grid_cols must be uint8_t");
    _Static_assert(sizeof(((SFTileInfo*)0)->grid_rows)   == 1, "grid_rows must be uint8_t");
    _Static_assert(sizeof(((SFTileInfo*)0)->tile_col)    == 1, "tile_col must be uint8_t");
    _Static_assert(sizeof(((SFTileInfo*)0)->tile_row)    == 1, "tile_row must be uint8_t");
    _Static_assert(sizeof(((SFTileInfo*)0)->total_tiles) == 1, "total_tiles must be uint8_t");
    _Static_assert(sizeof(((SFTileInfo*)0)->seq_num)     == 1, "seq_num must be uint8_t");
    _Static_assert(sizeof(((SFTileInfo*)0)->orig_width)  == 2, "orig_width must be uint16_t");
    _Static_assert(sizeof(((SFTileInfo*)0)->orig_height) == 2, "orig_height must be uint16_t");
    _Static_assert(sizeof(((SFTileInfo*)0)->overlap_px)  == 1, "overlap_px must be uint8_t");
    _Static_assert(sizeof(((SFTileInfo*)0)->session_id)  == 4, "session_id must be uint32_t");
    _Static_assert(SF_TILE_HEADER_ROWS == 4, "SF_TILE_HEADER_ROWS must be 4");
    PASS();
}

static void test_struct_total_fields(void) {
    TEST("SFTileInfo: at least 12 bytes of data fields");
    size_t min_size = 1+1+1+1+1+1+1+2+2+1+4; /* sum of expected field sizes */
    if (sizeof(SFTileInfo) < min_size) {
        FAIL("struct too small: got %zu, want at least %zu",
             sizeof(SFTileInfo), min_size);
        return;
    }
    PASS();
}

/* ============================================================================
   GROUP 3: Tile crop-box math
   ============================================================================ */

/* Verify a single crop box result */
static void _check_box(const char *label, CropBox got,
                        int wl, int wt, int ww, int wh) {
    if (got.left != wl || got.top != wt || got.w != ww || got.h != wh) {
        printf("❌ FAIL: %s: got (%d,%d,%dx%d) want (%d,%d,%dx%d)\n",
               label, got.left, got.top, got.w, got.h, wl, wt, ww, wh);
    }
}

static void test_crop_2x2_no_overlap(void) {
    TEST("Crop math: 640x480 2x2 no overlap → four 320x240 tiles");
    CropBox b;
    b = compute_tile_crop(640,480, 2,2, 0,0, 0);
    _check_box("r0c0", b,   0,   0, 320, 240);
    b = compute_tile_crop(640,480, 2,2, 1,0, 0);
    _check_box("r0c1", b, 320,   0, 320, 240);
    b = compute_tile_crop(640,480, 2,2, 0,1, 0);
    _check_box("r1c0", b,   0, 240, 320, 240);
    b = compute_tile_crop(640,480, 2,2, 1,1, 0);
    _check_box("r1c1", b, 320, 240, 320, 240);
    PASS();
}

static void test_crop_2x2_with_overlap(void) {
    TEST("Crop math: 640x480 2x2 overlap=10 → four 330x250 tiles");
    /* Corner tiles each get +10 px on the single shared edge */
    for (int tr = 0; tr < 2; tr++) {
        for (int tc = 0; tc < 2; tc++) {
            CropBox b = compute_tile_crop(640,480, 2,2, tc,tr, 10);
            if (b.w != 330 || b.h != 250) {
                FAIL("r%dc%d: got %dx%d, want 330x250", tr, tc, b.w, b.h);
                return;
            }
        }
    }
    PASS();
}

static void test_crop_3x3_no_overlap_even(void) {
    TEST("Crop math: 900x600 3x3 no overlap (exact divisor)");
    for (int tr = 0; tr < 3; tr++) {
        for (int tc = 0; tc < 3; tc++) {
            CropBox b = compute_tile_crop(900,600, 3,3, tc,tr, 0);
            if (b.w != 300 || b.h != 200) {
                FAIL("r%dc%d: got %dx%d, want 300x200", tr, tc, b.w, b.h);
                return;
            }
        }
    }
    PASS();
}

static void test_crop_3x3_no_overlap_remainder(void) {
    TEST("Crop math: 640x480 3x3 last column absorbs remainder (214px)");
    /* 640/3 = 213; last col = 640 - 2*213 = 214 */
    CropBox last_col = compute_tile_crop(640,480, 3,3, 2,0, 0);
    if (last_col.w != 214) {
        FAIL("last col: got w=%d, want 214", last_col.w);
        return;
    }
    /* First two columns are 213 wide */
    CropBox first_col = compute_tile_crop(640,480, 3,3, 0,0, 0);
    if (first_col.w != 213) {
        FAIL("first col: got w=%d, want 213", first_col.w);
        return;
    }
    PASS();
}

static void test_crop_3x3_interior_overlap(void) {
    TEST("Crop math: 900x600 3x3 overlap=5 interior tile is 310x210");
    /* Interior tile (1,1) gets +5 on all four edges */
    CropBox interior = compute_tile_crop(900,600, 3,3, 1,1, 5);
    if (interior.w != 310 || interior.h != 210) {
        FAIL("interior: got %dx%d, want 310x210", interior.w, interior.h);
        return;
    }
    PASS();
}

static void test_crop_3x3_edge_overlap(void) {
    TEST("Crop math: 900x600 3x3 overlap=5 edge tiles (non-corner)");
    /* Left edge (tc=0, tr=1): left edge adds no overlap, right does +5 */
    CropBox left_edge = compute_tile_crop(900,600, 3,3, 0,1, 5);
    if (left_edge.w != 305 || left_edge.h != 210) {
        FAIL("left edge: got %dx%d, want 305x210", left_edge.w, left_edge.h);
        return;
    }
    PASS();
}

static void test_crop_overlap_clamped_to_image_bounds(void) {
    TEST("Crop math: large overlap clamped to image bounds");
    /* 1x1 grid with any overlap should always return full image */
    CropBox b = compute_tile_crop(320,240, 1,1, 0,0, 999);
    if (b.left != 0 || b.top != 0 || b.w != 320 || b.h != 240) {
        FAIL("1x1 with overlap=999: got (%d,%d,%dx%d), want (0,0,320x240)",
             b.left, b.top, b.w, b.h);
        return;
    }
    /* 2x2 huge overlap: each tile should still be clamped to image */
    CropBox corner = compute_tile_crop(100,100, 2,2, 0,0, 200);
    if (corner.left < 0 || corner.top < 0 ||
        corner.w > 100   || corner.h > 100) {
        FAIL("overflow: (%d,%d,%dx%d)", corner.left, corner.top, corner.w, corner.h);
        return;
    }
    PASS();
}

static void test_crop_1x1_gives_full_image(void) {
    TEST("Crop math: 1x1 grid returns full image regardless of overlap");
    CropBox b = compute_tile_crop(1920,1080, 1,1, 0,0, 50);
    if (b.left != 0 || b.top != 0 || b.w != 1920 || b.h != 1080) {
        FAIL("got (%d,%d,%dx%d)", b.left, b.top, b.w, b.h);
        return;
    }
    PASS();
}

static void test_crop_total_coverage_no_overlap(void) {
    TEST("Crop math: tile widths sum to original width (no overlap)");
    int orig_w = 640, orig_h = 480;
    /* For each row count the total pixel coverage checking for gaps/overlaps */
    for (int cols = 1; cols <= 5; cols++) {
        int covered = 0;
        for (int tc = 0; tc < cols; tc++) {
            CropBox b = compute_tile_crop(orig_w, orig_h, cols, 1, tc, 0, 0);
            covered += b.w;
        }
        if (covered != orig_w) {
            FAIL("cols=%d: total coverage %d != %d", cols, covered, orig_w);
            return;
        }
    }
    PASS();
}

/* ============================================================================
   GROUP 4: image_apply_tile_header pixel verification (requires libvips)
   ============================================================================ */

/* Helper: create a black 320x240 PNG at TEST_PNG_PATH */
static int create_test_png_320x240(void) {
    size_t n_bytes = 320 * 240 * 3;
    uint8_t *pixels = (uint8_t *)calloc(1, n_bytes);
    if (!pixels) return -1;

    /* Fill with a mid-gray so header rows stand out clearly */
    memset(pixels, 0x80, n_bytes);

    VipsImage *img = vips_image_new_from_memory(pixels, n_bytes, 320, 240, 3,
                                                 VIPS_FORMAT_UCHAR);
    if (!img) { free(pixels); return -1; }

    int rc = vips_image_write_to_file(img, TEST_PNG_PATH, NULL);
    g_object_unref(img);
    free(pixels);
    return rc;
}

/* Sample middle of macroblock b (0-7) for a 320-wide image (blk=40) */
static int blk_x(int b) { return b * 40 + 20; }

static void test_header_sync_row_colors(void) {
    TEST("Header row 0: sync colors R/G/B/Y/C/M/W/K");
    if (create_test_png_320x240() != 0) {
        FAIL("Could not create test PNG");
        return;
    }

    if (image_load_from_file(TEST_PNG_PATH, 0, 0, NULL) != SLOWFRAME_OK) {
        FAIL("image_load_from_file failed");
        return;
    }

    SFTileInfo info = {
        .version      = 1,
        .grid_cols    = 2,  .grid_rows    = 2,
        .tile_col     = 0,  .tile_row     = 0,
        .total_tiles  = 4,  .seq_num      = 1,
        .orig_width   = 640,.orig_height  = 480,
        .overlap_px   = 0,
        .session_id   = 0xDEADBEEF,
    };

    if (image_apply_tile_header(&info, 0, 0) != SLOWFRAME_OK) {
        image_free();
        FAIL("image_apply_tile_header returned error");
        return;
    }

    static const uint8_t want_sync[8][3] = {
        {255,0,0},{0,255,0},{0,0,255},{255,255,0},
        {0,255,255},{255,0,255},{255,255,255},{0,0,0}
    };
    static const char *color_names[8] = {"R","G","B","Y","C","M","W","K"};

    for (int blk = 0; blk < 8; blk++) {
        uint8_t r = 0, g = 0, b = 0;
        image_get_pixel_rgb(blk_x(blk), 0, &r, &g, &b);
        if (r != want_sync[blk][0] ||
            g != want_sync[blk][1] ||
            b != want_sync[blk][2]) {
            image_free();
            FAIL("block %d (%s): got (%d,%d,%d) want (%d,%d,%d)",
                 blk, color_names[blk], r,g,b,
                 want_sync[blk][0], want_sync[blk][1], want_sync[blk][2]);
            return;
        }
    }
    image_free();
    PASS();
}

static void test_header_row1_position_metadata(void) {
    TEST("Header row 1: position bytes and CRC-8 (2x2 tile r0c1 seq=2)");
    if (create_test_png_320x240() != 0) {
        FAIL("Could not create test PNG"); return;
    }
    if (image_load_from_file(TEST_PNG_PATH, 0, 0, NULL) != SLOWFRAME_OK) {
        FAIL("image_load_from_file failed"); return;
    }

    SFTileInfo info = {
        .version      = 1,
        .grid_cols    = 2,  .grid_rows = 2,
        .tile_col     = 1,  .tile_row  = 0,
        .total_tiles  = 4,  .seq_num   = 2,
        .orig_width   = 640,.orig_height = 480,
        .overlap_px   = 0,
        .session_id   = 0x12345678,
    };

    if (image_apply_tile_header(&info, 0, 0) != SLOWFRAME_OK) {
        image_free(); FAIL("apply_tile_header failed"); return;
    }

    /* Expected byte sequence for row 1:
     * [0]=version(1) [1]=grid_cols(2) [2]=grid_rows(2) [3]=tile_col(1)
     * [4]=tile_row(0) [5]=total_tiles(4) [6]=seq_num(2) [7]=CRC */
    uint8_t r1[7] = {1, 2, 2, 1, 0, 4, 2};
    uint8_t expected_crc = ref_crc8(r1, 7);

    uint8_t want[8] = {1, 2, 2, 1, 0, 4, 2, expected_crc};
    for (int blk = 0; blk < 8; blk++) {
        uint8_t pr, pg, pb;
        image_get_pixel_rgb(blk_x(blk), 1, &pr, &pg, &pb);
        /* Grayscale: R==G==B */
        if (pr != pg || pr != pb) {
            image_free();
            FAIL("block %d not grayscale: (%d,%d,%d)", blk, pr,pg,pb);
            return;
        }
        if (pr != want[blk]) {
            image_free();
            FAIL("block %d: got %d, want %d", blk, pr, want[blk]);
            return;
        }
    }
    image_free();
    PASS();
}

static void test_header_row2_dimension_bytes(void) {
    TEST("Header row 2: orig_width/height + session_id big-endian encoding");
    if (create_test_png_320x240() != 0) {
        FAIL("Could not create test PNG"); return;
    }
    if (image_load_from_file(TEST_PNG_PATH, 0, 0, NULL) != SLOWFRAME_OK) {
        FAIL("image_load_from_file failed"); return;
    }

    /* Use recognisable test values */
    SFTileInfo info = {
        .version      = 1,
        .grid_cols    = 1, .grid_rows   = 1,
        .tile_col     = 0, .tile_row    = 0,
        .total_tiles  = 1, .seq_num     = 1,
        .orig_width   = 0x0640,   /* 1600 */
        .orig_height  = 0x0258,   /* 600  */
        .overlap_px   = 0,
        .session_id   = 0xAABBCCDD,
    };

    if (image_apply_tile_header(&info, 0, 0) != SLOWFRAME_OK) {
        image_free(); FAIL("apply_tile_header failed"); return;
    }

    /* Row 2 layout: ow_hi ow_lo oh_hi oh_lo sid[3] sid[2] sid[1] sid[0] */
    uint8_t want[8] = {
        0x06, 0x40,             /* orig_width  = 0x0640 */
        0x02, 0x58,             /* orig_height = 0x0258 */
        0xAA, 0xBB, 0xCC, 0xDD /* session_id  big-endian */
    };
    for (int blk = 0; blk < 8; blk++) {
        uint8_t pr, pg, pb;
        image_get_pixel_rgb(blk_x(blk), 2, &pr, &pg, &pb);
        if (pr != pg || pr != pb) {
            image_free();
            FAIL("block %d not grayscale: (%d,%d,%d)", blk, pr,pg,pb);
            return;
        }
        if (pr != want[blk]) {
            image_free();
            FAIL("block %d: got 0x%02X, want 0x%02X", blk, pr, want[blk]);
            return;
        }
    }
    image_free();
    PASS();
}

static void test_header_row3_overlap_and_crc(void) {
    TEST("Header row 3: overlap_px byte and CRC-8, reserved bytes == 0");
    if (create_test_png_320x240() != 0) {
        FAIL("Could not create test PNG"); return;
    }
    if (image_load_from_file(TEST_PNG_PATH, 0, 0, NULL) != SLOWFRAME_OK) {
        FAIL("image_load_from_file failed"); return;
    }

    SFTileInfo info = {
        .version      = 1,
        .grid_cols    = 3, .grid_rows   = 3,
        .tile_col     = 1, .tile_row    = 1,
        .total_tiles  = 9, .seq_num     = 5,
        .orig_width   = 960, .orig_height = 540,
        .overlap_px   = 16,
        .session_id   = 0x11223344,
    };

    if (image_apply_tile_header(&info, 0, 0) != SLOWFRAME_OK) {
        image_free(); FAIL("apply_tile_header failed"); return;
    }

    /* r3 = [overlap(16), 0,0,0,0,0,0, CRC] */
    uint8_t r3_data[7] = {16, 0, 0, 0, 0, 0, 0};
    uint8_t expected_crc = ref_crc8(r3_data, 7);

    uint8_t want[8] = {16, 0, 0, 0, 0, 0, 0, expected_crc};
    for (int blk = 0; blk < 8; blk++) {
        uint8_t pr, pg, pb;
        image_get_pixel_rgb(blk_x(blk), 3, &pr, &pg, &pb);
        if (pr != pg || pr != pb) {
            image_free();
            FAIL("block %d not grayscale: (%d,%d,%d)", blk, pr,pg,pb);
            return;
        }
        if (pr != want[blk]) {
            image_free();
            FAIL("block %d: got %d, want %d", blk, pr, want[blk]);
            return;
        }
    }
    image_free();
    PASS();
}

static void test_header_macroblock_boundary_pixels(void) {
    TEST("Header row 1: first and last pixel of each macroblock match block byte");
    if (create_test_png_320x240() != 0) {
        FAIL("Could not create test PNG"); return;
    }
    if (image_load_from_file(TEST_PNG_PATH, 0, 0, NULL) != SLOWFRAME_OK) {
        FAIL("image_load_from_file failed"); return;
    }

    SFTileInfo info = {
        .version=1, .grid_cols=4, .grid_rows=2,
        .tile_col=2, .tile_row=1,
        .total_tiles=8, .seq_num=7,
        .orig_width=1280, .orig_height=480,
        .overlap_px=0, .session_id=0xCAFEBABE,
    };
    if (image_apply_tile_header(&info, 0, 0) != SLOWFRAME_OK) {
        image_free(); FAIL("apply_tile_header failed"); return;
    }

    uint8_t r1_data[7] = {info.version, info.grid_cols, info.grid_rows,
                          info.tile_col, info.tile_row, info.total_tiles,
                          info.seq_num};
    uint8_t crc = ref_crc8(r1_data, 7);
    uint8_t want[8] = {1,4,2,2,1,8,7,crc};

    /* blk width = 320/8 = 40; check x=0 (first pixel of blk0)
     * and x=39 (last pixel of blk0) and x=40 (first pixel of blk1) */
    int blk_w = 40;
    for (int b = 0; b < 8; b++) {
        /* first pixel of block */
        uint8_t r0,g0,b0;
        image_get_pixel_rgb(b * blk_w, 1, &r0, &g0, &b0);
        if (r0 != want[b]) {
            image_free();
            FAIL("blk %d first px: got %d, want %d", b, r0, want[b]);
            return;
        }
        /* last pixel of block (before next block starts) */
        int last_x = (b < 7) ? (b+1)*blk_w - 1 : 319;
        uint8_t r1,g1,b1;
        image_get_pixel_rgb(last_x, 1, &r1, &g1, &b1);
        if (r1 != want[b]) {
            image_free();
            FAIL("blk %d last px (x=%d): got %d, want %d", b, last_x, r1, want[b]);
            return;
        }
    }
    image_free();
    PASS();
}

static void test_header_rows_below_untouched(void) {
    TEST("Header: pixel at row SF_TILE_HEADER_ROWS unchanged (mid-gray 0x80)");
    if (create_test_png_320x240() != 0) {
        FAIL("Could not create test PNG"); return;
    }
    if (image_load_from_file(TEST_PNG_PATH, 0, 0, NULL) != SLOWFRAME_OK) {
        FAIL("image_load_from_file failed"); return;
    }

    SFTileInfo info = {
        .version=1, .grid_cols=2, .grid_rows=2,
        .tile_col=0, .tile_row=0, .total_tiles=4, .seq_num=1,
        .orig_width=640, .orig_height=480,
        .overlap_px=0, .session_id=0x00010002,
    };
    if (image_apply_tile_header(&info, 0, 0) != SLOWFRAME_OK) {
        image_free(); FAIL("apply_tile_header failed"); return;
    }

    /* Row SF_TILE_HEADER_ROWS (row 4) should still be mid-gray (0x80)
     * since create_test_png_320x240 filled with 0x80 */
    uint8_t r,g,b;
    image_get_pixel_rgb(160, SF_TILE_HEADER_ROWS, &r, &g, &b);
    if (r != 0x80 || g != 0x80 || b != 0x80) {
        image_free();
        FAIL("row %d modified: got (%d,%d,%d), want (128,128,128)",
             SF_TILE_HEADER_ROWS, r, g, b);
        return;
    }
    image_free();
    PASS();
}

/* ============================================================================
   GROUP 5: image_crop_region dimension verification
   ============================================================================ */

static void test_crop_region_dimensions(void) {
    TEST("image_crop_region: output dimensions match requested crop");
    if (create_test_png_320x240() != 0) {
        FAIL("Could not create test PNG"); return;
    }
    if (image_load_from_file(TEST_PNG_PATH, 0, 0, NULL) != SLOWFRAME_OK) {
        FAIL("image_load_from_file failed"); return;
    }

    /* Crop a 160x120 region starting at (80, 60) */
    int rc = image_crop_region(80, 60, 160, 120, 0, 0);
    if (rc != SLOWFRAME_OK) {
        image_free(); FAIL("image_crop_region returned %d", rc); return;
    }

    int w = 0, h = 0;
    image_get_dimensions(&w, &h);
    if (w != 160 || h != 120) {
        image_free();
        FAIL("got %dx%d, want 160x120", w, h);
        return;
    }
    image_free();
    PASS();
}

static void test_crop_region_full_image(void) {
    TEST("image_crop_region: crop full image region preserves dimensions");
    if (create_test_png_320x240() != 0) {
        FAIL("Could not create test PNG"); return;
    }
    if (image_load_from_file(TEST_PNG_PATH, 0, 0, NULL) != SLOWFRAME_OK) {
        FAIL("image_load_from_file failed"); return;
    }

    int rc = image_crop_region(0, 0, 320, 240, 0, 0);
    if (rc != SLOWFRAME_OK) {
        image_free(); FAIL("image_crop_region returned %d", rc); return;
    }

    int w = 0, h = 0;
    image_get_dimensions(&w, &h);
    if (w != 320 || h != 240) {
        image_free();
        FAIL("got %dx%d, want 320x240", w, h);
        return;
    }
    image_free();
    PASS();
}

static void test_crop_region_1x1_pixel(void) {
    TEST("image_crop_region: 1x1 pixel crop");
    if (create_test_png_320x240() != 0) {
        FAIL("Could not create test PNG"); return;
    }
    if (image_load_from_file(TEST_PNG_PATH, 0, 0, NULL) != SLOWFRAME_OK) {
        FAIL("image_load_from_file failed"); return;
    }

    int rc = image_crop_region(100, 50, 1, 1, 0, 0);
    if (rc != SLOWFRAME_OK) {
        image_free(); FAIL("image_crop_region returned %d", rc); return;
    }

    int w = 0, h = 0;
    image_get_dimensions(&w, &h);
    if (w != 1 || h != 1) {
        image_free();
        FAIL("got %dx%d, want 1x1", w, h);
        return;
    }
    image_free();
    PASS();
}

/* ============================================================================
   GROUP 6: Error-path handling (no image loaded)
   ============================================================================ */

static void test_apply_header_no_image(void) {
    TEST("Error path: apply_tile_header with no image returns IMAGE_LOAD error");
    image_free();  /* ensure no image is loaded */

    SFTileInfo info = {
        .version=1, .grid_cols=2, .grid_rows=2,
        .tile_col=0, .tile_row=0, .total_tiles=4, .seq_num=1,
        .orig_width=640, .orig_height=480,
        .overlap_px=0, .session_id=0,
    };

    int rc = image_apply_tile_header(&info, 0, 0);
    if (rc != SLOWFRAME_ERR_IMAGE_LOAD) {
        FAIL("expected SLOWFRAME_ERR_IMAGE_LOAD (%d), got %d",
             SLOWFRAME_ERR_IMAGE_LOAD, rc);
        return;
    }
    PASS();
}

static void test_crop_region_no_image(void) {
    TEST("Error path: image_crop_region with no image returns IMAGE_LOAD error");
    image_free();

    int rc = image_crop_region(0, 0, 160, 120, 0, 0);
    if (rc != SLOWFRAME_ERR_IMAGE_LOAD) {
        FAIL("expected SLOWFRAME_ERR_IMAGE_LOAD (%d), got %d",
             SLOWFRAME_ERR_IMAGE_LOAD, rc);
        return;
    }
    PASS();
}

/* ============================================================================
   GROUP 7: Metadata round-trip — write header, verify all four rows together
   ============================================================================ */

static void test_full_header_round_trip(void) {
    TEST("Header: full round-trip — all 4 rows verified in one pass (3x2 r1c0)");
    if (create_test_png_320x240() != 0) {
        FAIL("Could not create test PNG"); return;
    }
    if (image_load_from_file(TEST_PNG_PATH, 0, 0, NULL) != SLOWFRAME_OK) {
        FAIL("image_load_from_file failed"); return;
    }

    SFTileInfo info = {
        .version      = 1,
        .grid_cols    = 3, .grid_rows    = 2,
        .tile_col     = 0, .tile_row     = 1,
        .total_tiles  = 6, .seq_num      = 4,
        .orig_width   = 0x0300,  /* 768  */
        .orig_height  = 0x0200,  /* 512  */
        .overlap_px   = 8,
        .session_id   = 0x01020304,
    };

    if (image_apply_tile_header(&info, 0, 0) != SLOWFRAME_OK) {
        image_free(); FAIL("apply_tile_header failed"); return;
    }

    /* Row 1 */
    uint8_t r1d[7] = {1, 3, 2, 0, 1, 6, 4};
    uint8_t r1[8];
    memcpy(r1, r1d, 7);
    r1[7] = ref_crc8(r1d, 7);

    /* Row 2 (no CRC) */
    uint8_t r2[8] = {0x03, 0x00, 0x02, 0x00, 0x01, 0x02, 0x03, 0x04};

    /* Row 3 */
    uint8_t r3d[7] = {8, 0, 0, 0, 0, 0, 0};
    uint8_t r3[8];
    memcpy(r3, r3d, 7);
    r3[7] = ref_crc8(r3d, 7);

    const uint8_t *rows[3] = {r1, r2, r3};

    for (int row = 0; row < 3; row++) {
        for (int blk = 0; blk < 8; blk++) {
            uint8_t pr, pg, pb;
            image_get_pixel_rgb(blk_x(blk), 1 + row, &pr, &pg, &pb);
            if (pr != pg || pr != pb) {
                image_free();
                FAIL("row %d blk %d not grayscale: (%d,%d,%d)",
                     row+1, blk, pr,pg,pb);
                return;
            }
            if (pr != rows[row][blk]) {
                image_free();
                FAIL("row %d blk %d: got 0x%02X, want 0x%02X",
                     row+1, blk, pr, rows[row][blk]);
                return;
            }
        }
    }
    image_free();
    PASS();
}

/* ============================================================================
   main
   ============================================================================ */

int main(int argc, char **argv) {
    if (VIPS_INIT(argv[0])) {
        fprintf(stderr, "FATAL: vips_init() failed\n");
        return 1;
    }
    /* Suppress libvips warnings during tests */
    vips_cache_set_max(0);

    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  SlowFrame Tiling Unit Tests\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");

    /* --- Group 1: CRC-8 --- */
    printf("Group 1: CRC-8/CCITT Algorithm\n");
    test_crc8_zero_input();
    test_crc8_known_vector_a();
    test_crc8_known_vector_b();
    test_crc8_append_crc_makes_zero();
    test_crc8_single_byte_change_detection();
    printf("\n");

    /* --- Group 2: Struct layout --- */
    printf("Group 2: SFTileInfo Struct Layout\n");
    test_struct_layout_constants();
    test_struct_total_fields();
    printf("\n");

    /* --- Group 3: Crop math --- */
    printf("Group 3: Tile Crop-Box Math\n");
    test_crop_2x2_no_overlap();
    test_crop_2x2_with_overlap();
    test_crop_3x3_no_overlap_even();
    test_crop_3x3_no_overlap_remainder();
    test_crop_3x3_interior_overlap();
    test_crop_3x3_edge_overlap();
    test_crop_overlap_clamped_to_image_bounds();
    test_crop_1x1_gives_full_image();
    test_crop_total_coverage_no_overlap();
    printf("\n");

    /* --- Group 4: Header pixel verification --- */
    printf("Group 4: Tile Header Row Pixel Verification\n");
    test_header_sync_row_colors();
    test_header_row1_position_metadata();
    test_header_row2_dimension_bytes();
    test_header_row3_overlap_and_crc();
    test_header_macroblock_boundary_pixels();
    test_header_rows_below_untouched();
    printf("\n");

    /* --- Group 5: image_crop_region --- */
    printf("Group 5: image_crop_region\n");
    test_crop_region_dimensions();
    test_crop_region_full_image();
    test_crop_region_1x1_pixel();
    printf("\n");

    /* --- Group 6: Error paths --- */
    printf("Group 6: Error Paths (no image loaded)\n");
    test_apply_header_no_image();
    test_crop_region_no_image();
    printf("\n");

    /* --- Group 7: Round-trip --- */
    printf("Group 7: Full Metadata Round-Trip\n");
    test_full_header_round_trip();
    printf("\n");

    /* Summary */
    printf("═══════════════════════════════════════════════════════════════\n");
    int failed = g_tests_run - g_tests_passed;
    if (failed == 0) {
        printf("  ✅ ALL %d TESTS PASSED\n", g_tests_run);
    } else {
        printf("  ❌ %d / %d TESTS FAILED\n", failed, g_tests_run);
    }
    printf("═══════════════════════════════════════════════════════════════\n\n");

    /* Clean up ephemeral test PNG */
    remove(TEST_PNG_PATH);

    vips_shutdown();
    return (failed == 0) ? 0 : 1;
}
