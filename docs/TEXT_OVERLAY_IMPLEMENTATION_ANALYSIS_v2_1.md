# Text Overlay Implementation - Critical Analysis & Task List
## PiSSTVpp2 v2.1 Status Review

**Analysis Date:** February 11, 2026  
**Status:** ⚠️ CRITICAL - Implementation Incomplete  
**Priority:** HIGH - Blocks v2.1 Release  

---

## Executive Summary

### Current Situation
The text overlay feature development has progressed significantly but remains **functionally incomplete**. While configuration infrastructure and test harnesses are in place, the actual text-to-image rendering pipeline is broken:

- ✅ **Configuration system** - Mostly complete (overlay_specs struct, list management)
- ✅ **CLI parsing** - Mostly complete (-O, -S, -G flags added)
- ✅ **Test framework** - Tests run but don't verify actual overlay output
- ❌ **Text rendering** - Text is rendered but NEVER composited onto image
- ❌ **Color support** - Only black placeholders, no RGB color rendering
- ❌ **Background rendering** - Infrastructure exists but implementation incomplete
- ❌ **Image integration** - Old image_text_overlay.c module is unused, wrong approach

### Root Cause
The implementation chose **two different approaches** that are now in conflict:

1. **Phase 2.4 Approach** (`src/image/image_text_overlay.c`) - Color bars module (placeholder)
2. **Recent Approach** (`src/overlay_spec.c` + config integration) - Text overlay with specs

These approaches aren't properly integrated, resulting in:
- `image_text_overlay.c` module fully compiled but unused
- `apply_single_overlay()` in `pisstvpp2_image.c` creates text but doesn't place it
- No actual text visible in output images
- Tests pass but feature doesn't work

### Timeline Impact
**Critical:** Text overlay **must be fixed before v2.1 release** for FCC Part 97 compliance.
- Feature is documented as complete but non-functional
- User-facing command-line flags (-S, -G) exist but don't produce expected results
- Test infrastructure passes but doesn't validate actual functionality

---

## Detailed Technical Analysis

### 1. Configuration Architecture Status

#### ✅ Completed
- `overlay_spec.h` - Comprehensive spec structure (327 lines)
- `overlay_spec.c` - List management fully implemented (299 lines)
- `pisstvpp2_config.h` - Config struct includes `OverlaySpecList overlay_specs` field
- `pisstvpp2_config.c` - CLI parsing for -O, -S, -G flags implemented

#### ⚠️ Issues
- Configuration parsing works but parameters not validated for conflicts
- No default overlay creation when -S/-G flags used
- overlay_specs list initialized but no helper to create specs from CLI args

#### 📝 Code Example
```c
// In pisstvpp2_config.c line ~180-200:
// CLI parsing works:
case 'O':
    config->overlay_enabled = 1;  // ✅ Flag parsing works
case 'S':
    // ✅ Station ID parsing works
    strncpy(config->cw_callsign, optarg, CONFIG_MAX_CALLSIGN);
case 'G':
    // Grid square parsing - need to add to config struct
    // ❌ No field for grid_square in PisstvppConfig!
```

### 2. Main Pipeline Status

#### Flow Chart (Current - Broken)
```
main (pisstvpp2.c)
  ↓
image_load_from_file()  ✅ Works
  ↓
image_correct_aspect_and_resize()  ✅ Works
  ↓
image_apply_overlay_list()  ❌ BROKEN HERE!
  └─→ apply_single_overlay() for each spec
      ├─ vips_text() renders text ✅
      ├─ Calculates position ✅
      ├─ Creates text_image ✅
      ├─ vips_text creates new image ✅
      └─ g_object_unref(text_image) ❌ NEVER COMPOSITED!
```

#### Problem Code (pisstvpp2_image.c lines 590-640)
```c
// Line 595-600: Text rendering works
if (vips_text(&text_image, spec->text, 
             "font", font_str,
             "rgba", TRUE,
             NULL)) {
    // error handling...
}

// Line 610-635: Position calculation works
switch (spec->placement) {
    case OVERLAY_PLACE_TOP:
        x_pos = (img_width - text_width) / 2;
        y_pos = 10;
        break;
    // ... other placements ...
}

// Line 638: CRITICAL BUG!
g_object_unref(text_image);  // ❌ DELETES TEXT WITHOUT USING IT!
return PISSTVPP2_OK;

// ✅ What's missing (should be between lines 635-638):
// VipsImage *composited = NULL;
// if (vips_insert(g_img.image, text_image, &composited, x_pos, y_pos, NULL)) {
//     error handling...
// }
// g_object_unref(g_img.image);
// g_img.image = composited;
// Update g_img.buffer with new image
```

### 3. Image Text Overlay Module Status

#### ⚠️ Unused Module
- **File:** `src/image/image_text_overlay.c` (336 lines)
- **Status:** Compiles cleanly, fully implemented... but NEVER CALLED
- **Reason:** Wrong approach (focuses on color bars instead of text overlays)

#### Module Functions (Not Used)
```c
image_text_overlay_create_config()      // Creates config
image_text_overlay_add_color_bar()      // Adds black bars
image_text_overlay_add_station_id()     // Adds ID bar
image_text_overlay_apply()              // Main function
```

#### Why Not Used
- Created colored bars (black only) for FCC compliance requirement
- Phase 2.4 deliverable focused on "color bars" not "text overlays"
- Later spec-based system implemented instead (overlay_spec.c)
- No integration point in main pipeline
- Function exported from `image_text_overlay.h` but not from `pisstvpp2_image.h`

---

## Impact Analysis: What's Broken

### 1. Text Overlay Output
**Current:** Text never appears on output images  
**Expected:** Station ID, grid square visible on image

**Evidence:**
```bash
# Command:
./pisstvpp2 -i photo.jpg -S W5ZZZ -G EM12ab -o output.wav

# Test suite shows PASSED but check image:
# - overlay spec parsed ✅
# - image_apply_overlay_list() called ✅
# - apply_single_overlay() called ✅
# - Text rendered ✅
# - Position calculated ✅
# - Text image DESTROYED without compositing ❌
# Result: No text in final image ❌
```

### 2. Color Support
**Current:** Only placeholder black color  
**Code:** Line 561 in apply_single_overlay() - hardcoded black font

```c
// Only mentions color in comments:
log_verbose(verbose, timestamp_logging,
           "      Rendering overlay text '%s' (font size: %d, color: blue)\n", 
           spec->text, font_size);
// But actual text rendered in black (vips_text default)
```

### 3. Module Fragmentation
**Issue:** Two separate text overlay implementations that don't work together:

**Set A: Low-level Color Bar Module (Phase 2.4)**
- `image_text_overlay.c/h` - Complete but unused
- Functions for bars, station ID formatting
- Intended for "low-level" image manipulation
- Problem: Only supports colored rectangles, not actual text

**Set B: High-level Spec System (Recent)**
- `overlay_spec.c/h` - Configuration and specs
- `pisstvpp2_config.c` - CLI and config integration
- Text rendering in `pisstvpp2_image.c`
- Problem: Compositing never happens

**Result:** Confusing codebase with incomplete implementations

---

## File-by-File Breakdown

### Critical Issues Found

#### 1. `src/pisstvpp2_image.c` - apply_single_overlay() [HIGH PRIORITY]
**Lines:** 553-640
**Issue:** Text rendered but not composited to image
**Status:** 90% complete, missing critical final step
**Fix:** Add vips_insert() call to composite text onto image

```c
// Missing implementation (pseudocode):
// After position calculation (line 635):
VipsImage *composited = NULL;
if (vips_insert(g_img.image, text_image, &composited, x_pos, y_pos, NULL)) {
    error_log(...);
    g_object_unref(text_image);
    return error;
}

// Replace old image
g_object_unref(g_img.image);
g_img.image = composited;

// Re-buffer with new image
int result = buffer_vips_image(composited, verbose, timestamp_logging);
if (result != PISSTVPP2_OK) {
    return result;
}
```

#### 2. `src/include/pisstvpp2_config.h` - PisstvppConfig struct [MEDIUM PRIORITY]
**Lines:** 127-148
**Issue:** Missing field for grid_square
**Current:**
```c
typedef struct {
    // ... existing fields ...
    char cw_callsign[CONFIG_MAX_CALLSIGN + 1];  // ✅ For CW
    // ❌ NO grid_square field!
    OverlaySpecList overlay_specs;  // Specs list added
} PisstvppConfig;
```

**Fix Needed:** Add grid_square field:
```c
char station_grid_square[CONFIG_MAX_GRID_SQUARE + 1];  // For overlay
```

#### 3. `src/pisstvpp2_config.c` - CLI parsing [MEDIUM PRIORITY]
**Lines:** 120-180 (getopt loop)
**Issue:** -S and -G flags parsed but not fully integrated
**Current:**
```c
case 'S':
    // ✅ Parses station ID
    strncpy(config->cw_callsign, optarg, CONFIG_MAX_CALLSIGN);
    // But: Should create overlay spec, not just set callsign!
case 'G':
    // ✅ Grid square parsing exists in help
    // But: No actual handler in getopt!
```

**Fix Needed:**
```c
case 'S':  // Station ID (for overlay, not just CW)
    // Create overlay spec from callsign + grid
    TextOverlaySpec spec = overlay_spec_create_station_id(
        optarg, 
        config->station_grid_square,  // After adding field
        OVERLAY_PLACE_BOTTOM
    );
    overlay_spec_list_add(&config->overlay_specs, &spec);
    
case 'G':  // Grid square
    strncpy(config->station_grid_square, optarg, CONFIG_MAX_GRID_SQUARE);
    // Update existing spec if already added
```

#### 4. `src/image/image_text_overlay.c` [DECISION NEEDED]
**Status:** Unused but complete module
**Lines:** 336
**Decision Required:** 
- Keep as low-level utility? (Might be useful for future color bar features)
- Remove entirely? (Current approach uses specs, not this module)
- Refactor to integrate with spec system?

**Recommendation:** Mark for future refactoring (Phase 2.5+) but remove from main pipeline for now

#### 5. `src/include/pisstvpp2_image.h` [LOW PRIORITY]
**Missing export:**
```c
// Should add this function signature:
/**
 * image_apply_overlay_list - Apply text overlays to current image
 * @param overlay_specs List of overlay specifications
 * @param verbose Debug output
 * @param timestamp_logging Add timestamps
 * @return Error code
 */
int image_apply_overlay_list(const OverlaySpecList *overlay_specs,
                            int verbose, int timestamp_logging);
```

---

## Integration Gaps

### Gap 1: Configuration Not Connected to Overlay Generation

**Current Flow:**
```
CLI: -S W5ZZZ -G EM12ab
  ↓
Parsed into: cw_callsign = "W5ZZZ", ??? = "EM12ab"
  ↓
overlay_specs list = EMPTY ❌
  ↓
image_apply_overlay_list() called with EMPTY list
  ↓
No overlays applied
```

**Expected Flow:**
```
CLI: -S W5ZZZ -G EM12ab
  ↓
Parsed into: station_id = "W5ZZZ", grid = "EM12ab"
  ↓
Create spec: overlay_spec_create_station_id("W5ZZZ", "EM12ab", BOTTOM)
  ↓
Add to overlay_specs list ✅
  ↓
image_apply_overlay_list() called with populated list
  ↓
Overlays applied and composited ✅
```

### Gap 2: No Color Rendering System

**Current:**
- Spec system has RGB color fields (text_color, bg_color, border_color)
- But apply_single_overlay() ignores them
- vips_text() called without color control
- No background rectangle rendering

**Missing:**
```c
// In apply_single_overlay():
// 1. Render background rectangle (bg_color)
// 2. Render text with specified color (text_color)
// 3. Render optional border (border_color)
// 4. Composite all layers in correct order
```

### Gap 3: Module Linkage Incomplete

**image_text_overlay.h** exported from src/include but:
- Never imported in pisstvpp2_image.c
- Never called from main pipeline
- Functions like `image_text_overlay_create_config()` unused
- Creates confusion about which system to use

---

## Legacy Code Cleanup Required

### Issue: `/src/legacy/` Directory
**Status:** Contains old pisstvpp and pifm_sstv (pre-v2.0)
**Impact:** Code fragmentation, confusion about which version to use
**Action:** Should be marked as archived/deprecated

**Files:**
- `legacy/pisstvpp.c` - Original SSTV encoder (v1.0)
- `legacy/pifm_sstv.c` - Original FM transmitter
- `legacy/Makefile.legacy` - Old build system
- `legacy/build_legacy.sh` - Legacy build script

**Recommendation:**
1. Keep for reference (useful for historical comparison)
2. Move to separate `/archive/` or `/deprecated/` directory
3. Document in README that v2.x is the current version
4. Remove from main source compilation

---

## Proposed Fix Strategy

### Phase A: Critical Fixes (Must Do Before v2.1)

#### A1. Fix Text Compositing [CRITICAL - 1-2 hours]
**File:** `src/pisstvpp2_image.c`
**Task:** Implement vips_insert() to composite text_image onto g_img.image
**Impact:** Enable text to actually appear on images

#### A2. Add Grid Square Config Field [CRITICAL - 30 min]
**File:** `src/include/pisstvpp2_config.h`
**Task:** Add `station_grid_square` field to PisstvppConfig struct
**Impact:** Allow grid square to be passed through CLI to overlay system

#### A3. Complete CLI Integration [CRITICAL - 1-2 hours]
**File:** `src/pisstvpp2_config.c`
**Task:** 
- Add -G flag handler in getopt loop
- Create overlay spec from -S and -G flags
- Add to overlay_specs list automatically
**Impact:** CLI flags (-S, -G) produce actual overlays

#### A4. Color Rendering System [CRITICAL - 2-3 hours]
**File:** `src/pisstvpp2_image.c` - apply_single_overlay()
**Task:**
- Implement vips_colourspace/vips_linear for RGB colors
- Render background rectangle with spec->bg_color
- Render border rectangle with spec->border_color
- Render text with spec->text_color (not hardcoded)
**Impact:** Support full color specification

#### A5. Test Validation [CRITICAL - 1-2 hours]
**File:** `tests/test_suite.py`
**Task:**
- Update tests to actually call pisstvpp2 with -S/-G flags
- Capture output images
- Verify overlays present in output
**Impact:** Verify feature actually works

### Phase B: Code Cleanup (Should Do Before v2.1)

#### B1. Document/Archive Legacy Code [1 hour]
**Action:** Create `/docs/LEGACY_STATUS.md` explaining:
- Why legacy code is kept
- That v2.x is current version
- When legacy might be useful

#### B2. Remove Unused image_text_overlay Module [30 min]
**Action:** Either:
- Delete if truly unused, OR
- Document why it's kept and mark for future use
- Remove from main compilation if not used

**Decision:** Based on user preference

#### B3. Consolidate Documentation [1-2 hours]
**Action:** Update master plan to reflect actual current state

### Phase C: Enhancements (Nice to Have for v2.1+)

#### C1. Full Alpha Transparency [2-3 hours]
Currently spec system has opacity field but not used

#### C2. Multiple Overlay Styles [2 hours]
Support bar background + text overlay (like original color bar system)

#### C3. Font Selection [1-2 hours]
Use spec->font_family field instead of hardcoded "sans"

---

## Validation Checklist

Before v2.1 release, must have:

- [ ] Station ID overlays actually appear on output images
- [ ] Grid square displays next to callsign
- [ ] Custom colors render correctly (not black)
- [ ] Background color renders correctly
- [ ] All placement modes work (top, bottom, left, right, center)
- [ ] Multiple overlays can be applied in sequence
- [ ] Tests verify by comparing actual image output
- [ ] CLI flags (-S, -G) produce expected results
- [ ] Error handling for invalid overlay specifications
- [ ] Documentation updated with overlay examples

---

## FCC Compliance Notes

**FCC Part 97 Section 97.113(a)(4) Requirement:**
> Slow scan television/video transmissions are not permitted to show any callsign other than that of the station licensee.

**Current Implementation Status:**
- ✅ Configuration system supports FCC-required callsign + grid format
- ✅ Spec system designed for "CALLSIGN GRID" format
- ❌ Feature not working - overlays don't appear on images

**Fix Required:**
Once text compositing is implemented, ham radio operators will be able to comply with FCC rules by including station ID and grid square on every SSTV transmission.

---

## Summary of Changes Needed

| File | Lines | Issue | Severity | Est. Time |
|------|-------|-------|----------|-----------|
| `pisstvpp2_image.c` | 590-640 | Text not composited | CRITICAL | 1-2h |
| `pisstvpp2_config.h` | 127-148 | Missing grid_square field | CRITICAL | 0.5h |
| `pisstvpp2_config.c` | 120-180 | -G flag not handled, no spec creation | CRITICAL | 1-2h |
| `pisstvpp2_image.c` | 553-640 | Color rendering not implemented | CRITICAL | 2-3h |
| `tests/test_suite.py` | various | Tests don't verify output | CRITICAL | 1-2h |
| `src/legacy/` | all | Should be archived | MEDIUM | 1h |
| `docs/PISSTVPP2_v2_1_MASTER_PLAN.md` | various | Needs status update | MEDIUM | 2h |
| `image_text_overlay.c/h` | 336/110 | Unused, may cause confusion | LOW | 0.5h |

**Total Estimated Time:** 9-13 hours for critical fixes

---

## Master Plan Status Update Required

Current v2.1 master plan states:
- Phase 2.4 (Text Overlay) - COMPLETE ✅

**Actual Status:**
- Phase 2.4 - 40% COMPLETE (configuration infrastructure in place, actual rendering broken)

---

## Appendix: Current Code Architecture

### Module Dependencies
```
main (pisstvpp2.c)
  ├─ pisstvpp2_config.h/c
  │  └─ overlay_spec.h/c ✅
  ├─ pisstvpp2_image.h/c
  │  ├─ image_loader.h/c ✅
  │  ├─ image_processor.h/c ✅
  │  ├─ image_aspect.h/c ✅
  │  ├─ image_text_overlay.h/c ❌ (unused)
  │  └─ overlay_spec.h ✅
  ├─ pisstvpp2_sstv.h/c ✅
  └─ audio_encoder_*.h/c ✅
```

### Text Overlay Pipeline (Current - Broken)
```
input: -S W5ZZZ -G EM12ab
  ↓
config_parse() ✅
  ├─ cw_callsign = "W5ZZZ" ✅
  └─ grid_square = MISSING ❌
  ↓
overlay_specs list = [] ❌ (empty, should have 1 spec)
  ↓
image_apply_overlay_list([]) called with empty list ✅
  ↓
for each spec: apply_single_overlay() ❌
  ├─ vips_text() creates text_image ✅
  ├─ Calculate position ✅
  ├─ text_image unreffed/deleted ❌ WITHOUT being composited
  └─ Return OK (but text lost)
  ↓
output: image with NO overlays ❌
```

---

## Conclusion

The text overlay feature is **incomplete but recoverable** with focused work on:

1. **Text compositing** - The critical missing piece (renders but doesn't place)
2. **Configuration integration** - Grid square field and CLI handling
3. **Color support** - Full RGB rendering instead of placeholders
4. **Testing** - Validation of actual image output

**Estimated total effort:** 9-13 hours for critical path to working feature

**Blockers for v2.1 release:** Text overlays must be functional and tested before release
