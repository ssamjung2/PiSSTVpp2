#!/usr/bin/env python3
"""Debug alpha channel in STRETCH mode output"""

from PIL import Image
import numpy as np

img = Image.open('/tmp/test_stretch.png')
print(f'Image: {img.size}, mode: {img.mode}')

# Convert to numpy array
arr = np.array(img)
print(f'Array shape: {arr.shape}')

# Check alpha channel
if arr.shape[2] == 4:
    alpha = arr[:,:,3]
    print(f'\nAlpha channel stats:')
    print(f'  Min: {alpha.min()}')
    print(f'  Max: {alpha.max()}')
    print(f'  Mean: {alpha.mean():.1f}')
    
    # Count pixels by alpha value
    transparent = np.sum(alpha == 0)
    semi = np.sum((alpha > 0) & (alpha < 255))
    opaque = np.sum(alpha == 255)
    total = arr.shape[0] * arr.shape[1]
    
    print(f'\nAlpha distribution:')
    print(f'  Fully transparent (0): {transparent}/{total} ({100*transparent/total:.1f}%)')
    print(f'  Semi-transparent: {semi}/{total} ({100*semi/total:.1f}%)')
    print(f'  Fully opaque (255): {opaque}/{total} ({100*opaque/total:.1f}%)')
    
    # Count black pixels (RGB < 10) with different alpha levels
    is_black_rgb = (arr[:,:,0] < 10) & (arr[:,:,1] < 10) & (arr[:,:,2] < 10)
    black_opaque = np.sum(is_black_rgb & (alpha == 255))
    black_transparent = np.sum(is_black_rgb & (alpha == 0))
    black_semi = np.sum(is_black_rgb & ((alpha > 0) & (alpha < 255)))
    
    print(f'\nBlack pixel breakdown:')
    print(f'  Black + opaque: {black_opaque}')
    print(f'  Black + transparent: {black_transparent}')
    print(f'  Black + semi-transparent: {black_semi}')
    print(f'  Total black-ish: {black_opaque + black_transparent + black_semi}')
