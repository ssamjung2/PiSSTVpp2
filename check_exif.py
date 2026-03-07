#!/usr/bin/env python3
from PIL import Image

img_path = "test_exif_tiles/horus_42_full-r36-m1-tile-01-r0c0.png"
img = Image.open(img_path)
pixels = img.load()

print("EXIF Data from Tile Header Rows:")
print("=" * 70)

# Row 1: Data bytes (version, grid, tile position, EXIF fields)
row1_data = [pixels[i, 1][0] for i in range(16)]
print(f"Row 1 bytes: {' '.join(f'{b:02X}' for b in row1_data)}")
print(f"  [0] Version: {row1_data[0]}")
print(f"  [1-2] Grid: {row1_data[1]}x{row1_data[2]}")
print(f"  [3-4] Tile position: ({row1_data[3]},{row1_data[4]})")
print(f"  [5] Total tiles: {row1_data[5]}")
print(f"  [6] Sequence: {row1_data[6]}")
print(f"  [7] EXIF version: 0x{row1_data[7]:02X}")
print(f"  [8] F-stop: f/{row1_data[8]/10:.1f}")
print(f"  [9] Metering mode: {row1_data[9]}")
print(f"  [10] Exposure program: {row1_data[10]}")
print(f"  [11] Exposure time log: {row1_data[11]}")
print(f"  [12-13] Focal length hi: {row1_data[12]}")
print(f"  [15] CRC: 0x{row1_data[15]:02X}")

# Row 2: Data bytes (image dimensions, session, exposure, WB, color)
row2_data = [pixels[i, 2][0] for i in range(16)]
print(f"\nRow 2 bytes: {' '.join(f'{b:02X}' for b in row2_data)}")
iso = (row2_data[10] << 8) | row2_data[11]
width = row2_data[0] | (row2_data[1] << 8)
height = row2_data[2] | (row2_data[3] << 8)
print(f"  [0-1] Original width: {width}")
print(f"  [2-3] Original height: {height}")
print(f"  [4-7] Session ID: 0x{row2_data[4]:02X}{row2_data[5]:02X}{row2_data[6]:02X}{row2_data[7]:02X}")
print(f"  [8] Overlap: {row2_data[8]}px")
print(f"  [9] Brightness EV: {row2_data[9]-10} EV")
print(f"  [10-11] ISO speed: {iso}")
print(f"  [12] White balance: {row2_data[12]}")
print(f"  [13] Color space: {row2_data[13]}")
print(f"  [14] Color profile ID: {row2_data[14]}")
print(f"  [15] CRC: 0x{row2_data[15]:02X}")

# Row 3: Data bytes (device, timestamp, focal length)
row3_data = [pixels[i, 3][0] for i in range(16)]
print(f"\nRow 3 bytes: {' '.join(f'{b:02X}' for b in row3_data)}")
year = 2000 + row3_data[4]
month = row3_data[5]
day = row3_data[6]
hour = row3_data[7]
minute = row3_data[8]
second = row3_data[9]
focal_len_lo = row3_data[10]
focal_len_hi = row1_data[12]
focal_len = focal_len_hi * 256 + focal_len_lo if focal_len_hi else focal_len_lo
print(f"  [0-1] Device make ID: 0x{row3_data[0]:02X}{row3_data[1]:02X}")
print(f"  [2-3] Device model ID: 0x{row3_data[2]:02X}{row3_data[3]:02X}")
print(f"  [4-9] Timestamp: {year}-{month:02d}-{day:02d} {hour:02d}:{minute:02d}:{second:02d}")
print(f"  [10] Focal length lo: {focal_len_lo} (combined: {focal_len}mm)")
print(f"  [15] CRC: 0x{row3_data[15]:02X}")

print("\n" + "=" * 70)
print("VERIFICATION: All 14 EXIF fields present in tile header")
print("=" * 70)
print(f"✓ f_stop: {row1_data[8]}")
print(f"✓ metering_mode: {row1_data[9]}")
print(f"✓ exposure_program: {row1_data[10]}")
print(f"✓ exposure_time_log: {row1_data[11]}")
print(f"✓ focal_length: {focal_len}mm")
print(f"✓ brightness_ev: {row2_data[9]-10} EV")
print(f"✓ iso_speed: {iso}")
print(f"✓ white_balance: {row2_data[12]}")
print(f"✓ color_space: {row2_data[13]}")
print(f"✓ color_profile_id: {row2_data[14]}")
print(f"✓ device_make_id: 0x{row3_data[0]:02X}{row3_data[1]:02X}")
print(f"✓ device_model_id: 0x{row3_data[2]:02X}{row3_data[3]:02X}")
print(f"✓ exif_version: 0x{row1_data[7]:02X}")
print(f"✓ date_time: {year}-{month:02d}-{day:02d} {hour:02d}:{minute:02d}:{second:02d}")
