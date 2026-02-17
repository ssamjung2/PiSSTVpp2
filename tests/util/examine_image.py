#!/usr/bin/env python3
"""Examine image cross-sections to locate black regions"""

from PIL import Image
import numpy as np

img = Image.open('/tmp/test_detail.png')
arr = np.array(img)

print(f'Image: {img.size}, mode: {img.mode}')
print(f' Array shape: {arr.shape}')

# Check a horizontal cross-section through middle
mid_y = arr.shape[0] // 2
row = arr[mid_y, :, :3]  # RGB only
print(f'\nHorizontal cross-section (row {mid_y}):')
print(f'  Shape: {row.shape}')

# Find black pixels in this row
is_black = (row[:,0] < 10) & (row[:,1] < 10) & (row[:,2] < 10)
black_indices = np.where(is_black)[0]

if len(black_indices) > 0:
    print(f'  Black pixels at columns: {list(black_indices[:10])}{"..." if len(black_indices) > 10 else ""}')
    print(f'  Total black in this row: {len(black_indices)}/320')
    
    # Find contiguous black regions
    if len(black_indices) > 0:
        # Check if black pixels are at edges
        if black_indices[0] < 20:
            print(f'  Black region at LEFT edge: columns 0-{black_indices[np.where(is_black[:20])[0][-1]] if np.any(is_black[:20]) else "none"}')
        if black_indices[-1] > 300:
            first_right_black = np.where(is_black[300:])[0]
            if len(first_right_black) > 0:
                print(f'  Black region at RIGHT edge: columns {300 + first_right_black[0]}-319')
else:
    print('  No black pixels in middle row')

# Check vertical cross-section through middle
mid_x = arr.shape[1] // 2
col = arr[:, mid_x, :3]  # RGB only
is_black_v = (col[:,0] < 10) & (col[:,1] < 10) & (col[:,2] < 10)
black_rows = np.where(is_black_v)[0]

print(f'\nVertical cross-section (column {mid_x}):')
if len(black_rows) > 0:
    print(f'  Black pixels at rows: {list(black_rows[:10])}{"..." if len(black_rows) > 10 else ""}')
    print(f'  Total black in this column: {len(black_rows)}/256')
else:
    print('  No black pixels in middle column')

# Sample some pixel values
print(f'\nSample pixel values:')
print(f'  Top-left (0,0): RGB{tuple(arr[0,0,:3])}')
print(f'  Top-right (0,319): RGB{tuple(arr[0,319,:3])}')
print(f'  Center ({mid_y},{mid_x}): RGB{tuple(arr[mid_y,mid_x,:3])}')
print(f'  Bottom-right (255,319): RGB{tuple(arr[255,319,:3])}')
