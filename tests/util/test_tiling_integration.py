#!/usr/bin/env python3
"""
Tiling Integration Tests
=========================

Black-box tests that run the SlowFrame binary and verify:
  1. Correct audio file count, naming convention, and duration
  2. Sync-row pixel colors in intermediate PNGs (row 0: R/G/B/Y/C/M/W/K)
  3. Per-tile position metadata in header row 1 (version, grid, col, row, CRC)
  4. Dimension / session-ID metadata in header row 2
  5. Overlap metadata in header row 3
  6. Consistent session ID across all tiles in a run
  7. Grid size variants: 1×1, 2×2, 3×2, 1×3

Pixel values are read with ``vips getpoint`` (libvips CLI), which is already
a hard dependency of the project.  No Pillow or other extra packages required.

Usage:
    python3 tests/util/test_tiling_integration.py [-v]

Author: SlowFrame Contributors
Version: 2.1.0
Date: February 2026
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

# ---------------------------------------------------------------------------
# Project paths
# ---------------------------------------------------------------------------
PROJECT_ROOT = os.path.dirname(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BINARY       = os.path.join(PROJECT_ROOT, "bin", "slowframe")
VIPS_BIN     = shutil.which("vips") or "vips"

# ---------------------------------------------------------------------------
# CRC-8/CCITT reference  (polynomial 0x07, init 0x00)
# ---------------------------------------------------------------------------
def crc8(data: bytes) -> int:
    crc = 0
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = ((crc << 1) ^ 0x07) & 0xFF if (crc & 0x80) else (crc << 1) & 0xFF
    return crc


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
def create_black_png(path: str, width: int = 640, height: int = 480) -> None:
    """Create a black RGB PNG at *path* using the vips CLI."""
    tmp_v = path + ".v"
    subprocess.run(
        [VIPS_BIN, "black", tmp_v, str(width), str(height), "--bands", "3"],
        check=True, capture_output=True)
    subprocess.run(
        [VIPS_BIN, "copy", tmp_v, path],
        check=True, capture_output=True)
    os.remove(tmp_v)


def vips_getpoint(image_path: str, x: int, y: int) -> tuple[int, int, int]:
    """Return the RGB pixel value at (x, y) using ``vips getpoint``.

    Handles both 3-band (RGB) and 4-band (RGBA) output safely.
    """
    result = subprocess.run(
        [VIPS_BIN, "getpoint", image_path, str(x), str(y)],
        capture_output=True, text=True, check=True)
    values = [int(v) for v in result.stdout.split() if v.strip()]
    return values[0], values[1], values[2]


def run_slowframe(*args, cwd: str | None = None) -> subprocess.CompletedProcess:
    """Run ./bin/slowframe with the given arguments."""
    cmd = [BINARY] + list(args)
    return subprocess.run(cmd, capture_output=True, text=True, cwd=cwd)


def blk_sample_x(block: int, image_width: int = 320) -> int:
    """X coordinate of the centre pixel of macroblock *block* (0-7)."""
    blk_w = image_width // 8
    return block * blk_w + blk_w // 2


# ---------------------------------------------------------------------------
# Expected sync-row palette
# ---------------------------------------------------------------------------
SYNC_COLORS = [
    (255,   0,   0),   # block 0 — R
    (  0, 255,   0),   # block 1 — G
    (  0,   0, 255),   # block 2 — B
    (255, 255,   0),   # block 3 — Y
    (  0, 255, 255),   # block 4 — C
    (255,   0, 255),   # block 5 — M
    (255, 255, 255),   # block 6 — W
    (  0,   0,   0),   # block 7 — K
]


# ---------------------------------------------------------------------------
# Test cases
# ---------------------------------------------------------------------------
class TestTileFileCreation(unittest.TestCase):
    """Verify that the correct audio files are created with correct names."""

    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="sf_tile_test_")
        self.input_png = os.path.join(self.tmp, "input.png")
        create_black_png(self.input_png)

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def _wav_basenames(self) -> list[str]:
        return sorted(f for f in os.listdir(self.tmp) if f.endswith(".wav"))

    def test_2x2_creates_four_audio_files(self):
        out = os.path.join(self.tmp, "out.wav")
        r = run_slowframe("-i", self.input_png, "-p", "r36",
                          "-X", "2x2", "-o", out, cwd=self.tmp)
        self.assertEqual(r.returncode, 0, msg=r.stdout + r.stderr)
        wavs = self._wav_basenames()
        self.assertEqual(len(wavs), 4, msg=f"Expected 4 wav files, got: {wavs}")

    def test_2x2_correct_naming(self):
        out = os.path.join(self.tmp, "out.wav")
        run_slowframe("-i", self.input_png, "-p", "r36",
                      "-X", "2x2", "-o", out, cwd=self.tmp)
        wavs = set(self._wav_basenames())
        expected = {
            "out-tile-01-r0c0.wav",
            "out-tile-02-r0c1.wav",
            "out-tile-03-r1c0.wav",
            "out-tile-04-r1c1.wav",
        }
        self.assertEqual(wavs, expected)

    def test_3x2_creates_six_audio_files(self):
        out = os.path.join(self.tmp, "out.wav")
        r = run_slowframe("-i", self.input_png, "-p", "r36",
                          "-X", "3x2", "-o", out, cwd=self.tmp)
        self.assertEqual(r.returncode, 0, msg=r.stdout + r.stderr)
        wavs = self._wav_basenames()
        self.assertEqual(len(wavs), 6, msg=f"Expected 6, got: {wavs}")

    def test_1x3_creates_three_audio_files(self):
        out = os.path.join(self.tmp, "out.wav")
        r = run_slowframe("-i", self.input_png, "-p", "r36",
                          "-X", "1x3", "-o", out, cwd=self.tmp)
        self.assertEqual(r.returncode, 0, msg=r.stdout + r.stderr)
        wavs = self._wav_basenames()
        self.assertEqual(len(wavs), 3, msg=f"Expected 3, got: {wavs}")

    def test_1x1_creates_one_audio_file(self):
        out = os.path.join(self.tmp, "out.wav")
        r = run_slowframe("-i", self.input_png, "-p", "r36",
                          "-X", "1x1", "-o", out, cwd=self.tmp)
        self.assertEqual(r.returncode, 0, msg=r.stdout + r.stderr)
        wavs = self._wav_basenames()
        self.assertEqual(len(wavs), 1)
        self.assertEqual(wavs[0], "out-tile-01-r0c0.wav")

    def test_all_tiles_same_r36_duration(self):
        """Robot 36 encodes 240 scan lines regardless of crop origin."""
        out = os.path.join(self.tmp, "out.wav")
        run_slowframe("-i", self.input_png, "-p", "r36",
                      "-X", "2x2", "-o", out, cwd=self.tmp)
        # Parse duration from stdout  "… (39.15 s at 22050 Hz)"
        r = run_slowframe("-i", self.input_png, "-p", "r36",
                          "-X", "2x2", "-o", out, cwd=self.tmp)
        durations = re.findall(r'\((\d+\.\d+) s at', r.stdout)
        self.assertEqual(len(durations), 4,
                         msg=f"Expected 4 duration lines in:\n{r.stdout}")
        self.assertEqual(len(set(durations)), 1,
                         msg=f"Not all durations equal: {durations}")

    def test_keep_intermediate_creates_png_files(self):
        out = os.path.join(self.tmp, "out.wav")
        r = run_slowframe("-i", self.input_png, "-p", "r36",
                          "-X", "2x2", "-K", "-o", out, cwd=self.tmp)
        self.assertEqual(r.returncode, 0, msg=r.stdout + r.stderr)
        pngs = sorted(f for f in os.listdir(self.tmp) if f.endswith(".png")
                      and "tile" in f)
        self.assertEqual(len(pngs), 4,
                         msg=f"Expected 4 intermediate PNGs, got: {pngs}")
        # Verify naming: input-r36-tile-NN-rRcC.png
        name_re = re.compile(r"input-r36-tile-\d{2}-r\dc\d\.png")
        for fn in pngs:
            self.assertRegex(fn, name_re,
                             msg=f"Unexpected intermediate name: {fn}")


class TestTileHeaderSyncRow(unittest.TestCase):
    """Verify the R/G/B/Y/C/M/W/K color pattern in header row 0."""

    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="sf_tile_sync_")
        self.input_png = os.path.join(self.tmp, "input.png")
        create_black_png(self.input_png)

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def _get_intermediates(self, grid: str) -> list[str]:
        out = os.path.join(self.tmp, "out.wav")
        run_slowframe("-i", self.input_png, "-p", "r36",
                      "-X", grid, "-K", "-o", out, cwd=self.tmp)
        return sorted(
            os.path.join(self.tmp, f)
            for f in os.listdir(self.tmp)
            if f.endswith(".png") and "tile" in f)

    def test_sync_row_colors_tile_r0c0(self):
        """First tile (r0c0) has correct RGBYCMWK sync row."""
        inters = self._get_intermediates("2x2")
        self.assertGreater(len(inters), 0, "No intermediate PNGs found")
        img_path = inters[0]   # tile-01-r0c0.png (sorted, comes first)

        # R36 is 320×240; macroblock = 40 px
        for blk in range(8):
            x = blk_sample_x(blk, 320)
            rgb = vips_getpoint(img_path, x, 0)
            self.assertEqual(
                rgb, SYNC_COLORS[blk],
                msg=f"Block {blk}: got {rgb}, want {SYNC_COLORS[blk]}")

    def test_sync_row_colors_all_tiles(self):
        """All four tiles share the identical sync-row pattern."""
        inters = self._get_intermediates("2x2")
        self.assertEqual(len(inters), 4)
        for img_path in inters:
            for blk in range(8):
                x = blk_sample_x(blk, 320)
                rgb = vips_getpoint(img_path, x, 0)
                self.assertEqual(
                    rgb, SYNC_COLORS[blk],
                    msg=f"{os.path.basename(img_path)} block {blk}: "
                        f"got {rgb}, want {SYNC_COLORS[blk]}")


class TestTileHeaderRow1Metadata(unittest.TestCase):
    """Verify position metadata (version, grid, col, row, seq, CRC) in row 1."""

    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="sf_tile_row1_")
        self.input_png = os.path.join(self.tmp, "input.png")
        create_black_png(self.input_png)

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def _run_and_get_intermediates(self, grid: str, overlap: int = 0) -> list[str]:
        # Remove any tile PNGs from a previous call in this same tmp dir
        # so successive grid runs don't see each other's intermediates.
        for f in os.listdir(self.tmp):
            if f.endswith(".png") and "tile" in f:
                os.remove(os.path.join(self.tmp, f))

        out = os.path.join(self.tmp, "out.wav")
        args = ["-i", self.input_png, "-p", "r36", "-X", grid, "-K",
                "-o", out]
        if overlap:
            args += ["-V", str(overlap)]
        run_slowframe(*args, cwd=self.tmp)
        return sorted(
            os.path.join(self.tmp, f)
            for f in os.listdir(self.tmp)
            if f.endswith(".png") and "tile" in f)

    def _read_row1_bytes(self, img_path: str) -> list[int]:
        """Return 8 grayscale data bytes from row 1."""
        vals = []
        for blk in range(8):
            x = blk_sample_x(blk, 320)
            r, _g, _b = vips_getpoint(img_path, x, 1)
            vals.append(r)
        return vals

    def test_version_byte_is_1(self):
        inters = self._run_and_get_intermediates("2x2")
        for img in inters:
            row1 = self._read_row1_bytes(img)
            self.assertEqual(row1[0], 1,
                             msg=f"{os.path.basename(img)}: version={row1[0]}")

    def test_grid_cols_and_rows_encoded(self):
        inters = self._run_and_get_intermediates("3x2")
        for img in inters:
            row1 = self._read_row1_bytes(img)
            self.assertEqual(row1[1], 3,
                             msg=f"{os.path.basename(img)}: grid_cols={row1[1]}")
            self.assertEqual(row1[2], 2,
                             msg=f"{os.path.basename(img)}: grid_rows={row1[2]}")

    def test_tile_position_bytes(self):
        """Each tile has correct tile_col (byte 3) and tile_row (byte 4)."""
        inters = self._run_and_get_intermediates("2x2")
        # Sorted: tile-01-r0c0, tile-02-r0c1, tile-03-r1c0, tile-04-r1c1
        expected_positions = [
            (0, 0),  # r0c0
            (1, 0),  # r0c1
            (0, 1),  # r1c0
            (1, 1),  # r1c1
        ]
        for img, (exp_col, exp_row) in zip(inters, expected_positions):
            row1 = self._read_row1_bytes(img)
            tile_col = row1[3]
            tile_row = row1[4]
            self.assertEqual(
                tile_col, exp_col,
                msg=f"{os.path.basename(img)}: tile_col={tile_col}, want {exp_col}")
            self.assertEqual(
                tile_row, exp_row,
                msg=f"{os.path.basename(img)}: tile_row={tile_row}, want {exp_row}")

    def test_total_tiles_byte(self):
        for grid, expected_total in [("2x2", 4), ("3x2", 6), ("1x3", 3)]:
            inters = self._run_and_get_intermediates(grid)
            for img in inters:
                row1 = self._read_row1_bytes(img)
                self.assertEqual(
                    row1[5], expected_total,
                    msg=f"{grid} {os.path.basename(img)}: "
                        f"total_tiles={row1[5]}, want {expected_total}")

    def test_seq_num_increments(self):
        """seq_num (byte 6) runs 1, 2, 3, 4 across the 2×2 grid."""
        inters = self._run_and_get_intermediates("2x2")
        seq_nums = []
        for img in inters:
            row1 = self._read_row1_bytes(img)
            seq_nums.append(row1[6])
        self.assertEqual(seq_nums, [1, 2, 3, 4],
                         msg=f"seq_nums: {seq_nums}")

    def test_crc8_byte_is_correct(self):
        """Byte 7 of row 1 must equal CRC-8/CCITT of bytes 0-6."""
        inters = self._run_and_get_intermediates("3x2")
        for img in inters:
            row1 = self._read_row1_bytes(img)
            data   = bytes(row1[:7])
            expected_crc = crc8(data)
            self.assertEqual(
                row1[7], expected_crc,
                msg=f"{os.path.basename(img)}: CRC={row1[7]:#04x}, "
                    f"want {expected_crc:#04x}  data={list(data)}")

    def test_row1_bytes_are_grayscale(self):
        """All row-1 macroblock pixels must satisfy R==G==B."""
        inters = self._run_and_get_intermediates("2x2")
        for img in inters:
            for blk in range(8):
                x = blk_sample_x(blk, 320)
                r, g, b = vips_getpoint(img, x, 1)
                self.assertEqual(
                    r, g,
                    msg=f"{os.path.basename(img)} blk {blk}: R({r}) != G({g})")
                self.assertEqual(
                    r, b,
                    msg=f"{os.path.basename(img)} blk {blk}: R({r}) != B({b})")


class TestTileHeaderRow2Dimensions(unittest.TestCase):
    """Verify orig_width/height and session_id encoding in header row 2."""

    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="sf_tile_row2_")
        # Use 640×480 → tile is exactly 320×240 for r36
        self.input_png = os.path.join(self.tmp, "input.png")
        create_black_png(self.input_png, 640, 480)

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def _run_2x2(self) -> list[str]:
        out = os.path.join(self.tmp, "out.wav")
        run_slowframe("-i", self.input_png, "-p", "r36",
                      "-X", "2x2", "-K", "-o", out, cwd=self.tmp)
        return sorted(
            os.path.join(self.tmp, f)
            for f in os.listdir(self.tmp)
            if f.endswith(".png") and "tile" in f)

    def _read_row2_bytes(self, img_path: str) -> list[int]:
        vals = []
        for blk in range(8):
            x = blk_sample_x(blk, 320)
            r, _, _ = vips_getpoint(img_path, x, 2)
            vals.append(r)
        return vals

    def test_orig_width_640_encoded_big_endian(self):
        """640 = 0x0280 → bytes [0x02, 0x80]."""
        inters = self._run_2x2()
        for img in inters:
            row2 = self._read_row2_bytes(img)
            ow = (row2[0] << 8) | row2[1]
            self.assertEqual(ow, 640,
                             msg=f"{os.path.basename(img)}: orig_width={ow}")

    def test_orig_height_480_encoded_big_endian(self):
        """480 = 0x01E0 → bytes [0x01, 0xE0]."""
        inters = self._run_2x2()
        for img in inters:
            row2 = self._read_row2_bytes(img)
            oh = (row2[2] << 8) | row2[3]
            self.assertEqual(oh, 480,
                             msg=f"{os.path.basename(img)}: orig_height={oh}")

    def test_session_id_same_across_tiles(self):
        """All tiles in the same run must carry the same 4-byte session ID."""
        inters = self._run_2x2()
        session_ids = []
        for img in inters:
            row2 = self._read_row2_bytes(img)
            sid = (row2[4] << 24) | (row2[5] << 16) | (row2[6] << 8) | row2[7]
            session_ids.append(sid)
        self.assertEqual(
            len(set(session_ids)), 1,
            msg=f"Session IDs differ across tiles: "
                f"{[f'{s:#010x}' for s in session_ids]}")

    def test_session_id_non_zero(self):
        """Session ID must not be zero (time ^ pid is essentially never 0)."""
        inters = self._run_2x2()
        self.assertGreater(len(inters), 0)
        row2 = self._read_row2_bytes(inters[0])
        sid = (row2[4] << 24) | (row2[5] << 16) | (row2[6] << 8) | row2[7]
        self.assertNotEqual(sid, 0, msg="session_id is 0")

    def test_orig_dims_consistent_with_verbose_output(self):
        """Dimensions in header row 2 must match the '--> Original dimensions' log."""
        out = os.path.join(self.tmp, "out.wav")
        r = run_slowframe("-i", self.input_png, "-p", "r36",
                          "-X", "2x2", "-K", "-v", "-o", out, cwd=self.tmp)
        self.assertEqual(r.returncode, 0, msg=r.stdout + r.stderr)

        m = re.search(r'Original dimensions:\s+(\d+)x(\d+)', r.stdout)
        self.assertIsNotNone(m, msg="Did not find 'Original dimensions' in output")
        log_w, log_h = int(m.group(1)), int(m.group(2))

        inters = sorted(os.path.join(self.tmp, f)
                        for f in os.listdir(self.tmp)
                        if f.endswith(".png") and "tile" in f)
        row2 = self._read_row2_bytes(inters[0])
        hdr_w = (row2[0] << 8) | row2[1]
        hdr_h = (row2[2] << 8) | row2[3]
        self.assertEqual(hdr_w, log_w, msg=f"width mismatch: header={hdr_w}, log={log_w}")
        self.assertEqual(hdr_h, log_h, msg=f"height mismatch: header={hdr_h}, log={log_h}")


class TestTileHeaderRow3Overlap(unittest.TestCase):
    """Verify overlap_px and reserved bytes in header row 3."""

    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="sf_tile_row3_")
        self.input_png = os.path.join(self.tmp, "input.png")
        create_black_png(self.input_png)

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def _run(self, grid: str, overlap: int = 0) -> list[str]:
        out = os.path.join(self.tmp, "out.wav")
        args = ["-i", self.input_png, "-p", "r36", "-X", grid, "-K",
                "-o", out]
        if overlap:
            args += ["-V", str(overlap)]
        run_slowframe(*args, cwd=self.tmp)
        return sorted(
            os.path.join(self.tmp, f)
            for f in os.listdir(self.tmp)
            if f.endswith(".png") and "tile" in f)

    def _read_row3_bytes(self, img_path: str) -> list[int]:
        vals = []
        for blk in range(8):
            x = blk_sample_x(blk, 320)
            r, _, _ = vips_getpoint(img_path, x, 3)
            vals.append(r)
        return vals

    def test_no_overlap_byte_is_zero(self):
        inters = self._run("2x2", overlap=0)
        for img in inters:
            row3 = self._read_row3_bytes(img)
            self.assertEqual(row3[0], 0,
                             msg=f"{os.path.basename(img)}: overlap_px={row3[0]}")

    def test_overlap_16_byte_encoded(self):
        inters = self._run("2x2", overlap=16)
        for img in inters:
            row3 = self._read_row3_bytes(img)
            self.assertEqual(row3[0], 16,
                             msg=f"{os.path.basename(img)}: overlap_px={row3[0]}")

    def test_overlap_10_byte_encoded(self):
        inters = self._run("3x2", overlap=10)
        for img in inters:
            row3 = self._read_row3_bytes(img)
            self.assertEqual(row3[0], 10,
                             msg=f"{os.path.basename(img)}: overlap_px={row3[0]}")

    def test_reserved_bytes_are_zero(self):
        """Bytes 1-6 of row 3 are reserved and must be 0x00."""
        inters = self._run("2x2", overlap=5)
        for img in inters:
            row3 = self._read_row3_bytes(img)
            for i in range(1, 7):
                self.assertEqual(
                    row3[i], 0,
                    msg=f"{os.path.basename(img)} row3[{i}]={row3[i]}, want 0")

    def test_crc8_in_row3(self):
        """Byte 7 of row 3 must equal CRC-8/CCITT of bytes 0-6."""
        inters = self._run("3x2", overlap=8)
        for img in inters:
            row3 = self._read_row3_bytes(img)
            expected_crc = crc8(bytes(row3[:7]))
            self.assertEqual(
                row3[7], expected_crc,
                msg=f"{os.path.basename(img)}: CRC={row3[7]:#04x}, "
                    f"want {expected_crc:#04x}")


class TestTileSessionIdUniqueness(unittest.TestCase):
    """Session ID must differ between separate SlowFrame invocations."""

    def setUp(self):
        self.tmp  = tempfile.mkdtemp(prefix="sf_tile_sid_")
        self.inp  = os.path.join(self.tmp, "input.png")
        create_black_png(self.inp)

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def _read_session_id_from_tile(self, img_path: str) -> int:
        vals = []
        for blk in range(4, 8):
            x = blk_sample_x(blk, 320)
            r, _, _ = vips_getpoint(img_path, x, 2)
            vals.append(r)
        return (vals[0] << 24) | (vals[1] << 16) | (vals[2] << 8) | vals[3]

    def test_two_runs_have_different_session_ids(self):
        session_ids = []
        for run_idx in range(2):
            # Clean up previous tile files
            for f in os.listdir(self.tmp):
                if "tile" in f:
                    os.remove(os.path.join(self.tmp, f))
            out = os.path.join(self.tmp, "out.wav")
            run_slowframe("-i", self.inp, "-p", "r36",
                          "-X", "2x2", "-K", "-o", out, cwd=self.tmp)
            inters = sorted(os.path.join(self.tmp, f)
                            for f in os.listdir(self.tmp)
                            if f.endswith(".png") and "tile" in f)
            self.assertGreater(len(inters), 0,
                               msg=f"Run {run_idx}: no intermediates found")
            session_ids.append(self._read_session_id_from_tile(inters[0]))

        # session_id = time() ^ getpid(); same PID but time should differ
        # (If both runs finish within the same second this *could* collide –
        # statistically extremely unlikely in a real test environment.)
        self.assertNotEqual(
            session_ids[0], session_ids[1],
            msg=f"Session IDs unexpectedly equal across runs: "
                f"{session_ids[0]:#010x}")


class TestTileCliOutput(unittest.TestCase):
    """Spot-check the human-readable CLI output for correctness."""

    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="sf_tile_cli_")
        self.inp = os.path.join(self.tmp, "input.png")
        create_black_png(self.inp)

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def _run(self, grid: str, extra_args: list[str] | None = None) -> str:
        out = os.path.join(self.tmp, "out.wav")
        r = run_slowframe("-i", self.inp, "-p", "r36",
                          "-X", grid, "-o", out,
                          *(extra_args or []), cwd=self.tmp)
        self.assertEqual(r.returncode, 0, msg=r.stderr)
        return r.stdout

    def test_completion_banner_present(self):
        stdout = self._run("2x2")
        self.assertIn("TILING COMPLETE", stdout)

    def test_tile_count_in_summary(self):
        stdout = self._run("3x2")
        self.assertIn("Tiles encoded: 6", stdout)
        self.assertIn("3x2 grid", stdout)

    def test_session_id_hex_in_summary(self):
        stdout = self._run("2x2")
        self.assertRegex(stdout, r"Session ID:\s+0x[0-9A-Fa-f]{8}")

    def test_tile_progress_lines(self):
        stdout = self._run("2x2")
        for i in range(1, 5):
            self.assertIn(f"[TILE {i}/4]", stdout)

    def test_ok_lines_per_tile(self):
        stdout = self._run("2x2")
        ok_lines = re.findall(r'\[OK\].*tile.*\.wav', stdout)
        self.assertEqual(len(ok_lines), 4,
                         msg=f"Expected 4 [OK] lines, found: {ok_lines}")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
def main() -> None:
    parser = argparse.ArgumentParser(
        description="SlowFrame tiling integration tests")
    parser.add_argument("-v", "--verbose", action="store_true")
    args, remaining = parser.parse_known_args()

    if not os.path.exists(BINARY):
        print(f"❌ Binary not found: {BINARY}")
        print("   Run 'make all' first.")
        sys.exit(1)

    if not shutil.which("vips"):
        print("❌ 'vips' CLI not found. Install libvips.")
        sys.exit(1)

    verbosity = 2 if args.verbose else 1
    loader = unittest.TestLoader()
    suite  = unittest.TestSuite()
    for cls in [
        TestTileFileCreation,
        TestTileHeaderSyncRow,
        TestTileHeaderRow1Metadata,
        TestTileHeaderRow2Dimensions,
        TestTileHeaderRow3Overlap,
        TestTileSessionIdUniqueness,
        TestTileCliOutput,
    ]:
        suite.addTests(loader.loadTestsFromTestCase(cls))

    runner = unittest.TextTestRunner(verbosity=verbosity, stream=sys.stdout)
    result = runner.run(suite)
    sys.exit(0 if result.wasSuccessful() else 1)


if __name__ == "__main__":
    main()
