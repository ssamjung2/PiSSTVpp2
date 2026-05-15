# SlowFrame Text Overlay Reference

**Complete reference for the `-T` text overlay system**

SlowFrame v2.1 supports compositing text overlays onto SSTV images before encoding.
Multiple overlays are supported by repeating the `-T` flag.

> **v2.1 limitation:** Text color is auto-selected (white on dark backgrounds, black on
> light backgrounds).  Custom text colors via FreeType are planned for v2.2.  The `color=`
> parameter sets the background bar color, not the text glyph color.

---

## Table of Contents

1. [Basic Syntax](#basic-syntax)
2. [All Parameters](#all-parameters)
3. [Positioning](#positioning)
4. [Background Bars](#background-bars)
5. [Vertical Bars](#vertical-bars)
6. [Timestamps](#timestamps)
7. [Multiple Overlays](#multiple-overlays)
8. [Color Reference](#color-reference)
9. [Examples](#examples)

---

## Basic Syntax

```bash
./bin/slowframe -i image.png -T "TEXT|param=value|param=value" -o output.wav
```

Parameters are separated by `|`.  The first field is the text content (can be empty for
timestamp-only overlays).

---

## All Parameters

### Text & Content

| Parameter | Values | Default | Description |
|-----------|--------|---------|-------------|
| *(first field)* | any text | — | Text to display |
| `timestamp-format` | strftime string | (disabled) | Auto-generate timestamp instead of static text |
| `size` | 8–96 | 16 | Font size in pixels |
| `align` | `left`, `center`, `right` | `left` | Horizontal text alignment |

### Position

| Parameter | Aliases | Values | Default | Description |
|-----------|---------|--------|---------|-------------|
| `pos` | — | `top`, `bottom`, `left`, `right`, `center`, `top-left`, `top-right`, `bottom-left`, `bottom-right` | `top-left` | Placement keyword |
| `x` | `offset-x`, `offsetx` | 0–1000 | — | Absolute X position (pixels from left) |
| `y` | `offset-y`, `offsety` | 0–1000 | — | Absolute Y position (pixels from top) |

When both `pos` and `x`/`y` are specified, `pos` sets the anchor and `x`/`y` are
applied as offsets from that anchor.

### Background

| Parameter | Values | Default | Description |
|-----------|--------|---------|-------------|
| `bg` | color name or `#RRGGBB` | `black` | Background fill color |
| `mode` | `opaque`, `semi`, `transparent` | `opaque` | Background opacity |
| `pad` | 0–50 | 8 | Padding between text and background edge (pixels) |
| `border` | 0–10 | 0 | Border width (pixels) |

### Background Bar

| Parameter | Aliases | Values | Default | Description |
|-----------|---------|--------|---------|-------------|
| `bgbar` | `backgroundbar`, `bg-bar` | `true`/`false`, `1`/`0`, `yes`/`enable` | `false` | Enable full-width background bar behind text |
| `bgbar-margin` | `bgbar_margin`, `bgbarmargin` | 0–30 | 4 | Extra margin around bar beyond padding (pixels) |
| `bgbar-orient` | — | `horizontal`, `vertical`, `v`, `vertical-stacked`, `vs`, `stacked` | `horizontal` | Bar/text orientation |
| `bgbar-width` | — | `auto`, `full`, `half`, 1–2000 | `auto` | Bar width for vertical orientation |

---

## Positioning

### Position Keywords

```bash
# Placement keywords
-T "W5ABC|pos=top"           # Centered along top edge
-T "W5ABC|pos=bottom"        # Centered along bottom edge
-T "W5ABC|pos=left"          # Left edge, vertically centered
-T "W5ABC|pos=right"         # Right edge, vertically centered
-T "W5ABC|pos=center"        # Image center
-T "W5ABC|pos=top-left"      # Top-left corner (default)
-T "W5ABC|pos=top-right"     # Top-right corner
-T "W5ABC|pos=bottom-left"   # Bottom-left corner
-T "W5ABC|pos=bottom-right"  # Bottom-right corner
```

### Absolute X,Y Coordinates

```bash
# Absolute pixel coordinates (origin = top-left)
-T "W5ABC|x=50|y=30"

# With additional options
-T "W5ABC|x=150|y=100|color=red|size=18"
-T "W5ABC|x=100|y=50|bg=navy|pad=4"
```

**Coordinate system:**
- Origin `(0, 0)` is the top-left corner
- X increases left → right; Y increases top → bottom
- Standard 320×256 SSTV image corners: top-left `(0,0)`, center `(160,128)`, bottom-right `(310,246)`

### Combining Placement and Offset

```bash
# Offset 10px right and 5px down from center
-T "W5ABC|pos=center|x=10|y=5"
```

---

## Background Bars

A background bar is a solid-color rectangle spanning the full width (for horizontal
orientation) or full height (for vertical orientation) behind the text.  Essential for
readability on weak or degraded HF signals.

```bash
# Basic background bar
-T "W5ABC|bgbar=true"

# Customized bar
-T "W5ABC|bg=navy|bgbar=true|bgbar-margin=4|size=16|pos=top"
```

### Margin vs Padding

- **`pad=N`** — space between text and bar edge (default 8px)
- **`bgbar-margin=N`** — additional space outside the padded area (default 4px)
- Total bar size = text + (pad × 2) + (bgbar-margin × 2)

### Opacity

Use `mode=` to control bar opacity:
```bash
-T "W5ABC|bgbar=true|mode=opaque"       # Fully opaque (default, best for weak signals)
-T "W5ABC|bgbar=true|mode=semi"         # 50% transparent
-T "W5ABC|bgbar=true|mode=transparent"  # No background rendered
```

---

## Vertical Bars

Place bars along the left or right edge of the image using `bgbar-orient`:

### Vertical Rotated (text tilted 90°)

```bash
-T "W5ABC|bgbar=1|bgbar-orient=vertical|pos=left"
-T "559|bgbar=1|bgbar-orient=v|bgbar-width=60|pos=right"
```

Text is rotated 90° clockwise.  Readable when the image is rotated.

### Vertical Stacked (letters upright, column layout)

```bash
-T "W5ABC|bgbar=1|bgbar-orient=vertical-stacked|pos=left"
-T "559|bgbar=1|bgbar-orient=vs|bgbar-width=50|pos=right"
```

Each character is placed on its own line; letters remain upright.

### Bar Width

```bash
bgbar-width=auto     # Automatic based on text (default)
bgbar-width=full     # Full image width/height
bgbar-width=half     # Half image width/height
bgbar-width=60       # Fixed 60 pixels
```

**Note:** Both vertical modes span the full image height.  Custom width via
`bgbar-width` controls the bar's pixel thickness.

---

## Timestamps

Use `timestamp-format=` to display the current system time instead of static text.
Format strings follow standard `strftime()` syntax.

```bash
# Current date
-T "|timestamp-format=%Y-%m-%d|pos=bottom"

# Full ISO 8601 timestamp with background bar
-T "|timestamp-format=%Y-%m-%dT%H:%M:%S|bgbar=1|bg=navy|pos=top"

# 12-hour time
-T "|timestamp-format=%I:%M %p|size=14|pos=bottom-right"
```

### Common Format Strings

| Format | Example Output | Use Case |
|--------|----------------|----------|
| `%Y-%m-%d` | 2026-03-09 | ISO date |
| `%H:%M:%S` | 14:32:05 | 24-hour time |
| `%Y-%m-%dT%H:%M:%S` | 2026-03-09T14:32:05 | ISO 8601 full |
| `%d/%m/%Y` | 09/03/2026 | European date |
| `%m/%d/%Y` | 03/09/2026 | US date |
| `%d-%b-%Y` | 09-Mar-2026 | Abbreviated |
| `%A, %B %d, %Y` | Monday, March 9, 2026 | Long form |
| `%H:%M` | 14:32 | Time without seconds |
| `%I:%M %p` | 02:32 PM | 12-hour with AM/PM |

When `timestamp-format` is set, the text field (first parameter) is ignored and the
generated timestamp is used as the overlay text.

---

## Multiple Overlays

Repeat `-T` for each overlay.  Each is processed independently.

```bash
./bin/slowframe -i image.png \
  -T "W5ABC|size=16|bg=black|bgbar=true|pos=top" \
  -T "EM12ab|size=14|bg=navy|bgbar=true|pos=bottom" \
  -T "|timestamp-format=%H:%M UTC|size=12|pos=bottom-right" \
  -o output.wav -K
```

---

## Color Reference

Colors are specified via `bg=COLOR`:

**Named colors:**
`red`, `lime`, `blue`, `cyan`, `magenta`, `yellow`, `white`, `black`, `orange`, `purple`, `pink`, `green`, `navy`, `darkblue`, `darkgreen`, `darkred`

**Hex colors:**
`#RRGGBB` (e.g., `#FF0000` for red, `#000080` for navy)

---

## Examples

### HF Contact (Weak Signal)

```bash
./bin/slowframe -i photo.jpg \
  -T "W5ABC|size=16|bg=black|bgbar=true|bgbar-margin=6|pos=top" \
  -T "EM12ab|size=14|bg=black|bgbar=true|bgbar-margin=4|pos=bottom" \
  -o contact.wav -K
```

### Contest Format

```bash
./bin/slowframe -i photo.jpg \
  -T "N0ABC|size=14|bg=navy|bgbar=true|bgbar-margin=4|pos=top" \
  -T "FN25AE|size=14|bg=navy|bgbar=true|bgbar-margin=4|pos=center" \
  -T "|timestamp-format=%Y-%m-%d %H:%M UTC|size=12|bg=darkgreen|bgbar=true|pos=bottom" \
  -o contest.wav -K
```

### Vertical Side Bars (Callsign + RST)

```bash
./bin/slowframe -i photo.jpg \
  -T "W5ABC|size=20|bg=navy|bgbar=1|bgbar-orient=vertical-stacked|bgbar-width=55|pos=left" \
  -T "559|size=18|bg=darkred|bgbar=1|bgbar-orient=vertical-stacked|bgbar-width=50|pos=right" \
  -o qso.wav -K
```

### Precise Pixel Placement

```bash
./bin/slowframe -i photo.jpg \
  -T "W5ABC|x=10|y=10|size=14|bg=black|pad=4" \
  -T "100W / Dipole|x=10|y=32|size=11" \
  -T "|timestamp-format=%H:%M UTC|x=10|y=50|size=11" \
  -o custom.wav -K
```
