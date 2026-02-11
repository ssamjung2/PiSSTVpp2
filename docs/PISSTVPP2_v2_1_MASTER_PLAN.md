
# PiSSTVpp2 v2.1 - Unified Master Plan & Development Roadmap

**Complete Strategy for Modernization & MMSSTV Integration**

**Document Version:** 1.0 (Unified)  
**Date:** February 10, 2026  
**Status:** Ready for Implementation  
**Scope:** PiSSTVpp2 v2.1 Modernization & MMSSTV Integration

---

## Overview: What This Document Is

This is the **single source of truth** for PiSSTVpp2 v2.1 development. It consolidates all planning, architecture, tasks, and workflow into one coherent strategy.

If you're looking for:
- **Quick overview:** See "Executive Summary" below
- **Navigation:** See "How to Use This Document" section
- **Specific tasks:** See "Phase Breakdown & Task List" section
- **Workflow:** See "Development Workflow" section
- **Architecture details:** See "Technical Architecture" section

---

## Executive Summary

### Vision
Transform PiSSTVpp2 v2.0 (solid 7-mode SSTV encoder) into v2.1: a modern, modular, community-friendly codebase that:
- ✅ Maintains 100% backward compatibility (all 55 tests pass)
- ✅ Dynamically detects MMSSTV library (optional, at runtime)
- ✅ Scales from 7 native modes → 57+ modes (if library present)
- ✅ Remains accessible to ham radio community
- ✅ Improves code structure and maintainability

### Strategy
**8 phases over 2-3 weeks:** Clean architecture foundation → modular refactoring → dynamic mode system → MMSSTV integration → Polish & release

### Effort
- **90-115 hours total** (added text overlay feature)
- **One developer: 2-3 weeks full-time**
- **Two developers: ~3-4 weeks with parallelization**

### Success
- ✅ 55/55 existing tests pass (no regressions)
- ✅ All native modes produce identical output
- ✅ MMSSTV detection works (with/without library)
- ✅ Text overlay working with FCC Part 97 compliance
- ✅ Code implements 50+ specific tasks
- ✅ Documentation complete and accessible

---

## How to Use This Document

### If You're the Project Lead
1. Read: "Executive Summary" (this section) - 10 min
2. Read: "Technical Architecture" sections A1-A3 - 30 min
3. Review: "Phase Breakdown & Task List" for scope - 20 min
4. **Decision:** Proceed? Timeline OK? Resources available?

### If You're Starting Development
1. Read: "Development Workflow" section - 15 min
2. Read: "Phase Breakdown" for Phase 1 - 20 min
3. Pick: Task 1.1 (Error Code System)
4. **Start:** Create feature branch, implement task

### If You're a Code Reviewer
1. Reference: "Technical Architecture" for design spec
2. Check: Against task success criteria in "Phase Breakdown"
3. Verify: Test suite still 55/55 passing
4. **Approve:** Based on spec compliance

### If You're the Community (Eventually)
1. Watch: For Phase 1 completion announcement
2. Test: Development builds when available
3. Reference: "Code Accessibility" in Architecture section
4. **Contribute:** Follow workflow guidelines

---

## Technical Architecture

### A1. Dynamic Mode Registry System

**Current State (v2.0):**
- 7 native modes hardcoded in `pisstvpp2_sstv.c`
- No plugin or dynamic mechanism
- Adding MMSSTV modes requires major rewrite

**Proposed State (v2.1):**
```c
// Unified mode registry available at runtime
mode_registry_t *reg = registry_create();
// Native 7 modes: always available
registry_add_mode(reg, &mode_martin_1);
// MMSSTV modes: added if library detected
mmsstv_adapter_init(reg);  // Non-fatal if library missing
```

**Benefits:**
- ✅ Native modes always work
- ✅ MMSSTV modes plug in dynamically
- ✅ CLI scales automatically (7 or 57+ modes)
- ✅ No recompilation needed for MMSSTV
- ✅ Extensible for future mode additions

### A2. MMSSTV Library Detection

**Environment Variables:**
```bash
MMSSTV_LIB_PATH="/usr/local/lib/libmmsstv.so"    # User override
MMSSTV_INCLUDE_PATH="/usr/local/include/mmsstv"  # Headers
```

**Detection Order:**
1. Check MMSSTV_LIB_PATH environment variable
2. Try pkg-config: `pkg-config --cflags --libs mmsstv-portable`
3. Check standard paths: /usr/lib, /usr/local/lib, /opt/mmsstv/lib
4. Fallback: Native-only mode (fully functional, that's OK)

**Key Point:** Graceful degradation. App works with or without library.

### A3. Unified Encoding Pipeline

```
Input Image
    ↓
Config/Registry Lookup
    ↓
Image Loading & Processing
    ↓
Aspect Ratio Correction (center/pad/stretch)
    ↓
Text Overlay & Color Bars (optional, for SSTV compliance)
    ↓
Mode Selection (native or MMSSTV)
    ↓
SSTV Encoding
    ↓
Audio Format Encoding (WAV/AIFF/OGG)
    ↓
Output File
```

**Note:** Text overlay (Task 2.4) is applied *after* image processing to ensure consistent sizing and positioning across different input images and aspect ratios.

### B. Code Modernization Areas

**1. Unified Error Handling**
- Replace scattered error codes (-1, 0, 1, 2) with named constants
- PISSTVPP2_OK, PISSTVPP2_ERR_IMAGE_LOAD, PISSTVPP2_ERR_SSTV_INIT, etc.
- Consistent error logging with context

**2. Modular Refactoring**
```
src/
├── core/               (entry, config, state)
├── image/              (loader, processor, aspect, text_overlay)
│   ├── image_loader.h/c
│   ├── image_processor.h/c
│   ├── image_aspect.h/c
│   └── image_text_overlay.h/c      ← NEW: Text & color bar overlays
├── sstv/               (native modes, registry, dispatcher)
├── mmsstv/             (detection, adapter)
├── audio/              (encoders, generators)
└── util/               (error, logging, memory)
```

**3. Code Accessibility**
- Comments explain WHY, not just WHAT
- Example code for common patterns
- Contribution guide for adding modes
- Focused files (single responsibility)

---

## Phase Breakdown & Task List

This section contains all 50+ tasks organized by phase. Each task includes:
- **Effort:** Estimated hours
- **Description:** What needs to be done
- **Files:** Create/modify
- **Checklist:** Implementation steps
- **Testing:** Success criteria

### Phase 1: Unified Error Handling & Configuration (10-18 hours)

#### Task 1.1: Implement Error Code System (4-6 hours)
**Files:** Create `src/util/error.h`, `src/util/error.c`
**Checklist:**
- [ ] Define 15-20 error codes (PISSTVPP2_OK, ERR_ARGS, ERR_IMAGE_LOAD, etc.)
- [ ] Create `error_string(code)` function for human-readable messages
- [ ] Create `error_log(code, context, ...)` for consistent logging
- [ ] Document error codes in header with examples
**Testing:**
- [ ] Each error code maps correctly to string
- [ ] error_log output format is consistent
- [ ] All 55 tests still pass
**Success Criteria:** Unified error system ready, tests passing

#### Task 1.2: Create Config Management Module (3-4 hours)
**Files:** Create `src/core/pisstvpp2_config.h/c`
**Checklist:**
- [ ] Move `config_t` structure to config.h
- [ ] Create `config_init()` with defaults
- [ ] Create `config_validate()` for validation
- [ ] Create `config_print()` for debugging
- [ ] Create getter functions for each field
**Testing:**
- [ ] All option combinations parse correctly
- [ ] Invalid combinations rejected properly
- [ ] Defaults are sensible
**Success Criteria:** Config module works identically to current behavior

#### Task 1.3: Create Context/State Management (3-4 hours)
**Files:** Create `src/core/pisstvpp2_context.h/c`
**Checklist:**
- [ ] Define `pisstvpp2_context_t` containing:
  - Config
  - Image buffer
  - SSTV state
  - Audio samples
  - MMSSTV loader state
- [ ] Create `context_create()` and `context_free()`
- [ ] Create thread-safe access patterns
- [ ] Document pointer ownership
**Testing:**
- [ ] No memory leaks on create/destroy
- [ ] Works in multi-mode encoding scenario
**Success Criteria:** Global state properly encapsulated

#### Task 1.4: Update All Modules to Use Error Codes (6-8 hours)
**Files:** Modify all src/*.c files
**Checklist per file:**
- [ ] Replace `fprintf(stderr, ...)` with `error_log()`
- [ ] Replace error returns (-1, 1, 2) with PISSTVPP2_* codes
- [ ] Update error checking pattern
- [ ] Add error propagation context
**Files to update:**
- [ ] pisstvpp2.c
- [ ] pisstvpp2_image.c
- [ ] pisstvpp2_sstv.c
- [ ] pisstvpp2_audio_encoder.c
- [ ] audio_encoder_*.c
**Testing:**
- [ ] All 55 tests pass
- [ ] Each error condition tested
- [ ] Error messages helpful
**Success Criteria:** Consistent error handling throughout codebase

---

### Phase 2: Image Module Refactoring (16-20 hours)

#### Task 2.1: Extract Image Loader (5-6 hours)
**Files:** Create `src/image/image_loader.h/c`
**Checklist:**
- [ ] Move vips image loading logic
- [ ] Move format detection
- [ ] Create `image_loader_t` structure
- [ ] Create `image_load_from_file()` function
- [ ] Add loading progress callbacks for verbose mode
**Testing:**
- [ ] Load all formats (PNG, JPEG, GIF, BMP, TIFF, WebP)
- [ ] Proper error handling for missing/corrupt files
- [ ] 55 tests still pass
**Success Criteria:** Image loading separated, identical behavior

#### Task 2.2: Extract Image Processor (4-5 hours)
**Files:** Create `src/image/image_processor.h/c`
**Checklist:**
- [ ] Move color space conversion
- [ ] Move scaling operations
- [ ] Create `image_to_rgb()` function
- [ ] Create `image_scale()` function
- [ ] Create `image_get_pixel_rgb()` utility
**Testing:**
- [ ] RGB conversion for all formats
- [ ] Scaling produces correct dimensions
- [ ] Pixel access at edge cases
**Success Criteria:** Processing logic separated, identical output

#### Task 2.3: Extract Aspect Ratio Correction (5-6 hours)
**Files:** Create `src/image/image_aspect.h/c`
**Checklist:**
- [ ] Move center-crop logic
- [ ] Move padding logic
- [ ] Move stretch logic
- [ ] Create `image_apply_aspect_correction()` function
- [ ] Create `aspect_calculate_dimensions()` helper
- [ ] Document algorithm in detail
**Testing:**
- [ ] All 3 modes (center, pad, stretch) work
- [ ] Various input ratios handled correctly
- [ ] Output dimensions exact
- [ ] Compare to v2.0 baseline (byte-identical if possible)
**Success Criteria:** Aspect logic separated, identical behavior

#### Task 2.4: Implement Text Overlay & Color Bars (8-10 hours)
**Files:** Create `src/image/image_text_overlay.h/c`

**Purpose:** Add call sign, grid square, timestamps, and other metadata overlays to SSTV images for FCC Part 97 compliance and professional ham radio QSOs.

**Checklist:**
- [ ] Create `text_overlay_config_t` structure with:
  - Text content (call sign, grid, message)
  - Font size (small, medium, large, custom)
  - Text color (RGB)
  - Background color for text block
  - Placement (top, bottom, left, right, custom coordinates)
  - Opacity control
  - Bar height/width for bordered areas
- [ ] Implement `image_apply_text_overlay()` function
- [ ] Implement text rendering using libvips text capabilities
- [ ] Support color bars (solid blocks with text):
  - Top bar with text
  - Bottom bar with text
  - Left bar with text
  - Right bar with text
  - Custom combined layouts
- [ ] Handle text clipping at image boundaries
- [ ] Support multiple text lines
- [ ] Create `image_add_color_bar()` helper
- [ ] Create `image_add_text_to_region()` helper
- [ ] Document FCC Part 97 Section 97.113(a)(4) compliance (station ID in transmissions)

**Testing:**
- [ ] Text renders correctly in all placements
- [ ] Text doesn't exceed image boundaries
- [ ] Color bars fill correctly
- [ ] Text color contrasts with background
- [ ] Multiple lines render without overlapping
- [ ] Works with all image formats
- [ ] Performance acceptable on Pi (no slowdown)
- [ ] Visual validation tests (sample images with overlays)

**Success Criteria:** 
- Text overlay fully functional
- All placement options work
- FCC compliance documentation included
- Ready for end-user configuration

**Example Feature Usage:**
```bash
# Add call sign to bottom bar
./pisstvpp2 -i image.png \
  --text-overlay-placement bottom \
  --text-overlay-content "K0ABC / EN96" \
  --text-size large \
  --text-color white \
  --bar-color black \
  -m m1 -o output.wav

# Add grid square to top with translucent background
./pisstvpp2 -i image.png \
  --text-overlay-placement top \
  --text-overlay-content "EN96ab" \
  --text-size medium \
  --text-color yellow \
  --text-bg-color "rgba(0,0,0,0.7)" \
  -m s1 -o output.wav

# Multiple text elements
./pisstvpp2 -i image.png \
  --text-overlay "K0ABC/EN96|top|white|black" \
  --text-overlay "2026-02-10|bottom|yellow|black" \
  -m pd120 -o output.wav
```

---

### Phase 2 Summary

Phase 2 is now 24-26 hours total (up from 16-20 hours):
- Task 2.1: Image loader (5-6 hours)
- Task 2.2: Image processor (4-5 hours)
- Task 2.3: Aspect ratio correction (5-6 hours)
- **Task 2.4: Text overlay & color bars (8-10 hours)** ← NEW

---

### Phase 3: SSTV & Mode System Refactoring (13-17 hours)

#### Task 3.1: Design & Implement Mode Registry (3-4 hours)
**Files:** Create `src/sstv/mode_registry.h/c`
**Checklist:**
- [ ] Define `mode_definition_t` structure (code, name, VIS, resolution, duration, function pointers)
- [ ] Define `mode_registry_t` to hold all modes
- [ ] Implement `registry_create()` - creates empty registry
- [ ] Implement `registry_add_mode()` - adds mode
- [ ] Implement `registry_lookup_by_code()` - finds mode by code
- [ ] Implement `registry_list_all()` - enumerates all modes
- [ ] Implement `registry_free()` - cleanup
**Testing:**
- [ ] Add/lookup/remove operations work
- [ ] No duplicates allowed
- [ ] Registry survives encode cycle
**Success Criteria:** Mode registry system functional

#### Task 3.2: Extract Native Mode Definitions (4-5 hours)
**Files:**
- Create `src/sstv/modes_martin.h/c` (Martin 1, Martin 2)
- Create `src/sstv/modes_scottie.h/c` (Scottie 1/2/DX)
- Create `src/sstv/modes_robot.h/c` (Robot 36/72)
**Checklist per file:**
- [ ] Move mode definitions from pisstvpp2_sstv.c
- [ ] Create function pointers for encode_frame
- [ ] Implement mode-specific encoding (buildaudio_m(), buildaudio_s(), buildaudio_r36(), buildaudio_r72())
- [ ] Register with registry in each file
**Examples:**
```c
// modes_martin.c
static int martin1_encode_frame(const image_buffer_t *img, audio_samples_t *audio) {
    // Implementation (moved from buildaudio_m)
}

const mode_definition_t mode_martin_1 = {
    .code = "m1",
    .name = "Martin 1",
    .vis_code = 44,
    // ... rest
    .encode_frame = martin1_encode_frame,
    .source = "native",
};
```
**Testing:**
- [ ] Each mode produces identical audio to v2.0
- [ ] All 7 modes listed in registry
- [ ] Mode lookup works correctly
**Success Criteria:** Native modes modularized, identical behavior

#### Task 3.3: Create Mode Initialization (2-3 hours)
**Files:** Create `src/sstv/sstv_native.h/c`
**Checklist:**
- [ ] Create `sstv_native_register(mode_registry_t *reg)` function
- [ ] Call registry_add_mode() for each native mode
- [ ] Ensure all modes have proper metadata
**Testing:**
- [ ] Registry contains exactly 7 native modes
- [ ] Mode lookup works for each mode
- [ ] No duplicate mode codes
**Success Criteria:** Native modes registered at startup

#### Task 3.4: Implement Mode Dispatcher (3-4 hours)
**Files:** Create `src/sstv/sstv_encoder.h/c`
**Checklist:**
- [ ] Create `sstv_encoder_encode()` function
- [ ] Look up mode from registry by code
- [ ] Call appropriate mode-specific encoder
- [ ] Handle errors from mode encoder
- [ ] Return samples to audio encoder
**Testing:**
- [ ] Works with each native mode
- [ ] Output identical to direct calls
- [ ] Error handling for invalid modes
**Success Criteria:** Unified encoder dispatcher working

---

### Phase 4: MMSSTV Library Integration (14-18 hours)

#### Task 4.1: Design MMSSTV Loader (4-5 hours)
**Files:**
- Create `src/include/mmsstv_stub.h` (MMSSTV API contract—stub for now)
- Create `src/mmsstv/mmsstv_loader.h/c` (Loader implementation)
**Checklist:**
- [ ] Define expected MMSSTV API in mmsstv_stub.h
- [ ] Create `mmsstv_loader_t` structure
- [ ] Implement environment variable checking (MMSSTV_LIB_PATH, MMSSTV_INCLUDE_PATH)
- [ ] Implement pkg-config detection
- [ ] Implement standard path searching
- [ ] Use dlopen/dlsym for dynamic loading
- [ ] Provide detailed error messages
**Testing:**
- [ ] Detection works with library present (when available)
- [ ] Graceful fallback without library
- [ ] Error messages are helpful
**Success Criteria:** MMSSTV library detection functional, non-fatal if missing

#### Task 4.2: Create MMSSTV Adapter (5-6 hours)
**Files:** Create `src/mmsstv/mmsstv_adapter.h/c`
**Checklist:**
- [ ] Create `mmsstv_adapter_init()` to load library
- [ ] Load mode list from MMSSTV library (if available)
- [ ] Create `mode_definition_t` for each MMSSTV mode
- [ ] Implement encoding bridge for MMSSTV modes
- [ ] Handle library-not-found gracefully
- [ ] Cache mode list
**Testing:**
- [ ] Modes loaded from library (if available)
- [ ] Graceful skip if library missing
- [ ] Mode structures valid
**Success Criteria:** MMSSTV modes available to registry when library present

#### Task 4.3: Integrate with Mode Registry (2-3 hours)
**Files:** Modify `src/sstv/mode_registry.c`
**Checklist:**
- [ ] Update `registry_create()` to:
  - Call `sstv_native_register()` first (always)
  - Call `mmsstv_adapter_init()` after (gracefully fails if no library)
- [ ] Verify no duplicate mode codes (native vs MMSSTV)
- [ ] Document initialization order
**Testing:**
- [ ] 7 native modes always present
- [ ] 50+ MMSSTV modes present if library available
- [ ] No duplicates
- [ ] Registry functions with both sets
**Success Criteria:** Dynamic mode system fully integrated

#### Task 4.4: Update CLI for Dynamic Modes (3-4 hours)
**Files:** Modify `src/pisstvpp2.c`
**Checklist:**
- [ ] Create `show_native_modes()` function
- [ ] Create `show_mmsstv_modes()` function (if available)
- [ ] Update `show_help()` to display modes dynamically
- [ ] Add `--list-modes` option
- [ ] Add `--mmsstv-status` option (show library status)
- [ ] Make mode selection examples dynamic
- [ ] Update help text with availability indicators
**Testing:**
- [ ] Help text shows available modes
- [ ] --list-modes lists all modes with source
- [ ] --mmsstv-status shows library status
- [ ] Invalid mode selection properly rejected
**Success Criteria:** CLI scales with available modes dynamically

---

### Phase 5: Build System Enhancement (8-10 hours)

#### Task 5.1: Enhance Makefile for MMSSTV Detection (4-5 hours)
**Files:** Modify `Makefile`
**Checklist:**
- [ ] Add MMSSTV library detection section
- [ ] Check MMSSTV_LIB_PATH environment variable first
- [ ] Fall back to pkg-config
- [ ] Fall back to standard paths
- [ ] Report detection results in build output
- [ ] Add HAVE_MMSSTV build flag (optional, mostly documentation)
- [ ] Ensure graceful fallback if not found
**Example output:**
```
[BUILD] Detecting MMSSTV library...
[INFO]  MMSSTV: ✓ found at /usr/local/lib/libmmsstv.so
[BUILD] Building with MMSSTV support...
```
**Testing:**
- [ ] Build succeeds with MMSSTV available
- [ ] Build succeeds without MMSSTV
- [ ] Correct flags set in both cases
- [ ] After `make install`, binary works on other systems
**Success Criteria:** Smart Makefile, correct feature set

#### Task 5.2: Add Build Targets (2-3 hours)
**Files:** Modify `Makefile`
**Checklist:**
- [ ] Add `make debug` (debugging symbols, no optimization)
- [ ] Add `make sanitize` (AddressSanitizer, UBSan)
- [ ] Add `make analyze` (clang static analysis)
- [ ] Add `make coverage` (code coverage)
- [ ] Add `make install` (system installation)
- [ ] Add `make help` (build options documentation)
- [ ] Enhance `make test` (if needed)
**Testing:**
- [ ] Each target produces expected output
- [ ] Debug binary has symbols
- [ ] Sanitized binary catches errors
**Success Criteria:** Rich build targets available

#### Task 5.3: Improve Build Messages (2 hours)
**Files:** Modify `Makefile`
**Checklist:**
- [ ] Use $(info) for status messages
- [ ] Color-code output (✓/✗ for found/not found)
- [ ] Show detected library paths
- [ ] Show final mode count
- [ ] Report missing dependencies clearly
- [ ] Provide helpful suggestions
**Testing:**
- [ ] Build output is informative
- [ ] Missing dependencies clearly shown
**Success Criteria:** User-friendly build output

---

### Phase 6: Testing & Validation (9-12 hours)

#### Task 6.1: Backward Compatibility Testing (3-4 hours)
**Files:** Create `tests/test_compatibility.py` or expand `tests/test_suite.py`
**Checklist:**
- [ ] Run all 55 existing tests (verify 100% pass)
- [ ] Compare audio output to v2.0 baseline (byte-identical or validated equivalent)
- [ ] Test all command-line option combinations
- [ ] Verify error messages are helpful
- [ ] Test edge cases (empty images, invalid modes, etc.)
**Testing Target:** 55/55 tests passing with identical output
**Success Criteria:** No regressions, identical output to v2.0

#### Task 6.2: Modernization Validation (3-4 hours)
**Files:** Create unit tests for new modules
**Checklist:**
- [ ] Test error code system (all codes, all strings)
- [ ] Test config validation (invalid combinations)
- [ ] Test mode registry (add, lookup, list)
- [ ] Test image module separation (loader, processor, aspect)
- [ ] Test SSTV module separation
**Testing Target:** New modules covered by unit tests
**Success Criteria:** All new code tested

#### Task 6.3: MMSSTV Integration Testing (3-4 hours) 
**Files:** Create `tests/test_mmsstv_integration.py`
**Checklist:**
- [ ] Test library detection (with/without)
- [ ] Test mode enumeration from library (when available)
- [ ] Test encoding with MMSSTV modes (if library available)
- [ ] Test graceful degradation (native-only if library missing)
- [ ] Test environment variable overrides
- [ ] Test error handling
**Note:** Tests should skip gracefully if MMSSTV library not available
**Testing Target:** MMSSTV functionality validated
**Success Criteria:** Dynamic detection and fallback working

---

### Phase 7: Documentation (16-19 hours)

#### Task 7.1: Update Architecture Documentation (4-5 hours)
**Files:** Update `docs/ARCHITECTURE.md`
**Checklist:**
- [ ] Add mode registry system diagram
- [ ] Add dynamic mode loading flow
- [ ] Add MMSSTV library integration diagram
- [ ] Add build system flow
- [ ] Add revised module dependency graph
- [ ] Update error handling architecture
- [ ] Update config management flow
- [ ] Add examples of new patterns
**Success Criteria:** Architecture docs reflect v2.1 design

#### Task 7.2: Create Developer Communication (4-5 hours)
**Files:**
- Create `docs/DEVELOPER_ACCESSIBILITY.md` (for contributors)
- Create `docs/CODE_WALKTHROUGH.md` (code structure guide)
**Checklist DEVELOPER_ACCESSIBILITY.md:**
- [ ] Project philosophy (keep code accessible)
- [ ] Code organization overview
- [ ] Key concepts explained simply
- [ ] How to find what you're looking for
- [ ] Building for development
- [ ] Running tests
- [ ] Contributing changes
- [ ] Code review process

**Checklist CODE_WALKTHROUGH.md:**
- [ ] Main program flow
- [ ] Image loading and processing
- [ ] SSTV encoding process
- [ ] Audio format writing
- [ ] Mode selection mechanism
- [ ] Error handling patterns
- [ ] Testing patterns
**Success Criteria:** Clear path for community contributions

#### Task 7.3: Create Contributor Playbooks (4-5 hours)
**Files:**
- Create `docs/CONTRIB_ADDING_SSTV_MODES.md` (step-by-step guide)
- Create `docs/CONTRIB_ADDING_AUDIO_FORMATS.md`
- Create `docs/CONTRIB_FIXING_BUGS.md`
- Create `docs/CONTRIB_IMPROVING_CODE.md`
**Each document includes:**
- [ ] Understanding the system
- [ ] Step-by-step example
- [ ] Real working code example
- [ ] Testing approach
- [ ] Contribution process
**Success Criteria:** Community can easily contribute

#### Task 7.4: Create MMSSTV Setup Guide (3-4 hours)
**Files:** Create `docs/MMSSTV_SETUP.md`
**Checklist:**
- [ ] Overview of what MMSSTV library provides
- [ ] Where to find mmsstv-portable
- [ ] How to build MMSSTV library
- [ ] Installation instructions
- [ ] Environment variable configuration
- [ ] Verification that library detected
- [ ] Troubleshooting
- [ ] Examples using new modes
**Success Criteria:** Users can find and install MMSSTV

---

### Phase 8: Final Integration & Release Prep (10-13 hours)

#### Task 8.1: Comprehensive Testing Pass (4-5 hours)
**Checklist:**
- [ ] All 55 native tests passing
- [ ] New test suites passing (if MMSSTV available)
- [ ] Backward compatibility validated
- [ ] Static analysis clean (clang)
- [ ] Memory leak detection clean (valgrind)
- [ ] Platform testing (Linux, macOS, RPi)
- [ ] Build system works correctly
- [ ] Help and documentation accurate
**Success Criteria:** All quality gates met

#### Task 8.2: Documentation Review (2-3 hours)
**Checklist:**
- [ ] README.md reflects v2.1 features
- [ ] BUILD.md includes MMSSTV setup
- [ ] QUICK_START.md examples work
- [ ] Architecture docs accurate
- [ ] Developer guides helpful
- [ ] API documentation complete
- [ ] Error messages match documentation
**Success Criteria:** Docs complete and accurate

#### Task 8.3: Release Notes & Version Updates (2-3 hours)
**Files:** Update/create
- `CHANGELOG.md` - Add v2.1 section
- `docs/RELEASE_NOTES_v2.1.md` - Release announcement
- `README.md` - Version bump
- `Makefile` or version constant - Update version
**Checklist:**
- [ ] List all new features
- [ ] List all improvements
- [ ] List any bug fixes
- [ ] Document breaking changes (should be none)
- [ ] Provide migration guide (if needed)
- [ ] Draft release announcement
**Success Criteria:** v2.1 release ready

---

## Development Workflow

### Getting Started

```bash
# 1. Read the complete plan
cat /Users/ssamjung/Desktop/WIP/PiSSTVpp2/docs/PISSTVPP2_v2_1_MASTER_PLAN.md | less

# 2. Understand current state
cd /Users/ssamjung/Desktop/WIP/PiSSTVpp2
git status
git log --oneline -5

# 3. Build and verify current version
make clean && make all
./tests/test_suite.py
# Expected: All 55 tests pass, 137 KB binary

# 4. Create feature branch for Phase 1
git checkout -b feature/phase-1-error-codes

# 5. Start with Task 1.1
# Read: Task 1.1 "Implement Error Code System" above
# Create: src/util/error.h and src/util/error.c
# Code following the checklist
```

### Commit Strategy

```bash
# Small, focused commits with clear messages
git commit -m "feat(error): Add unified error code system

- Create src/util/error.h with PISSTVPP2_OK, ERR_* constants
- Implement error_string() for human-readable messages
- Implement error_log() for consistent logging
- All 55 existing tests still passing

Completes: Task 1.1 from PISSTVPP2_v2_1_MASTER_PLAN.md"

# Link to task list for traceability
```

### Testing Between Tasks

```bash
# After each task, verify:
make clean && make all           # No compile errors
./tests/test_suite.py            # All 55 tests pass (or +new tests)
make analyze                     # No static analysis issues (if available)

# Only commit when all tests pass
git add .
git commit -m "..."
```

### Workflow for Code Review

```
Developer commits → Creates PR → Reviewer reviews against:
  1. Master plan architecture spec
  2. Task success criteria
  3. Test suite results (55/55 passing)
  4. Code quality standards
→ Approved & merged → Next task begins
```

---

## Code Accessibility Principles

This codebase is designed for ham radio enthusiasts. Keep these principles in mind:

1. **Simple & Readable**
   - Prefer straightforward code over clever patterns
   - Use clear variable names
   - Keep functions focused

2. **Well-Documented**
   - Comments explain WHY, not just WHAT
   - Include examples for common patterns
   - Document assumptions and constraints

3. **Modular**
   - Single responsibility per file
   - Clear interfaces between modules
   - Minimal interdependencies

4. **Community-Friendly**
   - Easy to understand without entire codebase knowledge
   - Example code for common tasks
   - Clear contribution guidelines
   - Responsive to feedback

---

## Success Criteria & Gates

### Phase Success Gates
Each phase has success criteria. Only proceed to next phase when:
- ✅ All tasks in phase complete
- ✅ All new tests passing
- ✅ 55 existing tests still passing
- ✅ No regressions in functionality
- ✅ Code review approved

### Release Readiness (Before v2.1)
- ✅ All 8 phases complete
- ✅ 55/55 + new tests passing
- ✅ Static analysis clean
- ✅ Memory leak detection clean
- ✅ Platform testing complete
- ✅ Documentation complete
- ✅ Release notes ready

### Success Metrics
| Metric | Target | Current | Status |
|--------|--------|---------|--------|
| Test pass rate | 100% | 55/55 (Pending expansion) | ✓ |
| Modes (native) | 7 | 7 | ✓ |
| Modes (with MMSSTV) | 50+ | TBD | ⏳ |
| Code modules | 25+ | 11 | ⏳ |
| Documentation files | 10+ core | 28 existing | ⏳ |
| Backward compatible | Yes | Designed as Yes | ⏳ |

---

## Risk Mitigation

| Risk | Mitigation Strategy |
|------|-------------------|
| Break existing functionality | Keep test suite at 55/55 throughout, run tests after each task |
| MMSSTV library API unknown | Create stub interface first, iterate as library arrives, non-fatal if missing |
| Build system too complex | Keep pkg-config detection simple, graceful fallback to native-only |
| Timeline slips | Phase 2 (image refactoring) is lowest priority, can defer to v2.2 |
| Community doesn't adopt | Strong documentation, keep code simple, responsive to feedback |

---

## Timeline & Milestones

### Suggested Schedule (One Developer)

**Week 1:**
- [ ] Phase 1: Error handling, config, context (10-18 hrs total)
- [ ] Verify: 55/55 tests passing
- [ ] Decision: Proceed to Phase 2?

**Week 2:**
- [ ] Phase 2: Image refactoring + text overlay (24-26 hrs total)
  - Tasks 2.1-2.3: Standard refactoring (14-16 hrs)
  - **Task 2.4: Text overlay & color bars (8-10 hrs)** ← NEW
- [ ] Verify: All image transformations working
- [ ] Begin: Phase 3 mode registry (parallel possible)

**Week 3:**
- [ ] Phase 3: Mode registry (13-17 hrs total)
- [ ] Phase 4: MMSSTV integration (14-18 hrs total)
- [ ] Verify: Mode selection working, MMSSTV detection working

**Week 4:**
- [ ] Phase 5: Build system (8-10 hrs total)
- [ ] Phase 6: Testing (9-12 hrs total)
- [ ] Verify: All 55+ tests passing

**Week 5:**
- [ ] Phase 7: Documentation (16-19 hrs total)
  - Includes FCC Part 97 compliance documentation
  - Text overlay usage guide for ham radio operations
- [ ] Phase 8: Release prep (10-13 hrs total)
- [ ] v2.1 Ready for release!

**Total:** 90-115 hours spread over 5 weeks (realistic with interruptions)

**Note:** Phase 2 expanded due to text overlay feature. Phase 2 is now most time-intensive phase (24-26 hrs).

---

## Appendix: Quick References

### Git Workflow
```bash
# Create feature branch
git checkout -b feature/phase-1-error-codes

# Work on task, commit frequently
git commit -m "feat(error): Clear, concise message

- Detailed description
- Explains what changed
- References task number"

# Test before committing
make clean && make all && ./tests/test_suite.py

# Push and create PR
git push origin feature/phase-1-error-codes
# Then create PR in GitHub/similar
```

### Build Commands Reference
```bash
make clean        # Remove build artifacts
make all          # Build main executable
make test         # Run test suite
make debug        # Build with debugging symbols (future)
make sanitize     # Build with sanitizers (future)
make analyze      # Static analysis (future)
make install      # System installation (future)
make help         # Show available targets (future)
```

### Testing Commands
```bash
# Run full test suite
./tests/test_suite.py

# Run with verbose output
./tests/test_suite.py -v

# Run specific test
./tests/test_suite.py::TestProtocols

# Check test coverage (future)
make coverage
```

### Environment Variables (for MMSSTV)
```bash
# Set library path explicitly
export MMSSTV_LIB_PATH="/path/to/libmmsstv.so"
export MMSSTV_INCLUDE_PATH="/path/to/mmsstv/headers"

# or use pkg-config
pkg-config mmsstv-portable --cflags --libs

# Verify detection
./bin/pisstvpp2 --mmsstv-status
```

---

## Appendix: CLI Options (Text Overlay - Phase 2.4)

**Note:** These options are planned for implementation in Phase 2, Task 2.4.

### Text Overlay Options

```bash
# Basic text overlay on bottom bar with call sign
./pisstvpp2 -i image.png \
  --text-overlay "K0ABC/EN96" \
  --text-placement bottom \
  --text-size medium \
  -m m1 -o output.wav

# Detailed control of appearance
./pisstvpp2 -i image.png \
  --text-overlay "K0ABC/EN96" \
  --text-placement bottom \
  --text-size large              # small, medium, large, or pixels (e.g., 24)
  --text-color white             # RGB (255,255,255) or named color
  --text-bg-color black          # Background color for text region
  --text-bg-opacity 0.8          # Background opacity (0.0-1.0)
  --text-padding 10              # Padding around text in pixels
  -m m1 -o output.wav

# Multiple text lines (FCC compliance)
./pisstvpp2 -i image.png \
  --text-overlay-top "K0ABC"         # Station identification
  --text-overlay-bottom "EN96ab"     # Grid square
  --text-size medium \
  --text-color yellow \
  --text-bg-color "0,0,0" \
  -m m1 -o output.wav

# Color bar with text (common for SSTV)
./pisstvpp2 -i image.png \
  --color-bar bottom \               # Add solid color bar at bottom
  --bar-height 40 \                  # Bar height in pixels
  --bar-color "0,0,0" \              # Black bar for contrast
  --text-overlay "K0ABC/EN96" \      # Text on the bar
  --text-color white \
  --text-size medium \
  -m m1 -o output.wav

# FCC Part 97 compliant transmission
./pisstvpp2 -i image.png \
  --color-bar top \                  # Station ID at top
  --bar-height 30 \
  --bar-color "180,180,180" \       # Light gray
  --text-overlay "K0ABC / EN96ab" \
  --text-color black \
  --text-size large \
  -m s2 -o output.wav
```

### CLI Option Summary for Task 2.4

| Option | Values | Example | Purpose |
|--------|--------|---------|---------|
| `--text-overlay` | String | `"K0ABC/EN96"` | Text content to overlay |
| `--text-placement` | top, bottom, left, right, center | `bottom` | Where to place text |
| `--text-coordinate` | x,y | `"10,10"` | Custom position (optional) |
| `--text-size` | small, medium, large, N | `large` or `24` | Font size |
| `--text-color` | RGB or name | `"255,255,255"` or `white` | Text color |
| `--text-bg-color` | RGB or name | `"0,0,0"` | Text background color |
| `--text-bg-opacity` | 0.0-1.0 | `0.8` | Background transparency |
| `--text-padding` | Pixels | `10` | Space around text |
| `--color-bar` | top, bottom, left, right | `bottom` | Add solid color bar |
| `--bar-height` | Pixels | `40` | Height/width of bar |
| `--bar-color` | RGB or name | `"0,0,0"` | Bar background color |

---

## References & Resources

### Existing Documentation (v2.0 - Reference)
- [ARCHITECTURE.md](./ARCHITECTURE.md) - Current code structure
- [USER_GUIDE.md](./USER_GUIDE.md) - Usage reference
- [BUILD.md](./BUILD.md) - Build instructions
- [QUICK_START.md](./QUICK_START.md) - Getting started
- [MODE_REFERENCE.md](./MODE_REFERENCE.md) - SSTV mode specs

### Technical References
- [SSTV Specifications (PDF)](./SSTV%20Description%20and%20mode%20specifications.pdf)
- [SSTV Mode Details (PDF)](./SSTV%20Mode%20Details.pdf)
- [SSTV Handbook (PDF)](./sstv-handbook.pdf)

### External Resources
- libvips: https://libvips.github.io/
- MMSSTV (original): http://www.qsl.net/mmsstv/
- Ham radio community resources (TBD)

### Regulatory Compliance (FCC Part 97)
- **FCC Part 97.113(a)(4)** - Station ID requirements
  - Requires station call sign transmission at least every 10 minutes
  - End of contact allowed (covered by text overlay feature)
  - Particularly important for SSTV operations
- **FCC Part 97.119** - Station record requirements
  - Text overlay helps document transmission metadata
- **ARRL Rules and Regulations** - Amateur Radio Service guidelines
  - Text overlay enables compliance with good operating practices

**Note:** Phase 2, Task 2.4 includes documentation on FCC Part 97 Section 97.113(a)(4) compliance for SSTV transmissions with text overlay.

---

## Document Management

### Updates & Revisions
- **Version:** 1.0 (Created Feb 10, 2026)
- **Next Review:** After Phase 1 completion
- **Update Policy:** Update after each phase, document changes

### Related Documents
- `DELIVERY_SUMMARY.md` - High-level overview
- `DEVELOPMENT_QUICK_REFERENCE.md` - Workflow guide
- `DOCUMENTATION_INDEX.md` - Full documentation navigation

### Deprecation Notes
The following documents from v2.0 planning are superseded by this v2.1 plan:
- MMSSTV_INTEGRATION.md (old approach, replaced by Section A2)
- PISSTVPP2_v2_0_MASTER_PLAN.md Parts 2-3 (archived as reference)
- Various analysis/summary documents (archived, not referenced)

---

## Final Checklist Before Starting

Before beginning Phase 1, verify:

- [ ] You've read this entire document
- [ ] You understand the 8-phase structure
- [ ] You know what Task 1.1 involves
- [ ] Current v2.0 builds successfully: `make clean && make all`
- [ ] All 55 tests pass: `./tests/test_suite.py`
- [ ] You have a clean feature branch for Phase 1
- [ ] You understand error handling unification (Task 1.1)

**Ready? Begin Phase 1!**

---

**Last Updated:** February 10, 2026  
**Status:** Ready for Implementation  
**Next Action:** Start Phase 1 - Error Handling & Configuration

