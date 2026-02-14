# Project Structure Audit Report

**Date:** February 12, 2026  
**Scope:** Complete analysis of file organization, include paths, and structural consistency

---

## Executive Summary

The project structure is **mostly consistent** with intended organization, but has **5 moderate issues** that could cause maintenance problems and inconsistent build behavior. Most are style/consistency issues rather than functional problems.

**Issues Found:** 5
- **Critical:** 1 (include path inconsistency)
- **Moderate:** 4 (organizational/path issues)

---

## Good Practices Found ✓

1. **Clear separation of concerns** - Code organized by module (image, audio, config, etc.)
2. **Proper header/source separation** - headers in `src/include/`, sources in `src/`
3. **Utility isolation** - Error handling utilities in `src/util/`
4. **Legacy code isolation** - Old code in `src/legacy/` keeping main tree clean
5. **Test directory isolation** - All tests in `tests/` directory
6. **Image fixtures organized** - Test images in `tests/images/`
7. **Makefile well-structured** - Proper directory variables and compilation rules

---

## Critical Issues Found ⚠️

### Issue 1: Inconsistent Include Paths in Image Modules

**Location:** `src/image/*.c` files  
**Severity:** Critical (Build consistency)

**Problem:**
Image modules use relative path includes while all other sources use simple includes:

```c
// src/image/image_loader.c (INCONSISTENT)
#include "../include/image/image_loader.h"
#include "../util/error.h"
#include "../include/logging.h"

// src/pisstvpp2.c (CONSISTENT)
#include "pisstvpp2_image.h"
#include "error.h"
#include "logging.h"

// src/util/error.c (CONSISTENT)
#include "error.h"
```

**Files Affected:**
- [src/image/image_loader.c](src/image/image_loader.c#L40-L42) - 3 includes with paths
- [src/image/image_processor.c](src/image/image_processor.c#L23-L26) - 4 includes with paths
- [src/image/image_text_overlay.c](src/image/image_text_overlay.c#L20-L22) - 3 includes with paths
- [src/image/image_aspect.c](src/image/image_aspect.c#L30-L31) - 2+ includes with paths

**Impact:**
- Works because Makefile adds `-I` paths correctly
- But creates confusion: "which style should new code use?"
- Harder to move modules without breaking includes
- Inconsistent with codebase convention

**Fix Required:** Change image module includes to simple format:
```c
#include "image/image_loader.h"  // not "../include/image/image_loader.h"
#include "error.h"                // not "../util/error.h"
#include "logging.h"              // not "../include/logging.h"
```

---

## Moderate Issues Found

### Issue 2: Audio Encoder Files Located in Wrong Directory

**Location:** `src/` root  
**Severity:** Moderate (Organization)

**Problem:**
Audio encoder implementations should follow the module pattern:

```
Current (Inconsistent):
src/
  ├── audio_encoder_wav.c
  ├── audio_encoder_aiff.c
  ├── audio_encoder_ogg.c
  └── pisstvpp2_audio_encoder.c

Should Be:
src/
  ├── audio/
  │   ├── audio_encoder_wav.c
  │   ├── audio_encoder_aiff.c
  │   └── audio_encoder_ogg.c
  └── pisstvpp2_audio_encoder.c (or audio/audio_encoder.c)
```

**Files Affected:**
- [src/audio_encoder_wav.c](src/audio_encoder_wav.c)
- [src/audio_encoder_aiff.c](src/audio_encoder_aiff.c)
- [src/audio_encoder_ogg.c](src/audio_encoder_ogg.c)

**Current Structure:**
- Header is in `src/include/` (correct)
- Implementations in `src/` root (inconsistent with image modules)
- Image modules have `src/include/image/` and `src/image/`
- Audio should follow same pattern

**Impact:**
- Main directory getting cluttered
- Inconsistent module organization pattern
- Makes future modularity harder

**Note:** [src/pisstvpp2_audio_encoder.c](src/pisstvpp2_audio_encoder.c) is the main dispatcher and can stay in root.

---

### Issue 3: Test Audio Fixtures Mixed with Image Fixtures

**Location:** `tests/images/` directory  
**Severity:** Moderate (Organization)

**Problem:**
Audio output files stored alongside image test fixtures:

```
tests/images/
├── (image files) ✓
│   ├── test_320x240.png
│   ├── test_panel_1600x1200.png
│   └── color_bars_640x480.jpg
└── (AUDIO FILES - WRONG LOCATION) ✗
    ├── alt_color_bars_320x256.wav
    ├── alt_test_panel_900x692.aiff
    ├── alt_test_panel_900x692.ogg
    └── test_panel_1600x1200.wav
```

**Files Affected:**
- [tests/images/alt_color_bars_320x256.wav](tests/images/alt_color_bars_320x256.wav)
- [tests/images/alt_test_panel_900x692.aiff](tests/images/alt_test_panel_900x692.aiff)
- [tests/images/alt_test_panel_900x692.ogg](tests/images/alt_test_panel_900x692.ogg)
- [tests/images/test_panel_1600x1200.wav](tests/images/test_panel_1600x1200.wav)

**Also Issue:**
- [tests/custom_output.wav](tests/custom_output.wav) - test output artifact in root, should be in `tests/test_outputs/`

**Impact:**
- Confuses purpose of directory
- Makes it harder to understand what's a test fixture vs. expected output
- Mixes inputs with reference fixtures

**Fix:** Create `tests/fixtures/` structure:
```
tests/
├── images/           (only image fixtures)
├── fixtures/         (reference outputs, audio files used as inputs)
│   └── audio/
│       ├── alt_color_bars_320x256.wav
│       ├── alt_test_panel_900x692.aiff
│       └── ...
└── test_outputs/     (generated test outputs)
```

---

### Issue 4: Root-Level Test Utility Script

**Location:** [show_overlay_tests.py](show_overlay_tests.py) in project root  
**Severity:** Moderate (Organization)

**Problem:**
Convenience script in root directory instead of tests/:

```
Project Root (not primary source location):
├── show_overlay_tests.py  ← Should be in tests/

tests/ (where test utilities belong):
├── run_all_tests.py ✓
├── show_results.py ✓
├── run_comprehensive_tests.py ✓
└── test_suite.py ✓
```

**Files Affected:**
- [show_overlay_tests.py](show_overlay_tests.py)

**Impact:**
- Inconsistent with other test utilities location
- Makes it unclear which files are user-facing vs. development utilities

**Note:** This is a minor concern - having convenience scripts in root for quick access is acceptable if documented, but should be symlinked OR moved to tests/ with README instructions.

---

### Issue 5: Hardcoded Absolute Paths in Test Files

**Location:** `tests/*.py` test files  
**Severity:** Moderate (Portability)

**Problem:**
Several test files have hardcoded absolute paths:

```python
# tests/test_integration.py - Line 43
def __init__(self, executable_path="/Users/ssamjung/Desktop/WIP/PiSSTVpp2/bin/pisstvpp2",
             images_dir="/Users/ssamjung/Desktop/WIP/PiSSTVpp2/tests/images"):

# tests/test_file_io_errors.py - Line 30
test_image="/Users/ssamjung/Desktop/WIP/PiSSTVpp2/tests/images/test_320x240.png"

# tests/run_all_tests.py - Line 28
def __init__(self, executable_path="/Users/ssamjung/Desktop/WIP/PiSSTVpp2/bin/pisstvpp2"):
```

**Files Affected:**
- [tests/test_integration.py#L43](tests/test_integration.py#L43)
- [tests/test_integration.py#L455](tests/test_integration.py#L455)
- [tests/test_file_io_errors.py#L30](tests/test_file_io_errors.py#L30)
- [tests/test_file_io_errors.py#L626](tests/test_file_io_errors.py#L626)
- [tests/run_all_tests.py#L28](tests/run_all_tests.py#L28)
- [tests/run_all_tests.py#L268](tests/run_all_tests.py#L268)

**Impact:**
- Tests won't work if cloned to different location
- Makes project non-portable
- CI/CD won't work
- Makes tests harder to run on other machines

**Should Use:** Relative paths or Path resolution:
```python
# Better:
from pathlib import Path
test_exe = Path(__file__).parent.parent / "bin" / "pisstvpp2"
test_images = Path(__file__).parent / "images"
```

---

## Structural Summary

### Current Organization

```
✓ GOOD: Follows intended patterns
├── src/
│   ├── include/              (headers - organized)
│   ├── image/                (image module - organized)
│   ├── util/                 (utilities - organized)
│   ├── legacy/               (old code - isolated)
│   ├── pisstvpp2*.c          (main modules)
│   └── audio_encoder_*.c     (✗ should be in audio/)
├── tests/
│   ├── images/               (✓ test fixtures)
│   │   └── (audio files) ✗   (should be elsewhere)
│   ├── test_outputs/         (generated outputs)
│   └── test_*.py             (test code ✓)
└── show_overlay_tests.py     (✗ should be in tests/)
```

### Include Path Summary

```
Consistent Pattern (Most Files):
  #include "header.h"

Inconsistent Pattern (Image Modules Only):
  #include "../include/image/header.h"
  #include "../util/error.h"
```

---

## Recommendations (Priority Order)

### Priority 1: FIX (Will cause issues)
1. **Fix image module include paths** - Change all `../` relative paths to simple includes
   - Estimated effort: 10 minutes
   - Affected lines: ~12 includes across 4 files

### Priority 2: IMPROVE (Better organization)
2. **Move audio encoder implementations to `src/audio/`** - Match image module pattern
   - Estimated effort: 30 minutes (updates Makefile + file moves + includes)
   - Benefits: Consistency, scalability, future-proofing

3. **Separate test fixtures from outputs** - Create `tests/fixtures/` structure
   - Estimated effort: 15 minutes
   - Benefits: Clarity, maintainability

4. **Fix hardcoded paths in test files** - Use relative/Path-based resolution
   - Estimated effort: 30 minutes
   - Benefits: Portability, CI/CD compatibility

### Priority 3: ORGANIZE (Nice to have)
5. **Move/symlink `show_overlay_tests.py`** - Keep convenience with organization
   - Estimated effort: 5 minutes
   - Benefits: Consistency

---

## Testing the Changes

Run this after applying fixes:
```bash
# Verify build still works
make clean && make all

# Verify tests still pass
cd tests && python3 test_suite.py

# Verify includes are correct
grep -r "#include.*\.\." src/  # Should show nothing
```

---

## File-by-File Fix Reference

| File | Issue | Line(s) | Fix |
|------|-------|---------|-----|
| [src/image/image_loader.c](src/image/image_loader.c) | Relative includes | 40-42 | Change `../include/` to just `image/` |
| [src/image/image_processor.c](src/image/image_processor.c) | Relative includes | 23-26 | Change `../include/` to just `image/` |
| [src/image/image_text_overlay.c](src/image/image_text_overlay.c) | Relative includes | 20-22 | Change `../include/` to just `image/` |
| [src/image/image_aspect.c](src/image/image_aspect.c) | Relative includes | 30-31 | Change `../include/` to just `image/` |
| [src/audio_encoder_*.c](src/) | Location | All | Move to `src/audio/` subdirectory |
| [tests/images/](tests/images/) | Mixed content | All | Move `.wav`, `.aiff`, `.ogg` to `tests/fixtures/` |
| [tests/custom_output.wav](tests/custom_output.wav) | Wrong location | N/A | Move to `tests/test_outputs/` |
| [tests/*.py](tests/) | Hardcoded paths | Various | Change to relative paths |
| [show_overlay_tests.py](show_overlay_tests.py) | Root location | N/A | Move to `tests/` or symlink |

---

## Conclusion

The project structure is **fundamentally sound** with only **style and portability issues**, not architectural problems. The most critical issue is the **include path inconsistency** in image modules, which while working now, violates the principle of consistency that makes code maintainable.

**Recommended Action:** Apply Priority 1 fixes immediately, Priority 2 before next major release, Priority 3 as cleanup.

All issues are **straightforward to fix** and involve no functional changes - just reorganization and path updates.
