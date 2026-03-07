#!/usr/bin/env python3
"""Extended tile alignment diagnostic — scan row-by-row for shift."""
import numpy as np
from PIL import Image
from pathlib import Path

base = Path("tests/test_outputs/tiling")
pre = np.array(Image.open(base / "test_color_panel-r36-tile-pre.png"))
t04 = np.array(Image.open(base / "test_color_panel-r36-tile-04-r1c0.png"))
t07 = np.array(Image.open(base / "test_color_panel-r36-tile-07-r2c0.png"))

print("=== tile04 rows 6-15 vs pre rows 228-242 (first 6 pixels, col 0) ===")
print(f"{'tile04 row':>12} | {'tile04[row,0:3]':>30} | {'pre match':>12} | {'vs pre row':>12}")
print("-" * 75)
for tr in range(6, 16):
    pix_t = t04[tr, 0]  # first pixel
    # Find which pre row this pixel matches (scan a range)
    match_row = None
    for pr in range(220, 244):
        if np.array_equal(t04[tr], pre[pr, 0:320]):
            match_row = pr
            break
    flag = "HEADER" if tr < 8 else ""
    print(f"  tile04[{tr:3d}] | {str(pix_t):>30} | {str(match_row):>12} | {flag}")

print()
print("=== tile07 rows 6-15 vs pre rows 460-474 ===")
print(f"{'tile07 row':>12} | {'tile07[row,0:3]':>30} | {'pre match':>12}")
print("-" * 60)
for tr in range(6, 16):
    match_row = None
    for pr in range(452, 476):
        if np.array_equal(t07[tr], pre[pr, 0:320]):
            match_row = pr
            break
    print(f"  tile07[{tr:3d}] | {str(t07[tr,0]):>30} | {str(match_row):>12}")

print()
print("=== tile04 content rows — compare full rows against pre ===")
print("Expected: tile04[8+k] == pre[232+k] for k=0,1,2...")
for k in range(10):
    tr = 8 + k
    pr = 232 + k
    m = np.array_equal(t04[tr], pre[pr, 0:320])
    # Also try offset
    m_off1 = np.array_equal(t04[tr], pre[pr-1, 0:320]) if pr > 0 else False
    m_off2 = np.array_equal(t04[tr], pre[pr+1, 0:320]) if pr < 695 else False
    print(f"  tile04[{tr}] == pre[{pr}]: {m}  | pre[{pr-1}]: {m_off1}  | pre[{pr+1}]: {m_off2}")
