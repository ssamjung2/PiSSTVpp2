#!/usr/bin/env python3
"""Confirm stride=2880 theory: buf_row N comes from parent[crop_y + N//3, (N%3)*320:(N%3)*320+320]"""
import numpy as np
from PIL import Image
from pathlib import Path

base = Path("tests/test_outputs/tiling")
pre = np.array(Image.open(base / "test_color_panel-r36-tile-pre.png"))
t04 = np.array(Image.open(base / "test_color_panel-r36-tile-04-r1c0.png"))
t01 = np.array(Image.open(base / "test_color_panel-r36-tile-01-r0c0.png"))

HEADER = 8
CONTENT_H = 232
crop_y_t04 = 224  # 232 - 8 borrow
crop_y_t01 = 0    # tile_row=0

print("=== STRIDE-2880 THEORY CHECK (tile04, crop_y=224) ===")
print("Theory: buf_row N → parent[224 + N//3, (N%3)*320 : (N%3)*320+320]")
print(f"{'buf_row':>8} | {'pred col':>9} | {'pred pre_row':>12} | {'match?':>8}")
print("-" * 50)
for N in range(0, 15):
    row_off = N // 3
    col_off = (N % 3) * 320
    pred_pre_row = crop_y_t04 + row_off
    pred_col_start = col_off
    match = np.array_equal(t04[N], pre[pred_pre_row, pred_col_start:pred_col_start+320])
    print(f"  row {N:3d} | col {col_off:4d}   | pre[{pred_pre_row:3d}]        | {match}")

print()
print("=== STRIDE-960 (CORRECT) CHECK (tile04, crop_y=224) ===")
print("Correct: buf_row N → parent[224 + N, 0:320]")
print(f"{'buf_row':>8} | {'pred pre_row':>12} | {'match?':>8}")
print("-" * 36)
for N in range(0, 15):
    pred_pre_row = crop_y_t04 + N
    match = np.array_equal(t04[N], pre[pred_pre_row, 0:320])
    print(f"  row {N:3d} | pre[{pred_pre_row:3d}]        | {match}")

print()
print("=== STRIDE-2880 THEORY CHECK (tile01, crop_y=0, then pad_top 8) ===")
print("Theory: buf_row N (before pad) → parent[0 + N//3, (N%3)*320 : +320]")
print(f"{'tile_row':>9} | {'buf_row':>8} | {'pred col':>9} | {'pred pre_row':>12} | {'match?':>8}")
print("-" * 58)
# tile01 has pad_top(8), so tile01[tile_row] = buf[tile_row-8] for tile_row>=8
for tile_row in range(8, 18):
    N = tile_row - HEADER  # buf row
    row_off = N // 3
    col_off = (N % 3) * 320
    pred_pre_row = 0 + row_off
    pred_col_start = col_off
    match = np.array_equal(t01[tile_row], pre[pred_pre_row, pred_col_start:pred_col_start+320])
    match_correct = np.array_equal(t01[tile_row], pre[N, 0:320])
    print(f"  t01[{tile_row:3d}] | buf[{N:3d}] | col {col_off:4d}   | pre[{pred_pre_row:3d}]        | stride2880={match}  correct={match_correct}")
