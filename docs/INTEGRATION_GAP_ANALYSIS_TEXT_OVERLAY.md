# Integration Gap Analysis: Text Overlay Module

## Problem Summary

The text overlay module (Phase 2.4) was successfully implemented with:
- ✅ [src/image/image_text_overlay.c](src/image/image_text_overlay.c) - Complete implementation (336 lines)
- ✅ [src/include/image/image_text_overlay.h](src/include/image/image_text_overlay.h) - Full API (110 lines)
- ✅ Module compiles cleanly, links correctly, no errors
- ✅ Test suite enhanced with infrastructure for testing

**However:** The text overlay module is **COMPLETELY UNUSED** in the actual image processing pipeline.

## Root Cause Analysis

### 1. Architecture Mismatch

**Phase 2 Module Structure (Low-Level):**
```
src/image/
├── image_loader.c      ← Phase 2.1: Load images from disk
├── image_processor.c   ← Phase 2.2: Scale, crop, embed
├── image_aspect.c      ← Phase 2.3: Aspect ratio correction
└── image_text_overlay.c ← Phase 2.4: COLOR BARS ONLY (never called!)
```

**High-Level Interface (`pisstvpp2_image.c`):**
```c
int image_load_from_file()              // ✅ Calls image_loader
int image_correct_aspect_and_resize()   // ✅ Calls image_aspect
// ❌ NO FUNCTION CALLS image_text_overlay
```

**Main Pipeline (pisstvpp2.c → pisstvpp2_image.c):**
```
Load Image
    ↓
Aspect Correct ← applies image_aspect
    ↓
Encode to SSTV ← SKIP: No text_overlay call!
    ↓
Write Audio
```

### 2. Configuration Not Extended

**Current PisstvppConfig (pisstvpp2_config.h):**
```c
typedef struct {
    char input_file[...];
    char protocol[...];
    // ... other fields ...
    int cw_enabled;                 // ✅ CW signature has config support
    char cw_callsign[...];
    // ❌ NO overlay_enabled field
    // ❌ NO station_id field
    // ❌ NO grid_square field
} PisstvppConfig;
```

**Result:** No way to pass overlay parameters from CLI to processing pipeline

### 3. CLI Not Extended

**Current Options (pisstvpp2.c help text):**
```
-i <file>       ✅ Input image
-o <file>       ✅ Output file
-p <protocol>   ✅ SSTV protocol
-f <fmt>        ✅ Audio format
-r <rate>       ✅ Sample rate
-a <mode>       ✅ Aspect ratio
-C <callsign>   ✅ CW signature
-W <wpm>        ✅ CW WPM
-T <freq>       ✅ CW tone
-K              ✅ Keep intermediate images
// ❌ NO OPTIONS FOR TEXT OVERLAY
```

**Missing:** `-O` flag, `-S` flag (station ID), `-G` flag (grid square), etc.

### 4. Test Suite Gap

**Current Test Suite (`tests/test_suite.py`):**
- ✅ Creates test directories
- ✅ Saves reference images
- ✅ Saves metadata JSON
- ❌ **Never actually calls text overlay functions**
- ❌ **Never generates output images with overlays**
- ❌ **Tests validate infrastructure, not functionality**

### 5. Image Module Missing Orchestration

**What `pisstvpp2_image.h` exports:**
```c
int image_load_from_file()              // Uses image_loader
int image_get_dimensions()
int image_get_pixel_rgb()
int image_correct_aspect_and_resize()   // Uses image_aspect
int image_save_to_file()
// ❌ NO image_apply_text_overlay()
// ❌ NO image_apply_color_bar()
// ❌ NO image_apply_station_id()
```

## Visual Pipeline Comparison

### Current (Non-Functional) Flow:
```
Load      Aspect         Encode
 ↓          ↓              ↓
[IMG] → [CORRECT] → [NO OVERLAY] → [SSTV] → [WAV]
```

### Expected Flow (After Integration):
```
Load      Aspect       Overlay        Encode
 ↓          ↓            ↓               ↓
[IMG] → [CORRECT] → [ADD TEXT/BARS] → [SSTV] → [WAV]
```

## What Needs to Be Done

### Phase 3A: Configuration Extension (CRITICAL)

**File: `src/include/pisstvpp2_config.h`**
- Add `overlay_enabled` flag (0/1)
- Add `overlay_mode` enum (COLOR_BAR, STATION_ID, BOTH)
- Add `station_id_callsign` field
- Add `station_id_grid_square` field
- Add `overlay_placement` enum (TOP, BOTTOM, LEFT, RIGHT, CENTER)

**File: `src/pisstvpp2_config.c`**
- Add CLI option parsing for `-O` (enable overlay)
- Add CLI option parsing for `-S <callsign>` (station ID)
- Add CLI option parsing for `-G <grid>` (grid square)
- Add validation for overlay parameters
- Update help text with overlay options

### Phase 3B: Image Pipeline Integration (CRITICAL)

**File: `src/include/pisstvpp2_image.h`**
- Add function: `image_apply_text_overlay()` that:
  - Wraps image_text_overlay module
  - Takes overlay config parameters
  - Returns updated VipsImage with overlay applied

**File: `src/pisstvpp2_image.c`**
- Import image_text_overlay.h
- Implement `image_apply_text_overlay()` orchestration
- Call this function after aspect correction, before SSTV encoding
- Pass VipsImage through overlay pipeline

### Phase 3C: Main Pipeline Integration (CRITICAL)

**File: `src/pisstvpp2.c` (main function)**
- After `image_correct_aspect_and_resize()` completes
- Before `sstv_encode_frame()` starts
- Add conditional call to `image_apply_text_overlay()` if `config.overlay_enabled`

### Phase 3D: Test Suite Update (IMPORTANT)

**File: `tests/test_suite.py`**
- Update tests to call pisstvpp2 WITH overlay options: `-O -S W5ZZZ -G EM12ab`
- Capture output files
- Compare against expected dimensions
- Eventually: image comparison tools to verify overlay placement

**Verification:**
- Instead of just copying reference images
- Actually run commands like:
  ```bash
  ./pisstvpp2 -i image.jpg -O -S W5ZZZ -G EM12ab -o test_overlay_output.wav
  ```
- Verify output .wav file is present and non-empty

## Current Functionality Analysis

| Component | Status | Reason |
|-----------|--------|--------|
| Text overlay C module | ✅ Works | Compiles, links, functions exist |
| High-level image interface | ❌ Doesn't call it | No function exported from pisstvpp2_image.h |
| Configuration system | ❌ No fields | PisstvppConfig has no overlay options |
| CLI options | ❌ No flags | Help text/getopt don't mention overlay |
| Main pipeline | ❌ Skips it | pisstvpp2.c never invokes overlay |
| Test infrastructure | ⚠️ Misleading | Tests PASS but don't verify actual overlays |

## Integration Checklist

- [ ] Extend PisstvppConfig struct with overlay fields
- [ ] Add CLI option parsing for -O, -S, -G flags
- [ ] Add validation for overlay parameters
- [ ] Export image_apply_text_overlay() from pisstvpp2_image.h
- [ ] Implement text overlay invocation in pisstvpp2_image.c
- [ ] Call image_apply_text_overlay() in pisstvpp2.c main pipeline
- [ ] Update help text with overlay option documentation
- [ ] Create working test cases that actually generate overlays
- [ ] Update test suite to verify overlay output
- [ ] Document expected behavior and examples

## Estimated Effort

- Configuration extension: 45-60 minutes
- Image pipeline integration: 30-45 minutes
- Main pipeline integration: 15-20 minutes
- Test suite updates: 30-45 minutes
- **Total: 2-3 hours for complete working integration**

## Files Requiring Modification

**Must Modify:**
1. `src/include/pisstvpp2_config.h` - Add overlay config fields
2. `src/pisstvpp2_config.c` - Add CLI parsing for overlay options
3. `src/include/pisstvpp2_image.h` - Export overlay functions
4. `src/pisstvpp2_image.c` - Implement overlay orchestration
5. `src/pisstvpp2.c` - Call overlay in main pipeline
6. `tests/test_suite.py` - Generate actual overlay outputs

**Already Complete:**
- `src/image/image_text_overlay.c` - No changes needed
- `src/include/image/image_text_overlay.h` - No changes needed

## Conclusion

The text overlay module is feature-complete and correct. The issue is that **it simply isn't connected to the rest of the system**. This is a straightforward integration task that requires:

1. Wiring configuration through the system
2. Wiring function calls through the pipeline
3. Updating tests to verify the actual behavior

Once these integration points are connected, the user will see text overlays appearing on the output images.
