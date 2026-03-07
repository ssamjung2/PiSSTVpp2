# QRM Testing Guide - Methodology & Usage

## Overview

SlowFrame includes a comprehensive QRM (RF Interference) testing framework to evaluate tile placement recovery and EXIF preservation under realistic reception conditions.

## Test Components

### 1. Noise Generator

**Purpose:** Apply realistic SSTV QRM patterns to PNG tiles

**File:** `tests/util/generate_sstv_noise.py`

**Available Presets:**
- `clean` - Minimal noise, baseline reference
- `light_qrm` - Light interference, occasional artifacts
- `medium_qrm` - Scattered corruption zones
- `heavy_qrm` - Severe interference with extreme data loss
- `extreme_qrm` - Near-unrecoverable signal
- `fading_only` - Ionospheric fading without QRM
- `tonal_interference` - Narrow-band interference (birdies)
- `burst_noise` - Intermittent burst noise events
- `header_corruption` - Noise focused on header region

**Usage Examples:**

```bash
# Single file with light QRM
python3 tests/util/generate_sstv_noise.py \
  --input tile.png \
  --output noisy.png \
  --preset light_qrm

# Batch process entire directory
python3 tests/util/generate_sstv_noise.py \
  --dir tests/test_outputs/tiling \
  --output-dir tests/test_outputs/noisy_medium \
  --preset medium_qrm

# Custom noise parameters
python3 tests/util/generate_sstv_noise.py \
  --input tile.png \
  --output custom_noise.png \
  --custom \
  --white-noise 0.15 \
  --hash-lines 0.20 \
  --impulses 5 \
  --burst-width 8

# Reproducible results with seed
python3 tests/util/generate_sstv_noise.py \
  --input tile.png \
  --output reproducible.png \
  --preset heavy_qrm \
  --seed 42

# List available presets
python3 tests/util/generate_sstv_noise.py --list-presets
```

### 2. QRM Testing Suite

**Purpose:** Comprehensive multi-stage testing of tile placement and EXIF preservation

**File:** `tests/util/test_qrm_exif_recovery.py`

**Test Stages:**
1. Noise Generation - Apply QRM presets to tiles
2. Discovery - Identify tile sets and session IDs
3. Header Analysis - Check position marker recovery
4. EXIF Check (Noisy) - Verify metadata in corrupted tiles
5. Stitching - Reassemble tiles using discovered positions
6. EXIF Check (Output) - Verify metadata in stitched output
7. Visual Quality - Analyze output dimensions and file size

**Usage:**

```bash
# Run full test suite (all presets)
python3 tests/util/test_qrm_exif_recovery.py

# Output includes:
# - Console summary with per-stage results
# - Detailed JSON results in tests/test_outputs/qrm_exif_recovery_results.json
# - Test-specific directories with intermediate files and outputs
```

### 3. Integration with Stitcher

The stitch_tiles binary includes recovery options for difficult QRM scenarios:

```bash
# Auto-fill missing tiles (for heavy QRM)
./bin/stitch_tiles --dir noisy_tiles/ --fill-missing -o output.png

# Override corrupted session IDs
./bin/stitch_tiles --dir tiles/ --override-session 0x12345678 -o output.png

# Detailed diagnostic output
./bin/stitch_tiles --dir tiles/ --verbose --list

# Select specific tile set if multiple available
./bin/stitch_tiles --dir tiles/ --session 0x69AB1283 -o output.png
```

## Running Full Test Suite

### Standard Test (All Presets)

```bash
cd SlowFrame
./venv/bin/python tests/util/test_qrm_exif_recovery.py 2>&1 | tee qrm_test.log
```

**Expected Output:**
- Stage-by-stage console output
- Test results saved to `tests/test_outputs/qrm_exif_recovery_results.json`
- Test directories created for each preset with intermediate files

**Runtime:** ~5-10 minutes depending on system

### Custom Test (Single Preset)

To test only one QRM level:

```bash
# Generate noisy tiles
python3 tests/util/generate_sstv_noise.py \
  --dir tests/test_outputs/tiling \
  --output-dir my_custom_test/noisy \
  --preset medium_qrm

# Discover tile sets
./bin/stitch_tiles --dir my_custom_test/noisy --list

# Stitch a specific session
./bin/stitch_tiles --dir my_custom_test/noisy \
  --session 0x69AB1283 \
  -o my_custom_test/output.png

# Verify EXIF preservation
python3 << 'EOF'
from PIL import Image
img = Image.open("my_custom_test/output.png")
exif = img.getexif()
print(f"EXIF present: {bool(exif)}")
if exif:
    print(f"Tags: {len(exif)}")
EOF
```

## Interpreting Results

### Session Discovery

Output shows:
```
Available Tile Sets

Tile Set #1:
  Session ID: 0x69AB1283
  Original:   1920×1440 px
  Grid:       3x3  (9 tiles)
  Available:  9/9 tiles  ✅ COMPLETE
  
Tile Set #2:
  Session ID: 0x69A98C9E
  Original:   1920×1440 px
  Grid:       3x3  (9 tiles)
  Available:  8/9 tiles  ⚠️ 1 MISSING
```

**Interpretation:**
- ✅ Complete sets can be stitched directly
- ⚠️ Incomplete sets can use `--fill-missing` option

### EXIF Preservation Status

```json
{
  "exif_check_noisy_tiles": {
    "tiles_checked": 5,
    "tiles_with_exif": 5,
    "sample_tiles": {
      "tile-01.png": {
        "has_exif": true,
        "iso": 50,
        "f_number": "2/1",
        "focal_length": "3039/1000",
        "date_time": "2016:12:29 19:52:50"
      }
    }
  },
  "exif_check_stitched": {
    "has_exif": true,
    "iso": null,
    "f_number": null,
    "focal_length": null,
    "date_time": "2016:12:29 19:52:50"
  }
}
```

**Interpretation:**
- `has_exif: true` = Metadata successfully preserved
- Specific fields (ISO, f-number, focal_length) may not be present in stitched output if reference tile didn't have them
- Date/Time is typically always preserved

### Stitching Results

**Success indicators:**
```
✓ Stitching successful
  Output size: 1738169 bytes
✓ Output dimensions: (960, 696)
```

**Failure indicators:**
```
✗ Stitching failed: [ERROR] 1 tile(s) missing from grid
```

**Recovery options for failures:**
1. Check if complete tile set exists: `--list`
2. If tiles exist but positions wrong: `--override-session` with known good session ID
3. If tiles are truly missing: `--fill-missing` to auto-fill

## Interpreting Header Recovery Metrics

```
Header detection rate: 5.6%
Avg header corruption: 78.45
```

**Explanation:**
- **Detection rate:** Percentage of tile headers where position markers are visually detectable
- **Corruption score:** Average color deviation in header region (0-255 scale)
  - <50 = Clean headers
  - 50-70 = Light corruption
  - 70-85 = Medium corruption  
  - >85 = Heavy corruption with position recovery failures

**Recovery Success by Corruption Level:**
- <50: ✅ 100% recovery
- 50-70: ✅ 100% recovery
- 70-85: ✅ 95%+ recovery
- >85: ⚠️ 80-90% recovery

## Test Data Organization

After running tests, results are organized as:

```
tests/test_outputs/
├── qrm_exif_test_clean_TIMESTAMP/
│   ├── noisy_tiles/
│   │   ├── *.png (18 noisy tiles)
│   ├── stitched_clean.png
│
├── qrm_exif_test_light_qrm_TIMESTAMP/
│   ├── noisy_tiles/
│   ├── stitched_light_qrm.png
│
├── qrm_exif_test_medium_qrm_TIMESTAMP/
│   ├── noisy_tiles/
│   ├── stitched_medium_qrm.png
│
├── qrm_exif_test_heavy_qrm_TIMESTAMP/
│   ├── noisy_tiles/
│   ├── (stitched output may not exist if failed)
│
├── qrm_exif_recovery_results.json  (comprehensive results)
└── qrm_test_output.log             (console output)
```

## Comparing Test Results

### Quick Comparison

```bash
# Compare file sizes across presets
for dir in tests/test_outputs/qrm_exif_test_*/; do
  preset=$(basename "$dir" | cut -d_ -f4-5)
  size=$(ls -lh "$dir"/stitched_*.png 2>/dev/null | awk '{print $5}')
  echo "$preset: $size"
done
```

### Statistical Analysis

```python
#!/usr/bin/env python3
import json

with open('tests/test_outputs/qrm_exif_recovery_results.json') as f:
    results = json.load(f)

for preset, data in results['preset_results'].items():
    stages = data['stages']
    print(f"\n{preset.upper()}:")
    
    # Stitching success
    stitch = stages.get('stitching', {})
    print(f"  Stitching: {'✅' if stitch.get('success') else '❌'}")
    
    # EXIF preservation
    exif_noisy = stages.get('exif_check_noisy_tiles', {})
    exif_stitched = stages.get('exif_check_stitched', {})
    print(f"  EXIF (noisy): {exif_noisy.get('tiles_with_exif', 0)}/{exif_noisy.get('tiles_checked', 0)}")
    print(f"  EXIF (stitched): {'✅' if exif_stitched.get('has_exif') else '❌'}")
```

## Troubleshooting

### "No complete tile set found"

**Problem:** Stitcher can't find a complete set of tiles

**Solutions:**
1. Check tile files exist: `ls noisy_tiles/*.png | wc -l`
2. List available sets: `./bin/stitch_tiles --dir noisy_tiles --list`
3. Use `--fill-missing` if set is incomplete
4. Specify session explicitly: `--session 0xSESSION_ID`

### "1 tile(s) missing from grid"

**Problem:** Header corruption is too severe for position recovery

**Solutions:**
1. Use `--fill-missing`:
   ```bash
   ./bin/stitch_tiles --dir tiles/ --fill-missing -o output.png
   ```

2. Lower QRM level and re-test:
   ```bash
   python3 tests/util/generate_sstv_noise.py \
     --dir source_tiles \
     --output-dir output \
     --preset light_qrm  # instead of heavy_qrm
   ```

### EXIF not preserved

**Problem:** Stitched output has no EXIF data

**Causes & Solutions:**
1. Source tiles don't have EXIF: Add EXIF when encoding
2. Noise generator didn't preserve EXIF: Update to latest version
3. Stitcher lost EXIF: Should not happen with current code

**Verify source has EXIF:**
```python
from PIL import Image
img = Image.open("source.png")
print(f"Has EXIF: {bool(img.getexif())}")
```

## Performance Considerations

### Noise Generation Performance

**Time per tile:** ~0.5-1.0 seconds
**Factors:** 
- Image resolution (320×240 = ~1s, 1920×1440 = ~2s)
- Noise complexity (clean < light < medium < heavy)
- System CPU performance

**Speed up:** Use `-j 4` flag if available for parallel processing

### Stitching Performance

**Time per stitch:** 2-5 seconds
**Factors:**
- Grid size (2×2 < 3×3 < 4×4)
- Tile complexity (plain < noisy)
- Gaussian blur options

### Memory Usage

**Peak memory:** <500MB for typical test suite
**Disk space:** ~100MB per preset (temporary tiles + output)

## Advanced Testing

### Creating Custom Noise Profiles

```bash
# High-fidelity QRM simulation
python3 tests/util/generate_sstv_noise.py \
  --input tile.png \
  --output custom.png \
  --custom \
  --noise-level 0.8 \
  --white-noise 0.25 \
  --hash-lines 0.15 \
  --hash-width 3 \
  --impulses 12 \
  --impulse-size 20 \
  --fading \
  --fading-frequency 2 \
  --bursts 3 \
  --burst-width 5
```

### Comparing Recovery Strategies

Test multiple session handling strategies:

```bash
# Strategy 1: Direct stitching (clean sets only)
./bin/stitch_tiles --dir tiles/ -o output_direct.png

# Strategy 2: With fill-missing for incomplete sets
./bin/stitch_tiles --dir tiles/ --fill-missing -o output_filled.png

# Strategy 3: With session override for header recovery
./bin/stitch_tiles --dir tiles/ --override-session 0x69AB1283 -o output_override.png
```

### Measuring QRM Resistance

```bash
#!/bin/bash
# Measure maximum recoverable QRM level

for qrm_level in clean light_qrm medium_qrm heavy_qrm; do
  echo "Testing $qrm_level..."
  
  # Generate
  python3 tests/util/generate_sstv_noise.py \
    --dir source_tiles \
    --output-dir test_$qrm_level \
    --preset $qrm_level
  
  # Stitch
  if ./bin/stitch_tiles --dir test_$qrm_level -o output_$qrm_level.png; then
    echo "✅ $qrm_level: SUCCESS"
  else
    echo "❌ $qrm_level: FAILED"
    break
  fi
done
```

## Next Steps

1. **Review Results:** Check `QRM_RECOVERY_EXECUTIVE_SUMMARY.md`
2. **Understand Limits:** Read `QRM_TEST_RESULTS.md` for detailed metrics
3. **Test Your Imagery:** Run against actual satellite reception samples
4. **Optimize Parameters:** Adjust noise presets based on your RF environment
5. **Deploy:** Use in production with understanding of documented limitations

## References

- Main test script: [test_qrm_exif_recovery.py](../tests/util/test_qrm_exif_recovery.py)
- Noise generator: [generate_sstv_noise.py](../tests/util/generate_sstv_noise.py)
- Stitcher documentation: [BUILD.md](BUILD.md)
- Results summary: [QRM_RECOVERY_EXECUTIVE_SUMMARY.md](QRM_RECOVERY_EXECUTIVE_SUMMARY.md)
- Detailed analysis: [QRM_TEST_RESULTS.md](QRM_TEST_RESULTS.md)
