# Phase 2: Architecture Refactoring - Readiness Summary

**Status:** 🟢 READY FOR IMPLEMENTATION  
**Date:** February 11, 2026  
**Phase 1 Final Test Result:** 54/55 tests passing ✅  
**Architecture:** Modular structure prepared and ready

---

## Phase 2 Overview

Phase 2 refactors the monolithic image processing module into specialized, composable components while adding a new text overlay feature for FCC compliance.

**Timeline:** 24-26 hours (4 tasks)  
**Target:** Complete modular refactoring + text overlay feature  

---

## ✅ Completed: Phase 2.0 - Architecture Setup

### Directory Structure Created
```
src/image/                          (new directory)
├── image_loader.c                  (to be created)
├── image_processor.c               (to be created)
├── image_aspect.c                  (to be created)
└── image_text_overlay.c            (to be created)

src/include/image/                  (new directory)
├── image_loader.h                  ✅ Created
├── image_processor.h               ✅ Created
├── image_aspect.h                  ✅ Created
└── image_text_overlay.h            ✅ Created
```

### Header Files: Complete API Design
All four modular components have fully designed APIs ready for implementation:

#### 1. **image_loader.h** ✅
- `image_loader_load_image()` - Load and buffer image from disk
- `image_loader_free_buffer()` - Clean up allocated buffers
- `image_loader_get_vips_image()` - Get VipsImage for advanced operations
- `image_loader_vips_to_buffer()` - Convert VipsImage → RGB buffer

**Lines in concept:** ~150-180  
**Responsibility:** File I/O, format detection, color space normalization

#### 2. **image_processor.h** ✅
- `image_processor_to_rgb()` - Ensure sRGB color space
- `image_processor_scale()` - Resize to dimensions
- `image_processor_crop()` - Extract rectangular region
- `image_processor_embed()` - Pad with black borders
- `image_processor_get_pixel_rgb()` - Sample pixel values
- `image_processor_get_dimensions()` - Query image size

**Lines in concept:** ~120-150  
**Responsibility:** Basic image transformations, pixel access

#### 3. **image_aspect.h** ✅
- `image_aspect_correct()` - Apply aspect ratio correction (3 modes)
- `image_aspect_get_name()` - Stringify mode names
- `image_aspect_parse()` - Parse mode from string

**Lines in concept:** ~180-220  
**Responsibility:** Center-crop, padding, stretch algorithms

#### 4. **image_text_overlay.h** ✅
- `image_text_overlay_create_config()` - Create default config
- `image_text_overlay_apply()` - Apply text with styling
- `image_text_overlay_add_color_bar()` - Bar with text label
- `image_text_overlay_add_station_id()` - FCC-compliant station ID

**Lines in concept:** ~250-350  
**Responsibility:** Text rendering, color bars, station identification

---

## ⏳ Prepared: Phase 2.1-2.4 Implementation Plan

### Task 2.1: Image Loader Extraction (5-6 hours)
**Status:** 🟡 READY - API designed, waiting implementation  
**Code source:** Extract from `src/pisstvpp2_image.c` lines:
- Image loading setup (VIPS_INIT, vips_image_new)
- Format detection
- Error handling for missing/corrupt files
- Buffer allocation and management

**Deliverable:** `src/image/image_loader.c` (~150-180 lines)

---

### Task 2.2: Image Processor Extraction (4-5 hours)
**Status:** 🟡 READY - API designed, waiting implementation  
**Code source:** Extract from `src/pisstvpp2_image.c` lines:
- Color space conversion (vips_colourspace)
- Scaling operations (vips_resize)
- Crop extraction (vips_crop)
- Embedding/padding (vips_embed)

**Deliverable:** `src/image/image_processor.c` (~120-150 lines)

---

### Task 2.3: Aspect Ratio Extraction (5-6 hours)
**Status:** 🟡 READY - API designed, waiting implementation  
**Code source:** Extract from `src/pisstvpp2_image.c` lines:
- apply_center_crop_transformation() function
- apply_pad_transformation() function
- apply_stretch_transformation() function
- Dimension calculation helpers

**Deliverable:** `src/image/image_aspect.c` (~180-220 lines)

---

### Task 2.4: Text Overlay Feature (8-10 hours)
**Status:** 🟡 READY - API designed, implementation framework prepared  
**Code source:** NEW FEATURE (not in current codebase)

**Deliverable:** `src/image/image_text_overlay.c` (~250-350 lines)

**Features:**
- Text rendering with font selection
- Color bar support (top, bottom, left, right)
- Station ID formatting (K0ABC/EN96)
- Opacity/transparency controls
- FCC Part 97.113(a)(4) compliance documentation

---

## Testing Baseline

**Current Test Status:**
```
✅ Phase 1 Tests: 54/55 PASSING (98.2%)
├── Help and information tests
├── Error handling - missing arguments
├── Error handling - invalid input
├── SSTV protocol tests (7 modes)
├── Audio format tests (WAV, AIFF, OGG)
├── Sample rate tests (8000-48000 Hz)
├── Aspect ratio mode tests (center, pad, stretch)
├── CW signature tests
├── Combined option tests
└── Output file naming

❌ 1 Known Issue (pre-existing):
   "Auto .wav extension" - File not found with .wav ext
   (Not critical - explicit file naming works)
```

**Phase 2 Success Criteria:**
- ✅ All 54 existing tests still passing after refactoring
- ✅ All 3 aspect modes produce identical output to current
- ✅ No behavioral changes (byte-identical if possible)
- ✅ New text overlay tests (10-15 additional tests)
- ✅ Zero compiler warnings/errors

---

## Integration Points

### With Phase 1 Foundation
✅ **Error Codes:** All error codes (200-208, 800-802) already defined  
✅ **Config System:** Ready for text overlay CLI options  
✅ **Context Management:** State container prepared for text config  
✅ **Error Logging:** Unified error_log() used throughout  

### Backward Compatibility
✅ **API Compatibility:** Current `pisstvpp2_image.h` remains unchanged during refactoring  
✅ **Behavioral Compatibility:** Same results for all existing operations  
✅ **Test Compatibility:** All 55 tests run unchanged  

### Future Phases
✅ **Phase 3:** Dynamic mode registry (independent of image refactoring)  
✅ **Phase 4:** Optional MMSSTV integration (independent)  
✅ **Phase 5+:** All depend on stable image module  

---

## Dependency Graph

```
Phase 1 (COMPLETE) ✅
├─ Error codes
├─ Config system
└─ Context management
     ↓
Phase 2 (CURRENT) 🟡 SETUP COMPLETE, IMPLEMENTATION READY
├─ Task 2.1: Image Loader
├─ Task 2.2: Image Processor
├─ Task 2.3: Aspect Correction
└─ Task 2.4: Text Overlay (NEW FEATURE)
     ↓
Phase 3: Dynamic Mode Registry
Phase 4: Optional MMSSTV
Phase 5-8: Advanced features
```

---

## Implementation Path Forward

### Recommended Approach
1. ✅ Setup directory structure (DONE)
2. ✅ Design module APIs (DONE)
3. ⏳ Implement Task 2.1 (Image Loader) - ~6 hours
4. ⏳ Implement Task 2.2 (Image Processor) - ~5 hours
5. ⏳ Implement Task 2.3 (Aspect Correction) - ~6 hours
6. ⏳ Implement Task 2.4 (Text Overlay) - ~10 hours
7. ⏳ Integration testing - ~2 hours
8. ⏳ FCC compliance documentation - ~1 hour

### Checkpoints
- After 2.1: Verify image loading works
- After 2.2: Verify processing operations work
- After 2.3: Verify aspect modes produce identical output
- After 2.4: Verify text rendering and CLI integration
- Final: All 55 tests pass, visual validation complete

---

## Resource Requirements

**Build System:**
- Update `makefile` to compile new modules
- Add `-I src/include/image` to include paths
- Link new object files to binary

**Dependencies:**
- libvips (already required)
- No new external dependencies

**Header File Chain:**
```
pisstvpp2_image.h (facade)
├─ image/image_loader.h
├─ image/image_processor.h
├─ image/image_aspect.h
└─ image/image_text_overlay.h
     └─ error.h (for error codes)
```

---

## Known Challenges & Mitigations

| Challenge | Likelihood | Mitigation |
|-----------|-----------|-----------|
| Text rendering performance | Medium | Profile on Pi, cache glyphs |
| Font availability | Low | Ship default fonts config |
| Dimension math errors | Low | Comprehensive unit tests |
| vips API changes | Low | Version pin in makefile |

---

## Success Metrics

✅ **Functional:**
- All image formats load correctly
- All aspect modes work identically
- Text renders without artifacts
- Color bars fill correctly

✅ **Quality:**
- Zero compiler errors
- All 55+ tests passing
- No memory leaks
- No performance regression on Pi

✅ **Documentation:**
- API docs complete
- FCC compliance explained
- Usage examples provided

---

## Next Steps

**Ready to proceed with Task 2.1** (Image Loader Implementation)

Key files prepared:
- ✅ `src/include/image/image_loader.h` - API design complete
- ⏳ `src/image/image_loader.c` - Awaiting extraction from current code
- ✅ All error codes mapped (PISSTVPP2_ERR_IMAGE_*)

**Estimated Time to Task 2.1 Completion:** 6 hours

---

## Quick Reference

**Commands:**
```bash
# Verify structure
find src/image* -type f | sort

# Compile Phase 2
make clean && make all

# Run tests
python3 tests/test_suite.py --exe ./bin/pisstvpp2

# Find code to extract (example)
grep -n "buffer_vips_image" src/pisstvpp2_image.c
```

**Documentation:**
- Master Plan: [PISSTVPP2_v2_1_MASTER_PLAN.md](./PISSTVPP2_v2_1_MASTER_PLAN.md)
- Quick Start: [PHASE_2_STARTUP.md](./PHASE_2_STARTUP.md)
- Current Code: [src/pisstvpp2_image.c](../src/pisstvpp2_image.c)

---

## Sign-Off

**Phase 2 Architecture:** ✅ COMPLETE  
**Ready for Implementation:** ✅ YES  
**Baseline Tests:** ✅ PASSING (54/55)  
**Go-No-Go Decision:** 🟢 **GO** - Ready to implement Task 2.1

---

*Next update: After Task 2.1 completion*
