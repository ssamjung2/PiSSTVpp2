#!/usr/bin/env python3
"""
QRM Stitching Test - Generate corrupted tile sets and test stitcher robustness

Creates noise-corrupted versions of all tiles in the tiling folder,
then runs the stitch_tiles binary on each QRM preset to see if it can
successfully reassemble the original image.
"""

import os
import sys
import subprocess
from pathlib import Path
from datetime import datetime


def run_command(cmd, description):
    """Run a shell command and return success status"""
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        return result.returncode == 0, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return False, "", "Command timeout"
    except Exception as e:
        return False, "", str(e)


def main():
    project_root = Path(__file__).parent.parent.parent
    tiling_dir = project_root / "tests" / "test_outputs" / "tiling"
    stitch_binary = project_root / "bin" / "stitch_tiles"
    noise_generator = project_root / "tests" / "util" / "generate_sstv_noise.py"
    venv_python = project_root / "venv" / "bin" / "python"

    # Verify requirements
    if not tiling_dir.exists():
        print(f"Error: Tiling directory not found: {tiling_dir}")
        sys.exit(1)

    if not stitch_binary.exists():
        print(f"Error: Stitch binary not found: {stitch_binary}")
        sys.exit(1)

    # Get list of tiles
    tiles = sorted(list(tiling_dir.glob("*.png")))
    if not tiles:
        print(f"Error: No PNG files found in {tiling_dir}")
        sys.exit(1)

    print("=" * 80)
    print("QRM STITCHING TEST")
    print("=" * 80)
    print(f"\nFound {len(tiles)} tiles to process")
    for tile in tiles[:3]:
        print(f"  {tile.name}")
    if len(tiles) > 3:
        print(f"  ... and {len(tiles)-3} more")

    presets = ["clean", "light_qrm", "medium_qrm", "heavy_qrm"]
    results = {}

    for preset in presets:
        print(f"\n{'='*80}")
        print(f"Processing {preset.upper()}")
        print(f"{'='*80}")

        # Create output directory for this preset
        output_dir = project_root / "tests" / "test_outputs" / f"qrm_test_{preset}"
        output_dir.mkdir(parents=True, exist_ok=True)

        print(f"\nGenerating {preset} tile variants...")
        success, stdout, stderr = run_command(
            [
                str(venv_python),
                str(noise_generator),
                "--dir", str(tiling_dir),
                "--output-dir", str(output_dir),
                "--preset", preset,
            ],
            f"Generate {preset} tiles"
        )

        if not success:
            print(f"✗ Failed to generate {preset} tiles")
            print(f"  Error: {stderr}")
            results[preset] = {"status": "generation_failed", "error": stderr}
            continue

        tile_count = len(list(output_dir.glob("*.png")))
        print(f"✓ Generated {tile_count} {preset} tiles")

        # Run stitcher on the corrupted tiles
        print(f"\nStitching {preset} tiles...")
        stitch_output = project_root / "tests" / "test_outputs" / f"stitch_{preset}.png"
        
        success, stdout, stderr = run_command(
            [
                str(stitch_binary),
                "--dir", str(output_dir),
                "-o", str(stitch_output),
            ],
            f"Stitch {preset} tiles"
        )

        if success and stitch_output.exists():
            size_mb = stitch_output.stat().st_size / (1024 * 1024)
            print(f"✓ Stitching succeeded: {stitch_output.name} ({size_mb:.2f} MB)")
            results[preset] = {
                "status": "success",
                "output": str(stitch_output),
                "size_mb": size_mb
            }
        else:
            print(f"✗ Stitching failed")
            # Extract relevant error info from stitch output
            if "ERROR" in stderr or "ERROR" in stdout:
                error_lines = [l for l in (stderr + stdout).split('\n') if "ERROR" in l or "error" in l]
                print(f"  Last error: {error_lines[-1] if error_lines else 'Unknown'}")
            results[preset] = {
                "status": "stitch_failed",
                "error": stderr or stdout
            }

    # Print summary
    print(f"\n{'='*80}")
    print("SUMMARY")
    print(f"{'='*80}\n")

    passed = sum(1 for r in results.values() if r["status"] == "success")
    total = len(results)

    print(f"Results: {passed}/{total} presets succeeded\n")

    for preset, result in results.items():
        status_symbol = "✓" if result["status"] == "success" else "✗"
        print(f"{status_symbol} {preset.ljust(15)} → {result['status']}")
        if result["status"] == "success":
            print(f"    Output: {result['output']}")
            print(f"    Size: {result['size_mb']:.2f} MB")
        elif "error" in result:
            error_msg = result["error"][:100].strip()
            print(f"    Error: {error_msg}")

    print(f"\n{'='*80}")

    if passed == total:
        print("✓ SUCCESS: All QRM levels produced valid stitched images!")
    elif passed > 0:
        print(f"⚠ PARTIAL: {passed}/{total} presets succeeded")
    else:
        print("✗ FAILURE: No presets succeeded")

    return 0 if passed == total else 1


if __name__ == "__main__":
    sys.exit(main())
