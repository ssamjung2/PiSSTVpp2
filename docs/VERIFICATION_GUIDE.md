# Test Verification Quick Reference

## Overview
Human verification system for validating SlowFrame test results with detailed documentation.

## Files Created
1. **`generate_verification_report.py`** - Generates interactive HTML reports
2. **`verify_tests_interactive.py`** - Terminal-based interactive verification
3. **`run_verification_workflow.sh`** - Automated workflow script

## Usage

### Method 1: HTML Report (Recommended for comprehensive review)

```bash
# Run tests and generate HTML report
cd tests
../venv/bin/python3 run_master_tests.py
../venv/bin/python3 generate_verification_report.py

# Open report (auto-opens on macOS)
open verification_reports/verification_report.html
```

**HTML Report Features:**
- ✅ Side-by-side input/output image comparison
- ✅ Test intent and expected behavior documentation
- ✅ Interactive checklist for each test
- ✅ Notes field for documenting findings
- ✅ Pass/Fail verdict buttons
- ✅ Saves verification state to browser localStorage
- ✅ Export results to JSON

**Workflow:**
1. Review each test card in the HTML report
2. Check verification checklist items
3. Compare input vs output images visually
4. Document observations in notes field
5. Mark verdict (PASS/FAIL)
6. Click "Save Verification" for each test
7. Export final results when complete

### Method 2: Interactive Terminal (Quick verification)

```bash
cd tests
../venv/bin/python3 verify_tests_interactive.py --suite aspect
```

**Terminal Features:**
- ✅ Step-through each test one-by-one
- ✅ Auto-opens images for viewing
- ✅ Prompt for Pass/Fail/Skip
- ✅ Capture notes directly in terminal
- ✅ Resume from last verified test
- ✅ Results saved to JSON

**Workflow:**
1. Script displays test intent and expected behavior
2. Prompts to view output image (auto-opens)
3. Enter verdict: PASS/FAIL/SKIP
4. Add notes (press Enter twice to finish)
5. Automatically moves to next test
6. Results saved after each test

### Method 3: Automated Workflow

```bash
cd tests
./run_verification_workflow.sh
```

This runs:
1. All test suites
2. Generates HTML report
3. Auto-opens report in browser

## Test Categories

### Aspect Ratio Tests (24 tests)
Tests 3 modes (CENTER, PAD, STRETCH) × 2 protocols (M1, R36) × 4 aspects (wide, tall, square, target)

**CENTER Mode:**
- **Intent:** Crop to target aspect, then scale
- **Expected:** No padding, center portion preserved
- **Verify:** No black edges beyond source content, correct dimensions

**PAD Mode:**
- **Intent:** Preserve aspect with letterbox/pillarbox padding
- **Expected:** Black bars on top/bottom OR left/right
- **Verify:** Bars symmetrical, source undistorted, correct total dimensions

**STRETCH Mode:**
- **Intent:** Non-uniform scaling (may distort)
- **Expected:** No padding added, aspect ratio changed
- **Verify:** Dimensions exact, source content preserved but squeezed/stretched

### Text Overlay Tests
- Basic overlay positioning
- Multi-line overlays
- Background bars for signal protection
- Custom styling (colors, fonts, opacity)

**Verify:**
- Text readable and correctly positioned
- Background bars present when specified
- No overlap with critical image content
- Colors match SSTV standards (blue text, white background)

### Image Format Tests (27 tests)
- PNG (with transparency)
- JPEG (various quality levels)
- GIF (animated and static)
- BMP, TIFF, WebP

**Verify:**
- Format loads without errors
- Transparency handled correctly (converted to opaque background)
- Colors preserved accurately
- No format-specific artifacts

## Verification Checklist

For each test, verify:

✅ **Dimensions**
- Output matches expected dimensions exactly
- No unexpected clipping or overflow

✅ **Visual Quality**
- Colors accurate and vibrant
- No compression artifacts
- Sharp edges (no excessive blurring)

✅ **Aspect Ratio**
- CENTER: No padding added
- PAD: Correct letterbox/pillarbox placement
- STRETCH: Proper distortion expected

✅ **Text Overlays** (when applicable)
- Text readable and positioned correctly
- Background bars present/absent as expected
- No text truncation

✅ **Source Content**
- Expected portions of source visible
- No unexpected cropping
- Source black pixels preserved (not created)

## Common Issues to Look For

### False Negatives (Test passes but shouldn't)
- Padding added in CENTER/STRETCH modes
- Text overlay missing or wrong position
- Wrong dimensions (test didn't validate size)
- Colors washed out or heavily distorted

### False Positives (Test fails but is actually correct)
- Source image contains black content (not padding)
- Expected distortion in STRETCH mode (not a bug)
- Format-specific color shifts (within tolerance)

## Output Locations

- **Test images:** `tests/test_outputs/*.png`
- **Audio files:** `tests/test_outputs/*.wav`
- **Verification reports:** `tests/verification_reports/*.html`
- **Verification results:** `tests/verification_results.json`

## Viewing Images

```bash
# View specific test output
open tests/test_outputs/center_m1_wide.png

# View all outputs for a mode
open tests/test_outputs/pad_*.png

# View input test images
open tests/images/alt2_color_bars_2000x1125.png
```

## Tips for Effective Verification

1. **Use HTML report for comprehensive review** - See all tests at once
2. **Check images at actual size** - Image rendering quality matters
3. **Compare input vs output side-by-side** - Understand transformation
4. **Document specific pixel counts** - "40px black bar on right edge"
5. **Test on different displays** - Color accuracy varies
6. **Verify SSTV standards** - Blue text, proper dimensions
7. **Check edge cases** - Square images, already-correct sizes

## Example Verification Notes

**Good notes:**
```
PASS - Letterbox bars correctly placed:
- Top bar: 20px black
- Bottom bar: 20px black  
- Bars symmetrical
- Source aspect preserved (no distortion)
- Center positioned correctly
- Dimensions verified: 320x256
```

**Detailed failure notes:**
```
FAIL - RIGHT EDGE PADDING ISSUE:
- Unexpected 42px black bar on right edge (columns 278-319)
- Should be full-width in STRETCH mode
- Source image has 13.2% black content
- Output has 13.1% black (correct %)
- BUT black concentrated on right edge (wrong placement)
- Root cause: vips_resize not scaling to exact width?
```

## Stress Testing Recommendations

To thoroughly stress test features:

### Aspect Ratio Stress Tests
- [ ] Extreme aspects (1:10, 10:1)
- [ ] Very small images (<50px)
- [ ] Very large images (>10000px)
- [ ] Exact target size (no scaling needed)
- [ ] Off-by-one dimensions (319x255)

### Text Overlay Stress Tests
- [ ] Very long text (>100 characters)
- [ ] Special characters (UTF-8, emoji)
- [ ] Overlapping overlays
- [ ] All 9 position combinations
- [ ] Maximum font sizes
- [ ] Transparent backgrounds

### Format Stress Tests  
- [ ] Corrupted file headers
- [ ] Unsupported formats
- [ ] Extremely large files (>100MB)
- [ ] 1-bit, 8-bit, 16-bit, 32-bit depths
- [ ] Obscure color profiles (CMYK, LAB)

## Automation Potential

Future enhancements:
- Pixel-perfect diff against reference images
- Automated OCR for text overlay verification
- Color histogram comparison
- Perceptual image hashing (detect visual changes)
- CI/CD integration with automated verification

## Questions During Verification

If you find issues, document:
1. What did you expect to see?
2. What did you actually see?
3. Is this a test validation bug or actual code bug?
4. Can you reproduce it manually?
5. Does it affect SSTV transmission quality?
6. What's the severity? (cosmetic vs critical)

## Support

For questions or issues with the verification system:
- Check test intent in `generate_verification_report.py`
- Review test implementation in `test_aspect_comprehensive.py`
- Examine validation logic in `tests/util/aspect_validator.py`
