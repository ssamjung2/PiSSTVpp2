# Timestamp Feature for Text Overlays

## Overview

The **Timestamp** feature automatically generates and displays the current system time and date on text overlays using standard `strftime()` format strings.

When enabled, the overlay displays the current time in a user-specified format instead of static text, making it perfect for documenting when an SSTV image was created, captured, or transmitted.

## Feature Highlights

- **Automatic Time Generation**: Uses current system time (no manual updates needed)
- **Flexible Formatting**: Supports full `strftime()` format string capabilities
- **Common Formats Built-in**: ISO 8601, 12/24-hour time, long/short date styles
- **Full Integration**: Works with all existing overlay features (colors, sizes, positions, background bars)
- **Zero Performance Impact**: Time formatting happens during image processing, not during transmission

## Usage

### Basic Syntax

```bash
bin/pisstvpp2 -i image.png -T "|timestamp-format=%Y-%m-%d %H:%M:%S" -o output.wav -K
```

To use timestamp AND display text, timestamp takes precedence when the format is specified:

```bash
# Timestamp format specified - displays time, ignores "TEXT"
-T "TEXT|timestamp-format=%Y-%m-%d"

# No timestamp format - displays "TEXT"
-T "TEXT"
```

### Command-Line Parameters

| Parameter | Aliases | Values | Default | Description |
|-----------|---------|--------|---------|-------------|
| `timestamp-format` | `timestamp_format`, `timestampformat`, `ts` | strftime format string | (empty = disabled) | Enable timestamp generation with specified format |

### Common Format Strings

| Format String | Example Output | Use Case |
|---------------|-----------------|----------|
| `%Y-%m-%d` | 2026-02-13 | ISO date |
| `%H:%M:%S` | 01:55:21 | ISO time (24-hour) |
| `%Y-%m-%dT%H:%M:%S` | 2026-02-13T01:55:21 | ISO 8601 full timestamp |
| `%d/%m/%Y` | 13/02/2026 | European date format |
| `%m/%d/%Y` | 02/13/2026 | US date format |
| `%d-%b-%Y` | 13-Feb-2026 | Abbreviated date |
| `%A, %B %d, %Y` | Friday, February 13, 2026 | Long date format |
| `%H:%M` | 01:55 | Time without seconds (24-hour) |
| `%I:%M %p` | 01:55 AM | 12-hour time with AM/PM |
| `%Y%m%d_%H%M%S` | 20260213_015521 | Compact ISO format (good for filenames) |

### Complete Parameter List

Full control over timestamp overlay appearance using standard overlay parameters:

```bash
# Timestamp generation
timestamp-format=FORMAT   # strftime format (empty = disabled)

# Position and offset
pos=top|center|bottom|left|right|top-left|top-right|bottom-left|bottom-right
x=0-1000          # Absolute X position (pixels)
y=0-1000          # Absolute Y position (pixels)

# Text styling
size=8-96         # Font size in pixels
color=COLOR       # Text color (named or hex)
align=left|center|right  # Horizontal alignment

# Background styling
bg=COLOR          # Background fill color
mode=transparent|opaque|semi  # Background rendering mode

# Background bar styling
bgbar=true        # Enable background bar behind timestamp
bgbar-color=COLOR # Bar color
bgbar-margin=0-30 # Extra margin around bar
bgbar-width=auto|full|half|N  # Bar width mode
bgbar-orient=horizontal|vertical|vertical-stacked  # Bar orientation

# Padding and borders
pad=0-50          # Padding around text within bar
border=0-10       # Border width in pixels
```

---

## Examples

### Example 1: Simple Date Overlay

```bash
bin/pisstvpp2 -i image.png \
  -T "|size=18|color=white|mode=transparent|timestamp-format=%Y-%m-%d|pos=top" \
  -o output.wav -K
```

**Result**: Current date displayed at top of image using white text with transparent background.
**Output**: 2026-02-13

### Example 2: ISO 8601 Timestamp with Background Bar

```bash
bin/pisstvpp2 -i image.png \
  -T "|size=16|color=black|bgbar=1|bgbar-color=yellow|timestamp-format=%Y-%m-%dT%H:%M:%S|pos=top" \
  -o output.wav -K
```

**Result**: Full ISO 8601 timestamp on yellow background bar.
**Output**: 2026-02-13T01:55:21

### Example 3: Multiple Time Formats on Single Image

```bash
bin/pisstvpp2 -i image.png \
  -T "|size=18|color=white|mode=transparent|timestamp-format=%d-%b-%Y|pos=top" \
  -T "|size=16|color=yellow|mode=transparent|timestamp-format=%H:%M:%S|pos=bottom" \
  -T "|size=14|color=cyan|mode=transparent|timestamp-format=%A, %B %d|pos=left" \
  -o output.wav -K
```

**Result**: Three timestamps with different formats and colors displayed simultaneously.
**Output**: 
- Top: 13-Feb-2026
- Bottom: 01:55:21  
- Left: Friday, February 13

### Example 4: Vertical Timestamp with Background Bar

```bash
bin/pisstvpp2 -i image.png \
  -T "|size=20|color=white|bgbar=1|bgbar-orient=vertical-stacked|bgbar-color=navy|timestamp-format=%Y-%m-%d|pos=left" \
  -o output.wav -K
```

**Result**: Date displayed vertically (one letter per line) on navy vertical background bar.
**Output**: 
```
2
0
2
6
-
0
2
-
1
3
```

### Example 5: HF Radio Contact with Timestamp

```bash
bin/pisstvpp2 -i image.png \
  -T "W5ABC|size=16|color=white|bg=black|bgbar=1|pos=top" \
  -T "|size=14|color=yellow|mode=transparent|timestamp-format=%H:%M UTC|bgbar=1|bgbar-color=darkblue|pos=bottom" \
  -o output.wav -K
```

**Result**: Call sign at top and current time in UTC at bottom, both with background bars.
**Output**: 
- Top: W5ABC (with black background bar)
- Bottom: 01:55 UTC (with dark blue background bar)

---

## Implementation Details

### How It Works

1. **Parameter Parsing**: When `-T` overlay specification includes `timestamp-format=...`, the format string is stored in `TextOverlaySpec.timestamp_format`

2. **Timestamp Generation**: During image processing in `apply_single_overlay()`:
   - If `timestamp_format` is non-empty, current system time is obtained via `time()` and `localtime()`
   - Format is applied using standard `strftime()` function
   - Generated timestamp string is used instead of `spec->text`

3. **Standard Processing**: Once timestamp is generated, it's treated as normal text:
   - Rendered with specified color, size, and font
   - Positioned according to `placement` specification
   - Background bar applied if `bgbar` is enabled
   - Composited onto image with alpha blending

### Format String Reference

Full `strftime()` format documentation available at: https://man7.org/linux/man-pages/man3/strftime.3.html

## Combining with Other Features

### Timestamp + Background Bar + Vertical Layout

```bash
-T "|timestamp-format=%Y|bgbar=1|bgbar-orient=vertical-stacked|bgbar-color=red|pos=left"
```

Creates a 4-character vertical timestamp "2026" on a red background bar.

### Timestamp + Static Text (Timestamp Wins)

```bash
-T "CALL|timestamp-format=%H:%M"  # Displays "01:55", ignores "CALL"
-T "CALL"                          # Displays "CALL" (no timestamp)
```

### Timestamp with Custom Position

```bash
-T "|timestamp-format=%Y-%m-%d|x=100|y=150|size=20|color=white"
```

Displays timestamp at pixel position (100, 150) with white 20-point font.

---

## Technical Notes

### Timezone Support

- Uses local system timezone
- `%z` parameter shows UTC offset (e.g., -0600)
- `%Z` shows timezone name but may vary by system
- For UTC times, wrap in a shell script that calls with `TZ=UTC`

### Performance

- Negligible impact - timestamp formatting happens once during image loading
- No noticeable delay in rendering pipeline
- Time is "frozen" at image processing time (not updated per frame)

### Precision

- **Resolution**: Seconds (full second precision, not milliseconds)
- **Accuracy**: Depends on system clock accuracy
- **Consistency**: All overlays processed with same time (not updated between overlays)

### Character Limit

- Format string limited to 128 characters
- Output message limited to 512 characters (sufficient for any standard format)
- Very long custom formats may be truncated

---

## Troubleshooting

### Timestamp Not Appearing

**Problem**: Timestamp format is specified but nothing appears

**Solutions**:
1. Verify `timestamp-format=...` parameter is present
2. Check format string is valid `strftime()` syntax
3. Ensure background color is visible (use `bgbar-color=` for compatibility)
4. Use `-v` flag to see generated timestamp in debug output

### Garbage Characters Displayed

**Problem**: Strange characters instead of timestamp

**Solutions**:
1. Format string may be malformed - check syntax
2. Some characters may need shell escaping in terminal (use single quotes: `'%Y-%m-%d'`)
3. Verify system locale supports all format codes

### Wrong Time Displayed

**Problem**: Timestamp shows incorrect time

**Solutions**:
1. Set system clock via `date` command: `date 020313552026` (MMDDHHmm[[CC]YY])
2. Check system timezone: `date +%Z`
3. For UTC: Run with `TZ=UTC bin/pisstvpp2 ...` in shell

---

## See Also

- [BACKGROUND_BAR_FEATURE.md](BACKGROUND_BAR_FEATURE.md) - Background bars for visibility
- [TEXT_OVERLAY_DEVELOPMENT_TASK_LIST_v2_1.md](TEXT_OVERLAY_DEVELOPMENT_TASK_LIST_v2_1.md) - Full overlay system
