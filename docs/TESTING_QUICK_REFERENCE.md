# SlowFrame Test Suite - Quick Reference Guide

## Quick Start

### Run All Tests
```bash
cd /Users/ssamjung/Desktop/WIP/SlowFrame
python3 tests/run_master_tests.py
```

Expected output:
```
Overall Results:
  Total Tests: 55
  Passed:      55 (100.0%)
  Failed:      0
  
✓ No issues found - all tests passed!
```

## Individual Test Suites

### 1. Aspect Ratio Tests (24 tests)

Test all three aspect modes with diverse image shapes:

```bash
# Run all aspect tests
python3 tests/test_aspect_comprehensive.py

# Run with verbose output
python3 tests/test_aspect_comprehensive.py --verbose

# Run only aspect tests from master suite
python3 tests/run_master_tests.py --suite aspect
```

**What it tests:**
- CENTER mode: 8 tests (wide/tall/square/target × 2 protocols)
- PAD mode: 8 tests (wide/tall/square/target × 2 protocols)
- STRETCH mode: 8 tests (wide/tall/square/target × 2 protocols)

**Test images used:**
- Wide: `alt2_color_bars_2000x1125.png` (1.78:1)
- Tall: `alt2_color_bars_680×1209.png` (0.56:1)  
- Square: `alt5_test_panel_1728x1728.jpg` (1:1)
- Target: `alt_color_bars_320x256.png` (1.25:1)

**Protocols tested:**
- M1: 320x256 (4:3.2 aspect)
- R36: 320x240 (4:3 aspect)

### 2. Format Tests (27 tests)

Test all supported image formats:

```bash
# Run format tests
python3 tests/util/format_validator.py tests/images

# With verbose output
python3 tests/util/format_validator.py tests/images --verbose

# From master suite
python3 tests/run_master_tests.py --suite formats
```

**Formats tested:**
- PNG: 12 images ✓
- JPEG: 11 images ✓
- GIF: 1 image ✓
- BMP: 1 image ✓
- PPM: 1 image ✓
- TIFF: 1 image ✓
- WebP: 1 image ✓

### 3. Text Overlay Tests (4 tests)

Test text overlay functionality:

```bash
# From master suite only
python3 tests/run_master_tests.py --suite overlay
```

**Overlay specs tested:**
- Simple text: `N0CALL`
- Colored text: `TEST|color=yellow`
- Positioned text: `BOTTOM|placement=bottom`
- Sized text: `BIG|size=32`

## Validation Tools

### Aspect Ratio Validator

Validate a single aspect transformation:

```bash
python3 tests/util/aspect_validator.py \
    output_image.png \
    <mode> \
    <src_width> <src_height> \
    <target_width> <target_height>
```

**Example:**
```bash
# Validate PAD mode transformation
python3 tests/util/aspect_validator.py \
    tests/test_outputs/aspect/pad_m1_wide_debug.png \
    pad \
    2000 1125 \
    320 256 \
    --verbose
```

**Modes:** center, pad, stretch

### Format Validator

Test format support:

```bash
# Test all images in directory
python3 tests/util/format_validator.py tests/images

# Test specific image
python3 tests/util/format_validator.py tests/images \
    --binary ./bin/slowframe

# JSON output
python3 tests/util/format_validator.py tests/images --json
```

## Test Outputs

All outputs saved to `tests/test_outputs/`:

```
tests/test_outputs/
├── aspect/
│   ├── center_m1_wide.wav
│   ├── center_m1_wide_debug.png
│   ├── pad_m1_tall.wav
│   ├── stretch_r36_square.wav
│   └── ... (72 files total)
├── formats/
│   ├── test_format_png_*.wav
│   ├── test_format_jpeg_*.wav
│   └── ... (27 files)
├── overlay/
│   └── ... (4 files)
├── comprehensive_test_report.json
└── master_test_run.log
```

## Interpreting Results

### Success
```
✓ Aspect Ratio   : 24/24 passed
✓ Image Formats  : 27/27 passed
✓ Text Overlay   : 4/4 passed
```

### Failure Example
```
✗ Aspect Ratio   : 22/24 passed
  Issues Found (2):
    1. Aspect PAD: Image does not fit in canvas
    2. Aspect STRETCH: Processing timeout
```

## Common Issues

### Issue: PIL Not Available

**Symptom:**
```
Warning: PIL not available, using basic validation only
✓ PIL not available, basic check only
```

**Impact:** Limited - tests still verify execution succeeds
**Fix (optional):** Install Pillow for enhanced validation:
```bash
python3 -m pip install --user Pillow
```

### Issue: Missing Test Images

**Symptom:**
```
Missing test coverage for formats: TIFF, WebP
```

**Fix:** Add test images to `tests/images/`:
```bash
cd tests/images
convert existing_image.png test_image.tiff
convert existing_image.png test_image.webp
```

### Issue: SlowFrame Binary Not Found

**Symptom:**
```
Error: SlowFrame binary not found: bin/slowframe
```

**Fix:** Build SlowFrame:
```bash
make clean && make all
```

## Advanced Usage

### Generate JSON Report

```bash
python3 tests/run_master_tests.py \
    --report-file my_report.json

# View report
cat my_report.json | python3 -m json.tool
```

### Test Specific Directory

```bash
python3 tests/run_master_tests.py \
    --image-dir /path/to/test/images \
    --output-dir /path/to/outputs
```

### Custom Binary Location

```bash
python3 tests/run_master_tests.py \
    --binary /path/to/slowframe
```

### Verbose Debugging

```bash
python3 tests/run_master_tests.py --verbose 2>&1 | tee debug.log
```

## Performance Benchmarks

**Expected execution times:**

| Suite | Tests | Time | Per-Test Avg |
|-------|-------|------|--------------|
| Aspect Ratio | 24 | ~3.7s | ~0.15s |
| Image Formats | 27 | ~4.3s | ~0.16s |
| Text Overlay | 4 | ~0.8s | ~0.20s |
| **Total** | **55** | **~8.8s** | **~0.16s** |

**Large image performance:**
- 9600x5400 JPEG: processes successfully
- Average processing time: <1s per image

## CI/CD Integration

### Exit Codes
- `0`: All tests passed
- `1`: One or more tests failed
- `2`: Fatal error (missing binary, etc.)

### Example CI Script
```bash
#!/bin/bash
set -e

# Build
make clean && make all

# Run tests
python3 tests/run_master_tests.py \
    --report-file test_results.json

# Archive results
tar -czf test_outputs.tar.gz tests/test_outputs/

# Exit with test status
exit $?
```

## Adding New Tests

### Add Aspect Ratio Test

1. Add test image to `tests/images/`
2. Categorized automatically by aspect ratio
3. Re-run: `python3 tests/test_aspect_comprehensive.py`

### Add Format Test

1. Add image with new format to `tests/images/`
2. Re-run: `python3 tests/util/format_validator.py tests/images`

### Add Overlay Test

1. Edit `tests/run_master_tests.py`
2. Add test case to `run_overlay_tests()` method
3. Re-run: `python3 tests/run_master_tests.py --suite overlay`

## Troubleshooting

### Tests Running Slowly

Check for audio encoding - use `-N` flag in test commands to skip audio:
```bash
# Already configured in test suite, but for manual testing:
./bin/slowframe -i input.png -o output.wav -N
```

### Validation Skipped

Install Pillow for detailed validation:
```bash
# Option 1: User install
python3 -m pip install --user --break-system-packages Pillow

# Option 2: Virtual environment
python3 -m venv venv
source venv/bin/activate
pip install Pillow
python3 tests/run_master_tests.py
```

### Debug Individual Test

```bash
# Run SlowFrame manually with same params as test
./bin/slowframe \
    -i tests/images/alt2_color_bars_2000x1125.png \
    -p m1 \
    -a pad \
    -o /tmp/test_pad.wav \
    -K \
    -v

# Check intermediate image
open /tmp/test_pad.png  # or your debug output location
```

## Support

For issues or questions:
1. Check `tests/TESTING_COMPREHENSIVE_README.md` for detailed documentation
2. Review test logs in `tests/test_outputs/master_test_run.log`
3. Examine JSON report: `tests/test_outputs/comprehensive_test_report.json`
4. Run individual tests with `--verbose` flag for detailed output
