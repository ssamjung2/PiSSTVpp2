#!/usr/bin/env python3
"""
Generate test manifest JSON file for verification reporting

Creates a structured JSON file containing:
- All test results with input/output image paths
- Test parameters and configuration
- Execution details and timing
- Ready for use by HTML report generator

Usage:
    venv/bin/python3 tests/generate_test_manifest.py
    venv/bin/python3 tests/generate_test_manifest.py --output tests/test_manifest.json
"""

import sys
import json
import subprocess
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Optional

def find_test_images(test_name: str, test_outputs_dir: Path) -> Optional[str]:
    """Find the actual output image for a test (with or without test number prefix)"""
    
    # Search in aspect, overlay, formats subdirectories
    for subdir in ['aspect', 'overlay', 'formats']:
        # Direct match
        img_path = test_outputs_dir / subdir / f"{test_name}.png"
        if img_path.exists():
            return str(img_path)
        
        # Try with T00X_ prefix pattern (from numbered test runs)
        # Look for T???_{test_name}.png
        subdir_path = test_outputs_dir / subdir
        if subdir_path.exists():
            for png_file in subdir_path.glob(f"T*_{test_name}.png"):
                return str(png_file)
    
    # Direct match in root
    img_path = test_outputs_dir / f"{test_name}.png"
    if img_path.exists():
        return str(img_path)
    
    return None


def map_input_image(test_name: str, images_dir: Path) -> Optional[str]:
    """Map test name to input image"""
    
    # Extract aspect type from test name
    parts = test_name.split('_')
    if len(parts) < 3:
        return None
    
    aspect = parts[2]  # 'wide', 'tall', 'square', 'target'
    
    input_map = {
        'wide': 'alt2_color_bars_2000x1125.png',
        'tall': 'alt2_color_bars_680×1209.png',
        'square': 'alt3_color_bars_1370×1080.png',
        'target': 'alt_color_bars_320x256.png'
    }
    
    if aspect in input_map:
        img_path = images_dir / input_map[aspect]
        if img_path.exists():
            return str(img_path)
    
    return None


def generate_test_manifest(output_file: Path):
    """Generate test manifest JSON from actual test results"""
    
    # Directories - resolve relative to this script's location
    script_dir = Path(__file__).parent
    test_outputs = script_dir / "test_outputs"
    test_images = script_dir / "images"
    
    manifest = {
        "version": "2.1.0",
        "generated": datetime.now().isoformat(),
        "tests": {}
    }
    
    # Aspect ratio tests
    print("Scanning aspect ratio tests...")
    aspect_tests = [
        'center_m1_wide', 'center_m1_tall', 'center_m1_square', 'center_m1_target',
        'center_r36_wide', 'center_r36_tall', 'center_r36_square', 'center_r36_target',
        'pad_m1_wide', 'pad_m1_tall', 'pad_m1_square', 'pad_m1_target',
        'pad_r36_wide', 'pad_r36_tall', 'pad_r36_square', 'pad_r36_target',
        'stretch_m1_wide', 'stretch_m1_tall', 'stretch_m1_square', 'stretch_m1_target',
        'stretch_r36_wide', 'stretch_r36_tall', 'stretch_r36_square', 'stretch_r36_target',
    ]
    
    for test in aspect_tests:
        parts = test.split('_')
        mode = parts[0]  # center, pad, stretch
        protocol = parts[1]  # m1, r36
        aspect = parts[2]  # wide, tall, square, target
        
        input_img = map_input_image(test, test_images)
        output_img = find_test_images(test, test_outputs)
        
        manifest["tests"][test] = {
            "category": "aspect_ratio",
            "mode": mode,
            "protocol": protocol,
            "aspect": aspect,
            "target": "Martin M1 (320x256)" if protocol == "m1" else "Robot 36 (320x240)",
            "input_image": input_img,
            "output_image": output_img,
            "input_exists": input_img is not None and Path(input_img).exists(),
            "output_exists": output_img is not None and Path(output_img).exists()
        }
        
        if manifest["tests"][test]["input_exists"] and manifest["tests"][test]["output_exists"]:
            print(f"  ✅ {test}")
        else:
            status = []
            if not manifest["tests"][test]["input_exists"]:
                status.append("no input")
            if not manifest["tests"][test]["output_exists"]:
                status.append("no output")
            print(f"  ⚠️  {test} ({', '.join(status)})")
    
    # Overlay tests
    print("\nScanning overlay tests...")
    overlay_tests = [
        'overlay_basic', 'overlay_positioning', 'overlay_multi_line', 'overlay_background_bar'
    ]
    
    for test in overlay_tests:
        input_img = test_images / 'alt_color_bars_320x256.png'
        output_img = find_test_images(test, test_outputs)
        
        manifest["tests"][test] = {
            "category": "text_overlay",
            "input_image": str(input_img) if input_img.exists() else None,
            "output_image": output_img,
            "input_exists": input_img.exists(),
            "output_exists": output_img is not None and Path(output_img).exists()
        }
        
        if manifest["tests"][test]["input_exists"] and manifest["tests"][test]["output_exists"]:
            print(f"  ✅ {test}")
        else:
            status = []
            if not manifest["tests"][test]["input_exists"]:
                status.append("no input")
            if not manifest["tests"][test]["output_exists"]:
                status.append("no output")
            print(f"  ⚠️  {test} ({', '.join(status)})")
    
    # Format tests
    print("\nScanning format tests...")
    format_tests = ['format_png', 'format_jpeg', 'format_gif', 'format_bmp', 'format_tiff']
    
    for test in format_tests:
        output_img = find_test_images(test, test_outputs)
        
        manifest["tests"][test] = {
            "category": "image_format",
            "format": test.split('_')[1],
            "input_image": None,  # Varies by test
            "output_image": output_img,
            "input_exists": False,
            "output_exists": output_img is not None and Path(output_img).exists()
        }
        
        if manifest["tests"][test]["output_exists"]:
            print(f"  ✅ {test}")
        else:
            print(f"  ⚠️  {test} (no output)")
    
    # Save manifest
    output_file.parent.mkdir(parents=True, exist_ok=True)
    with open(output_file, 'w') as f:
        json.dump(manifest, f, indent=2)
    
    # Summary
    total = len(manifest["tests"])
    with_both = sum(1 for t in manifest["tests"].values() if t.get("input_exists") and t.get("output_exists"))
    
    print(f"\n{'='*60}")
    print(f"📊 Summary:")
    print(f"  Total tests: {total}")
    print(f"  With both images: {with_both}")
    print(f"  Manifest saved: {output_file}")
    print(f"{'='*60}")
    
    return manifest


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Generate test manifest JSON")
    parser.add_argument('--output', '-o', default='test_manifest.json',
                       help='Output JSON file path')
    
    args = parser.parse_args()
    
    # Resolve output file relative to script location if it's a relative path
    output_path = Path(args.output)
    if not output_path.is_absolute():
        output_path = Path(__file__).parent / output_path
    
    generate_test_manifest(output_path)


if __name__ == '__main__':
    main()
