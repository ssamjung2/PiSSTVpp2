#!/usr/bin/env python3
"""Diagnose tile content alignment after scale-once architecture."""
import sys
import numpy as np
from PIL import Image
from pathlib import Path

base = Path("tests/test_outputs/tiling")

pre = np.array(Image.open(base / "test_color_panel-r36-tile-pre.png"))
t01 = np.array(Image.open(base / "test_color_panel-r36-tile-01-r0c0.png"))
t04 = np.array(Image.open(base / "test_color_panel-r36-tile-04-r1c0.png"))
t07 = np.array(Image.open(base / "test_color_panel-r36-tile-07-r2c0.png"))

print(f"pre:    {pre.shape}")
print(f"tile01: {t01.shape}")
print(f"tile04: {t04.shape}")
print(f"tile07: {t07.shape}")
print()

# For R36 3x3: required_height=240, content_h=232, SF_TILE_HEADER_ROWS=8
HEADER = 8
CONTENT_H = 232

# tile01 (r0c0): rows 0..7 = header, rows 8..239 = pre rows 0..231
print("=== tile01 (r0c0) ===")
for tile_row, pre_row in [(8,0),(9,1),(239,231)]:
    m = np.array_equal(t01[tile_row], pre[pre_row, 0:320])
    print(f"  tile01[{tile_row}] == pre[{pre_row},0:320]: {m}")
    if not m:
        print(f"    tile: {t01[tile_row,:5]}")
        print(f"    pre:  {pre[pre_row,0:5]}")
print()

# tile04 (r1c0): rows 0..7 = header (overwrites borrowed rows),
#                rows 8..239 = pre rows 232..463
print("=== tile04 (r1c0) ===")
for tile_row, pre_row in [(8,232),(9,233),(16,240),(239,463)]:
    m = np.array_equal(t04[tile_row], pre[pre_row, 0:320])
    print(f"  tile04[{tile_row}] == pre[{pre_row},0:320]: {m}")
    if not m:
        print(f"    tile: {t04[tile_row,:5]}")
        print(f"    pre:  {pre[pre_row,0:5]}")

# Also check: do the top borrowed rows before header match pre?
# tile04 was cropped from pre[224:464, 0:320] then header overwrote rows 0-7
# We can't check rows 0-7 (overwritten), but let's see pixel samples
print()
print(f"  tile04 row 0 (header-overwritten): {t04[0,:3]}")
print(f"  pre row 224 (should have been borrowed): {pre[224,0:3]}")
print()

# tile07 (r2c0): rows 0..7 = header, rows 8..239 = pre rows 464..695
print("=== tile07 (r2c0) ===")
for tile_row, pre_row in [(8,464),(239,695)]:
    m = np.array_equal(t07[tile_row], pre[pre_row, 0:320])
    print(f"  tile07[{tile_row}] == pre[{pre_row},0:320]: {m}")
    if not m:
        print(f"    tile: {t07[tile_row,:5]}")
        print(f"    pre:  {pre[pre_row,0:5]}")
print()

# Check the stitched image content
stitched_path = base / "test_color_panel-r36-stitched.png"
if stitched_path.exists():
    stitched = np.array(Image.open(stitched_path))
    print(f"stitched: {stitched.shape}")
    # Stitched row 0 should match pre row 0
    print(f"stitched[0] == pre[0,0:320]: {np.array_equal(stitched[0], pre[0, 0:320])}")
    # Stitched row 232 should match pre row 232
    print(f"stitched[232] == pre[232,0:320]: {np.array_equal(stitched[232], pre[232,0:320])}")
    print(f"stitched[231] == pre[231,0:320]: {np.array_equal(stitched[231], pre[231,0:320])}")
    print(f"stitched[464] == pre[464,0:320]: {np.array_equal(stitched[464], pre[464,0:320])}")
