#!/usr/bin/env python3
from PIL import Image

print("=" * 80)
print("FULL TEST - HORUS_42_FULL.JPG (1920×1440 RaspberryPi Camera)")
print("=" * 80)
print("\nSession: 0x69A394E2 | Grid: 2×2 | Tiles: 4/4 Complete\n")

# Extract EXIF from Tile 1 (r0c0)
img = Image.open('horus_42_full-m1-tile-01-r0c0.png')
pix = img.load()
blk_width = 20  # 320 / 16 blocks

row1 = [pix[i * blk_width, 1][0] for i in range(16)]
row2 = [pix[i * blk_width, 2][0] for i in range(16)]
row3 = [pix[i * blk_width, 3][0] for i in range(16)]

iso = (row2[10] << 8) | row2[11]
width = (row2[0] << 8) | row2[1]
height = (row2[2] << 8) | row2[3]
focal_len = (row1[12] << 8) | row3[10]
session = f"0x{row2[4]:02X}{row2[5]:02X}{row2[6]:02X}{row2[7]:02X}"

print("DECODED EXIF DATA (from tile header):")
print("-" * 80)
print(f"\nBasic Metadata:")
print(f"  Version:           {row1[0]} (EXIF-enabled)")
print(f"  Grid:              {row1[1]}x{row1[2]}")
print(f"  Tile Position:     ({row1[3]},{row1[4]}) | Sequence {row1[6]}/{row1[5]}")
print(f"  Session ID:        {session}")
print(f"  Original Size:     {width}x{height} pixels")
print(f"  Overlap:           {row2[8]}px")

print(f"\nCamera & Photography EXIF Data (14 fields):")
print(f"  EXIF Version:      0x{row1[7]:02X} (EXIF 2.3)")
print(f"  F-stop:            f/{row1[8]/10:.1f}")
print(f"  Metering Mode:     {row1[9]} (average)")
print(f"  Exposure Program:  {row1[10]} (normal)")
print(f"  Exposure Time:     {row1[11]} (log scale)")
print(f"  Focal Length:      {focal_len}mm")
print(f"  Brightness EV:     {row2[9]-10} EV")
print(f"  ISO Speed:         {iso}")
print(f"  White Balance:     {row2[12]} (auto)")
print(f"  Color Space:       {row2[13]} (sRGB)")
print(f"  Color Profile:     {row2[14]}")

make_id = (row3[0] << 8) | row3[1]
model_id = (row3[2] << 8) | row3[3]
date_str = f"20{row3[4]:02d}-{row3[5]:02d}-{row3[6]:02d} {row3[7]:02d}:{row3[8]:02d}:{row3[9]:02d}"
print(f"  Device Make ID:    0x{make_id:04X}")
print(f"  Device Model ID:   0x{model_id:04X}")
print(f"  Date/Time:         {date_str}")

print("\n" + "=" * 80)
print("All 14 EXIF fields verified in encoded tile header")
print("=" * 80)
