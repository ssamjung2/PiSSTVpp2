# PiSSTVpp2 v2.1 - COMPREHENSIVE PROJECT HOLISTIC REVIEW
## February 12, 2026

---

## EXECUTIVE SUMMARY

### Current Status
- **Master Plan:** Complete design with 8 phases outlined (Phase 1-8)
- **Implementation:** Phases 1-2 largely complete, Phase 2.4 has critical bugs
- **Build Status:** Compiles successfully, 55 existing tests pass, but text overlay feature is non-functional
- **Code Quality:** Well-structured, modularized, documented, but incomplete feature delivery
- **Test Suite:** Passes but doesn't validate actual functionality (infrastructure only)
- **v2.1 Release Readiness:** BLOCKED - text overlay feature doesn't work despite "complete" status

### Critical Issues
1. **Text Overlay Compositing (CRITICAL):** Text rendered but never placed on image
2. **Grid Square Config (HIGH):** CLI -G flag missing implementation
3. **Color Support (HIGH):** Configuration exists but not implemented
4. **Test Validation (MEDIUM):** Tests check setup, not functionality
5. **Legacy Code (MEDIUM):** Old functions mixed with new modular code

### What's Working
- ✅ Error handling system (unified, comprehensive)
- ✅ Config management (mostly complete)
- ✅ Context state management (complete)
- ✅ Image module refactoring (mostly complete)
- ✅ SSTV encoding (unchanged, working)
- ✅ Audio encoding (unchanged, working)
- ✅ Test infrastructure (complete but needs enhancement)
- ✅ Build system (working)
- ✅ Documentation (comprehensive but outdated in places)

---

## SECTION 1: CODE ARCHITECTURE ANALYSIS

### 1.1 Current Folder Structure

```
/src
├── pisstvpp2.c                          ← Main entry point
├── pisstvpp2_image.c                    ← OLD: Image handling (to refactor)
├── pisstvpp2_sstv.c                     ← Audio tone synthesis (unchanged)
├── pisstvpp2_audio_encoder.c            ← Unified audio format handler
├── audio_encoder_*.c                    ← Format-specific implementations
├── pisstvpp2_config.c                   ← ✅ NEW: Config management
├── pisstvpp2_context.c                  ← ✅ NEW: State management
├── pisstvpp2_mmsstv_adapter.c           ← Stub: MMSSTV integration (incomplete)
├── overlay_spec.c                       ← ✅ Text overlay specifications
│
├── include/
│   ├── pisstvpp2_image.h                ← Image API (consolidated)
│   ├── pisstvpp2_config.h               ← Config API
│   ├── pisstvpp2_context.h              ← Context API
│   ├── pisstvpp2_sstv.h                 ← SSTV API
│   ├── pisstvpp2_audio_encoder.h        ← Audio encoder API
│   ├── overlay_spec.h                   ← Overlay spec API
│   ├── mmsstv_stub.h                    ← Stub for MMSSTV API
│   ├── logging.h                        ← Logging utilities
│   └── image/                           ← Refactored image modules
│       ├── image_loader.h/c             ← ✅ NEW: Image loading
│       ├── image_processor.h/c          ← ✅ NEW: Image processing
│       ├── image_aspect.h/c             ← ✅ NEW: Aspect ratio correction
│       └── image_text_overlay.h/c       ← ⚠️  INCOMPLETE: Text overlay
│
├── util/
│   ├── error.h/c                        ← ✅ NEW: Error code system
│
└── legacy/
    ├── pisstvpp.c                       ← OLD: v1.x original code
    └── pifm_sstv.c                      ← OLD: Radio transmission code
```

### 1.2 Module Dependencies

```
Main Program (pisstvpp2.c)
    ├── Config Management → pisstvpp2_config.c → error.h
    ├── Image Loading → image_loader.c → error.h
    ├── Image Processing → image_processor.c → error.h
    ├── Aspect Ratio → image_aspect.c → error.h
    ├── Text Overlay → image_text_overlay.c ⚠️ BROKEN CHAIN
    ├── SSTV Encoding → pisstvpp2_sstv.c
    │   └── Audio Tone Synthesis (tone generation)
    ├── Audio Encoding → pisstvpp2_audio_encoder.c
    │   ├── audio_encoder_wav.c
    │   ├── audio_encoder_aiff.c
    │   └── audio_encoder_ogg.c
    ├── MMSSTV Adapter → pisstvpp2_mmsstv_adapter.c (incomplete)
    └── Error Handling → error.c (throughout)
```

### 1.3 Architecture Strengths

**✅ Modular Design**
- Image processing separated from encoding
- Audio format handling separated from sample generation
- Error handling centralized and consistent
- Config parsing modularized

**✅ Code Documentation**
- Comprehensive header comments
- Architecture diagrams
- Function parameter documentation
- Clear design philosophy documented

**✅ Separation of Concerns**
- Image module independent of audio
- Audio encoder independent of SSTV mode
- SSTV modes independent of image format
- Config parsing independent of execution

**✅ Error Handling**
- Unified error code system (15 categories, 50+ specific codes)
- Consistent error logging
- Propagation of errors through call stack
- Context provided in error messages

---

## SECTION 2: LEGACY CODE ANALYSIS

### 2.1 Legacy Code Identified

#### File: `src/legacy/pisstvpp.c`
- **Purpose:** Original PiSSTVpp v1.x implementation
- **Status:** NOT USED - completely replaced by v2.x
- **Size:** ~500 lines
- **Should be:** REMOVED (not needed for v2.1) or archived

#### File: `src/legacy/pifm_sstv.c`
- **Purpose:** Radio transmission via GPIO pins
- **Status:** NOT USED - not part of v2.x vision
- **Size:** ~400 lines
- **Should be:** REMOVED or made optional module

#### Mixed in main codebase:

**In pisstvpp2.c:**
- Many hardcoded constants that could be in config
- Protocol mapping (m1/m2/s1/s2/sdx/r36/r72) hardcoded instead of registry-based
- No mode registry system (Phase 3 still pending)

**In pisstvpp2_image.c:**
- `image_calculate_crop_box()` - Utility function used only once
- `aspect_mode_to_string()` - Debug utility, not part of main flow
- Manual image format detection code

**In pisstvpp2_sstv.c:**
- Hard-coded protocol dispatching in `sstv_encode_frame()` - should use registry
- Build audio functions (`buildaudio_m`, `buildaudio_s`, etc.) - needed but not modularized

### 2.2 Unused or Dead Code

**Partially Implemented:**
- `pisstvpp2_mmsstv_adapter.c` - Stub only, no actual MMSSTV support
- Mode registry system - Designed but not implemented
- Dynamic mode loading - Planned but not coded

**Test Infrastructure:**
- `show_overlay_tests.py`, `show_results.py` - Exist but not well integrated
- Path issues fixed but test infrastructure spotty

---

## SECTION 3: IMPLEMENTATION COMPLETENESS BY PHASE

### Phase 1: Error Handling & Configuration ✅ ~95% COMPLETE

**Completed:**
- ✅ Error code system (error.h/c) - 347 lines, 50+ error codes
- ✅ Config management (pisstvpp2_config.h/c) - 373 lines, full parsing
- ✅ Context management (pisstvpp2_context.h/c) - 278 lines, state encapsulation
- ✅ Error codes integrated into image module
- ✅ Error codes integrated into config module
- ✅ Error codes integrated into SSTV module

**Remaining:**
- ⚠️ Complete integration into all audio encoder functions (~2 hours)
- ⚠️ Complete integration into context initialization (~1 hour)
- ⚠️  MMSSTV error codes and handling (~2 hours)

**Status:** Ready for production with minor refinements

---

### Phase 2.1: Image Loader Extraction ✅ COMPLETE

**Files:**
- `src/image/image_loader.h/c` - 450 lines
- Tests: All formats (PNG, JPEG, GIF, BMP, TIFF) load correctly

**Features:**
- ✅ Auto-format detection via libvips
- ✅ Error handling with context
- ✅ File validation
- ✅ Size reporting
- ✅ Progress callbacks for verbose mode

**Status:** Production-ready

---

### Phase 2.2: Image Processor Extraction ✅ COMPLETE

**Files:**
- `src/image/image_processor.h/c` - 380 lines

**Features:**
- ✅ Color space normalization (all formats → RGB)
- ✅ Scaling to target dimensions
- ✅ Pixel access interface
- ✅ Format preservation metadata

**Status:** Production-ready

---

### Phase 2.3: Aspect Ratio Correction ✅ COMPLETE

**Files:**
- `src/image/image_aspect.h/c` - 520 lines

**Features:**
- ✅ Center-crop mode (default)
- ✅ Padding mode (black bars)
- ✅ Stretch mode (distort to fit)
- ✅ Dimension calculations
- ✅ Proper error handling

**Status:** Production-ready

**Testing:**
- ✅ 55/55 existing tests pass
- ✅ Output verified against v2.0 baseline
- ✅ Edge cases tested

---

### Phase 2.4: Text Overlay & Color Bars ⚠️ 40% COMPLETE - CRITICAL BUGS

**Files:**
- `src/image/image_text_overlay.h/c` - 1000+ lines
- `src/include/overlay_spec.h/c` - 300 lines (complete)
- `src/include/pisstvpp2_config.h` - Modified to include overlay_specs field

**What's Working:**
- ✅ Configuration structure (TextOverlaySpec) - 95% complete
- ✅ CLI parsing for -S flag (callsign) - 95% complete
- ✅ Spec list management - 100% complete
- ✅ Text rendering with vips_text() - 50% complete
- ✅ Position calculation - 90% complete

**What's Broken:**
- ❌ **CRITICAL: Text compositing missing** - text is rendered but never placed on image
  - Missing `vips_insert()` call
  - Text image created then destroyed
  - Effect: text output invisible despite successful execution
  
- ❌ **CRITICAL: Grid square config missing** - field not in struct
  - -G flag parsing incomplete
  - Cannot specify grid square for station ID
  
- ❌ Color rendering - not implemented
  - Specs have RGB fields but never used
  - Text hardcoded to black
  - Background colors ignored
  
- ❌ Background boxes - not implemented
  - Would improve station ID readability
  - Infrastructure exists, implementation missing
  
- ❌ Border rendering - not implemented

**Testing Issue:**
- ✅ 7/7 text overlay tests passing
- ❌ BUT: Tests only verify infrastructure (directories, files, specs)
- ❌ Tests don't verify actual image output or text presence
- ❌ Tests don't verify colors or placement visually

**Impact on v2.1:**
- **BLOCKS RELEASE** - Feature marked complete but non-functional
- Users cannot add callsigns to SSTV transmissions
- FCC Part 97 compliance impossible
- Documented feature broken

**Estimate to Fix:**
- Text compositing: 1-2 hours
- Grid square config: 30 minutes
- -G flag handler: 1 hour
- Color support: 2-3 hours
- Background boxes: 1-2 hours
- Test enhancement: 1-2 hours
- **Total: 7-12 hours (1-2 days)**

**Status:** BROKEN - Cannot ship in this state

---

### Phase 3: SSTV Module Refactoring ❌ 0% IMPLEMENTED

**Planned:**
- Design mode registry system
- Extract native mode definitions (m1, m2, s1, s2, sdx, r36, r72)
- Create mode initialization system
- Implement mode dispatcher

**Current State:**
- Protocol mapping still hardcoded in pisstvpp2.c main()
- Mode encoding functions exist but not registered in any system
- No mode discovery mechanism
- No dynamic mode addition capability

**Impact:**
- MMSSTV integration blocked (Phase 4 depends on this)
- Dynamic mode system not possible
- Phase 3 tasks estimated at 13-17 hours

**Status:** NOT STARTED

---

### Phase 4: MMSSTV Integration ❌ 0% IMPLEMENTED

**Planned:**
- MMSSTV library detection
- Dynamic mode registry
- Mode adapter system
- CLI integration for dynamic modes

**Current State:**
- `src/include/mmsstv_stub.h` exists but empty
- `src/pisstvpp2_mmsstv_adapter.c` exists but is stub only
- No library detection code
- No dynamic loading capability

**Impact:**
- 50+ additional modes not available
- Feature marketed in master plan not available
- Graceful fallback system not implemented

**Status:** NOT STARTED (blocked by Phase 3)

---

### Phase 5: Build System Enhancement ❌ 0% IMPLEMENTED

**Planned:**
- Makefile MMSSTV detection
- Build target enhancements (debug, sanitize, analyze)
- Improved build messages

**Current State:**
- Makefile is functional but basic
- No MMSSTV detection
- Limited build targets
- Messages are plain

**Status:** NOT STARTED

---

### Phase 6: Testing & Validation ⚠️ 50% COMPLETE

**Completed:**
- ✅ 55/55 backward compatibility tests passing
- ✅ Test infrastructure established
- ✅ Test framework working
- ✅ Multiple test suites (integration, edge cases, error codes, etc.)

**Issues:**
- ⚠️ Text overlay tests don't validate functionality
- ⚠️ No image comparison/visual validation
- ⚠️ No end-to-end feature tests (only infrastructure)
- ⚠️ Missing MMSSTV testing (not implemented yet)

**Status:** Partially complete, needs enhancement

---

### Phase 7: Documentation ✅ 90% COMPLETE

**Completed:**
- ✅ Architecture documentation (ARCHITECTURE.md) - comprehensive
- ✅ Master plan (PISSTVPP2_v2_1_MASTER_PLAN.md) - 1100+ lines
- ✅ Quick start (QUICK_START.md)
- ✅ Build documentation (BUILD.md)
- ✅ User guide (USER_GUIDE.md)
- ✅ 40+ other reference documents
- ✅ Code comments and header documentation

**Issues:**
- ⚠️ Status documents outdated (claim Phase 2.4 complete but broken)
- ⚠️ Some documentation contradicts actual implementation
- ⚠️ No MMSSTV setup guide (Phase 4 not implemented)
- ⚠️ Developer guides incomplete

**Status:** Functionally complete but needs updates

---

### Phase 8: Release Preparation ❌ 0% STARTED

**Planned:**
- Comprehensive testing pass
- Documentation review
- Release notes and version updates

**Current State:**
- No release branch
- No release notes for v2.1
- No version bumps
- No release checklist

**Status:** NOT STARTED

---

## SECTION 4: CODE QUALITY ASSESSMENT

### 4.1 Strengths

**Architecture & Design**
- Clear separation of concerns (image, SSTV, audio)
- Modular structure enables future enhancements
- Consistent error handling throughout
- Well-documented design decisions

**Code Style**
- Consistent formatting
- Clear variable names
- Detailed comments explaining WHY
- Function documentation with examples

**Error Handling**
- Unified error code system (50+ specific codes)
- Context-aware error messages
- Proper cleanup on error paths
- Thread-safe error logging (within single-threaded context)

**Testing**
- Comprehensive test suite (55+ tests)
- Multiple test categories (integration, edge cases, etc.)
- CLI argument validation tested
- All 7 SSTV modes testable

### 4.2 Weaknesses

**Feature Incompleteness**
- Text overlay feature claimed complete but broken
- MMSSTV integration planned but not implemented
- Mode registry system designed but not coded
- Build system enhancements missing

**Testing Gaps**
- No functional validation of output (image content, colors)
- No visual regression testing
- Tests pass but don't verify actual features work
- Text overlay "working" despite broken pipeline

**Legacy Code**
- Old SSTV mode functions mixed with new module structure
- Protocol mapping hardcoded instead of registry-based
- Two unused legacy files in repository
- Partial implementations scattered throughout

**Documentation**
- Master plan contradicts actual implementation status
- Status documents claim completeness for broken features
- No explanation of what still needs to be done for v2.1
- Developer guide incomplete

### 4.3 Code Metrics

**Files:**
- Total source files: 18 C files + 14 headers = 32 files
- Size: ~15,000 lines of code
- Documentation: ~15,000 lines of documentation
- Ratio: 1:1 documentation-to-code (very well documented)

**Modularity:**
- 7 major components (image, SSTV, audio, config, error, context, overlay)
- Clear interfaces between components
- Minimal coupling
- Good separation into separate files

**Compilation:**
- ✅ Compiles without errors
- ✅ No compiler warnings
- ✅ Builds with standard make
- ✅ Binary size: ~157 KB

---

## SECTION 5: CURRENT TECHNICAL DEBT

### HIGH PRIORITY

1. **Text Overlay Compositioning (CRITICAL)**
   - Impact: Feature doesn't work despite appearing to
   - Effort: 1-2 hours to fix
   - Blocks: v2.1 release

2. **Test Validation Gap**
   - Impact: Tests pass but don't verify functionality
   - Effort: 2-3 hours to enhance
   - Blocks: Quality assurance confidence

3. **Phase 3 Not Implemented**
   - Impact: Can't add MMSSTV (Phase 4 blocked)
   - Effort: 13-17 hours to implement
   - Blocks: Full feature set for v2.1

4. **Legacy Code Not Cleaned**
   - Impact: Confusion about what's active vs old
   - Effort: 1-2 hours to archive
   - Blocks: Clean codebase

### MEDIUM PRIORITY

5. **Documentation/Reality Gap**
   - Impact: Master plan claims don't match actual implementation
   - Effort: 3-4 hours to update
   - Blocks: Clear understanding of project status

6. **Grid Square Config Missing**
   - Impact: Can't fully specify station ID
   - Effort: 2-3 hours to add
   - Blocks: FCC compliance

7. **Color Support Not Implemented**
   - Impact: Text overlay limited to black on white
   - Effort: 2-3 hours to implement
   - Blocks: Professional-grade overlays

### LOW PRIORITY

8. **Build System Enhancements**
   - Impact: Limited build targets
   - Effort: 2-3 hours to enhance
   - Blocks: Developer convenience

9. **MMSSTV Stub Files**
   - Impact: Empty placeholder files
   - Effort: 1 hour to remove or complete
   - Blocks: Clean codebase

---

## SECTION 6: NEXT STEPS & RECOMMENDATIONS

### IMMEDIATE (This Week - 1-2 Days)

**Priority 1: Fix Text Overlay (CRITICAL)**
```
1. Add vips_insert() call in apply_single_overlay() to composite text
2. Test text appears on output images
3. Verify all placement modes work
Effort: 2-3 hours
Unblocks: v2.1 release
```

**Priority 2: Add Grid Square Support (HIGH)**
```
1. Add grid_square field to PisstvppConfig struct
2. Add -G flag handler in pisstvpp_config_parse()
3. Create overlay spec from grid square
4. Test -S and -G flags together
Effort: 2-3 hours
Unblocks: FCC compliance
```

**Priority 3: Enhanced Testing (MEDIUM)**
```
1. Modify test suite to validate image output
2. Add visual regression testing
3. Verify text appears in generated images
4. Check color rendering
Effort: 2-3 hours
Unblocks: Confidence in feature quality
```

**Subtotal: 6-8 hours (restore feature to actually working state)**

### SHORT TERM (Next 1-2 Weeks)

**Priority 4: Update Documentation**
```
1. Revise master plan with actual vs planned status
2. Document current Phase 2.4 state
3. Update v2.1 release blockers list
4. Create implementation plan for Phases 3-4
Effort: 3-4 hours
```

**Priority 5: Clean up Legacy Code**
```
1. Move legacy/ files to archive/
2. Add note explaining archive
3. Remove unused helper functions
4. Document removed code location
Effort: 1-2 hours
```

**Priority 6: Implement Phase 3 (Mode Registry)**
```
1. Design mode registry system
2. Extract mode definitions from pisstvpp2_sstv.c
3. Implement mode discovery
4. Create dispatcher system  
Effort: 13-17 hours (can work in parallel with above)
```

**Subtotal: 17-23 hours**

### MEDIUM TERM (2-3 Weeks)

**Priority 7: Add Color Support**
```
1. Implement RGB text rendering
2. Add background color support
3. Add border rendering
4. Validate color handling
Effort: 3-4 hours
```

**Priority 8: Implement Phase 4 (MMSSTV Integration)**
```
1. Add library detection code
2. Implement dynamic loading
3. Create adapter functions
4. Integrate with mode registry
Effort: 14-18 hours (depends on Phase 3)
```

**Priority 9: Build System Enhancements**
```
1. Add dynamic library detection
2. Add build targets (debug, sanitize, analyze)
3. Improve build messages
Effort: 3-4 hours
```

**Subtotal: 20-26 hours**

### BEFORE v2.1 RELEASE

**Checklist:**
- [ ] All Phases 1-2 bugs fixed and tested
- [ ] Phase 3 Mode Registry implemented
- [ ] Documentation updated to reflect actual status
- [ ] 55/55 backward compatibility tests passing
- [ ] New feature tests (text overlay) working
- [ ] No compiler warnings
- [ ] No memory leaks (valgrind clean)
- [ ] Builds cleanly on Linux, macOS, RPi
- [ ] Help text current and accurate
- [ ] Release notes drafted
- [ ] Version number bumped to 2.1.0

---

## SECTION 7: STRATEGIC RECOMMENDATIONS

### Recommendation 1: Fix Before Advancing
**Action:** Complete text overlay fixes (Priority 1-3 above) before starting Phase 3.
**Rationale:** 
- Current feature is broken despite "complete" status
- Release quality requires working features
- Minimal impact  (8 hours) vs. massive impact (broken feature)
- Establishes pattern for proper verification

**Timeline:** Complete by end of this week

### Recommendation 2: Decouple from Phase Dependencies
**Action:** Implement Phase 3 (Mode Registry) and Phase 4 (MMSSTV) as separate feature branches.
**Rationale:**
- Phases 1-2 are solid and releasable
- Phases 3-4 are independent (don't affect current functionality)
- Could release v2.1 "Stable" (Phases 1-2 complete) then v2.2 "Full" (Phases 3-4)
- Allows community feedback on core feature before major refactor

**Options:**
```
Option A: Release v2.1 with just Phases 1-2 complete
  - Pros: Solid, tested, working features
  - Cons: Partial master plan
  
Option B: Complete all phases before release
  - Pros: Full feature set as planned
  - Cons: Delayed release, larger risk
  
Option C: Release v2.1.0 (Phases 1-2), then v2.1.1 (Phase 3), then v2.1.2 (Phase 4)
  - Pros: Incremental delivery, feedback loops
  - Cons: More complex release process
```

**Recommendation:** Option B (complete all phases) but execute Priority 1-3 immediately

### Recommendation 3: Test-Driven Feature Development
**Action:** For all remaining phases, implement tests BEFORE features.
**Rationale:**
- Current text overlay tests don't verify functionality
- Root cause: tests written after feature
- Solution: Define test requirements first, implement features to pass tests

**Process:**
```
1. Define feature scope (what should happen)
2. Write tests that verify feature works (not infrastructure)
3. Implement feature to pass tests
4. Code review against both spec and tests
```

### Recommendation 4: Update Master Plan
**Action:** Create "v2.1 Revised" plan with realistic estimates and current status.
**Rationale:**
- Current master plan claims things are done that aren't
- Estimates were optimistic
- New developers need clear picture of what's left

**Include:**
- Current status of each phase (actual, not claimed)
- Revised time estimates
- List of known issues
- Blocking issues for release
- Recommended execution order

---

## SECTION 8: SUMMARY TABLE - PHASE STATUS

| Phase | Component | Planned | Actual | Complete | Ready | Issues |
|-------|-----------|---------|--------|----------|-------|--------|
| 1 | Error Handling | Full system | 50+ codes | 95% | ✅Yes | Minor integrations |
| 1 | Config Mgmt | Parser + validation | Full system | 95% | ✅Yes | Grid square field |
| 1 | Context Mgmt | State encapsulation | Full system | 100% | ✅Yes | None |
| 2.1 | Image Loader | Extraction | Complete | 100% | ✅Yes | None |
| 2.2 | Image Processor | Extraction | Complete | 100% | ✅Yes | None |
| 2.3 | Aspect Ratio | 3 modes | All 3 modes | 100% | ✅Yes | None |
| 2.4 | Text Overlay | Full feature | Partial | 40% | ❌No | **Compositing broken** |
| 3 | Mode Registry | Dynamic system | Designed | 0% | ❌No | Not started |
| 4 | MMSSTV Integration | Full support | Stub only | 0% | ❌No | Not started (blocked) |
| 5 | Build Enhancement | Enhancements | Basic | 0% | ❌No | Not started |
| 6 | Testing | Validation | Infrastructure | 50% | ⚠️Partial | Functional gaps |
| 7 | Documentation | Complete | Comprehensive | 90% | ⚠️Partial | Status outdated |
| 8 | Release Prep | Polish | Not started | 0% | ❌No | Not started |

---

## SECTION 9: CRITICAL PATH TO v2.1 RELEASE

### Must Complete (Blocking Release)
1. Fix text overlay compositing (2-3 hours)
2. Add grid square config field (0.5 hours)
3. Fix -G flag handling (1 hour)
4. Enhance tests to verify functionality (2-3 hours)
5. Update documentation for actual status (2 hours)
**Subtotal: 7.5-9 hours (1 day)**

### Should Complete (For Full Feature Set)
6. Implement Phase 3 - Mode Registry (13-17 hours)
7. Implement Phase 4 - MMSSTV Integration (14-18 hours)
8. Color support for overlays (3-4 hours)
9. Build system enhancements (3-4 hours)
**Subtotal: 33-43 hours (4-5 days)**

### Nice to Have (Polish)
10. Background boxes for text overlay (2-3 hours)
11. Advanced build targets (1-2 hours)
12. Legacy code cleanup (1-2 hours)
**Subtotal: 4-7 hours (0.5 days)**

### Total for Full v2.1
**~45-60 hours (6-8 days for one developer, 3-4 days with two developers)**

---

## CONCLUSION

PiSSTVpp2 v2.1 is **45% complete** toward its stated master plan:
- Core functionality (error handling, config, image processing) is solid
- Major feature (text overlay) is broken despite apparent completeness
- Major systems (mode registry, MMSSTV) not yet implemented
- Test infrastructure is in place but incomplete

**To ship v2.1 Release:**
1. **Immediate (1 day):** Fix text overlay bugs, enhance testing
2. **Short term (4-5 days):** Implement Phase 3-4 (mode registry, MMSSTV)
3. **Validation (1-2 days):** Comprehensive testing, documentation update

**Current state allows:** Shipping as v2.0.2 patch (stable, tested, complete)
**Master plan requires:** Additional 30-40 hours (~4-5 days) for full v2.1 feature set

**Recommendation:** Complete Phases 1-2 bug fixes immediately, schedule Phases 3-4 for following week with clear ownership and checkpoints.
