# Comprehensive Text Overlay Test Suite Documentation

**File**: `tests/test_text_overlay_comprehensive.py`  
**Purpose**: Complete stress testing and QSO unit testing for text overlay features  
**Creation Date**: February 11, 2026  
**Status**: Ready for execution  

## Overview

This test suite provides comprehensive validation of the text overlay system with two main categories:

1. **STRESS TESTS** (8 tests) - Validate all styling options work correctly together
2. **QSO UNIT TESTS** (8 test groups) - Verify common radio QSO exchange scenarios

All tests save intermediate debug images (`-K` flag) for visual verification.

## Key Features

- **Complete Option Coverage**: Tests every supported parameter
  - All 12 named colors (red, lime, blue, cyan, magenta, yellow, white, black, orange, purple, pink, green)
  - All 12 hexadecimal colors (#FF0000 through #008000)
  - All 5 placements (top, bottom, left, right, center)
  - All 3 horizontal alignments (left, center, right)
  - All 3 vertical alignments (top, center, bottom)
  - Font sizes 8-32px (13 variations)
  - Padding sizes 0-16px (9 variations)
  - Border widths 0-4px (5 variations)
  - Background color combinations

- **Realistic QSO Scenarios**: Tests practical use cases
  - Basic callsign + grid exchanges
  - Full RST (Readability-Strength-Tone) reports with dates
  - Detailed station information (mode, power, antenna, time)
  - Callsign variants (portable /P, mobile /M, maritime /MM)
  - Multiple SSTV modes (M1, M2, M3, Robot, Scottie)
  - Power/antenna combinations
  - Official contest exchange format
  - Multi-color professional displays

- **Debug Image Preservation**: All tests with `-K` flag
  - Intermediate PNG images saved to `test_outputs/text_overlay_comprehensive/`
  - Each test creates a timestamped subdirectory
  - Images named after test for easy identification
  - File sizes tracked for verification

- **Comprehensive Results Reporting**: JSON output tracking
  - Individual test results with timestamps
  - Pass/fail status with error messages
  - Summary statistics (pass rate, total tests)
  - Test metadata for each execution

## Running the Tests

### Quick Start (All Tests)

```bash
cd tests
python3 test_text_overlay_comprehensive.py --exe ../bin/pisstvpp2
```

### Expected Output Structure

```
test_outputs/text_overlay_comprehensive/
├── colors_stress_20260211_213800/
│   ├── output.wav
│   └── colors_stress_20260211_213800.png
├── hex_colors_stress_20260211_213801/
│   ├── output.wav
│   └── hex_colors_stress_20260211_213801.png
├── qso_basic_20260211_213802/
│   ├── output.wav
│   └── qso_basic_20260211_213802.png
├── qso_contest_20260211_213803/
│   ├── output.wav
│   └── qso_contest_20260211_213803.png
└── comprehensive_results_20260211_214000.json
```

### Results File Example

The `comprehensive_results_*.json` file contains:

```json
{
  "summary": {
    "passed": 45,
    "failed": 0,
    "total": 45,
    "percentage": 100.0,
    "timestamp": "2026-02-11T21:40:00.123456",
    "test_dir": "test_outputs/text_overlay_comprehensive"
  },
  "results": [
    {
      "name": "Named colors stress (12 colors, 3 positions, 4 sizes)",
      "status": "PASSED",
      "message": "Debug image: colors_stress_20260211_213800.png (45KB)",
      "timestamp": "2026-02-11 21:38:00"
    }
    // ... more test results
  ]
}
```

## Test Categories

### STRESS TESTS (8 tests)

These tests validate that multiple overlays work correctly with extensive option combinations.

#### 1. Named Colors Stress Test
- **Purpose**: Validate all 12 named colors render correctly
- **Coverage**: 12 colors × 3 positions × 4 sizes = 144 overlays per test
- **Debug Image**: Shows all named colors in different sizes and positions
- **Pass Criteria**: 
  - All 144 overlays compose without errors
  - Debug image created and contains visible colored text
  - No rendering artifacts or corruption

#### 2. Hexadecimal Colors Stress Test
- **Purpose**: Verify hex color format (#RRGGBB) works correctly
- **Coverage**: 12 hex colors × 3 positions × 2 alignments = 72 overlays
- **Options Tested**:
  - Color format: `#FF0000`, `#00FF00`, `#0000FF`, etc.
  - Positions: top, center, bottom
  - Alignments: left, right (alternating)
- **Pass Criteria**: All hex colors render with correct RGB values

#### 3. All Positions Stress Test
- **Purpose**: Test all 5 placement options work correctly
- **Coverage**: 5 placements × 2 overlay types = 10 overlays
- **Placements Tested**:
  - `top` - Top edge of image
  - `bottom` - Bottom edge of image
  - `left` - Left edge of image
  - `right` - Right edge of image
  - `center` - Center of image
- **Variations**: Different sizes and colors at each position

#### 4. All Alignments Stress Test
- **Purpose**: Validate horizontal and vertical text alignment
- **Coverage**: 3 h-align × 3 v-align = 9 combinations
- **Options Tested**:
  - Horizontal: left, center, right
  - Vertical: top, center, bottom
  - Background colors for visual clarity
- **Pass Criteria**: Text positioned correctly within each alignment box

#### 5. All Font Sizes Stress Test
- **Purpose**: Verify text rendering at all supported sizes
- **Coverage**: 13 font sizes (8px-32px) × 3 positions = 39 overlays
- **Sizes Tested**: 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32px
- **Variations**: Cycling colors for visual distinction
- **Pass Criteria**: All sizes render legibly without clipping

#### 6. All Padding Sizes Stress Test
- **Purpose**: Test padding around text with background
- **Coverage**: 9 padding values × 3 positions = 27 overlays
- **Padding Values Tested**: 0, 1, 2, 3, 4, 6, 8, 12, 16 pixels
- **Background**: Red background to show padding effect
- **Pass Criteria**: Padding applied correctly, text centered within box

#### 7. All Border Styles Stress Test
- **Purpose**: Validate border rendering at different widths
- **Coverage**: 5 border widths × 3 positions = 15 overlays
- **Border Widths**: 0 (none), 1, 2, 3, 4 pixels
- **Background**: Blue background for contrast
- **Pass Criteria**: Borders render without affecting text position

#### 8. Background Color Combinations Stress Test
- **Purpose**: Test all text/background color pairings
- **Coverage**: 3 text colors × 6 background colors = 18 overlays
- **Text Colors**: white, black, yellow
- **Background Colors**: red, green, blue, cyan, magenta, yellow
- **Pass Criteria**: All combinations readable with good contrast

### QSO UNIT TESTS (8 test groups)

These tests validate realistic radio QSO (contact) scenario with proper exchange information.

#### 1. Basic QSO Exchange
- **Purpose**: Fundamental contact information
- **Overlays**:
  - Callsign: `W5ZZZ` (white, 20px, top center)
  - Grid: `EM12ab` (yellow, 16px, top center)
- **Use Case**: Minimal exchange for quick identification
- **Pass Criteria**: Both overlays visible and readable

#### 2. QSO with RST Report
- **Purpose**: Complete basic exchange with signal report
- **Overlays**:
  - Callsign: `W5ZZZ` (lime, 18px, top)
  - Grid: `EM12ab` (cyan, 14px, top)
  - Report: `RST 579` (yellow, 16px, center)
  - Date: `2026-02-11` (white, 12px, bottom)
- **RST Format**: Readability(5)-Strength(7)-Tone(9)
- **Pass Criteria**: Complete exchange clearly displayed

#### 3. Detailed Station Information
- **Purpose**: Comprehensive station details for logging
- **Overlays** (7 total):
  - Station: `W5ZZZ` (white, 18px, top-left)
  - Grid: `EM12ab` (yellow, 14px, top-right)
  - Mode: `SSB 80M` (lime, 14px, center-left)
  - Report: `RST 579` (cyan, 14px, center-right)
  - Power: `100W` (orange, 12px, center)
  - Antenna: `Dipole` (pink, 12px, bottom-left)
  - Time: `17:30 UTC` (white, 12px, bottom-right)
- **Use Case**: Complete contact documentation
- **Pass Criteria**: All 7 overlays visible and organized

#### 4. Callsign Variants
- **Purpose**: Support portable, mobile, maritime callsigns
- **Test Cases**:
  - Portable: `W5ZZZ/P` in CM97bj (Portable operation)
  - Mobile: `K4ABC/M` in EM75 (Vehicle operation)
  - Maritime Mobile: `N0CALL/MM` in AN75 (Ship/boat operation)
  - Cross-border: `VE3XYZ/VE2` in FN25 (Canadian provinces)
- **Pass Criteria**: All variants render correctly

#### 5. Various SSTV Modes
- **Purpose**: Identify transmission mode for decoders
- **Test Cases**:
  - Black & White: Scottie 1, Scottie 2
  - Color: SSTV M1, M2, M3, Robot 36, Robot 72
- **Each Test Includes**:
  - Station ID: `W5ZZZ`
  - Mode name: `SSTV M1` etc.
  - Mode type: `VIS 44`, `Color`, `BW`
- **Pass Criteria**: Mode information clearly displayed

#### 6. Power and Antenna Combinations
- **Purpose**: Equipment identification for QSO logging
- **Tested Combinations**:
  - Power levels: 5W, 10W, 50W, 100W, 500W, 1000W
  - Antennas: Dipole, Yagi, Vertical, Loop, End-fed, Mobile whip
- **Display**: Power on left, antenna on right
- **Pass Criteria**: Equipment clearly readable

#### 7. Contest Exchange Format
- **Purpose**: Official competition QSO format (6 lines)
- **Format**:
  ```
  ← STATION (large, white on navy)
  ← W5ZZZ (large, yellow)
  ← GRID SQUARE    |    SIGNAL REPORT (labeled)
  ← EM12ab         |    RST 579 (large, aligned)
  ← EQUIPMENT (bottom-left) | ← LOCATION (bottom-right)
  ← 100W Dipole            | ← Texas, USA (with colors)
  ```
- **Pass Criteria**: Professional appearance with proper organization

#### 8. Multi-Color Realistic QSO
- **Purpose**: Visually appealing professional display
- **Overlays** (9 total with colors and backgrounds):
  - Header: `STATION` labeled in navy background
  - Identity: `W5ZZZ` in yellow
  - Grid section: `GRID SQUARE` label + `EM12ab` in green
  - Signal section: `SIGNAL REPORT` label + `RST 579` in cyan
  - Equipment: `EQUIPMENT` label + `100W Dipole` in orange
  - Location: `LOCATION` label + `Texas, USA` in magenta/pink
- **Pass Criteria**: Professional appearance, all colors visible

## Testing Check Points

### Visual Verification (Debug Images)

1. **Color Accuracy**: Do colors appear as specified?
   - Named colors match expected RGB values
   - Hex colors render with correct hue

2. **Position Accuracy**: Is text placed correctly?
   - Top/bottom/left/right overlays at proper edges
   - Center overlays centered horizontally and/or vertically
   - No clipping or off-screen positioning

3. **Alignment Accuracy**: Is text aligned correctly within bounds?
   - Left-aligned text starts at left edge
   - Center-aligned text centered
   - Right-aligned text ends at right edge

4. **Font Size Accuracy**: Are text sizes visible?
   - Small sizes (8-12px) tiny but readable
   - Medium sizes (14-20px) clearly visible
   - Large sizes (22-32px) take up significant space

5. **Background Rendering**: Do backgrounds appear correctly?
   - Opaque backgrounds completely hide image
   - Colors match specified RGB/hex values
   - Padding creates proper spacing around text

6. **Text Quality**: Is rendering clean?
   - No artifacts or corruption
   - Anti-aliasing smooth (not blocky)
   - Font metrics appropriate for size

### Automated Verification (Results File)

- Test execution status (PASSED/FAILED)
- Debug image creation (file exists and has content)
- Command exit codes (0 = success)
- File sizes (confirms image data written)

## Implementation Notes

### Command Line Syntax

All tests use the unified `-T` flag format:

```bash
-T "text|key1=value1|key2=value2|..."
```

### Supported Parameters

| Parameter | Aliases | Values | Example |
|-----------|---------|--------|---------|
| `pos`, `position` | — | top, bottom, left, right, center | `pos=top` |
| `size`, `s` | — | 8-32 (pixels) | `size=18` |
| `align`, `a` | — | left, center, right | `align=center` |
| `v-align`, `va`, `valign` | — | top, center, bottom | `v-align=center` |
| `color`, `c` | — | color_name or #RRGGBB | `color=red` or `color=#FF0000` |
| `bg`, `background` | — | color_name or #RRGGBB | `bg=blue` |
| `pad`, `padding`, `x` | — | 0-255 (pixels) | `pad=4` |
| `border`, `b`, `d` | — | 0-255 (pixels) | `border=2` |

### Named Color Reference

```
Primary colors:
  red     - #FF0000    Primary red
  green   - #008000    Standard green
  blue    - #0000FF    Primary blue

Extended colors:
  lime    - #00FF00    Bright green
  cyan    - #00FFFF    Blue-green
  magenta - #FF00FF    Red-blue
  yellow  - #FFFF00    Red-green
  white   - #FFFFFF    All channels on
  black   - #000000    All channels off
  
Named additions:
  orange  - #FFA500    Red-yellow mix
  purple  - #800080    Dark magenta
  pink    - #FFC0CB    Light magenta
```

## Running Individual Test Groups

To run specific test categories (if needed in future):

```python
# Run only stress tests
suite = TextOverlayComprehensiveTests()
test_methods = [
    suite.test_all_named_colors_stress,
    suite.test_all_hex_colors_stress,
    # ... etc
]
for test in test_methods:
    test()
suite.print_summary()

# Run only QSO tests
suite = TextOverlayComprehensiveTests()
test_methods = [
    suite.test_qso_basic_exchange,
    suite.test_qso_with_rst_report,
    # ... etc
]
```

## Expected Results

When all tests pass:

```
Total tests:    45
✓ PASSED:       45 (100.0%)
✗ FAILED:       0
Test directory: test_outputs/text_overlay_comprehensive/
```

Each test should:
1. Create a subdirectory with timestamp
2. Generate `output.wav` file (audio encoding)
3. Generate debug PNG image (visual verification)
4. Report PASSED status in console and JSON results

## Development Notes

### Adding New Tests

To add a new stress test or unit test:

1. Create method in `TextOverlayComprehensiveTests` class
2. Follow naming convention: `test_<category>_<scenario>`
3. Build arguments list with `-T` options
4. Call `self._run_command(args)` to execute
5. Verify debug image exists with `debug_img.exists()`
6. Log result with `self._log_test(name, status, message)`
7. Add method to appropriate list in `run_all_tests()`

Example:

```python
def test_new_feature(self):
    """Test new overlay feature"""
    print("\n" + "="*90)
    print("TEST: New Feature Name")
    print("="*90)
    
    test_name = f"new_feature_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
    test_dir = self.test_dir / test_name
    test_dir.mkdir(exist_ok=True)
    
    args = [
        '-i', self.test_image,
        '-o', str(test_dir / 'output.wav'),
        '-K',
        '-T', 'TEXT|size=18|color=red|pos=top'
    ]
    
    ret, stdout, stderr = self._run_command(args)
    
    debug_img = test_dir / f"{test_name}.png"
    if debug_img.exists():
        self._log_test("New Feature Test", "PASSED", "Description")
    else:
        self._log_test("New Feature Test", "FAILED", "Debug image not created")
```

### Debugging Failed Tests

If a test fails:

1. Check the debug image exists in `test_outputs/text_overlay_comprehensive/`
2. View PNG with image viewer to verify visual output
3. Check `comprehensive_results_*.json` for error details
4. Run command manually to see full stderr output:
   ```bash
   ./pisstvpp2 -i images/test_color_bars.png -T "Text|..." -o /tmp/test.wav -K -v
   ```
5. Compare with passing test to identify differences

## Future Enhancements

Possible additions to test suite:

1. **Performance Testing**: Large number of text objects (20+)
2. **Edge Cases**: Very long text, special characters, Unicode
3. **Integration Tests**: Combined with other CLI options (CW fade, aspect ratio, etc.)
4. **Benchmark Tests**: Measure rendering time with profiling
5. **Cross-Platform Tests**: Verify consistent rendering on Linux/Windows
6. **Memory Tests**: Verify no memory leaks with many overlays
7. **Rotation Tests**: Test text rotation when implemented

## References

- [Overlay Specification Header](../src/include/overlay_spec.h)
- [Image Rendering Implementation](../src/pisstvpp2_image.c)
- [Parser Implementation](../src/overlay_spec.c)
- [Main Test Suite](test_suite.py)
