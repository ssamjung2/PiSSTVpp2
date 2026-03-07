#!/usr/bin/env python3
"""
Visual Tiling Verification
===========================

Runs SlowFrame in tiling mode on a test image, saves intermediate PNGs,
then composites them into two side-by-side inspection images:

  1. FULL TILE GRID   — all tiles laid out in their transmission order,
                        separated by a 4-px white shim, so you can verify
                        the crop geometry and that image content stitches.

  2. HEADER ROW ZOOM  — the top 8 rows of every tile scaled up so the
                        sync pattern and metadata macroblocks are clearly
                        visible.  Each row is labelled in the filename.

Both composites are saved to tests/test_outputs/tiling/ and opened in
the default image viewer (macOS: Preview).

Usage:
    python3 tests/util/visual_tiling_check.py [options]

Options:
    -g GRID         Tile grid, e.g. 2x2 or 3x2  (default: 3x2)
    -i IMAGE        Input image path             (default: auto-select)
    -p PROTOCOL     SSTV protocol                (default: r36)
    -V OVERLAP      Edge overlap pixels          (default: 0)
    --no-open       Save composites but do not open them
    --keep-audio    Keep the audio .wav files    (default: delete them)

Author: SlowFrame Contributors
Version: 2.1.0
Date: February 2026
"""

import argparse
import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
PROJECT_ROOT = Path(__file__).resolve().parents[2]
BINARY       = PROJECT_ROOT / "bin" / "slowframe"
OUT_DIR      = PROJECT_ROOT / "tests" / "test_outputs" / "tiling"
VIPS         = shutil.which("vips") or "vips"

# Priority-ordered list of candidate test images (most colourful first)
CANDIDATE_IMAGES = [
    "tests/images/alt2_color_bars_2000x1125.png",
    "tests/images/alt5_color_bars_1920x1080.jpg",
    "tests/images/alt3_color_bars_1370×1080.png",
    "tests/images/alt6_test_panel_1920x1080.png",
    "tests/images/alt3_test_panel_2310x1360.png",
    "tests/images/alt5_test_panel_1920x1230.jpg",
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
def run(cmd: list, **kwargs) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, check=True, capture_output=True, text=True,
                          **kwargs)


def vips_cmd(*args) -> None:
    """Run a vips CLI command, printing it if verbose."""
    subprocess.run([VIPS] + list(str(a) for a in args),
                   check=True, capture_output=True)


def pick_image(user_path: str | None) -> Path:
    if user_path:
        p = Path(user_path)
        if not p.exists():
            sys.exit(f"❌ Image not found: {user_path}")
        return p
    for rel in CANDIDATE_IMAGES:
        p = PROJECT_ROOT / rel
        if p.exists():
            return p
    sys.exit("❌ No suitable test image found. Use -i to specify one.")


def parse_grid(grid_str: str) -> tuple[int, int]:
    m = re.fullmatch(r"(\d+)x(\d+)", grid_str, re.IGNORECASE)
    if not m:
        sys.exit(f"❌ Invalid grid format '{grid_str}'. Use e.g. 2x2 or 3x2")
    cols, rows = int(m.group(1)), int(m.group(2))
    if cols < 1 or rows < 1 or cols > 16 or rows > 16:
        sys.exit("❌ Grid dimensions must be 1-16")
    return cols, rows


# ---------------------------------------------------------------------------
# Composite: full tile grid
# ---------------------------------------------------------------------------
def make_tile_grid(tile_paths: list[Path], grid_cols: int,
                   out_path: Path) -> None:
    """
    Lay out tiles in a grid with a 4-pixel white shim between them.
    """
    # Build the space-separated list for vips arrayjoin
    joined = " ".join(str(p) for p in tile_paths)
    vips_cmd("arrayjoin", joined, str(out_path),
             "--across", str(grid_cols),
             "--shim", "4",
             "--background", "255")
    print(f"  ✅ Full tile grid  → {out_path.name}")


# ---------------------------------------------------------------------------
# Composite: header row zoom
# ---------------------------------------------------------------------------
HEADER_ROWS       = 8   # rows to capture (SF_TILE_HEADER_ROWS=4 + 4 margin)
ZOOM_H_SCALE      = 3   # horizontal scale factor
ZOOM_V_SCALE      = 16  # vertical scale factor per row

def make_header_zoom(tile_paths: list[Path], grid_cols: int,
                     tmp_dir: Path, out_path: Path) -> None:
    """
    Crop the top HEADER_ROWS rows from each tile, scale them up, then
    lay out as a grid matching the tile grid layout.
    """
    zoomed = []
    for idx, tp in enumerate(tile_paths):
        # Step 1: crop top N rows
        w, _h = _tile_dims(tp)
        crop_path = tmp_dir / f"hdr_crop_{idx:02d}.png"
        vips_cmd("extract_area", str(tp), str(crop_path),
                 "0", "0", str(w), str(HEADER_ROWS))

        # Step 2: scale up
        zoom_path = tmp_dir / f"hdr_zoom_{idx:02d}.png"
        vips_cmd("resize", str(crop_path), str(zoom_path),
                 str(ZOOM_H_SCALE),
                 "--vscale", str(ZOOM_V_SCALE))
        zoomed.append(zoom_path)

    # Step 3: arrayjoin into header-zoom grid
    joined = " ".join(str(p) for p in zoomed)
    vips_cmd("arrayjoin", joined, str(out_path),
             "--across", str(grid_cols),
             "--shim", "2",
             "--background", "192")   # light-grey shim between tiles
    print(f"  ✅ Header row zoom → {out_path.name}")


def _tile_dims(tile_path: Path) -> tuple[int, int]:
    """Return (width, height) of a tile image using vipsheader."""
    vipsheader = VIPS.replace("vips", "vipsheader", 1)
    def _get(field: str, fallback: int) -> int:
        r = subprocess.run([vipsheader, "-f", field, str(tile_path)],
                           capture_output=True, text=True)
        return int(r.stdout.strip()) if r.returncode == 0 else fallback
    return _get("Xsize", 320), _get("Ysize", 240)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main() -> None:
    parser = argparse.ArgumentParser(
        description="Visual tiling verification for SlowFrame")
    parser.add_argument("-g", "--grid",     default="3x2",
                        help="Tile grid, e.g. 2x2 or 3x2 (default: %(default)s)")
    parser.add_argument("-i", "--image",    default=None,
                        help="Input image path (auto-selected if not given)")
    parser.add_argument("-p", "--protocol", default="r36",
                        help="SSTV protocol (default: %(default)s)")
    parser.add_argument("-V", "--overlap",  type=int, default=0,
                        help="Edge overlap pixels (default: 0)")
    parser.add_argument("--no-open",        action="store_true",
                        help="Do not open composites after generation")
    parser.add_argument("--keep-audio",     action="store_true",
                        help="Keep .wav tile audio files (default: delete)")
    args = parser.parse_args()

    grid_cols, grid_rows = parse_grid(args.grid)
    tile_total           = grid_cols * grid_rows
    image_path           = pick_image(args.image)
    protocol             = args.protocol
    overlap              = args.overlap

    OUT_DIR.mkdir(parents=True, exist_ok=True)

    print()
    print("═" * 64)
    print("  SlowFrame Visual Tiling Verification")
    print("═" * 64)
    print(f"  Image:    {image_path.name}")
    print(f"  Grid:     {grid_cols}×{grid_rows}  ({tile_total} tiles)")
    print(f"  Protocol: {protocol}")
    print(f"  Overlap:  {overlap} px")
    print(f"  Output:   {OUT_DIR}")
    print()

    # ------------------------------------------------------------------
    # Step 1: Run slowframe with -K in a temp directory
    # ------------------------------------------------------------------
    with tempfile.TemporaryDirectory(prefix="sf_visual_") as tmp_str:
        tmp_dir = Path(tmp_str)

        audio_out = tmp_dir / "tile.wav"
        sf_args   = [
            str(BINARY),
            "-i", str(image_path.resolve()),
            "-p", protocol,
            "-X", args.grid,
            "-K",
            "-o", str(audio_out),
        ]
        if overlap:
            sf_args += ["-V", str(overlap)]

        print("  Running slowframe…")
        result = subprocess.run(sf_args, capture_output=True, text=True,
                                cwd=str(tmp_dir))
        if result.returncode != 0:
            print("❌ slowframe failed:")
            print(result.stdout)
            print(result.stderr)
            sys.exit(1)
        print("  Encoding complete.")

        # Collect tile intermediates in transmission order (tile-01, tile-02, …)
        # Slowframe uses the input file's extension for intermediates (.png or .jpg).
        tile_pngs = sorted(
            [p for p in tmp_dir.glob("*-tile-*-r*c*.*")
             if p.suffix.lower() in (".png", ".jpg", ".jpeg")],
            key=lambda p: int(re.search(r"tile-(\d+)", p.name).group(1)))

        if len(tile_pngs) != tile_total:
            print(f"❌ Expected {tile_total} tile PNGs, found {len(tile_pngs)}")
            for p in tile_pngs:
                print(f"   {p.name}")
            sys.exit(1)

        print(f"  Found {len(tile_pngs)} tile intermediates:")
        for p in tile_pngs:
            print(f"    {p.name}")
        print()

        # ------------------------------------------------------------------
        # Step 2: Build composite images
        # ------------------------------------------------------------------
        label = f"{image_path.stem}-{protocol}-{args.grid}"
        if overlap:
            label += f"-ov{overlap}"

        # Clear any previous output files for this label
        for old in OUT_DIR.glob(f"{label}*.png"):
            old.unlink()

        grid_out  = OUT_DIR / f"{label}-full-grid.png"
        zoom_out  = OUT_DIR / f"{label}-header-zoom.png"

        print("  Building composites…")
        make_tile_grid(tile_pngs, grid_cols, grid_out)
        make_header_zoom(tile_pngs, grid_cols, tmp_dir, zoom_out)

        # ------------------------------------------------------------------
        # Step 3: Copy individual tile PNGs + pre-header intermediate
        #         to output dir for inspection / round-trip verification
        # ------------------------------------------------------------------
        for tp in tile_pngs:
            dst = OUT_DIR / tp.name
            shutil.copy(tp, dst)
        print(f"  Copied {len(tile_pngs)} individual tile PNGs to output dir")

        # Copy pre-header intermediate if present (saved by slowframe -K)
        pre_imgs = list(tmp_dir.glob("*-tile-pre.png"))
        for pi in pre_imgs:
            dst = OUT_DIR / pi.name
            shutil.copy(pi, dst)
            print(f"  Copied pre-header intermediate: {pi.name}")

        # Keep or delete audio files
        if args.keep_audio:
            for wav in tmp_dir.glob("*.wav"):
                shutil.copy(wav, OUT_DIR / wav.name)
            print(f"  Kept audio files in {OUT_DIR}")

    # ------------------------------------------------------------------
    # Step 4: Print file summary and open
    # ------------------------------------------------------------------
    print()
    print("─" * 64)
    print("  Output files:")
    for f in sorted(OUT_DIR.iterdir()):
        if f.name.startswith(label):
            size_kb = f.stat().st_size // 1024
            print(f"    {f.name}  ({size_kb} KB)")

    print()
    print("  Legend for header-zoom image:")
    print("    Row 0  — Sync:  ██R ██G ██B ██Y ██C ██M ██W ██K")
    print("    Row 1  — Data:  [version][cols][rows][tile_c][tile_r][total][seq][CRC]")
    print("    Row 2  — Data:  [w_hi][w_lo][h_hi][h_lo][sid3][sid2][sid1][sid0]")
    print("    Row 3  — Data:  [overlap][0][0][0][0][0][0][CRC]")
    print("    Rows 4-7 — unchanged image content (gray margin)")
    print()

    if not args.no_open and shutil.which("open"):
        print("  Opening in Preview…")
        # Open header zoom first (more informative), then full grid
        subprocess.run(["open", str(zoom_out), str(grid_out)])
    elif not args.no_open:
        print(f"  (No 'open' command found — view manually in {OUT_DIR})")

    print("═" * 64)
    print()


if __name__ == "__main__":
    main()
