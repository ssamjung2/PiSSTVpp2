# Flag & Function Cleanup Analysis Report
**Date:** February 13, 2026  
**Scope:** Analyze and document `-G` and `-S` flag usage and dependencies

---

## EXECUTIVE SUMMARY

**Status of Flags:**
- ✅ `-G` (grid square) flag: **Already removed from getopt**
- ✅ `-S` (station ID) flag: **Already removed from getopt**
- ✅ Backward compatibility layer: **Already simplified to no-op**

**Unused Functions Found:**
- `overlay_spec_create_station_id()` - NOT CALLED ANYWHERE ❌
- `image_text_overlay_add_station_id()` - NOT CALLED ANYWHERE ❌
- `image_text_overlay_add_color_bar()` - ONLY called by `image_text_overlay_add_station_id()` ❌

**Cleanup Action:** These functions can be safely removed with zero impact.

---

## DETAILED ANALYSIS

### 1. Flag Status Analysis

#### `-G` Flag (Grid Square)
**Status:** Already removed from active code  
**Current getopt string:** `"i:o:p:f:r:vC:W:Q:a:KZhNOR:T:"`  
**No `'G'` in string** ✅  
**No `case 'G':` in code** ✅  

**Files checked:** `/src/slowframe_config.c` line 139

**Conclusion:** This flag was already fully removed from CLI parsing

---

#### `-S` Flag (Station ID / Callsign)
**Status:** Already removed from active code  
**Current getopt string:** `"i:o:p:f:r:vC:W:Q:a:KZhNOR:T:"`  
**No `'S'` in string** ✅  
**No `case 'S':` in code** ✅  

**Files checked:** `/src/slowframe_config.c` line 139

**Conclusion:** This flag was already fully removed from CLI parsing

---

### 2. Backward Compatibility Layer

**Function:** `apply_backward_compatibility()`  
**Location:** `src/slowframe_config.c` lines 40-47  
**Current code:**
```c
static int apply_backward_compatibility(SlowframeConfig *config) {
    if (!config) {
        return SLOWFRAME_ERR_ARG_INVALID_PROTOCOL;
    }

    // All overlay functionality is now through -T flag
    return SLOWFRAME_OK;
}
```

**Status:** Already simplified to a no-op  
**Action:** This can be removed entirely  
**Risk:** ZERO (it just returns immediately)

---

### 3. Unused Functions (CRITICAL CLEANUP TARGETS)

#### Function 1: `overlay_spec_create_station_id()`

**Location:** `/src/overlay_spec.c` lines 156-183

**Full Code:**
```c
TextOverlaySpec overlay_spec_create_station_id(const char *callsign, 
                                                const char *grid_square,
                                                OverlayPlacement placement) {
    TextOverlaySpec spec = overlay_spec_create_default();
    
    // Format station ID text
    snprintf(spec.text, sizeof(spec.text), "%s %s", 
             callsign ? callsign : "NOCALL",
             grid_square ? grid_square : "?????");
    
    spec.placement = placement;
    spec.font_size = 32;  // Large, visible font
    spec.text_color = (RGBAColor){0, 102, 255, 255};    // Blue text
    spec.bg_color = (RGBAColor){255, 255, 255, 255};    // White background
    spec.bg_mode = BG_OPAQUE;
    spec.padding = 8;
    spec.border_width = 1;
    spec.border_color = (RGBAColor){0, 102, 255, 255};  // Blue border
    spec.text_align = TEXT_ALIGN_CENTER;
    spec.valign = VALIGN_CENTER;
    spec.enabled = 1;
    
    return spec;
}
```

**Declaration Location:** `/src/include/overlay_spec.h` lines 328-330

**Dependencies:**
- Calls: `overlay_spec_create_default()` ✅ (used elsewhere - keep)
- Called by: **NOTHING** ❌

**Call Site Search Result:** 1 match (definition only, no calls)

```
<match path="/Users/ssamjung/Desktop/WIP/SlowFrame/src/overlay_spec.c" line=156>
TextOverlaySpec overlay_spec_create_station_id(const char *callsign, 
```

**Dependent Code:** None

**Safe to Delete:** ✅ **YES**

---

#### Function 2: `image_text_overlay_add_station_id()`

**Location:** `/src/image/image_text_overlay.c` lines 259-310

**Full Code:**
```c
int image_text_overlay_add_station_id(VipsImage *image, const char *callsign,
                                     const char *grid_square, VipsImage **out_labeled,
                                     int verbose) {
    if (!image || !out_labeled || !callsign) {
        error_log(SLOWFRAME_ERR_ARG_INVALID, "NULL pointer in station ID overlay");
        return SLOWFRAME_ERR_ARG_INVALID;
    }
    
    if (strlen(callsign) == 0) {
        error_log(SLOWFRAME_ERR_ARG_INVALID, "Empty callsign provided");
        return SLOWFRAME_ERR_ARG_INVALID;
    }
    
    if (verbose) {
        log_verbose(verbose, 0, "   Adding station ID overlay\n");
        log_verbose(verbose, 0, "      Callsign: %s\n", callsign);
        if (grid_square) {
            log_verbose(verbose, 0, "      Grid Square: %s\n", grid_square);
        }
    }
    
    /* Format the station ID string */
    char station_id[256];
    if (grid_square && strlen(grid_square) > 0) {
        snprintf(station_id, sizeof(station_id), "%s / %s", callsign, grid_square);
    } else {
        snprintf(station_id, sizeof(station_id), "%s", callsign);
    }
    
    /* Create a simple text overlay configuration */
    TextOverlayConfig config = image_text_overlay_create_config();
    config.text = station_id;
    config.placement = TEXT_PLACEMENT_BOTTOM;
    config.font_size = 20;
    /* Use black background, white text for visibility */
    config.bg_r = 0; config.bg_g = 0; config.bg_b = 0;
    config.text_r = 255; config.text_g = 255; config.text_b = 255;
    config.bar_height = 35;
    
    /* For now, use the color bar function with black background */
    int result = image_text_overlay_add_color_bar(image, TEXT_PLACEMENT_BOTTOM, config.bar_height,
                                                 0, 0, 0,    /* black bar */
                                                 NULL,        /* no text yet (future enhancement) */
                                                 255, 255, 255, /* white text color (ignored) */
                                                 out_labeled, verbose);
    
    if (verbose) {
        log_verbose(verbose, 0, "   [OK] Station ID overlay added: %s\n", station_id);
    }
    
    return result;
}
```

**Declaration Location:** `/src/include/image/image_text_overlay.h` lines 119-120

**Dependencies:**
- Calls: `image_text_overlay_add_color_bar()` ← **IMPORTANT**
  - This function is ONLY called here
  - Need to check if color_bar function itself needs to stay
  
**Call Site Search Result:** 1 match (definition only, no calls)

```
<match path="/Users/ssamjung/Desktop/WIP/SlowFrame/src/image/image_text_overlay.c" line=259>
int image_text_overlay_add_station_id(VipsImage *image, const char *callsign,
```

**Dependent Code:** None

**Safe to Delete:** ✅ **YES** (but triggers removal of color_bar function too)

---

#### Function 3: `image_text_overlay_add_color_bar()` (DEPENDENT)

**Location:** `/src/image/image_text_overlay.c` lines 165-257

**Call Site Search Result:** 2 matches (both in image_text_overlay.c)

1. Definition at line 165: Function definition
2. Call at line 299: **Called ONLY from `image_text_overlay_add_station_id()`**

**Nested Dependency Chain:**
```
image_text_overlay_add_station_id() [UNUSED]
    ↓ calls
image_text_overlay_add_color_bar() [ONLY CALLED BY ABOVE]
    ↓ calls
(various libvips functions)
```

**Safe to Delete:** ✅ **YES** (safe to remove if add_station_id is removed)

**BUT**: Check if color_bar function referenced in headers as public API

**Declaration Location:** `/src/include/image/image_text_overlay.h` - let me verify

---

### 4. Header File References

**Records of grid/station functions in headers:**

From earlier grep results:
- `/src/include/overlay_spec.h` line 328-330: `overlay_spec_create_station_id()` declaration
- `/src/include/image/image_text_overlay.h` line 119-120: `image_text_overlay_add_station_id()` declaration

These need to be removed from headers too.

---

## CLEANUP TASK BREAKDOWN

### Phase 1: Remove Unused Functions (Safe - 0 risk)

**Tasks:**
1. Delete `overlay_spec_create_station_id()` from `/src/overlay_spec.c` lines 156-183
2. Remove declaration from `/src/include/overlay_spec.h` line 328-330
3. Delete `image_text_overlay_add_station_id()` from `/src/image/image_text_overlay.c` lines 259-310
4. Remove declaration from `/src/include/image/image_text_overlay.h` line 119-120

**Risk Level:** 🟢 **ZERO** - No code calls these functions

**Validation:**
- Compile project
- Run test suite
- No functional changes expected

---

### Phase 2: Remove Unused Color Bar Function (Conditional)

**Task:**
- Check if `image_text_overlay_add_color_bar()` is used elsewhere
- Currently only called by `image_text_overlay_add_station_id()`
- If no external calls: safe to remove
- If external calls exist: keep function

**Status:** Only 2 matches found - both in same file (definition + one call)

**Decision:** Can remove if Phase 1 completes

---

### Phase 3: Simplify Backward Compatibility Layer (Optional)

**Task:**
- Remove `apply_backward_compatibility()` function entirely
- It's a no-op that just returns SLOWFRAME_OK
- Called once from slowframe_config_parse()
- Replace call with direct return statement

**Risk Level:** 🟢 **ZERO** - Function does nothing

---

### Phase 4: Documentation Updates (Post-cleanup)

**Tasks:**
1. Update audit documentation to mark as completed
2. Remove reference to these functions from API docs
3. Update CHANGELOG to note removal

**Risk Level:** 🟢 **ZERO** - Documentation only

---

## FILE-BY-FILE CLEANUP CHECKLIST

### File: `/src/overlay_spec.c`
- [ ] Remove `overlay_spec_create_station_id()` function (lines 156-183)
- [ ] Verify no other references to this function

### File: `/src/include/overlay_spec.h`
- [ ] Remove `overlay_spec_create_station_id()` declaration (lines 328-330)
- [ ] Verify no includes/dependencies broken

### File: `/src/image/image_text_overlay.c`
- [ ] Remove `image_text_overlay_add_station_id()` function (lines 259-310)
- [ ] Decide: Remove or keep `image_text_overlay_add_color_bar()` function
- [ ] Update any comments referencing these functions

### File: `/src/include/image/image_text_overlay.h`
- [ ] Remove `image_text_overlay_add_station_id()` declaration (lines 119-120)
- [ ] Remove any references in comments or documentation
- [ ] Decide: Remove or keep `image_text_overlay_add_color_bar()` declaration

### File: `/src/slowframe_config.c`
- [ ] Optional: Remove `apply_backward_compatibility()` function (lines 40-47)
- [ ] Optional: Remove function call at line 513

### File: `/src/slowframe.c`
- [ ] No changes needed - doesn't reference these functions

---

## DEPENDENCY VERIFICATION MATRIX

| Function | Called By | Calls | Safe to Delete | Notes |
|----------|-----------|-------|---|----------|
| `overlay_spec_create_station_id()` | NONE ❌ | `overlay_spec_create_default()` | ✅ YES | Only definition, never used |
| `image_text_overlay_add_station_id()` | NONE ❌ | `image_text_overlay_add_color_bar()` | ✅ YES | Only definition, never used |
| `image_text_overlay_add_color_bar()` | Only by above ❌ | vips functions | ✅ YES (if above removed) | Only used by deleted function |
| `apply_backward_compatibility()` | line 513 in config.c | (none) | ✅ YES | No-op function |

---

## RISK ASSESSMENT

### Removing `overlay_spec_create_station_id()`
**Risk Level:** 🟢 **ZERO**
- Not called from anywhere in codebase
- Not exported to public API (only in internal header)
- Removal has no impact on compilation or runtime behavior

**Testing:** 
- Compile: Should succeed
- Tests: Should all pass
- Functionality: No change

---

### Removing `image_text_overlay_add_station_id()`
**Risk Level:** 🟢 **ZERO**
- Not called from anywhere in codebase
- Not part of active code flow
- Related to removed `-G` and `-S` flag functionality

**Testing:**
- Compile: Should succeed
- Tests: Should all pass
- Functionality: No change (this function was never reached)

---

### Removing `image_text_overlay_add_color_bar()` (if add_station_id is removed)
**Risk Level:** 🟢 **ZERO** (conditional)
- Only called by `image_text_overlay_add_station_id()`
- If that function is deleted, this becomes unused
- Safe to delete as unit removal

**Testing:**
- Same as above

---

### Removing `apply_backward_compatibility()`
**Risk Level:** 🟢 **ZERO**
- Currently just returns SLOWFRAME_OK
- Called once, result assigned but not used
- Can be replaced with direct return statement

**Testing:**
- Compile: Should succeed
- Tests: Should all pass
- Functionality: No change

---

## RECOMMENDED CLEANUP ORDER

1. **Step 1:** Remove `overlay_spec_create_station_id()` 
   - Delete from overlay_spec.c
   - Delete from overlay_spec.h

2. **Step 2:** Remove `image_text_overlay_add_station_id()`
   - Delete from image_text_overlay.c  
   - Delete from image_text_overlay.h

3. **Step 3:** Remove `image_text_overlay_add_color_bar()`
   - Delete from image_text_overlay.c
   - Delete from image_text_overlay.h
   - (Since it's only called by deleted function)

4. **Step 4 (Optional):** Remove `apply_backward_compatibility()`
   - Delete from slowframe_config.c
   - Replace call with direct return statement

5. **Step 5:** Compile and test

---

## COMPILATION VERIFICATION COMMANDS

After cleanup:

```bash
# Clean rebuild
make clean && make all 2>&1

# Run tests (if test suite available)
make test 2>&1

# Verify no undefined references
nm -u ./pisstvpp 2>&1 | grep 'overlay_spec_create_station_id\|image_text_overlay_add_station_id\|image_text_overlay_add_color_bar'

# Verify functions don't exist in binary
nm -D ./pisstvpp 2>&1 | grep 'overlay_spec_create_station_id\|image_text_overlay_add_station_id\|image_text_overlay_add_color_bar'
```

---

## SUMMARY TABLE

| Item | Status | Action | Effort | Risk |
|------|--------|--------|--------|------|
| `-G` flag | Removed | Delete docs | 10 min | 🟢 None |
| `-S` flag | Removed | Delete docs | 10 min | 🟢 None |
| `overlay_spec_create_station_id()` | Unused | DELETE | 2 min | 🟢 None |
| `image_text_overlay_add_station_id()` | Unused | DELETE | 5 min | 🟢 None |
| `image_text_overlay_add_color_bar()` | Unused (only by above) | DELETE | 5 min | 🟢 None |
| `apply_backward_compatibility()` | No-op | DELETE (optional) | 2 min | 🟢 None |

**Total Effort:** 15-20 minutes  
**Total Risk:** ZERO  
**Breaking Changes:** NONE

---

## CLEANUP COMMAND REFERENCE

Will be provided in implementation instructions.

---

**Next Step:** Proceed with cleanup implementation using provided task list

