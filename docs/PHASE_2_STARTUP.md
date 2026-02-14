# Phase 2: Image Module Refactoring - Startup Guide

**Date Started:** February 11, 2026  
**Phase Duration:** 24-26 hours (4 tasks)  
**Completion Target:** Week 2 of development  
**Status:** 🟢 READY TO BEGIN

---

## Executive Summary

Phase 2 involves major refactoring of the image processing module plus implementation of a new text overlay feature with color bars. All work builds on the stable Phase 1 foundation (error codes, config, context).

**Key Changes:**
- Move from monolithic `pisstvpp2_image.c` (578 lines) to modular architecture
- Create 4 new specialized modules: loader, processor, aspect, text_overlay
- Implement text overlay feature for FCC compliance (station ID in transmissions)
- Maintain 100% behavioral compatibility - tests must not change

---

## Deliverables Overview

### Phase 2.1: Image Loader (5-6 hours)
**Goal:** Extract image file loading and format detection  
**Output:** `src/image/image_loader.h/c`

**What moves:**
- `vips_read()` initialization
- Format detection logic
- File error handling
- Progress callbacks (new - for verbose mode)

**Current code location:** `src/pisstvpp2_image.c:200-350`

---

### Phase 2.2: Image Processor (4-5 hours)
**Goal:** Extract core image transformation operations  
**Output:** `src/image/image_processor.h/c`

**What moves:**
- Color space conversions (RGB normalization)
- Scaling/resizing operations
- Pixel buffer operations
- Dimension queries

**Current code location:** `src/pisstvpp2_image.c:75-200, 235-270`

---

### Phase 2.3: Aspect Ratio Correction (5-6 hours)
**Goal:** Extract aspect ratio transformation algorithms  
**Output:** `src/image/image_aspect.h/c`

**What moves:**
- Center-crop logic (ASPECT_CENTER)
- Padding logic (ASPECT_PAD)
- Stretch logic (ASPECT_STRETCH)
- All aspect calculation helpers

**Current code location:** `src/pisstvpp2_image.c:275-450`

---

### Phase 2.4: Text Overlay & Color Bars (8-10 hours)
**Goal:** Implement new text overlay feature for FCC compliance  
**Output:** `src/image/image_text_overlay.h/c`

**What's new:**
- Text rendering infrastructure
- Color bar support (top, bottom, left, right)
- Multiple text line support
- Opacity/transparency controls
- FCC Part 97.113(a)(4) compliance documentation

**CLI Integration (Post-Phase-2):**
```bash
# Text with background bar
./pisstvpp2 -i photo.png -m s1 -o output.wav \
  --text-overlay "K0ABC" --text-placement bottom \
  --text-color white --bar-color black

# Grid square on top
./pisstvpp2 -i photo.png -m m1 -o output.wav \
  --text-overlay "EN96" --text-placement top
```

---

## Architecture After Phase 2

```
src/include/
├── pisstvpp2_image.h (refactored facade, 50 lines)
└── image/ (new)
    ├── image_loader.h
    ├── image_processor.h
    ├── image_aspect.h
    └── image_text_overlay.h

src/image/ (new directory)
├── image_loader.c (150-180 lines)
├── image_processor.c (120-150 lines)
├── image_aspect.c (180-220 lines)
└── image_text_overlay.c (250-350 lines)

src/pisstvpp2_image.c (refactored from 578 → 200 lines)
├── ImageState struct (existing)
├── Module initialization
├── Facade functions (wrappers)
└── Cleanup/resource management
```

**Total new code:** ~700-900 lines (split across modules)  
**Code moved:** ~400 lines from monolithic → modular  
**Code deleted:** ~0 lines (all functionality preserved)  
**Net change:** +300-500 lines (mostly text overlay feature)

---

## Phase 1 → Phase 2 Dependencies

✅ **Ready to proceed:**
- Phase 1.1: Error codes (351 lines of infrastructure)
- Phase 1.2: Config management (200 lines, all options parsed)
- Phase 1.3: Context/state management (proper lifecycle)
- Phase 1.4: Module error unification (6/8 modules complete)

✅ **Prerequisites met:**
- Image module has proper error codes (Task 1.4 done)
- Config system supports future text overlay options
- Context structure ready for text overlay state
- All 54/55 tests passing

---

## Implementation Strategy

### Week 1: Core Refactoring (Tasks 2.1-2.3)
- Monday: Task 2.1 (Image Loader)
- Tuesday: Task 2.2 (Image Processor)
- Wednesday: Task 2.3 (Aspect Ratio)
- Thursday: Integration testing, fix any issues

### Week 2: Text Overlay Feature (Task 2.4)
- Friday: Core text overlay implementation
- Weekend: Color bars, advanced features
- Final: Integration tests, FCC compliance docs

### Validation Checkpoints
- After each task: All 55 tests still pass
- After 2.3: Image dimension/aspect behavior identical to Phase 1
- After 2.4: Visual validation (sample images with text)

---

## Success Criteria

✅ **Functional:**
- All image loading formats work (PNG, JPEG, GIF, BMP, TIFF, WebP)
- Aspect ratio modes (CROP, PAD, STRETCH) produce identical output
- Text overlay renders without distortion
- Color bars fill correctly

✅ **Testing:**
- All 55 original tests pass
- New image tests added for text overlay (10-15 new tests)
- Performance: No slowdown on Raspberry Pi

✅ **Code Quality:**
- No compiler errors or warnings
- Proper error handling throughout
- Module dependencies clean
- No circular includes

✅ **Documentation:**
- FCC Part 97.113(a)(4) compliance explanation
- API documentation for all new functions
- Text overlay usage examples

---

## Known Risks & Mitigations

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|-----------|
| Text rendering complexity | Medium | 2-3 hour slip | Use libvips text capabilities, test early |
| Dimension mismatches | Low | Code correctness | Byte-identical comparison tests |
| Module circular deps | Low | Build failure | Careful header organization |
| Performance regression | Low | Pi compatibility | Profile before/after on hardware |

---

## File Structure Changes

**Before Phase 2:**
```
src/
├── pisstvpp2_image.c (578 lines monolithic)
├── include/
│   ├── pisstvpp2_image.h (257 lines, all functions here)
```

**After Phase 2:**
```
src/
├── pisstvpp2_image.c (200 lines, refactored facade)
├── image/ (new directory)
│   ├── image_loader.c
│   ├── image_processor.c
│   ├── image_aspect.c
│   └── image_text_overlay.c
├── include/
│   ├── pisstvpp2_image.h (50 lines, top-level API)
│   ├── image/ (new directory)
│   │   ├── image_loader.h
│   │   ├── image_processor.h
│   │   ├── image_aspect.h
│   │   └── image_text_overlay.h
```

---

## Next Steps

1. ✅ Review this startup guide
2. ⏳ Create modular directory structure (Task 2.1 setup)
3. ⏳ Extract image loader (Task 2.1)
4. ⏳ Extract image processor (Task 2.2)
5. ⏳ Extract aspect ratio (Task 2.3)
6. ⏳ Implement text overlay (Task 2.4)
7. ⏳ Full integration testing
8. ⏳ Phase 2 completion review

**Estimated Start:** Now  
**Estimated Completion:** +24-26 hours  
**Target:** End of Week 2

---

## References

- [PISSTVPP2_v2_1_MASTER_PLAN.md](./PISSTVPP2_v2_1_MASTER_PLAN.md) - Full Phase 2 specification
- [src/pisstvpp2_image.c](../src/pisstvpp2_image.c) - Current monolithic module
- [docs/PHASE_1_QUICK_START.md](./PHASE_1_QUICK_START.md) - Phase 1 foundation reference
