# Vertical Bar Orientation Feature - Complete Implementation

## Overview
Successfully implemented two types of vertical bar orientations for SSTV text overlays:
1. **Vertical (Rotated)**: Text rotated 90° - entire text block is tilted
2. **Vertical-Stacked**: Text stacked top-to-bottom with normal letter orientation - each letter remains upright but arranged in a column

Both modes enable callsign and RST displays along image edges.

## Features Implemented

### 1. Vertical Bar Orientation Modes
- **`BGBAR_ORIENT_HORIZONTAL`** (0): Horizontal bar (default) - text reads normally left-to-right
- **`BGBAR_ORIENT_VERTICAL`** (1): Vertical rotated bar - entire text rotated 90° clockwise
- **`BGBAR_ORIENT_VERTICAL_STACKED`** (2): Vertical stacked bar - letters stacked top-to-bottom, each upright
- Default is horizontal to maintain backward compatibility

### 2. Text Rendering Modes
#### Rotated (`bgbar-orient=vertical`)
- Automatically rotates text 90° clockwise using `vips_rot90()`
- Entire text block is tilted
- Useful when you want compact display that reads correctly when image is rotated

#### Stacked (`bgbar-orient=vertical-stacked` or `bgbar-orient=vs`)
- Places each character on a new line
- All letters maintain normal upright orientation
- Useful when text needs to be readable without rotating the image
- Characters are centered horizontally within the bar

### 3. Vertical Bar Positioning
- **Left placement**: Bar spans full image height at x=0
- **Right placement**: Bar spans full image height at x=(image_width - bar_width)
- Bar width controlled by `bgbar-width` parameter (default: 50 pixels)
- Works with custom widths: `bgbar-width=60` for 60-pixel bars

### 4. Mixed Mode Support
- Horizontal, rotated vertical, and stacked vertical bars can all be used in the same image
- Each overlay specifies its own orientation independently
- Supports unlimited overlays with different configurations

## Usage Examples

### Vertical-Stacked (Normal Letters, Column Layout)
```bash
# Callsign stacked vertically on left (letters upright)
./bin/pisstvpp2 -i image.png -T "CALL|bgbar=1|bgbar-orient=vertical-stacked|pos=left"

# RST stacked on right
./bin/pisstvpp2 -i image.png -T "559|bgbar=1|bgbar-orient=vs|bgbar-width=50|pos=right"
```

### Rotated Vertical (Tilted 90°)
```bash
# Callsign rotated vertically on left
./bin/pisstvpp2 -i image.png -T "CALL|bgbar=1|bgbar-orient=vertical|pos=left"

# RST rotated on right with custom width
./bin/pisstvpp2 -i image.png -T "N0XYZ|bgbar=1|bgbar-orient=vertical|bgbar-width=70|pos=right"
```

### Mixed Mode (Stacked + Horizontal + Rotated)
```bash
./bin/pisstvpp2 -i image.png \
  -T "CALL|bgbar=1|bgbar-orient=vertical-stacked|bgbar-width=60|pos=left" \
  -T "RST|bgbar=1|bgbar-width=100|pos=top" \
  -T "FN|bgbar=1|bgbar-orient=vertical|bgbar-width=50|pos=right"
```

## Parameter Reference

| Parameter | Values | Default | Notes |
|-----------|--------|---------|-------|
| `bgbar-orient` | "vertical", "v", "vertical-stacked", "vs", "stacked", or "horizontal" | "horizontal" | Controls bar/text orientation |
| `bgbar-width` | "full", "half", "auto", or 1-2000 (pixels) | "auto" | Bar width for vertical bars (custom pixel width) |
| `pos` | "left", "right", "top", "bottom", "center" | "left" | Text/bar placement |

### Orientation Comparison

| Mode | Parameter Value | Text Display | Bar Direction | Use Case |
|------|---|---|---|---|
| Horizontal | "horizontal" (default) | Normal, left-to-right | Horizontal across width | Standard text labels |
| Vertical Rotated | "vertical" or "v" | Tilted 90° clockwise | Vertical along edges | Compact display for rotated viewing |
| Vertical Stacked | "vertical-stacked", "vs", or "stacked" | Column layout, upright letters | Vertical along edges | Normal readability, professional look |

## Test Results

✅ **Single Overlay Tests**
- Horizontal bars with various widths (full, half, fixed)
- Vertical rotated bar: Text rotated 90°, bar at (0, 0), size 50x256
- Vertical stacked bar: Letters upright in column, bar positioned on edge, custom widths work

✅ **Multi-Overlay Tests**
- Side-by-side bars (horizontal + vertical stacked)
- All three modes together (stacked left + horizontal top + rotated right)
- Mixed orientations with different colors and positions
- All overlays properly composited with correct color and alpha blending

✅ **Backward Compatibility**
- Horizontal bars continue to work with all existing modes
- Full-width, half-width, fixed-width modes all functional
- Existing overlay specs work unchanged

## Implementation Details

### Files Modified

1. **src/include/overlay_spec.h**
   - Added `BGBAR_ORIENT_HORIZONTAL` (0), `BGBAR_ORIENT_VERTICAL` (1), `BGBAR_ORIENT_VERTICAL_STACKED` (2) constants
   - Added `bg_bar_orientation` field to TextOverlaySpec structure

2. **src/overlay_spec.c**
   - Initialize `bg_bar_orientation = BGBAR_ORIENT_HORIZONTAL` in defaults
   - Parse `bgbar-orient` parameter supporting: "vertical", "v", "vertical-stacked", "vs", "stacked", or "horizontal"

3. **src/pisstvpp2_image.c**
   - **Vertical stacked mode**: Uses Pango's built-in line-break rendering
     - Creates text with newlines between each character
     - Pango renders each character on separate line
     - All letters remain upright and centered
   - **Vertical rotated mode**: Uses `vips_rot90(text_image, &rotated_text, NULL)` 
     - Rotates entire text block 90° clockwise
   - **Bar dimension calculation**: Conditional logic for all three orientation modes
     - Both vertical modes span full image height
     - Bar width controlled by `bg_bar_custom_width` or defaults to 50px
     - Horizontal mode preserves original width mode logic

### Key Design Decisions

1. **Stacking Implementation**: Uses Pango's native line-breaking
   - Simple: Just add `\n` between characters before rendering
   - Efficient: No extra image manipulation needed
   - Robust: Pango handles character spacing and centering automatically

2. **Text Positioning**: Different for each mode
   - Rotated vertical: Position calculated based on rotated dimensions
   - Stacked vertical: Position based on natural character column width
   - Horizontal: Original positioning logic unchanged

3. **Bar Spanning**: Both vertical modes span full image height
   - Better consistency for SSTV transmission
   - Similar vertical bar appearance regardless of text orientation

4. **Backward Compatibility**: Horizontal remains default
   - Existing overlay specs work without modification
   - All horizontal bar modes continue to function identically
   - New modes are opt-in via `bgbar-orient` parameter

## Performance Impact

- **Minimal**: Stacking uses Pango's native rendering (no extra operations)
- **Rotated**: Single `vips_rot90()` call per overlay
- **Memory**: Text image references released appropriately
- **Compositing**: Uses efficient blend modes across all modes

## Known Limitations

- Stacked text characters cannot be individually positioned (except horizontal centering within bar)
- Rotated text is always 90° clockwise (cannot be reversed/counterclockwise)
- Character spacing in stacked mode follows Pango's line-height rules
- Vertical placement (top/bottom) defaults to left bar for non-left/right positions

## Future Enhancement Ideas

- Custom character spacing in stacked mode
- Support for 270° (counterclockwise) rotation option
- Different edge positioning modes (left-outer, right-outer, etc.)
- Text alignment options within vertical bars (top, center, bottom)
- Rotation angles beyond 90° multiples (45°, etc.) using `vips_affine()`

## Testing Checklist

- [x] Horizontal bars (all widths: auto, full, half, fixed)
- [x] Vertical rotated bar (left position)
- [x] Vertical rotated bar (right position)
- [x] Vertical rotated bar with custom width
- [x] Vertical stacked bar (left position)
- [x] Vertical stacked bar (right position)
- [x] Vertical stacked bar with custom width
- [x] Multiple stacked overlays
- [x] Mixed mode (horizontal + rotated + stacked in one image)
- [x] Text colors and styling with all bar modes
- [x] Multi-band image support
- [x] Backward compatibility with existing specs

## Compilation & Build

```bash
make clean && make
```

All warnings are pre-existing (unrelated to vertical bar feature).

## Example SSTV QSO Usage

### Vertical Rotated (Tilted Letters)
```bash
# Vertical rotated callsign on left, vertical rotated RST on right
./bin/pisstvpp2 -i qso_image.png \
  -T "W5XYZ|size=24|color=white|bg=navy|bgbar=1|bgbar-orient=vertical|bgbar-width=60|pos=left" \
  -T "559|size=20|color=yellow|bg=darkred|bgbar=1|bgbar-orient=vertical|bgbar-width=50|pos=right" \
  -o output.wav
```

**Result**: Clean, professional edge-mounted callsigns and RST reports with tilted text

### Vertical Stacked (Upright Letters in Column)
```bash
# Vertical stacked callsign on left, vertical stacked grid on right
./bin/pisstvpp2 -i qso_image.png \
  -T "CALL|size=22|color=white|bg=navy|bgbar=1|bgbar-orient=vertical-stacked|bgbar-width=60|pos=left" \
  -T "EM29|size=20|color=yellow|bg=darkred|bgbar=1|bgbar-orient=vs|bgbar-width=50|pos=right" \
  -o output.wav
```

**Result**: Professional edge labels with normal letter orientation (no head rotation needed)

### Mixed Mode (All Three Orientations)
```bash
# Stacked callsign left, horizontal RST center, rotated grid right
./bin/pisstvpp2 -i qso_image.png \
  -T "CALL|size=20|color=white|bg=darkblue|bgbar=1|bgbar-orient=vertical-stacked|bgbar-width=60|pos=left" \
  -T "RST 559|size=18|color=yellow|bg=darkred|bgbar=1|bgbar-width=full|pos=top" \
  -T "EM|size=18|color=lime|bg=navy|bgbar=1|bgbar-orient=vertical|bgbar-width=50|pos=right" \
  -o output.wav
```

**Result**: Comprehensive QSO information clearly displayed with mixed orientations for optimal readability
