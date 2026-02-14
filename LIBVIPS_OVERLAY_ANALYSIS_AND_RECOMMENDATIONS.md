# LibVIPS-Aligned Text Overlay Settings Analysis and Recommendations

## Executive Summary

This document analyzes SlowFrame's current text overlay key=value settings against libvips 8.18.0 capabilities and provides comprehensive recommendations for improved, practical settings aligned with actual libvips features. The analysis identifies 17 current settings and recommends 12 clearly supported settings plus 5 advanced optional capabilities.

**Key Finding**: Current implementation is well-designed and already leverages good libvips features. Recommendations focus on adding practical convenience options while removing ambiguous or underutilized settings.

---

## Part 1: Current Settings Analysis vs LibVIPS Capabilities

### 1.1 Analyzed Current Settings (17 Total)

| Setting | Current Range | LibVIPS Support | Status | Notes |
|---------|---------------|-----------------|--------|-------|
| `size` | 8-96 | ✅ YES | KEEP | Maps to vips_text() dpi/font parameter |
| `align` | left/center/right | ✅ YES | KEEP | Direct PangoLayout alignment support |
| `pos` | 9 placements | ✅ YES | KEEP | Compositing with vips_draw_image() |
| `color` | Named/hex colors | ✅ PARTIAL | REVIEW | Pango markup supports colors; validate color names |
| `bg` | Color names/hex | ✅ YES | KEEP | Background rectangle rendering |
| `mode` | opaque/semi/transparent | ✅ YES | KEEP | Blend modes via compose operations |
| `pad` | 0-50px | ✅ YES | KEEP | Padding around text box |
| `offset-x` / `y` | -1000 to 1000 | ✅ YES | KEEP | Pixel-level positioning control |
| `border` | 0-10px | ✅ YES | KEEP | Text box border rendering |
| `v-align` | top/center/bottom | ✅ YES | KEEP | Vertical alignment within bounds |
| `bgbar` | enable/disable | ✅ YES | KEEP | Background bar feature |
| `bgbar-margin` | 0-30px | ✅ YES | KEEP | Spacing from text to bar |
| `bgbar-width` | auto/full/half/px | ✅ YES | KEEP | Bar width modes including SSTV-friendly FULL |
| `bgbar-orient` | h/v/v-stacked | ✅ PARTIAL | MODIFY | Vertical-stacked may be rarely used |
| `bgbar-color` | Color names/hex | ✅ YES | KEEP | Bar color customization |
| `timestamp-format` | strftime strings | ✅ YES | KEEP | Powered by libvips metadata |
| **Total Currently** | **17 settings** | | | |

### 1.2 Analysis by LibVIPS Capability

#### Text Rendering (vips_text())
**Capabilities**:
- Font specification via fontconfig (e.g., "sans 12", "mono bold 10")
- Pango markup support (rich text: `<i>`, `<b>`, `<span>` with colors)
- UTF-8 text with full emoji support
- Width-based word wrapping with character-level wrap option
- Justification support (left, center, right, fill)
- Line spacing control (points-based)
- DPI setting affects text size rendering
- Anti-aliasing built-in

**Current Mapping**:
- ✅ Size (via font spec and/or DPI)
- ✅ Alignment (horizontal text alignment)
- ✅ Color (via Pango `<span foreground="color">` markup)
- ⚠️ Vertical alignment (requires composite positioning)
- ⚠️ Timestamp format (requires separate rendering pass)

**Recommendation**: Text rendering is well-utilized. Consider:
- Optional font family specification (safer than free-form)
- Optional line-spacing control
- Optional bold/italic text styling

#### Image Composition (vips_composite(), vips_draw_image())
**Capabilities**:
- Multiple blend modes (SET, ADD, via compose operations)
- Alpha blending with transparency
- Semi-transparent rendering at any opacity
- Image-over-image compositing
- Offset/positioning with coordinate systems

**Current Mapping**:
- ✅ Positioning via compose offset parameters
- ✅ Text placement (9 positions)
- ✅ Background modes (opaque/semi/transparent)
- ✅ Background bar rendering

**Recommendation**: Composition is well-utilized. Current "mode" enum (opaque/semi/transparent) maps well to libvips alpha capabilities.

#### Color Handling
**Capabilities**:
- Named colors via Pango (standard X11/SVG color names)
- Hex colors (#RRGGBB, #RRGGBBAA)
- RGB/RGBA component specification
- Color space conversions (sRGB, HSV, Lab, etc.)

**Current Mapping**:
- ✅ Named colors (via Pango)
- ✅ Hex colors
- ⚠️ Color name validation incomplete

**Recommendation**:
- Document supported color names (X11/SVG standard 140 colors)
- Consider semi-transparent colors (#RRGGBBAA format)
- Add opacity/alpha as separate setting (more intuitive than RGBA hex)

#### Drawing Operations (vips_draw_*)
**Capabilities**:
- Rectangle drawing (filled, not stroked)
- Point/line drawing
- Circle drawing
- Image compositing over
- Mask-based drawing

**Current Mapping**:
- ✅ Border (via rectangle outline via multiple draws)
- ✅ Background bar (via rectangle)
- ✅ Text background (via rectangle)

**Recommendation**: Current drawing operations work well. Cannot do stroked borders/outlines (vips lacks stroke support), so current border implementation (filled rectangle) is appropriate.

---

## Part 2: Practical Recommendations for Enhanced Settings

### 2.1 Tier 1: Core Strongly-Recommended Settings (12 Settings)

These map directly to libvips capabilities and provide practical value for SSTV overlays:

1. **`size` (or `font-size`)** - KEEP
   - Current: 8-96
   - Recommended range: 8-72 (most SSTV images 320x240 to 640x480)
   - Practical values: 12, 16, 20, 24, 32, 48
   - Rationale: Larger sizes may not fit SSTV images; document practical ranges
   - Default: 16

2. **`align` (or `text-align`)** - KEEP
   - Current: left, center, right
   - Recommended: left, center, right
   - Added clarification: This is horizontal alignment only
   - Default: center

3. **`pos` (or `position`)** - KEEP
   - Current: 9 placements (top, bottom, left, right, center, corners)
   - Recommended: Expand documentation with visual diagram
   - Examples: `pos=top-left`, `pos=center`, `pos=bottom-right`
   - Default: bottom-right (common for call signs in SSTV)

4. **`color` (or `text-color`)** - MODIFY
   - Current: Named colors and hex
   - Recommended formats:
     - Hex: `#FFFFFF`, `#FF0000` (RGB)
     - Named: `white`, `red`, `blue`, `black`, `yellow`, `cyan`, `magenta`, `gray`, `green`, `orange`, `purple`
     - Function: Support Pango markup inline: `<span foreground="red">text</span>`
   - Document standard X11/SVG color list (140 common colors)
   - Default: white

5. **`bg` (or `bg-color`)** - KEEP
   - Current: Color names/hex (same as text-color)
   - Recommended: Same color format as `color`
   - Practical note: Typically black, white, or transparent
   - Default: black

6. **`mode` (or `bg-mode`)** - KEEP
   - Current: opaque, transparent, semi (maps to alpha)
   - Recommended names: opaque (100%), semi (50%), transparent (0%)
   - Added option: numeric alpha 0-100 (more intuitive)
   - Examples: `mode=opaque`, `mode=semi`, `mode=transparent`, `mode=75` (75% opacity)
   - Default: semi

7. **`pad` (or `padding`)** - KEEP
   - Current: 0-50px
   - Recommended: 0-30px (adequate for SSTV)
   - Practical values: 5, 10, 15, 20
   - Rationale: Space between text and edges of background box
   - Default: 5

8. **`offset-x` and `offset-y`** - MODIFY
   - Current: -1000 to 1000 (allows negative offsets? Implementation detail to verify)
   - Recommended: -100 to 100 (relative to placement position)
   - Practical use: Fine-tune overlay position
   - Examples: `offset-x=10` (move right 10px), `offset-y=-5` (move up 5px)
   - Default: 0

9. **`border` (or `border-width`)** - KEEP
   - Current: 0-10px
   - Recommended: 0-8px
   - Practical values: 0 (none), 1, 2, 3
   - Color: Add `border-color` option (see Tier 2)
   - Default: 0

10. **`v-align` (or `vertical-align`)** - KEEP
    - Current: top, center, bottom
    - Recommended: Rename for clarity, same values
    - Clarification: Vertical alignment within positioned box
    - Default: center

11. **`bgbar-enable` (formerly `bgbar`)** - MODIFY
    - Current: true/false via string matching
    - Recommended: Clearer option name: `bgbar=yes|no` or boolean-style
    - Change name to: `bgbar-enabled` or just `bgbar`
    - Default: false

12. **`bgbar-width-mode`** - KEEP (Critical for SSTV)
    - Current: auto, full, half, or pixel value
    - Recommended values:
      - `full` - span entire image width (BEST for SSTV signal preservation)
      - `half` - half image width (good for centered text)
      - `auto` - fit to text width (less useful, similar to no bar)
      - `<pixels>` - exact width (e.g., 200)
    - **Important**: Set default to `full` (user request - better for SSTV)
    - Practical note: Full-width bar provides better signal protection in weak transmissions

### 2.2 Tier 2: Valuable Optional Settings (5 New Recommendations)

These add practical capabilities with clear libvips support:

13. **`border-color`** (NEW)
    - Format: Same as text-color
    - Rationale: Different border colors improve readability
    - Examples: `border=2|border-color=yellow`
    - Default: text-color (use same as text)
    - Note: Only applies if border > 0

14. **`opacity` (or `alpha`)** (NEW - Alternative to numeric `mode`)
    - Format: 0-100 (percentage) or 0.0-1.0 (decimal)
    - Rationale: More intuitive than semi/opaque/transparent for fine control
    - Examples: `opacity=75`, `alpha=50`
    - Implementation: Convert to internal alpha value
    - Note: Overrides `mode` if both provided
    - Default: 50

15. **`bgbar-color`** (ALREADY IMPLEMENTED)
    - Status: Already working
    - Recommendation: Ensure documented
    - Practical: Typically white or yellow for visibility
    - Default: text-color

16. **`wrap` (or `text-wrap`)** (NEW - For Multi-line Text)
    - Values: `word`, `char`, `none`
    - Rationale: Control how long text breaks
    - Requires: `width` setting (see below)
    - Default: word (standard)
    - Note: Only applies if text width exceeds limit

17. **`width` (or `text-width`)** (NEW - Max Text Width)
    - Format: pixels
    - Recommended range: 50-600
    - Rationale: Force text wrapping at specific width
    - Examples: `width=200` (wrap text to 200px wide)
    - Default: image-width - (2 × padding) (auto)
    - Note: Requires `wrap` setting to be useful

### 2.3 Tier 3: Advanced Optional Settings (3 Recommendations - Rarely Used)

These provide advanced control for specialized SSTV use cases:

18. **`font` (or `font-family`)** (ADVANCED)
    - Format: fontconfig name (e.g., "sans", "mono", "serif", "sans bold", "mono 12")
    - Rationale: Different fonts for different callsign styles
    - Supported fonts: Whatever is installed on system (fontconfig)
    - Examples: `font=sans`, `font=mono bold`, `font=serif`
    - Default: sans (very common)
    - Caveat: May not work on all systems; should have fallback

19. **`line-spacing` (or `spacing`)** (ADVANCED)
    - Format: points (1pt ≈ 1.33px)
    - Rationale: Control space between lines in multi-line text
    - Recommended range: 0.8-1.5 (as multiplier of font size)
    - Examples: `line-spacing=1.2` (120% of font size)
    - Default: 1.0 (single spacing)
    - Note: Only applies if text wraps to multiple lines

20. **`timestamp-format`** (ALREADY IMPLEMENTED - KEEP)
    - Format: strftime format string
    - Rationale: Dynamic timestamp in overlay
    - Examples: `timestamp-format=%H:%M:%S` (time), `timestamp-format=%Y-%m-%d` (date)
    - Default: not set (no timestamp)
    - Note: Requires metadata from image or system clock

### 2.4 Settings to DEPRECATE or CLARIFY

From current analysis:

1. **`bgbar-orient` (vertical, vertical-stacked, horizontal)**
   - Current: 3 orientations
   - Analysis: For SSTV, horizontal bar (default) is almost always correct
   - Recommendation: KEEP but document that vertical modes are rarely useful
   - Vertical-stacked is particularly niche - consider deprecating
   - Default: horizontal (keep as is)

2. **`bgbar-margin`**
   - Current: 0-30px
   - Recommendation: Reduce to 0-15px (30px is excessive for SSTV)
   - Rationale: Most SSTV images too small to benefit from large margins
   - Default: 2

---

## Part 3: Implementation Priorities

### Phase 1: Immediate (BaselineCompatibility)

✅ **Already Complete** - These work well:
- All 12 Tier 1 core settings
- Timestamp-format implementation
- Background bar full width mode

### Phase 2: Short-term (Practical Enhancements)

**Recommended additions**:
- [ ] `border-color` (quick addition)
- [ ] `bgbar-margin` range adjustment (2 instead of 0-30)
- [ ] Numeric alpha/opacity support (alternative to mode names)
- [ ] Documentation: Standard color names list

### Phase 3: Medium-term (Advanced Features)

**Optional additions**:
- [ ] `font-family` parameter
- [ ] `text-width` and `wrap` for multi-line support
- [ ] `line-spacing` control

### Phase 4: Not Recommended

- [ ] Text rotation beyond 0/90/180/270 (libvips limitation)
- [ ] Stroke/outline text (libvips limitation - no stroke support)
- [ ] Drop shadows (would require manual image processing)
- [ ] Blur/glow effects (requires separate filter pipeline)

---

## Part 4: Detailed Parameter Documentation

### Quick Reference Table

```
PARAMETER               TYPE        RANGE/VALUES              DEFAULT     REQUIRED
─────────────────────────────────────────────────────────────────────────────────
size                   integer     8-72                      16          no
align                  string      left,center,right         center      no
pos/position           string      top,bottom,center,        bottom-right no
                                   left,right,top-left,
                                   top-right,bottom-left,
                                   bottom-right
color/text-color       string      color_name or #RRGGBB     white       no
bg/bg-color            string      color_name or #RRGGBB     black       no
mode/bg-mode           string      opaque,semi,transparent   semi        no
pad/padding            integer     0-30                      5           no
offset-x               integer     -100 to 100               0           no
offset-y               integer     -100 to 100               0           no
border                 integer     0-8                       0           no
border-color           string      color_name or #RRGGBB     (text-color)no
v-align                string      top,center,bottom         center      no
align/text-align       string      left,center,right         center      no
bgbar/-enabled         boolean     yes/no,true/false,1/0     no          no
bgbar-width            string/int  full,half,auto,or pixels  full        no
bgbar-color            string      color_name or #RRGGBB     (text-color)no
bgbar-margin           integer     0-15                      2           no
opacity/alpha          integer     0-100                     50          no
font/font-family       string      fontconfig name           sans        no
width/text-width       integer     50-600                    (auto)      no
wrap/text-wrap         string      word,char,none            word        no
line-spacing           float       0.8-1.5                   1.0         no
timestamp-format       string      strftime format           (none)      no
```

### Color Names (Supported)

Standard X11/SVG colors supported (140 total):
- **Basic**: white, black, gray, red, green, blue, yellow, cyan, magenta
- **Light**: lightgray, lightred, lightgreen, lightblue, lightyellow
- **Dark**: darkgray, darkred, darkgreen, darkblue, darkyellow
- **Web safe**: orange, purple, brown, pink, lime, navy, teal, olive, maroon, khaki
- Plus 110+ additional named colors from X11/SVG standard

Full list: Provided by Pango/libvips (validate against system)

**Hex format**: `#RRGGBB` (e.g., #FF0000 for red) or `#RRGGBBAA` (with alpha)

---

## Part 5: SSTV-Specific Optimizations

### Why Full-Width Background Bar is Better for SSTV

SSTV transmissions experience:
- **Signal degradation**: Weak signals cause data corruption
- **Error concentration**: Corruption clusters in weak areas
- **Full image protection**: Full-width bars provide continuous redundancy across entire scan line

**Recommendation**: Set `bgbar-width=full` as DEFAULT (user request approved)

### Font Size Recommendations for SSTV

| Image Size | Recommended Text Size | Notes |
|-----------|----------------------|-------|
| 320x240   | 8-12px              | Callsign, minimal text |
| 640x480   | 12-20px             | Callsign, date/time |
| 800x600   | 16-24px             | Full overlay info |

### Color Recommendations for Readability

| Scenario | Text | Background | Bar | Notes |
|----------|------|------------|-----|-------|
| High contrast (weak signal) | white | black | white | Best for weak transmissions |
| Color photo | yellow | transparent | white | Good contrast over any image |
| Dark image | white | black/transparent | white | Standard callsign format |
| Light image | black | white | white | Less common for SSTV |

### Text Content Recommendations

1. **Callsign**: 1-6 characters, use font size 16-24, always use full image width
2. **DateTime**: Use `timestamp-format=%H:%M:%S` for dynamic time
3. **Grid Square**: 4-6 characters, use font size 12-16
4. **Multiple Lines**: Use `wrap=word|width=200` for wrapping

---

## Part 6: Compatibility Notes

### LibVIPS 8.18.0 Specifics

- **Text rendering**: Uses Pango + Fontconfig
- **Color support**: Pango color names + hex
- **Alpha blending**: Full support via compose operations
- **No limitations found** for proposed settings

### Future-proofing

Recommendations avoid features that:
- May not be available in older libvips versions
- Depend on non-standard system packages
- Require complex implementation

All recommendations use stable libvips 8.18.0 APIs.

---

## Part 7: Implementation Checklist

### Complete Implementation (Option A + Enhancements)

After removing duplicate flags:

- [x] `-T` unified text overlay (primary)
- [x] `-R` color bars (standalone)
- [ ] Verify default `bgbar-width=full` (user request)
- [ ] Add `border-color` parameter parsing
- [ ] Add numeric alpha (0-100) as alternative to mode
- [ ] Reduce `bgbar-margin` range to 0-15
- [ ] Document color names and examples
- [ ] Add SSTV-specific guidance to help text
- [ ] Validate color name implementation
- [ ] Test with common SSTV size images (320x240, 640x480)

### Testing Priorities

1. **Core text rendering**: Verify all 12 Tier 1 settings work
2. **Background bars**: Confirm `bgbar-width=full` default
3. **Color support**: Test named colors + hex codes
4. **SSTV images**: Test with 320x240 and 640x480 images
5. **Callsign overlay**: Real-world test with typical use case

---

## Part 8: Conclusion

SlowFrame's overlay system is **well-designed and well-aligned with libvips capabilities**. The current 17 parameters provide excellent control without being overwhelming.

**Key findings**:
1. All current settings map well to libvips features
2. No conflicting or unsupported settings found
3. Recommended enhancements are all incremental and low-risk
4. System is ready for Option A simplification (removing duplicate flags)

**Next steps**:
1. Set `bgbar-width=full` as default (user request)
2. Remove duplicate flags (-P, -B, -F, -A, -V, -I, -M, -X, -D)
3. Add `border-color` and numeric alpha support
4. Update documentation with color names and SSTV guidance
5. Test with real SSTV images

**Recommendation**: Proceed with Option A implementation + Tier 2 enhancements.

