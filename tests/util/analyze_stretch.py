#!/usr/bin/env python3
"""Analyze STRETCH mode debug image"""

from PIL import Image
import numpy as np

img = Image.open('/tmp/test_stretch.png')
print(f'Image size: {img.size}')
print(f'Image mode: {img.mode}')

# Convert to numpy array
arr = np.array(img)
print(f'Array shape: {arr.shape}')

# Count black pixels (all RGB channels < 10)
if len(arr.shape) == 3:
    black_mask = (arr[:,:,0] < 10) & (arr[:,:,1] < 10) & (arr[:,:,2] < 10)
    black_pixels = np.sum(black_mask)
    total_pixels = arr.shape[0] * arr.shape[1]
    black_percent = (black_pixels / total_pixels) * 100
    print(f'Black pixels: {black_pixels}/{total_pixels} ({black_percent:.1f}%)')
    
    # Check if padding is on edges
    top_black = np.sum(black_mask[0:20, :])
    bottom_black = np.sum(black_mask[-20:, :])
    left_black = np.sum(black_mask[:, 0:20])
    right_black = np.sum(black_mask[:, -20:])
    
    print(f'\nEdge analysis (first/last 20 pixels):')
    print(f'  Top edge:    {top_black} black pixels')
    print(f'  Bottom edge: {bottom_black} black pixels')
    print(f'  Left edge:   {left_black} black pixels')
    print(f'  Right edge:  {right_black} black pixels')
