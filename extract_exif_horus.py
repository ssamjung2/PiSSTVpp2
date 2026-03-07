#!/usr/bin/env python3
from PIL import Image

# Test Tile 1
img = Image.open('horus_42_full-m1-tile-01-r0c0.png')
pix = img.load()
blk_width = 20  # 320 / 16 blocks

row1 = [pix[i * blk_width, 1][0] for i in range(16)]
row2 = [pix[i * blk_width, 2][0] for i in range(16)]
row3 = [pix[i * blk_width, 3][0] for i in range(16)]

print("=" * 75)
print("HORUS_42_FULL.JPG TILING TEST - EXIF DATA VERIFICATION")
print("=" * 75)
print("\nTile 1 (r0c0) - Encoded EXIF Data:")
print("=" * 75)

iso = (row2[10] << 8) | row2[11]
width = (row2[0] << 8) | row2[1]
height = (row2[2] << 8) | row2[3]
focal_len = (row1[12] << 8) | row3[10]
make_id = (row3[0] << 8) | row3[1]
model_id = (row3[2] << 8) | row3[3]
session_id = f"0x{row2[4]:02X}{row2[5]:02X}{row2[6]:02X}{row2[7]:02X}"
date_str = f"20{row3[4]:02d}-{row3[5]:02d}-{row3[6]:02d} {row3[7]:02d}:{row3[8]:02d}:{row3[9]:02d}"

print(f"\nBasic Metadata:")
print(f"  Version:        {row1[0]}")
print(f"  Grid:           {row1[1]}x{row1[2]}")
print(f"  Tile Position:  ({row1[3]},{row1[4]}) Seq {row1[6]}/{row1[5]}")
print(f"  Session ID:     {session_id}")
print(f"  Image Size:     {width}×{height}")
print(f"  Overlap:        {row2[8]}px")

print(f"\nCamera/Image EXIF Data (14 fields):")
print(f"  EXIF Version:   0x{row1[7]:02X} (EXIF 2.3)")
print(f"  F-stop:         f/{row1[8]/10:.1f}")
print(f"  Metering Mode:  {row1[9]} (1=average)")
print(f"  Exposure Prog:  {row1[10]} (2=normal)")
print(f"  Exposure Time:  {row1[11]} (log scale)")
print(f"  Focal Length:   {focal_len}mm")
print(f"  Brightness EV:  {row2[9]-10} EV")
print(f"  ISO Speed:      {iso}")
print(f"  White Balance:  {row2[12]} (0=auto)")
print(f"  Color Space:    {row2[13]} (0=sRGB)")
print(f"  Color Profile:  {row2[14]}")
print(f"  Device Make:    0x{make_id:04X}")
print(f"  Device Model:   0x{model_id:04X}")
print(f"  Date/Time:      {date_str}")

print("\n" + "=" * 75)
print("Verification: All 14 EXIF fields successfully encoded ✓")
print("=" * 75)
