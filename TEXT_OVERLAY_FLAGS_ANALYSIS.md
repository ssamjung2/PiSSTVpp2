# Text Overlay Flags Analysis
**Date:** February 13, 2026  
**Project:** SlowFrame  
**Status:** COMPREHENSIVE ANALYSIS COMPLETE

---

## Executive Summary

The text overlay system has a **primary unified interface** (-T flag) that supports embedded styling, plus **10 additional flags** for modifying overlays. Upon detailed analysis:

- **9 flags are PURE DUPLICATES** of -T functionality
- **1 flag (-R)** is a standalone feature unrelated to text overlays
- **1 flag (-I)** is a redundant, less-complete version of another flag (-P)

**Recommendation:** Simplify to 2-3 flags focused on truly useful convenience functions.

---

## Detailed Flag Analysis

### PRIMARY INTERFACE: -T (Unified Specification)

**Format:** `-T "text|key1=value1|key2=value2|..."`

**Supports all styling in a single, parseable spec:**
```bash
slowframe -i photo.jpg -T "N0CALL|size=24|color=white|bg=blue|pad=4|v-align=center"
```

**Supported Key-Value Parameters:**
| Key | Aliases | Accepts | Example |
|-----|---------|---------|---------|
| size | s | 8-96 | `size=20` |
| align | a | left, center, right | `align=center` |
| pos | p, position | top, bottom, left, right, center, top-left, bottom-right, etc. | `pos=top` |
| color | c | Named or #RRGGBB | `color=white` or `color=#FF0000` |
| bg | background | Named or #RRGGBB | `bg=blue` |
| mode | m | opaque, transparent, semi | `mode=opaque` |
| pad | padding | 0-50 | `pad=4` |
| border | b, d | 0-10 | `border=2` |
| v-align | va, valign | top, center, bottom | `v-align=center` |
| x, offset-x, offsetx | | -1000 to 1000 | `x=100` |
| y, offset-y, offsety | | -1000 to 1000 | `y=50` |
| bgbar, background-bar | | true, 1, yes, enable | `bgbar=true` |

---

## Individual Supplementary Flags

### FLAG: -P (Overlay Placement)
**Status:** ❌ **DUPLICATE** - Available as `-T pos=...`

**Code Location:** slowframe_config.c:360-369

**What it does:**
```c
config->current_overlay->placement = overlay_parse_placement(optarg);
```

**Accepts:** top, bottom, left, right, center, top-left, top-right, bottom-left, bottom-right (and variants)

**Example Usage:**
```bash
slowframe -i photo.jpg -T "N0CALL" -P top
# Equivalent to:
slowframe -i photo.jpg -T "N0CALL|pos=top"
```

**Redundancy:** Pure duplicate. -T pos= is already more flexible.

---

### FLAG: -B (Background Color)
**Status:** ❌ **DUPLICATE** - Available as `-T bg=...`

**Code Location:** slowframe_config.c:371-381

**What it does:**
```c
overlay_parse_color(optarg, &config->current_overlay->bg_color);
```

**Accepts:** Named colors (red, blue, white, #RRGGBB hex codes)

**Example Usage:**
```bash
slowframe -i photo.jpg -T "N0CALL" -B blue
# Equivalent to:
slowframe -i photo.jpg -T "N0CALL|bg=blue"
```

**Redundancy:** Exact duplicate. -T bg= is already available.

---

### FLAG: -F (Font Size)
**Status:** ❌ **DUPLICATE** - Available as `-T size=...`

**Code Location:** slowframe_config.c:383-402

**What it does:**
```c
config->current_overlay->font_size = (uint8_t)font_size;
```

**Accepts:** 8-96 pixels

**Example Usage:**
```bash
slowframe -i photo.jpg -T "N0CALL" -F 24
# Equivalent to:
slowframe -i photo.jpg -T "N0CALL|size=24"
```

**Redundancy:** Exact duplicate. -T size= is already available.

---

### FLAG: -A (Horizontal Text Alignment)
**Status:** ❌ **DUPLICATE** - Available as `-T align=...`

**Code Location:** slowframe_config.c:404-411

**What it does:**
```c
config->current_overlay->text_align = overlay_parse_alignment(optarg);
```

**Accepts:** left, center, right

**Example Usage:**
```bash
slowframe -i photo.jpg -T "N0CALL" -A center
# Equivalent to:
slowframe -i photo.jpg -T "N0CALL|align=center"
```

**Redundancy:** Exact duplicate. -T align= is already available.

---

### FLAG: -V (Vertical Text Alignment)
**Status:** ❌ **DUPLICATE** - Available as `-T v-align=...`

**Code Location:** slowframe_config.c:620-639

**What it does:**
```c
if (strcmp(optarg, "top") == 0) {
    config->current_overlay->valign = VALIGN_TOP;
} // ... etc
```

**Accepts:** top, center, bottom

**Example Usage:**
```bash
slowframe -i photo.jpg -T "N0CALL" -V center
# Equivalent to:
slowframe -i photo.jpg -T "N0CALL|v-align=center"
```

**Redundancy:** Exact duplicate. -T v-align= is already available.

---

### FLAG: -I (Image/Overlay Placement)
**Status:** ❌ **DUPLICATE OF -P** (Plus inferior implementation)

**Code Location:** slowframe_config.c:413-450

**What it does:**
```c
// Sets same field as -P but with less complete parsing
config->current_overlay->placement = OVERLAY_PLACE_TOP;  // etc.
```

**Accepts:** top, bottom, left, right, center, middle

**Limitations vs -P:**
- Does NOT support corner positions (top-left, bottom-right, etc.)
- Less complete than overlay_parse_placement() which -P uses
- Duplicates -P with fewer options

**Example Usage:**
```bash
slowframe -i photo.jpg -T "N0CALL" -I top
# Could use -P instead:
slowframe -i photo.jpg -T "N0CALL" -P top
# Or better, use -T:
slowframe -i photo.jpg -T "N0CALL|pos=top"
```

**Redundancy:** **WORST CASE** - Duplicate of -P, but with inferior functionality (missing corner positions)

---

### FLAG: -M (Background Mode)
**Status:** ❌ **DUPLICATE** - Available as `-T mode=...`

**Code Location:** slowframe_config.c:452-476

**What it does:**
```c
if (strcmp(optarg, "opaque") == 0) {
    config->current_overlay->bg_mode = BG_OPAQUE;
} // ... etc
```

**Accepts:** opaque, transparent, semi, semi-transparent

**Example Usage:**
```bash
slowframe -i photo.jpg -T "N0CALL" -M opaque
# Equivalent to:
slowframe -i photo.jpg -T "N0CALL|mode=opaque"
```

**Redundancy:** Exact duplicate. -T mode= is already available.

---

### FLAG: -R (Color Bars)
**Status:** ✅ **STANDALONE FEATURE** - NOT available in -T

**Code Location:** slowframe_config.c:478-560

**What it does:**
Adds decorative colored bar separators to the image (completely separate from text overlay styling).

**Accepts:** `position:color1,color2,...`

**Example Usage:**
```bash
slowframe -i photo.jpg -R "top:red"
slowframe -i photo.jpg -R "bottom:white,cyan,magenta"
slowframe -i photo.jpg -T "N0CALL" -R "top:navy" -R "bottom:white"
```

**Functionality:** 
- Adds horizontal/vertical color bars at specified positions
- Multiple -R flags can be used for multiple bars
- Creates visual separation/framing
- Not available in -T specification

**Assessment:** ✅ **USEFUL** - Standalone feature not duplicated elsewhere

---

### FLAG: -X (Padding)
**Status:** ❌ **DUPLICATE** - Available as `-T pad=...`

**Code Location:** slowframe_config.c:562-582

**What it does:**
```c
config->current_overlay->padding = (uint16_t)padding;
```

**Accepts:** 0-50 pixels

**Example Usage:**
```bash
slowframe -i photo.jpg -T "N0CALL" -X 6
# Equivalent to:
slowframe -i photo.jpg -T "N0CALL|pad=6"
```

**Redundancy:** Exact duplicate. -T pad= is already available.

---

### FLAG: -D (Border Width)
**Status:** ❌ **DUPLICATE** - Available as `-T border=...`

**Code Location:** slowframe_config.c:584-605

**What it does:**
```c
config->current_overlay->border_width = (uint16_t)border_width;
```

**Accepts:** 0-10 pixels

**Example Usage:**
```bash
slowframe -i photo.jpg -T "N0CALL" -D 2
# Equivalent to:
slowframe -i photo.jpg -T "N0CALL|border=2"
```

**Redundancy:** Exact duplicate. -T border= is already available.

---

## Summary Table

| Flag | Feature | Duplicates -T | Useful | Recommendation |
|------|---------|---------------|--------|-----------------|
| -P | Placement | Yes (pos=) | For convenience | **CONSIDER REMOVING** |
| -B | Background color | Yes (bg=) | For convenience | **CONSIDER REMOVING** |
| -F | Font size | Yes (size=) | For convenience | **CONSIDER REMOVING** |
| -A | Horiz. alignment | Yes (align=) | For convenience | **CONSIDER REMOVING** |
| -V | Vert. alignment | Yes (v-align=) | For convenience | **CONSIDER REMOVING** |
| -I | Placement (alt) | Yes (-P duplicate) | Not useful | **REMOVE** |
| -M | Background mode | Yes (mode=) | For convenience | **CONSIDER REMOVING** |
| -X | Padding | Yes (pad=) | For convenience | **CONSIDER REMOVING** |
| -D | Border width | Yes (border=) | For convenience | **CONSIDER REMOVING** |
| -R | Color bars | NO (standalone) | Very useful | **KEEP** |

---

## Recommendations

### Option A: Minimal (Recommended)
**Keep:** -T (primary), -R (standalone feature)  
**Remove:** -P, -B, -F, -A, -V, -I, -M, -X, -D (all duplicates)

**Rationale:**
- Forces use of powerful, flexible, single-command -T interface
- Users learn one way to specify overlays (embedded in -T spec)
- Reduces CLI surface area and maintenance burden
- Keeps -R for truly standalone color bar functionality

**User command evolution:**
```bash
# Old (cluttered):
slowframe -i photo.jpg -T "N0CALL" -F 24 -A center -P top -B blue -X 4

# New (clean, single spec):
slowframe -i photo.jpg -T "N0CALL|size=24|align=center|pos=top|bg=blue|pad=4"
```

### Option B: Keep Commonly-Used Convenience Flags
**Keep:** -T, -R, -F, -B, -P (most commonly used individual options)  
**Remove:** -A, -V, -I, -X, -M, -D (less commonly used)

**Rationale:**
- Provides convenience flags for basic use cases
- Reduces need for complex -T syntax for simple operations
- Balance between simplicity and real usage patterns

### Option C: Keep All (Current State)
**Pros:**
- Provides multiple ways to specify same thing
- Might suit some workflows

**Cons:**
- Confusing for users (which method is best?)
- Maintenance burden
- Contradictory documentation
- -I is actually broken (missing corner positions)

---

## Additional Issues Found

### Issue 1: -I is Functionally Broken
The -I flag only supports 5 positions (top, bottom, left, right, center) while -P/-T support 9 positions (adds corners). The hardcoded parsing in -I is inferior to the shared overlay_parse_placement() function used by -P and -T.

### Issue 2: Confusing Documentation
The help system lists -P, -B, -F, etc. as separate options, when they're actually just convenience wrappers around -T embedded parameters. This creates confusion about which method to use.

### Issue 3: Multiple Application Methods
- `-T "text|key=value"` - In one spec string
- `-T "text" -F 20` - Spec + separate flag
- Both methods work, creating inconsistency

---

## Proposed Changes

### If Choosing Option A (Recommended):
1. **Remove flags:** -A, -V, -I, -M, -P, -B, -F, -X, -D
2. **Keep flags:** -T, -R
3. **Update help** to only show -T for overlay styling
4. **Update documentation** with -T syntax examples

### If Choosing Option B:
1. **Remove flags:** -A, -V, -I, -X, -M, -D
2. **Keep flags:** -T, -R, -F, -B, -P (most useful convenience)
3. **Mark as convenience:** Indicate in help these are shortcuts
4. **Add warning:** In help, encourage using -T for full feature access

---

## Conclusion

**Current State:** Redundant, confusing CLI with 9 duplicate flags and 1 lesser-quality duplicate

**Recommended State:** Clean interface with -T for text overlay styling and -R for color bars

**Impact:** Simpler, easier to maintain, clearer user experience, no loss of functionality

