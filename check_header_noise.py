#!/usr/bin/env python3
"""Check if SSTV tile header is being corrupted by noise"""

from PIL import Image
import numpy as np

tiles = ['clean', 'light_qrm', 'medium_qrm', 'heavy_qrm']
header_rows = 10  # First 10 rows are header

print("Checking header corruption in test tiles:\n")
print("=" * 70)

for tile_name in tiles:
    img_path = f'tests/test_outputs/tile09_{tile_name}.png'
    
    try:
        img = Image.open(img_path)
        arr = np.array(img.convert('RGB'))
        
        # Calculate noise metrics in header vs body
        header = arr[:header_rows, :, :]
        body = arr[header_rows:, :, :]
        
        header_std = np.std(header)
        body_std = np.std(body)
        header_mean = np.mean(header)
        body_mean = np.mean(body)
        header_min = np.min(header)
        header_max = np.max(header)
        
        print(f"\n{tile_name.upper()}:")
        print(f"  Header (rows 0-9):")
        print(f"    Mean intensity:  {header_mean:.1f} (0-255)")
        print(f"    Std deviation:   {header_std:.1f} (noise amount)")
        print(f"    Range:           {header_min}-{header_max}")
        print(f"  Body (rows 10+):")
        print(f"    Mean intensity:  {body_mean:.1f}")
        print(f"    Std deviation:   {body_std:.1f}")
        print(f"  Header noise vs Body: {header_std/body_std:.2f}x")
        
    except Exception as e:
        print(f"Error reading {tile_name}: {e}")

print("\n" + "=" * 70)
print("✓ If header StdDev > 20, the header is being corrupted by noise")
print("✓ Header/Body ratio near 1.0 means equal noise throughout")
