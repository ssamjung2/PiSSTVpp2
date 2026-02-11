
# PiSSTVpp2 v2.1 Development Task List

**Organized by Phase, Priority, and Estimated Effort**

---

## Phase 1: Unified Error Handling & Configuration Management

### Task 1.1: Implement Error Code System
**Effort:** 4-6 hours  
**Status:** Not Started  
**Description:** Create centralized error definitions and string mapping

**Files to Create:**
- `src/util/error.h` - Error code constants
- `src/util/error.c` - Error string mapping

**Implementation:**
- [ ] Define 15-20 distinct error codes (PISSTVPP2_OK, ERR_ARGS, ERR_IMAGE_LOAD, etc.)
- [ ] Create `error_string(code)` function
- [ ] Create `error_log(code, context, ...)` function for logging
- [ ] Create `error_is_critical(code)` to classify errors
- [ ] Document error codes in header with examples

**Testing:**
- [ ] Verify each error code maps to human-readable string
- [ ] Test error_log output format

---

### Task 1.2: Create Configuration Management
**Effort:** 3-4 hours  
**Status:** Not Started  
**Description:** Centralized config structure and validation

**Files to Create:**
- `src/core/pisstvpp2_config.h` - Config structures
- `src/core/pisstvpp2_config.c` - Config parsing and validation

**Implementation:**
- [ ] Move `config_t` structure to config.h
- [ ] Create `config_init()` with sensible defaults
- [ ] Create `config_validate()` to check for invalid combinations
- [ ] Create `config_print()` for debugging
- [ ] Create getters for each config field

**Testing:**
- [ ] Test parsing all valid command-line option combinations
- [ ] Test rejection of invalid combinations
- [ ] Verify defaults are sensible

---

### Task 1.3: Create Context/State Management
**Effort:** 3-4 hours  
**Status:** Not Started  
**Description:** Unified global state container

**Files to Create:**
- `src/core/pisstvpp2_context.h` - Global state structure
- `src/core/pisstvpp2_context.c` - Context lifecycle

**Implementation:**
- [ ] Create `pisstvpp2_context_t` containing:
  - Config
  - Image buffer
  - SSTV state
  - Audio samples
  - MMSSTV loader state
- [ ] Create `context_create()` and `context_free()`
- [ ] Create thread-safe access patterns for future use
- [ ] Document ownership of pointers

**Testing:**
- [ ] Verify context creation/destruction doesn't leak memory
- [ ] Test context in multi-mode encoding scenario

---

### Task 1.4: Update All Modules to Use New Error Codes
**Effort:** 6-8 hours  
**Status:** Not Started  
**Description:** Replace scattered error handling with unified system

**Files to Update:**
- `src/pisstvpp2.c` - Main entry point
- `src/pisstvpp2_image.c` - Image loading
- `src/pisstvpp2_sstv.c` - SSTV encoding
- `src/pisstvpp2_audio_encoder.c` - Audio format orchestration
- `src/audio_encoder_*.c` - Format encoders

**Implementation per file:**
- [ ] Replace all `fprintf(stderr, ...)` with `error_log()`
- [ ] Replace all error returns (-1, 1, 2) with error codes
- [ ] Update error checking to use `if (rc != PISSTVPP2_OK)`
- [ ] Add error propagation context

**Testing:**
- [ ] Run test suite: all 55 tests must pass
- [ ] Test each error condition in error_handling tests
- [ ] Verify error messages are informative

---

## Phase 2: Image Module Refactoring

### Task 2.1: Extract Image Loader
**Effort:** 5-6 hours  
**Status:** Not Started  
**Description:** Separate image loading logic from processing

**Files to Create:**
- `src/image/image_loader.h` - Image loading interface
- `src/image/image_loader.c` - Implementation

**Files to Modify:**
- `src/pisstvpp2_image.c` - Remove loading code
- `src/pisstvpp2_image.h` - Update includes

**Implementation:**
- [ ] Move vips image loading to image_loader.c
- [ ] Move format detection to image_loader.c
- [ ] Create `image_loader_t` structure
- [ ] Create `image_load_from_file()` function
- [ ] Create `image_get_format()` function
- [ ] Add loading progress callbacks (for verbose mode)

**Testing:**
- [ ] Test loading all image formats (PNG, JPEG, GIF, BMP)
- [ ] Test error handling for missing/corrupt files
- [ ] Verify 55 tests still pass

---

### Task 2.2: Extract Image Processor
**Effort:** 4-5 hours  
**Status:** Not Started  
**Description:** Separate image transformation logic

**Files to Create:**
- `src/image/image_processor.h` - Processing interface
- `src/image/image_processor.c` - Implementation

**Files to Modify:**
- `src/pisstvpp2_image.c` - Remove processing code

**Implementation:**
- [ ] Move color space conversion to image_processor.c
- [ ] Move scaling operations to image_processor.c
- [ ] Create `image_to_rgb()` function
- [ ] Create `image_scale()` function
- [ ] Create `image_get_pixel_rgb()` function (utility)

**Testing:**
- [ ] Test RGB conversion for all image formats
- [ ] Verify scaling produces correct dimensions
- [ ] Test pixel access at edge cases

---

### Task 2.3: Extract Aspect Ratio Correction
**Effort:** 5-6 hours  
**Status:** Not Started  
**Description:** Dedicated module for aspect handling

**Files to Create:**
- `src/image/image_aspect.h` - Aspect correction interface
- `src/image/image_aspect.c` - Implementation

**Files to Modify:**
- `src/pisstvpp2_image.c` - Remove aspect code

**Implementation:**
- [ ] Move center-crop logic to image_aspect.c
- [ ] Move padding logic to image_aspect.c
- [ ] Move stretch logic to image_aspect.c
- [ ] Create `image_apply_aspect_correction()` function
- [ ] Create `aspect_calculate_dimensions()` helper
- [ ] Document aspect algorithm in detail

**Testing:**
- [ ] Test all 3 aspect modes (center, pad, stretch)
- [ ] Test with various input aspect ratios
- [ ] Verify output dimensions are exact for all modes
- [ ] Compare output to v2.0 baseline (verify identical)

---

### Task 2.4: Update Main Module for New Image Structure
**Effort:** 2-3 hours  
**Status:** Not Started  
**Description:** Update pisstvpp2.c to use refactored image modules

**Files to Modify:**
- `src/pisstvpp2.c` - Update image calls

**Implementation:**
- [ ] Replace `image_load_from_file()` with `image_loader_load()`
- [ ] Update argument passing to new modular functions
- [ ] Update error handling for new error codes
- [ ] Remove old `pisstvpp2_image.c` includes

**Testing:**
- [ ] Verify 55/55 tests pass
- [ ] Test with various image formats and aspect modes

---

## Phase 3: SSTV & Mode System Refactoring

### Task 3.1: Design Mode Registry System
**Effort:** 3-4 hours  
**Status:** Not Started  
**Description:** Architecture for dynamic mode management

**Files to Create:**
- `src/sstv/mode_registry.h` - Registry interface
- `src/sstv/mode_registry.c` - Registry implementation

**Implementation:**
- [ ] Define `mode_definition_t` structure:
  - Mode code (m1, pd120, etc.)
  - Mode name (Martin 1, PD-120)
  - VIS code
  - Resolution
  - Duration
  - Function pointers for encode
- [ ] Create `mode_registry_t` to hold all modes
- [ ] Implement `registry_create()`
- [ ] Implement `registry_add_mode()`
- [ ] Implement `registry_lookup_by_code()`
- [ ] Implement `registry_list_all()`
- [ ] Implement `registry_free()`

**Documentation:**
- [ ] Document mode registration process
- [ ] Provide examples for adding new modes

---

### Task 3.2: Extract Native Mode Definitions
**Effort:** 4-5 hours  
**Status:** Not Started  
**Description:** Move hardcoded modes to modular structure

**Files to Create:**
- `src/sstv/modes_martin.h` - Martin modes
- `src/sstv/modes_martin.c` - Martin implementations
- `src/sstv/modes_scottie.h` - Scottie modes
- `src/sstv/modes_scottie.c` - Scottie implementations
- `src/sstv/modes_robot.h` - Robot modes
- `src/sstv/modes_robot.c` - Robot implementations

**Implementation per file:**
- [ ] Move mode definitions from pisstvpp2_sstv.c
- [ ] Create function pointers for encode_frame
- [ ] Move mode-specific encoding logic
- [ ] Implement `buildaudio_m()`, `buildaudio_s()`, etc. as mode functions

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
    .width = 320,
    .height = 256,
    .transmission_time_ms = 114000,
    .encode_frame = martin1_encode_frame,
    .source = "native",
};
```

**Testing:**
- [ ] Each mode produces identical audio to v2.0
- [ ] All 7 modes listed in registry
- [ ] Mode lookup returns correct definitions

---

### Task 3.3: Create Mode Initialization
**Effort:** 2-3 hours  
**Status:** Not Started  
**Description:** Populate registry with native modes on startup

**Files to Create:**
- `src/sstv/sstv_native.h` - Native modes interface
- `src/sstv/sstv_native.c` - Registration function

**Implementation:**
- [ ] Create `sstv_native_register(mode_registry_t *reg)` function
- [ ] Add all 7 native modes to registry
- [ ] Ensure all modes have proper metadata

**Testing:**
- [ ] Verify registry contains exactly 7 native modes
- [ ] Test mode lookup for each mode
- [ ] Verify no duplicate mode codes

---

### Task 3.4: Implement Mode Dispatcher
**Effort:** 3-4 hours  
**Status:** Not Started  
**Description:** Unified encoder supporting mode selection

**Files to Create:**
- `src/sstv/sstv_encoder.h` - Unified encoder interface
- `src/sstv/sstv_encoder.c` - Dispatcher implementation

**Implementation:**
- [ ] Create `sstv_encoder_encode()` function
- [ ] Look up mode from registry
- [ ] Call appropriate mode-specific encoder
- [ ] Handle errors from mode encoder
- [ ] Return samples to audio encoder

**Testing:**
- [ ] Test with each native mode
- [ ] Verify output identical to direct mode calls
- [ ] Error handling for invalid modes

---

### Task 3.5: Update Main Module for Mode Registry
**Effort:** 2-3 hours  
**Status:** Not Started  
**Description:** Integrate mode registry into main application

**Files to Modify:**
- `src/pisstvpp2.c` - Replace SSTV calls

**Implementation:**
- [ ] Create mode registry on startup
- [ ] Validate requested mode in registry
- [ ] Display registry contents in help (-h)
- [ ] Display extended help (--list-modes) showing all modes
- [ ] Call sstv_encoder_encode() instead of mode-specific functions

**Testing:**
- [ ] Verify 55/55 tests pass
- [ ] Test --list-modes output
- [ ] Test help text includes all native modes

---

## Phase 4: MMSSTV Library Integration

### Task 4.1: Design MMSSTV Loader
**Effort:** 4-5 hours  
**Status:** Not Started  
**Description:** Runtime library detection mechanism

**Files to Create:**
- `src/include/mmsstv_stub.h` - MMSSTV API contract (stub)
- `src/mmsstv/mmsstv_loader.h` - Loader interface
- `src/mmsstv/mmsstv_loader.c` - Loader implementation

**Implementation:**
- [ ] Define expected MMSSTV API in mmsstv_stub.h
- [ ] Create mmsstv_loader_t structure
- [ ] Implement environment variable checking:
  - MMSSTV_LIB_PATH (direct library path)
  - MMSSTV_INCLUDE_PATH (header path)
- [ ] Implement pkg-config detection
- [ ] Implement standard path searching
- [ ] Use dlopen/dlsym for dynamic loading
- [ ] Provide detailed error messages if not found

**Configuration:**
```bash
export MMSSTV_LIB_PATH="/usr/local/lib/libmmsstv.so"
export MMSSTV_INCLUDE_PATH="/usr/local/include/mmsstv"
```

**Testing:**
- [ ] Test with library present (if available)
- [ ] Test without library (graceful degradation)
- [ ] Verify error messages are helpful

---

### Task 4.2: Create MMSSTV Adapter
**Effort:** 5-6 hours  
**Status:** Not Started  
**Description:** Bridge between registry and MMSSTV library

**Files to Create:**
- `src/mmsstv/mmsstv_adapter.h` - Adapter interface
- `src/mmsstv/mmsstv_adapter.c` - Adapter implementation (bridges to mmsstv_stub)

**Implementation:**
- [ ] Create `mmsstv_adapter_init()` function
- [ ] Load MMSSTV library if available
- [ ] Query available modes from library
- [ ] Create mode_definition_t for each MMSSTV mode
- [ ] Implement encoding bridge for MMSSTV modes
- [ ] Add adapter to mode registry

**Pseudo-code:**
```c
int mmsstv_adapter_init(mode_registry_t *reg) {
    mmsstv_loader_t *loader = mmsstv_loader_create();
    if (mmsstv_loader_load(loader) != 0) {
        // Library not available, that's OK
        mmsstv_loader_free(loader);
        return 0;  // Non-fatal
    }
    
    // Load mode list from library
    mmsstv_mode_t *modes;
    int count = loader->list_modes(&modes);
    
    for (int i = 0; i < count; i++) {
        mode_definition_t mode = {
            .code = modes[i].code,
            .name = modes[i].name,
            .source = "mmsstv",
            // ... metadata
            .encode_frame = mmsstv_encode_bridge,
        };
        registry_add_mode(reg, &mode);
    }
    
    return 0;
}
```

**Testing:**
- [ ] Test with MMSSTV library available
- [ ] Test without MMSSTV library (graceful skip)
- [ ] Verify modes added to registry
- [ ] Test environment variable override

---

### Task 4.3: Integrate with Mode Registry
**Effort:** 2-3 hours  
**Status:** Not Started  
**Description:** Add MMSSTV modes to registry at startup

**Files to Modify:**
- `src/sstv/mode_registry.c` - Update registry_create()

**Implementation:**
- [ ] Call `sstv_native_register()` first (always available)
- [ ] Call `mmsstv_adapter_init()` after (gracefully fails if no library)
- [ ] Verify no duplicate mode codes
- [ ] Document registry initialization order

**Testing:**
- [ ] 7 native modes always present
- [ ] 50+ MMSSTV modes present if library available
- [ ] No duplicates
- [ ] Registry functions correctly with both sets

---

### Task 4.4: Update Help and Mode Listing
**Effort:** 3-4 hours  
**Status:** Not Started  
**Description:** Dynamic CLI support reflecting available modes

**Files to Modify:**
- `src/pisstvpp2.c` - Update show_help() and new functions

**Implementation:**
- [ ] Create `show_native_modes()` function
- [ ] Create `show_mmsstv_modes()` function  
- [ ] Create `show_native_only_message()` if MMSSTV unavailable
- [ ] Update show_help() to display mode list dynamically
- [ ] Add `--list-modes` option
- [ ] Add `--mmsstv-status` option to show library status
- [ ] Make mode selection examples dynamic

**Output Example (with MMSSTV):**
```
Usage: pisstvpp2 -i INPUT -o OUTPUT [OPTIONS]

SSTV Modes:
  Native Modes (7):
    m1   - Martin 1 (320x256, 114s)
    m2   - Martin 2 (320x256, 58s)
    ... [5 more]
  
  MMSSTV Modes (50+):
    pd120  - PD-120 (512x256, 240s)
    ... [49 more]
```

---

## Phase 5: Build System Enhancement

### Task 5.1: Enhance Makefile with MMSSTV Detection
**Effort:** 4-5 hours  
**Status:** Not Started  
**Description:** Build-time MMSSTV library detection

**Files to Modify:**
- `Makefile` - Add detection and build options

**Implementation:**
- [ ] Add MMSSTV library detection section
- [ ] Check MMSSTV_LIB_PATH environment variable first
- [ ] Fall back to pkg-config
- [ ] Fall back to standard paths (/usr/lib, etc.)
- [ ] Report detection results in build output
- [ ] Add HAVE_MMSSTV build flag (optional, mostly for documentation)
- [ ] Ensure graceful fallback if not found

**Build Output:**
```
[BUILD] Detecting MMSSTV library...
[INFO]  MMSSTV: ✓ found at /usr/local/lib/libmmsstv.so
[BUILD] Building with MMSSTV support...
```

---

### Task 5.2: Add Build Targets
**Effort:** 2-3 hours  
**Status:** Not Started  
**Description:** Useful build and development targets

**Files to Modify:**
- `Makefile` - Add new targets

**Implementation:**
- [ ] Add `make debug` (debugging symbols, no optimization)
- [ ] Add `make sanitize` (AddressSanitizer, UBSan)
- [ ] Add `make analyze` (clang static analysis)
- [ ] Add `make coverage` (code coverage)
- [ ] Add `make install` (system installation)
- [ ] Add `make help` (build options documentation)

**Test targets (should already exist):**
- [ ] Ensure `make test` runs test suite
- [ ] Ensure `make test-unit` exists for unit tests (future)
- [ ] Ensure `make test-integration` exists (future)

---

### Task 5.3: Improve Build Messages
**Effort:** 2 hours  
**Status:** Not Started  
**Description:** Informative build output

**Files to Modify:**
- `Makefile` - Add message formatting

**Implementation:**
- [ ] Use $(info) for status messages
- [ ] Color-code output (✓/✗ for found/not found)
- [ ] Show detected library paths
- [ ] Show final mode count
- [ ] Report any missing dependencies clearly
- [ ] Provide helpful suggestions for missing dependencies

---

## Phase 6: Testing & Validation

### Task 6.1: Backward Compatibility Testing
**Effort:** 3-4 hours  
**Status:** Not Started  
**Description:** Ensure refactoring doesn't break functionality

**Files to Create:**
- `tests/test_compatibility.py` - Backward compatibility tests

**Implementation:**
- [ ] Run all 55 existing tests against refactored code
- [ ] Verify 100% pass rate
- [ ] Compare audio output byte-for-byte with v2.0 baseline
- [ ] Test all command-line option combinations
- [ ] Verify error messages are helpful
- [ ] Test edge cases (empty images, invalid modes, etc.)

**Testing Checklist:**
- [ ] All native modes produce identical audio
- [ ] All audio formats produce identical output
- [ ] All sample rates work correctly
- [ ] All aspect modes produce correct dimensions
- [ ] Default options work as before
- [ ] Help text is complete and accurate

---

### Task 6.2: MMSSTV Integration Testing (When Library Available)
**Effort:** 4-5 hours  
**Status:** Not Started  
**Description:** Validate MMSSTV modes and library detection

**Files to Create:**
- `tests/test_mmsstv_integration.py` - MMSSTV integration tests

**Implementation:**
- [ ] Test library detection (with/without available)
- [ ] Test mode enumeration from library
- [ ] Test encoding with MMSSTV modes (if library available)
- [ ] Test graceful degradation (native-only when library missing)
- [ ] Test environment variable overrides
- [ ] Test error handling for corrupted library

**Note:** These tests should skip gracefully if MMSSTV library not available

---

### Task 6.3: Modernization Validation Testing
**Effort:** 2-3 hours  
**Status:** Not Started  
**Description:** Verify code quality of refactored modules

**Files to Create:**
- `tests/test_refactoring.py` - Refactoring validation tests

**Implementation:**
- [ ] Test error code system (all codes return correct strings)
- [ ] Test config validation (invalid option combinations)
- [ ] Test mode registry (add, lookup, list)
- [ ] Test image module separation (loader, processor, aspect)
- [ ] Test SSTV module separation (modes, dispatcher)
- [ ] Test audio module separation (generator, encoders)

**Testing Approaches:**
- Direct function calls
- Scenario testing (mode encoding with registry)
- Memory leak detection (valgrind)
- Static analysis (clang)

---

## Phase 7: Documentation

### Task 7.1: Create MMSSTV Setup Guide
**Effort:** 3-4 hours  
**Status:** Not Started  
**Description:** Instructions for obtaining and configuring MMSSTV

**Files to Create:**
- `docs/MMSSTV_SETUP.md` - MMSSTV installation guide

**Content:**
- [ ] Overview of what MMSSTV library provides
- [ ] How to obtain mmsstv-portable (where to find it)
- [ ] Build instructions for MMSSTV library
- [ ] Installation to standard locations
- [ ] Environment variable configuration
- [ ] Verification that library is detected
- [ ] Troubleshooting detection issues
- [ ] Examples of using new modes

---

### Task 7.2: Create Developer Accessibility Guide
**Effort:** 4-5 hours  
**Status:** Not Started  
**Description:** Guide for community contributors

**Files to Create:**
- `docs/DEVELOPER_ACCESSIBILITY.md` - For contributors
- `docs/CODE_WALKTHROUGH.md` - Code structure guide

**Content - DEVELOPER_ACCESSIBILITY.md:**
- [ ] Project philosophy (keep code accessible)
- [ ] Code organization overview
- [ ] Key concepts explained simply
- [ ] How to find what you're looking for
- [ ] Building from source (development build)
- [ ] Running tests
- [ ] Contributing changes (process)
- [ ] Code review process
- [ ] Getting help/asking questions

**Content - CODE_WALKTHROUGH.md:**
- [ ] Main program flow
- [ ] Image loading and processing
- [ ] SSTV encoding process
- [ ] Audio format writing
- [ ] Mode selection mechanism
- [ ] Error handling patterns
- [ ] Testing patterns

---

### Task 7.3: Create Contributor Playbook
**Effort:** 3-4 hours  
**Status:** Not Started  
**Description:** Step-by-step guides for common tasks

**Files to Create:**
- `docs/CONTRIB_ADDING_MODES.md` - Add new SSTV mode howto
- `docs/CONTRIB_ADDING_FORMATS.md` - Add new audio format howto
- `docs/CONTRIB_FIXING_BUGS.md` - Bug fix workflow
- `docs/CONTRIB_IMPROVING_CODE.md` - Refactoring guidance

**Content - CONTRIB_ADDING_MODES.md:**
- [ ] Understanding mode structure
- [ ] Define new mode constants
- [ ] Implement encode function
- [ ] Register with mode registry
- [ ] Test the new mode
- [ ] Update documentation
- [ ] Example: step-by-step adding a real mode

**Similar for other playbooks**

---

### Task 7.4: Update Architecture Documentation
**Effort:** 4-5 hours  
**Status:** Not Started  
**Description:** Comprehensive updated architecture docs

**Files to Modify:**
- `docs/ARCHITECTURE.md` - Update for v2.1 structure

**New Content:**
- [ ] Mode registry system diagram
- [ ] Dynamic mode loading flow
- [ ] MMSSTV library integration diagram
- [ ] Build system flow
- [ ] Module dependency graph
- [ ] Error handling architecture
- [ ] Config management flow

---

## Phase 8: Final Integration & Release Prep

### Task 8.1: Comprehensive Testing Pass
**Effort:** 4-5 hours  
**Status:** Not Started  
**Description:** Final validation before release

**Testing Checklist:**
- [ ] All 55 native tests passing
- [ ] New test suites passing (if MMSSTV available)
- [ ] Backward compatibility validated
- [ ] Static analysis clean
- [ ] Memory leak detection (valgrind) clean
- [ ] Platform testing (Linux, macOS, RPi)
- [ ] Build system working correctly
- [ ] Help and documentation accurate

---

### Task 8.2: Documentation Review
**Effort:** 2-3 hours  
**Status:** Not Started  
**Description:** Ensure all documentation is current

**Review Checklist:**
- [ ] README.md reflects v2.1 features
- [ ] BUILD.md includes MMSSTV setup
- [ ] QUICK_START.md examples work
- [ ] Architecture docs accurate
- [ ] Developer guides helpful
- [ ] API documentation complete
- [ ] Error messages match documentation

---

### Task 8.3: Release Notes & Changelog
**Effort:** 2-3 hours  
**Status:** Not Started  
**Description:** Document changes for release

**Files to Create/Update:**
- `CHANGELOG.md` - v2.1 changes
- `docs/RELEASE_NOTES_v2.1.md` - Release announcement

**Content:**
- [ ] New features (mode registry, dynamic modes)
- [ ] Improvements (refactored modules, better errors)
- [ ] Bug fixes (if any in this phase)
- [ ] Breaking changes (none expected)
- [ ] Known issues (if any)
- [ ] Migration guide (if needed)

---

## Summary Task Matrix

### By Effort
| Effort | Count | Tasks |
|--------|-------|-------|
| 2-3 hrs | 9 | Tasks 1.4, 3.5, 4.2, 5.2, 5.3, 7.1, 8.1, 8.2 |
| 3-4 hrs | 10 | Tasks 1.1, 1.2, 1.3, 2.1, 3.4, 4.1, 5.1, 7.3, 8.3 |
| 4-5 hrs | 8 | Tasks 2.2, 3.2, 4.3, 5.4, 6.1, 7.2, 7.4 |
| 5-6 hrs | 6 | Tasks 1.4, 2.3, 3.1, 4.2, 6.2 |

**Total Estimated Effort:** 80-100 hours

### By Priority
**Must-Do (Critical Path):**
- Phase 1: Error handling & config (10-18 hrs)
- Phase 3: Mode registry (12-16 hrs)
- Phase 4: MMSSTV integration (11-14 hrs)
- Phase 5: Build system (6-8 hrs)
- Phase 8: Testing & validation (8-10 hrs)
- **Subtotal: 47-66 hours**

**Should-Do (Quality & Accessibility):**
- Phase 2: Image refactoring (16-20 hrs)
- Phase 7: Documentation (16-19 hrs)
- **Subtotal: 32-39 hours**

**Nice-to-Have (Polish):**
- Audio module refactoring
- Advanced build targets (sanitize, coverage)
- Comprehensive contributor playbooks

---

## Tracking & Milestones

### Weekly Targets
- **Week 1:** Phase 1 complete (error handling, config, context)
- **Week 2:** Phase 2 & 3 begin (image refactoring, mode registry)
- **Week 3-4:** Phase 3 complete (mode dispatcher, native mode extraction)
- **Week 5:** Phase 4 begin (MMSSTV loader design)
- **Week 6:** Phase 4 & 5 (adapter, build system)
- **Week 7:** Phase 6 & 7 (testing, documentation)
- **Week 8:** Phase 8 (final validation, release prep)

### Success Gates
- [ ] Each phase passes its local tests
- [ ] Full test suite maintains 55/55 pass rate
- [ ] No regressions in existing functionality
- [ ] Code quality metrics acceptable (static analysis, valgrind)
- [ ] Documentation complete and accurate

