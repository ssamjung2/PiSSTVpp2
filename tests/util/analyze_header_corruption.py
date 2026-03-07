#!/usr/bin/env python3
"""
SSTV Tile Header Analysis Tool

Analyzes the header region (first ~10 rows) of SSTV tiles to understand:
1. What header data survives corruption
2. How much visible vs corrupted header exists
3. Patterns that could be used for robust tile positioning

SSTV tiles typically encode metadata in the header including:
- Tile position (row/column)
- Mode information
- Synchronization patterns
- Timestamp/session info
"""

import sys
from pathlib import Path
from PIL import Image
import numpy as np


def analyze_header_region(image_path, header_rows=10, output_path=None):
    """Analyze header region of an SSTV tile"""
    
    img = Image.open(image_path)
    arr = np.array(img)
    
    height, width = arr.shape[:2]
    
    print(f"\n{'='*80}")
    print(f"Image: {Path(image_path).name}")
    print(f"{'='*80}")
    print(f"Dimensions: {width} x {height}")
    print(f"Mode: {img.mode}")
    
    # Extract header region
    if len(arr.shape) == 3:  # RGB
        header_region = arr[:header_rows, :, :]
        body_region = arr[header_rows:, :, :]
    else:  # Grayscale
        header_region = arr[:header_rows, :]
        body_region = arr[header_rows:, :]
    
    # Analyze header
    header_mean = np.mean(header_region)
    header_std = np.std(header_region)
    header_min = np.min(header_region)
    header_max = np.max(header_region)
    
    # Analyze body
    body_mean = np.mean(body_region)
    body_std = np.std(body_region)
    body_min = np.min(body_region)
    body_max = np.max(body_region)
    
    print(f"\nHEADER REGION (rows 0-{header_rows-1}):")
    print(f"  Mean: {header_mean:.1f}")
    print(f"  Std Dev: {header_std:.1f}")
    print(f"  Range: [{header_min}, {header_max}]")
    print(f"  Entropy: {header_std/header_mean*100:.1f}% of mean")
    
    print(f"\nBODY REGION (rows {header_rows}-{height-1}):")
    print(f"  Mean: {body_mean:.1f}")
    print(f"  Std Dev: {body_std:.1f}")
    print(f"  Range: [{body_min}, {body_max}]")
    print(f"  Entropy: {body_std/body_mean*100:.1f}% of mean")
    
    # Corruption ratio (higher std = more noise corruption)
    header_corruption_score = header_std
    body_corruption_score = body_std
    corruption_ratio = header_corruption_score / body_corruption_score if body_corruption_score > 0 else 1.0
    
    print(f"\nCORRUPTION ANALYSIS:")
    print(f"  Header Corruption Score: {header_corruption_score:.1f}")
    print(f"  Body Corruption Score: {body_corruption_score:.1f}")
    print(f"  Header/Body Ratio: {corruption_ratio:.2f}x")
    
    # Check for visible structure in header
    if len(arr.shape) == 3:  # RGB, check luminance
        header_lum = np.mean(header_region, axis=2)
        body_lum = np.mean(body_region, axis=2)
    else:
        header_lum = header_region
        body_lum = body_region
    
    # Detect horizontal patterns (lines/bars)
    header_row_variance = np.var(np.mean(header_lum, axis=1))
    body_row_variance = np.var(np.mean(body_lum, axis=1))
    
    print(f"\nPATTERN DETECTABILITY:")
    print(f"  Header row variance: {header_row_variance:.1f}")
    print(f"  Body row variance: {body_row_variance:.1f}")
    print(f"  Structure preservation: {header_row_variance/body_row_variance:.2f}x")
    
    # If output path given, save header visualization
    if output_path:
        # Create a visualization showing header (red) vs body (blue)
        vis = Image.new('RGB', (width, height), color=(0, 0, 0))
        vis_arr = np.array(vis)
        
        # Highlight header in red
        vis_arr[:header_rows, :] = [255, 100, 100]
        # Highlight body in blue
        vis_arr[header_rows:, :] = [100, 100, 255]
        
        vis = Image.fromarray(vis_arr.astype('uint8'))
        vis.save(str(output_path))
        print(f"\nVisualization: {output_path}")
    
    return {
        'name': Path(image_path).name,
        'dimensions': (width, height),
        'header_std': header_std,
        'body_std': body_std,
        'corruption_ratio': corruption_ratio,
        'header_variance': header_row_variance,
        'body_variance': body_row_variance,
    }


def compare_tiles(tile1_path, tile2_path):
    """Compare header corruption between original and corrupted tile"""
    
    print(f"\n\n{'='*80}")
    print("COMPARATIVE ANALYSIS")
    print(f"{'='*80}")
    
    stats1 = analyze_header_region(tile1_path)
    stats2 = analyze_header_region(tile2_path)
    
    print(f"\n\nCOMPARISON: {stats1['name']} vs {stats2['name']}")
    print(f"{'='*80}")
    print(f"\nHeader Corruption Increase:")
    print(f"  Original StdDev: {stats1['header_std']:.1f}")
    print(f"  Corrupted StdDev: {stats2['header_std']:.1f}")
    print(f"  Increase: {stats2['header_std']/stats1['header_std']:.2f}x")
    print(f"\nHeader/Body Ratio Change:")
    print(f"  Original: {stats1['corruption_ratio']:.2f}x")
    print(f"  Corrupted: {stats2['corruption_ratio']:.2f}x")
    print(f"  Change: {stats2['corruption_ratio']/stats1['corruption_ratio']:.2f}x")


def main():
    project_root = Path(__file__).parent.parent.parent
    test_output_dir = project_root / "tests" / "test_outputs"
    
    # Analyze original imagec.png
    original_imagec = test_output_dir / "tiling" / "imagec.png"
    heavy_imagec = test_output_dir / "qrm_test_heavy_qrm" / "imagec.png"
    
    if not original_imagec.exists():
        print(f"Error: {original_imagec} not found")
        sys.exit(1)
    
    if not heavy_imagec.exists():
        print(f"Error: {heavy_imagec} not found")
        sys.exit(1)
    
    print("SSTV TILE HEADER CORRUPTION ANALYSIS")
    print("="*80)
    print("Analyzing imagec.png to understand header corruption patterns")
    print("Focus: Heavy QRM impact on header recoverability")
    
    # Analyze original
    print("\n[ORIGINAL - Clean Reference]")
    analyze_header_region(original_imagec)
    
    # Analyze heavy QRM version
    print("\n[HEAVY QRM - Corrupted Version]")
    analyze_header_region(heavy_imagec)
    
    # Comparative analysis
    print("\n[COMPARATIVE ANALYSIS]")
    compare_tiles(original_imagec, heavy_imagec)


if __name__ == "__main__":
    main()
