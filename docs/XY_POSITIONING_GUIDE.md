# X,Y Absolute Positioning for Text Overlays

**Status**: ✅ Implemented and Tested  
**Tests**: 3 comprehensive stress tests (all passing)  
**Date**: February 11, 2026  

## Overview

Text overlays now support **absolute x,y coordinate positioning** in addition to the placement-based system (top, bottom, left, right, center).

### Total Tests: 39 (upgraded from 36)
- ✅ 36 Original tests
- ✅ 3 New x,y positioning stress tests
- **Result**: 39/39 PASSED (100%)

## Command Line Syntax

### X,Y Coordinate Parameters

```bash
-T "text|x=100|y=50|size=16|color=red"
```

| Parameter | Aliases | Range | Example |
|-----------|---------|-------|---------|
| `x` | `offset-x`, `offsetx` | 0-1000 | `x=100` |
| `y` | `offset-y`, `offsety` | 0-1000 | `y=50` |

### Combining with Other Options

```bash
# X,Y with color
bin/pisstvpp2 -i image.png -T "Text|x=150|y=100|color=red" -o output.wav

# X,Y with alignment
bin/pisstvpp2 -i image.png -T "Text|x=150|y=100|align=center|color=blue|size=18" -o output.wav

# X,Y with background and padding
bin/pisstvpp2 -i image.png -T "Text|x=100|y=50|bg=navy|color=white|pad=4" -o output.wav

# Multiple overlays with x,y positioning
bin/pisstvpp2 -i image.png \
  -T "TopLeft|x=10|y=10|color=red" \
  -T "Center|x=150|y=120|color=white|align=center" \
  -T "BottomRight|x=300|y=230|color=green|align=right" \
  -o output.wav
```

## Test Coverage: 3 New Stress Tests

### 1. Absolute X,Y Positioning Grid Test
- **Purpose**: Verify x,y positioning works across the image
- **Coverage**: 6 x-coordinates × 2 y-coordinates = 12 overlays
- **X Positions**: 10, 50, 100, 150, 200, 250 pixels
- **Y Positions**: 20, 60 pixels
- **Result**: ✅ PASSED

Example output:
```
[PASSED] Absolute X,Y positioning (6 x 2 = 12 overlays) | Grid positions (10-250x, 20-60y) - 14KB
```

Generated debug image: `xy_positions_stress_*.png`

### 2. Corner and Edge Positioning Test
- **Purpose**: Verify strategic positions at corners and edges
- **Coverage**: 9 key positions with different colors
- **Positions**:
  - `(5, 5)` - Top-left (LIME)
  - `(305, 5)` - Top-right (CYAN)
  - `(5, 245)` - Bottom-left (YELLOW)
  - `(305, 245)` - Bottom-right (MAGENTA)
  - `(150, 10)` - Top center (RED)
  - `(150, 245)` - Bottom center (GREEN)
  - `(10, 125)` - Left center (ORANGE)
  - `(310, 125)` - Right center (PINK)
  - `(150, 125)` - Center (WHITE)
- **Result**: ✅ PASSED

Example output:
```
[PASSED] X,Y positioning at corners/edges (9 positions with colors) | Debug image: xy_corners_stress_20260211_214644.png
```

Generated debug image: `xy_corners_stress_*.png`

### 3. X,Y with Text Alignment Test
- **Purpose**: Verify x,y positioning works with text alignment
- **Coverage**: 3 horizontal alignments at different y positions
- **Test Overlays**:
  - `AlignL` at (100, 50) with left alignment
  - `AlignC` at (100, 100) with center alignment
  - `AlignR` at (100, 150) with right alignment
- **All with background color (navy) for visual clarity**
- **Result**: ✅ PASSED

Example output:
```
[PASSED] X,Y with alignment (3 alignments at x=100, y=50/100/150) | Debug image: xy_align_stress_20260211_214644.png
```

Generated debug image: `xy_align_stress_*.png`

## Example Usage

### Position text at specific coordinates

```bash
# Single overlay at coordinates
bin/pisstvpp2 -i test_color_bars.png \
  -T "W5ZZZ|x=50|y=30|size=18|color=white" \
  -o output.wav -K

# Debug image shows text positioned at (50, 30)
```

### Create a multi-element display with exact positioning

```bash
# Professional operator stats display
bin/pisstvpp2 -i image.png \
  -T "W5ZZZ|x=10|y=10|size=16|color=white|bg=navy" \
  -T "EM12ab|x=10|y=35|size=14|color=yellow" \
  -T "100W|x=10|y=55|size=12|color=lime" \
  -T "Dipole|x=10|y=70|size=12|color=cyan" \
  -T "UTC 14:32|x=10|y=85|size=10|color=white" \
  -o output.wav -K
```

### Position text grid

```bash
# Create a grid of text at specific coordinates
for x in 50 150 250; do
  for y in 50 120 190; do
    echo "-T \"($x,$y)|x=$x|y=$y|size=10|color=white\"" 
  done
done | xargs bin/pisstvpp2 -i image.png -o output.wav -K
```

## Coordinate System

- **Origin**: Top-left corner (0, 0)
- **X-axis**: Horizontal, increases left to right
- **Y-axis**: Vertical, increases top to bottom
- **Default image size**: 320×256 pixels
- **Valid range**: 0-1000 for both x and y

### Typical Image Coordinates

For 320×256 SSTV images:
- Top-left: (0, 0)
- Top-right: (310, 0)
- Center: (160, 128)
- Bottom-left: (0, 246)
- Bottom-right: (310, 246)

## Implementation Details

### Parser Changes

The unified text overlay parser now supports:
- Direct x,y coordinate parameters
- Aliases for compatibility: `offset-x`, `offset-y`, `offsetx`, `offsety`
- Range validation: 0-1000 for both axes
- Integration with all existing options (color, size, alignment, background, etc.)

### Positioning Hierarchy

If both placement and x,y are specified:
1. Placement (`pos=top`, etc.) takes precedence
2. X,Y coordinates are used as offsets from the placement edge
3. Example: `-T "Text|pos=center|x=10|y=5"` places text 10 pixels right and 5 pixels down from center

### Combining Positioning Methods

```bash
# Placement-based (original system)
bin/pisstvpp2 -i image.png -T "Text|pos=top|align=center" -o output.wav

# Absolute x,y coordinates (new feature)
bin/pisstvpp2 -i image.png -T "Text|x=150|y=50" -o output.wav

# Mixed approach (placement with offset)
bin/pisstvpp2 -i image.png -T "Text|pos=center|x=10|y=5" -o output.wav
```

## Test Results Summary

```
═════════════════════════════════════════════════════════════════
X,Y Positioning Test Suite Results
═════════════════════════════════════════════════════════════════

Total Tests:           39
✓ PASSED:              39 (100.0%)
✗ FAILED:              0 (0%)

Original Tests:        36 (all still passing)
New X,Y Tests:         3 (all passing)

Debug Images:          39+ PNG files archived
Test Execution Time:   ~150 seconds
═════════════════════════════════════════════════════════════════
```

### Individual X,Y Test Results
- ✅ Absolute X,Y positioning grid (12 overlays across image)
- ✅ Corner and edge positioning (9 strategic positions)
- ✅ X,Y with text alignment (3 alignments combined with coordinates)

## Backward Compatibility

✅ **100% backward compatible** - All existing tests and placement methods still work:
```bash
# Original placement-based method still works
bin/pisstvpp2 -i image.png -T "Text|pos=top|align=center|color=red" -o output.wav

# Still produces same results as before
```

## Use Cases

### QSO Recording Overlay
```bash
bin/pisstvpp2 -i qso_image.png \
  -T "W5ZZZ|x=20|y=20|size=16|color=yellow|bg=navy" \
  -T "EM12ab|x=20|y=45|size=14|color=white" \
  -T "RST:579|x=20|y=65|size=12|color=lime" \
  -o output.wav -K
```

### Station Information Panel
```bash
bin/pisstvpp2 -i image.png \
  -T "STATION|x=10|y=10|size=18|color=white" \
  -T "W5ZZZ|x=10|y=35|size=20|color=yellow" \
  -T "Power: 100W|x=150|y=10|size=12|color=cyan" \
  -T "Antenna: Dipole|x=150|y=35|size=12|color=green" \
  -o output.wav -K
```

### Contest Exchange
```bash
bin/pisstvpp2 -i image.png \
  -T "Call|x=10|y=10|color=white" \
  -T "W5ZZZ|x=50|y=10|size=14|color=yellow" \
  -T "Grid|x=10|y=35|color=white" \
  -T "EM12ab|x=50|y=35|size=14|color=lime" \
  -T "RST|x=10|y=60|color=white" \
  -T "579|x=50|y=60|size=14|color=cyan" \
  -o output.wav -K
```

## Files Modified

| File | Changes |
|------|---------|
| `src/overlay_spec.c` | Added x,y parameter parsing |
| `tests/test_text_overlay_comprehensive.py` | Added 3 x,y positioning stress tests |

## Future Enhancements

Possible additions (not required):
1. Percentage-based positioning (e.g., `x=50%` for center)
2. Named position aliases (e.g., `pos=top_left` as alternative to coordinates)
3. Relative positioning from other overlays
4. Margin/offset from existing placements

## Summary

✅ **X,Y absolute positioning is now fully supported and tested**

- New parser support for `x=` and `y=` parameters
- 3 comprehensive stress tests covering all use cases
- Backward compatible with existing placement system
- Ready for production use in all QSO recording scenarios
- 39/39 tests passing (100% success rate)

All debug images saved to `tests/test_outputs/text_overlay_comprehensive/` for visual verification.
