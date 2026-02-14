# PiSSTVpp2 v2.1 - Text Overlay System Redesign
## Generic Multi-Overlay Architecture

**Date:** February 11, 2026  
**Purpose:** Support arbitrary text overlays with per-overlay customization  
**Status:** Design Complete - Ready for Implementation  

---

## Overview

Replace the current QSO-specific approach (`-S callsign`, `-G grid`) with a flexible system supporting:
- **Arbitrary text content** - Any string, not just QSO data
- **Multiple overlays** - Add as many text elements as needed
- **Per-overlay customization** - Each overlay has independent styling
- **Simple CLI UX** - Intuitive flags with sensible defaults

---

## Design Approach: Layered Flag Model

Each overlay is created with a **text flag** (`-O`), then **customized** with additional flags that apply to the most recent overlay.

### User Experience

```bash
# Simple: Two overlays with defaults
./pisstvpp2 -i photo.jpg -O "W5ZZZ" -O "EM12ab"

# Medium: Multiple overlays, custom placement
./pisstvpp2 -i photo.jpg \
  -O "W5ZZZ" -P top \
  -O "EM12ab" -P bottom

# Advanced: Full customization
./pisstvpp2 -i photo.jpg \
  -O "W5ZZZ" -P top -C blue -F 28 -B white \
  -O "EM12ab" -P bottom -C white -F 24 -B blue \
  -O "Feb 11, 2026" -P right -C gray -F 16 -B transparent
```

### Advantages
- ✅ Simple to use: `-O "text"` creates overlay
- ✅ Intuitive: flags after `-O` apply to that overlay
- ✅ No delimiter issues: One flag = one value
- ✅ Scalable: Can add 10+ overlays easily
- ✅ Extensible: Easy to add new per-overlay options later
- ✅ Backward compatible: Can map old `-S/-G` to new system

---

## Proposed CLI Flags

### Core Overlay Text
```
-O <text>               Create new overlay with this text
                        Examples: -O "W5ZZZ" -O "Grid Square" -O "Feb 11"
```

### Overlay Styling (apply to most recent -O)
```
-P <placement>          Placement mode (default: bottom)
                        Options: top, bottom, left, right, center, 
                                 top-left, top-right, bottom-left, bottom-right
                        Example: -P top

-C <color>              Text color (default: blue)
                        Format: hex (FF0000) or name (red, blue, white, black, etc)
                        Example: -C white or -C 0066FF

-B <color>              Background color (default: white)
                        Same format as -C
                        Example: -B 000000 or -B black

-F <size>               Font size in pixels (default: 28)
                        Range: 8-96
                        Example: -F 24

-A <align>              Text alignment (default: center)
                        Options: left, center, right
                        Example: -A left

-K <bg_mode>            Background mode (default: opaque)
                        Options: opaque, transparent, semi
                        Example: -K transparent
```

### Optional/Meta
```
-O                      (flag alone, with no text) - If 2nd use, list current overlays
--overlay-list          List all currently configured overlays
```

---

## Implementation Architecture

### Config Structure Changes

**Before (QSO-specific):**
```c
typedef struct {
    char cw_callsign[...];              // CW signature
    char station_grid_square[...];      // Grid square
    OverlaySpecList overlay_specs;      // List of specs
} PisstvppConfig;
```

**After (Generic):**
```c
typedef struct {
    // Existing fields
    char cw_callsign[...];              // CW signature (unchanged)
    
    // Text overlay system
    OverlaySpecList overlay_specs;      // List of all overlay specs
    TextOverlaySpec *current_overlay;   // Current overlay being configured
    
} PisstvppConfig;
```

### Parsing Logic

```c
// Simplified pseudo-code for getopt loop
while ((option = getopt(argc, argv, "i:o:p:f:r:vC:W:T:a:KZhO:P:C:B:F:A:K:")) != -1) {
    switch (option) {
        // ... existing options ...
        
        case 'O':  // Create new overlay
            // If optarg is empty, list overlays (optional)
            spec = overlay_spec_create_default();
            spec.text = strdup(optarg);
            config->current_overlay = &spec;  // Mark as current
            break;
            
        case 'P':  // Placement (for current overlay)
            if (config->current_overlay) {
                config->current_overlay->placement = overlay_parse_placement(optarg);
            }
            break;
            
        case 'C':  // Text color (for current overlay)
            if (config->current_overlay) {
                overlay_parse_color(optarg, &config->current_overlay->text_color);
            }
            break;
            
        case 'B':  // Background color (for current overlay)
            if (config->current_overlay) {
                overlay_parse_color(optarg, &config->current_overlay->bg_color);
            }
            break;
            
        case 'F':  // Font size (for current overlay)
            if (config->current_overlay) {
                config->current_overlay->font_size = atoi(optarg);
            }
            break;
            
        // ... add to list when next -O encountered or at end of parsing
    }
}
```

### Finalization

After parsing all arguments, add any pending overlay to list:
```c
// Add current overlay to list when done
if (config->current_overlay && strlen(config->current_overlay->text) > 0) {
    overlay_spec_list_add(&config->overlay_specs, config->current_overlay);
}
```

---

## Examples & Use Cases

### Use Case 1: FCC Compliance (Current)
```bash
# Old syntax (still works via mapping):
./pisstvpp2 -i photo.jpg -S W5ZZZ -G EM12ab

# New syntax:
./pisstvpp2 -i photo.jpg -O "W5ZZZ" -P bottom -O "EM12ab" -P bottom
```

### Use Case 2: Multiple Station IDs
```bash
# Add callsign at top, grid square at bottom
./pisstvpp2 -i photo.jpg \
  -O "W5ZZZ" -P top -C blue -F 32 \
  -O "EM12ab" -P bottom -C blue -F 32
```

### Use Case 3: Rich Metadata
```bash
# Callsign, grid, date, operator
./pisstvpp2 -i photo.jpg \
  -O "W5ZZZ" -P top -C blue -F 28 -B white \
  -O "EM12ab" -P bottom -C blue -F 24 -B white \
  -O "2026-02-11" -P left -C gray -F 16 -B transparent \
  -O "John K5ABC" -P right -C gray -F 14 -B transparent
```

### Use Case 4: Professional Contest Overlay
```bash
./pisstvpp2 -i photo.jpg \
  -O "ARCI SSTV Contest" -P top -C white -F 36 -B black \
  -O "W5ZZZ" -P top-left -C yellow -F 24 -B black \
  -O "EM12ab" -P top-right -C yellow -F 24 -B black \
  -O "Feb 11, 2026 - UTC" -P bottom -C white -F 18 -B black
```

### Use Case 5: Minimal (Just Text)
```bash
# Text overlay with all defaults
./pisstvpp2 -i photo.jpg -O "73 ES HAPPY SSTV!"
```

---

## Backward Compatibility

### Option A: Automatic Mapping
Keep old `-S` and `-G` flags, internally convert to new system:

```c
// In parser:
case 'S':  // Old: station callsign only
    spec = overlay_spec_create_default();
    spec.text = strdup(optarg);
    spec.placement = OVERLAY_PLACE_BOTTOM;
    overlay_spec_list_add(&config->overlay_specs, spec);
    break;

case 'G':  // Old: grid square only (requires prior -S)
    // Find most recent overlay, append grid square
    if (config->overlay_specs.count > 0) {
        TextOverlaySpec *last = overlay_spec_list_get(&config->overlay_specs, 
                                                      config->overlay_specs.count - 1);
        strcat(last->text, " ");
        strcat(last->text, optarg);
    }
    break;
```

### Option B: Deprecation
Mark `-S` and `-G` as deprecated, users migrate to `-O` explicitly:
```c
// Log warning if old flags used
if (old_flag_used) {
    error_log(PISSTVPP2_WARN_DEPRECATED_FLAG,
              "Option -%c is deprecated. Use -O instead.", old_flag);
}
```

**Recommendation:** Use Option A initially (backward compatible), deprecate in v2.2+

---

## Technical Details

### Text Parameter Validation
```c
// In overlay spec creation/validation:
if (strlen(text) == 0) {
    error_log(PISSTVPP2_ERR_ARG_INVALID, "Overlay text cannot be empty");
    return error;
}

if (strlen(text) > OVERLAY_MAX_TEXT_LENGTH) {
    error_log(PISSTVPP2_ERR_ARG_INVALID, "Text too long (max %d)", OVERLAY_MAX_TEXT_LENGTH);
    return error;
}

// Validate placement, color, font size, alignment
```

### Color Specification
Support multiple formats for user convenience:

```c
// Hex color
-C FF0000        // Red
-C 0066FF        // Blue
-C FFFFFF        // White

// Named colors
-C red
-C blue
-C white
-C black
-C yellow
-C cyan
-C magenta
-C gray

// RGB (future extension)
// -C rgb(255,0,0)  -- could add later
```

### Font Size Constraints
```c
#define OVERLAY_MIN_FONT_SIZE 8
#define OVERLAY_MAX_FONT_SIZE 96
#define OVERLAY_DEFAULT_FONT_SIZE 28

// Validation:
if (font_size < MIN || font_size > MAX) {
    error_log(..., "Font size must be %d-%d", MIN, MAX);
    return error;
}
```

### Placement Modes
```
TOP              ┌─────────────┐
                 │  [TEXT]     │
                 └─────────────┘

BOTTOM           ┌─────────────┐
                 │             │
                 │  [TEXT]     │
                 └─────────────┘

LEFT    │ ┌─────────────┐
        │ │             │
    [T] │ │             │
    [E] │ │             │
    [X] │ │             │
    [T] │ │             │
        │ └─────────────┘

RIGHT           ┌─────────────┐ │
                │             │ │
                │             │ [T]
                │             │ [E]
                │             │ [X]
                │             │ [T]
                └─────────────┘ │

CENTER          ┌─────────────┐
                │   [TEXT]    │
                │             │
                └─────────────┘

TOP-LEFT    [TEXT] ┌─────────────┐
                   │             │
                   │             │
                   └─────────────┘
```

---

## Migration Path

### Phase 1: Implement New System (Parallel)
- Add new `-O`, `-P`, `-C`, `-B`, `-F`, `-A`, `-K` flags
- Keep old `-S`, `-G` flags working via automatic mapping
- Both systems work simultaneously during v2.1

### Phase 2: Documentation (v2.1)
- Document new `-O` flags as primary method
- Document `-S`/`-G` as legacy but supported
- Update examples to use new system
- Migration guide for users

### Phase 3: Deprecation (v2.2+)
- Mark `-S`/`-G` as deprecated in help text
- Add deprecation warnings when used
- Announce removal plan (e.g., v3.0)

### Phase 4: Removal (v3.0+)
- Remove old `-S`/`-G` code
- Code cleanup complete

---

## Implementation Tasks (Updated)

### New Tasks Replacing Text Overlay Tasks 2.1-2.3

#### Task A: Update Config Structure & Parsing
**Files:** `src/include/pisstvpp2_config.h`, `src/pisstvpp2_config.c`

1. Add to PisstvppConfig:
   - `TextOverlaySpec *current_overlay` pointer
   - Helper function to finalize current overlay

2. Add to getopt options: `O:P:C:B:F:A:K:`

3. Implement handlers for each flag that modify current_overlay

4. Finalize/add overlay when next `-O` encountered or parsing ends

#### Task B: Update Config Help Text
**Files:** `src/pisstvpp2.c` (show_help function)

Add section documenting new text overlay system with examples

#### Task C: Mapping Old Flags to New System
**Files:** `src/pisstvpp2_config.c`

Implement automatic conversion of old `-S`/`-G` to new spec system for backward compatibility

#### Task D: Update Text Overlay Rendering
**Files:** `src/pisstvpp2_image.c`

No changes needed - rendering already generic enough to handle any text

#### Task E: Enhanced Testing
**Files:** `tests/test_suite.py`

Test cases for:
- Single overlay with various parameters
- Multiple overlays with different settings
- Color variations (hex and named)
- Placement modes
- Font sizes
- Backward compatibility with old flags

#### Task F: Documentation
**Files:** New `docs/TEXT_OVERLAY_USER_GUIDE.md`

Create user guide showing:
- Basic usage examples
- FCC compliance example
- Advanced multi-overlay example
- Color reference
- Placement reference
- Parameter constraints

---

## Comparison: Old vs New

| Aspect | Old System | New System |
|--------|-----------|-----------|
| Max overlays | 1-2 (callsign + grid) | Unlimited (10+) |
| Text source | QSO-specific fields | Any arbitrary text |
| Per-overlay customization | Limited (based on field type) | Full (color, size, placement) |
| User Experience | Simple but limited | Simple and flexible |
| CLI Complexity | Low (2 flags) | Medium (7 flags total) |
| Learning Curve | Shallow | Gentle slope |
| Extensibility | Hard (field-centric) | Easy (add new flags) |
| Example count | ~3 | ~20+ possible |

---

## Success Criteria

- [ ] Arbitrary text input supported
- [ ] Multiple overlays fully functional
- [ ] Each overlay has independent styling
- [ ] CLI is intuitive and easy to use
- [ ] Backward compatibility with old `-S`/`-G` flags
- [ ] Help text shows examples
- [ ] Test suite covers various overlay combinations
- [ ] Documentation clear and complete
- [ ] Performance unchanged

---

## Notes on Implementation

1. **Positional argument handling:** The key trick is tracking `current_overlay` in config as we parse flags. When a new `-O` is encountered, finalize the previous one.

2. **Color parsing:** Already exists (`overlay_parse_color()`) - supports hex and named colors.

3. **Placement parsing:** Already exists (`overlay_parse_placement()`) - supports all modes.

4. **Font size validation:** New - add range checking (8-96 pixels recommended).

5. **Memory management:** Each overlay spec is copied into the list, so temporary pointers are fine.

6. **Help text:** Will be significantly longer but more useful.

7. **Test expectations:** Much more comprehensive - multiple combinations to test.

---

## Conclusion

This redesign provides:
- ✅ **Flexibility** - Support any text, unlimited overlays
- ✅ **Usability** - Simple flags, intuitive order
- ✅ **Extensibility** - Easy to add new per-overlay options
- ✅ **Compatibility** - Old `-S`/`-G` flags still work
- ✅ **Power** - Support complex scenarios (contest overlays, metadata)

Ready to implement when approved.
