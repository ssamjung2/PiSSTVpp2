# Comprehensive Project Rename Plan: pisstvpp2 → slowframe
**Date:** February 13, 2026  
**Status:** Pre-execution  
**Risk Level:** Medium (Requires careful sequencing)

---

## Table of Contents
1. [Overview](#overview)
2. [Scope Analysis](#scope-analysis)
3. [Execution Strategy](#execution-strategy)
4. [Detailed Implementation Steps](#detailed-implementation-steps)
5. [Validation Procedures](#validation-procedures)
6. [Rollback Plan](#rollback-plan)

---

## Overview

This document outlines a comprehensive, step-by-step approach to rename the project from **pisstvpp2** (Raspberry Pi SSTV Image to Audio Encoder v2) to **slowframe** throughout the entire codebase, build system, tests, and documentation.

### Why This Rename?
- **slowframe** is a more memorable, modern brand name
- Aligns project identity with workspace folder name
- Maintains consistency across all project artifacts

### Key Principles
1. **Non-breaking**: Validate build and tests at each major checkpoint
2. **Traceable**: Update configuration files first, then executables, then content
3. **Comprehensive**: Every reference must be updated to prevent confusion
4. **Reversible**: Git history preserved for rollback if needed

---

## Scope Analysis

### Total References Found: 150+ occurrences across 60+ files

#### 1. Source Files (Primary Modules)
**Files to Rename:**
- `src/pisstvpp2.c` → `src/slowframe.c` (main entry point)
- `src/pisstvpp2_image.c` → `src/slowframe_image.c`
- `src/pisstvpp2_sstv.c` → `src/slowframe_sstv.c`
- `src/pisstvpp2_audio_encoder.c` → `src/slowframe_audio_encoder.c`
- `src/pisstvpp2_config.c` → `src/slowframe_config.c`
- `src/pisstvpp2_context.c` → `src/slowframe_context.c`
- `src/pisstvpp2_mmsstv_adapter.c` → `src/slowframe_mmsstv_adapter.c`

**Status:** 7 files

#### 2. Header Files (Include Directory)
**Files to Rename:**
- `src/include/pisstvpp2_image.h` → `src/include/slowframe_image.h`
- `src/include/pisstvpp2_sstv.h` → `src/include/slowframe_sstv.h`
- `src/include/pisstvpp2_audio_encoder.h` → `src/include/slowframe_audio_encoder.h`
- `src/include/pisstvpp2_config.h` → `src/include/slowframe_config.h`
- `src/include/pisstvpp2_context.h` → `src/include/slowframe_context.h`
- `src/include/pisstvpp2_mmsstv_adapter.h` → `src/include/slowframe_mmsstv_adapter.h`

**Status:** 6 files

#### 3. Build Configuration
**Critical Files:**
- `makefile` - Update all references and targets
- `makefile.original` - Legacy makefile (careful rename consideration)
- All build targets and object file references

**References:**
- ~30 occurrences in makefile
- Binary output targets
- Source file lists
- Object file references

**Status:** Multiple changes per file

#### 4. Test Infrastructure
**Test Scripts:**
- `tests/util/run_all_tests.py` - Update executable paths
- `tests/util/run_comprehensive_tests.py`
- `tests/util/test_integration.py` - Update temp directories, executable names
- `tests/util/test_edge_cases.py`
- `tests/util/test_error_codes.py` - Error code constant names
- `tests/util/test_file_io_errors.py`
- `tests/util/test_performance_stress.py`
- `tests/util/test_security_exploits.py`
- `tests/util/test_suite.py`
- `tests/util/test_suite.sh` - Shell script executables
- `tests/util/test_suite_bash.sh`

**Bash Scripts:**
- `util/sstvcam.sh`
- `util/test_aspect_modes.sh`
- `util/test_dimension_verification.sh`
- `util/test_visual_comparison.sh`
- `src/legacy/build_legacy.sh`

**Binary References:**
- `tests/pisstvpp2` → `tests/slowframe` (test executable)

**Status:** 14+ test files

#### 5. Binary/Executable Names
**Primary Targets:**
- `bin/pisstvpp2` → `bin/slowframe` (main executable)
- `bin/pisstvpp2_libgd` → `bin/slowframe_libgd`
- `bin/pisstvpp2_san` → `bin/slowframe_san`
- `bin/vips_test` → `bin/vips_test` (no change needed)

**Status:** 3 main executables

#### 6. Code Content Updates
**Constant Names:**
- All `PISSTVPP2_*` constants → `SLOWFRAME_*`
- Examples: `PISSTVPP2_OK`, `PISSTVPP2_ERR_*`, etc.
- ~50+ constants across multiple files

**Function Names:**
- `pisstvpp_*` → `slowframe_*` (lower-level functions)
- `pisstvpp2_*` → `slowframe_*` (main functions)
- Examples: `pisstvpp2_image_load()`, `pisstvpp2_init()`, etc.

**Structure Names:**
- `PisstvppContext` → `SlowframeContext`
- Other `Pisstvpp*` structs

**Comments and Documentation:**
- Update file headers
- Update function documentation comments
- Update example commands in comments
- Update author/copyright notices

**Status:** Hundreds of occurrences

#### 7. Documentation Files
**Main Documentation:**
- `README.md` - Title, features description, examples
- `CONTRIBUTING.md` - Project name, setup instructions, examples
- `LICENSE.md`
- `ARCHITECTURE.md`
- `BUILD.md` - Build instructions and examples
- And 50+ other documentation files in `/docs/`

**Status:** Major documentation updates needed

#### 8. Test Output and Artifacts
**Test Results:**
- `tests/test_run.log` - Contains executable paths and project name references
- `tests/test_report_latest.json` - JSON with paths and project references
- `tests/comprehensive_test_report.json`

**Status:** Updated after verification

---

## Execution Strategy

### Phase 1: Preparation (Risk Mitigation)
1. **Create Git branch** for this rename work
2. **Document baseline** - Current git commit
3. **Backup** test results and known-good builds

### Phase 2: File System Operations (Must be first)
1. Rename all source files
2. Rename all header files
3. Create new binary output structure

### Phase 3: Build System Updates
1. Update makefile with new filenames
2. Update makefile targets and output names
3. Verify build completes successfully

### Phase 4: Code Content Updates
1. Update all #include statements
2. Update all PISSTVPP2_* constants to SLOWFRAME_*
3. Update all function names
4. Update all struct/type names
5. Update comments and documentation strings

### Phase 5: Test Infrastructure
1. Update test scripts with new executable names
2. Update test framework references
3. Rebuild and run tests

### Phase 6: Documentation
1. Update README, CONTRIBUTING, and main docs
2. Update inline code comments
3. Update LICENSE and other meta files

### Phase 7: Validation
1. Build project from scratch
2. Run full test suite
3. Verify no "pisstvpp2" references remain
4. Test executable functionality

---

## Detailed Implementation Steps

### STEP 1: Create Git Branch
```bash
git checkout -b rename/pisstvpp2-to-slowframe
git commit -m "Initial commit before rename"
```
**Impact:** None - just creates working branch  
**Risk:** Minimal

### STEP 2: Rename Source Files
Files to rename in `src/`:
1. `pisstvpp2.c` → `slowframe.c`
2. `pisstvpp2_image.c` → `slowframe_image.c`
3. `pisstvpp2_sstv.c` → `slowframe_sstv.c`
4. `pisstvpp2_audio_encoder.c` → `slowframe_audio_encoder.c`
5. `pisstvpp2_config.c` → `slowframe_config.c`
6. `pisstvpp2_context.c` → `slowframe_context.c`
7. `pisstvpp2_mmsstv_adapter.c` → `slowframe_mmsstv_adapter.c`

**Commands:**
```bash
cd src/
for f in pisstvpp2_*.c pisstvpp2.c; do
  mv "$f" "${f/pisstvpp2/slowframe}"
done
```
**Impact:** File system only - no code breakage yet  
**Risk:** Low

### STEP 3: Rename Header Files
Files to rename in `src/include/`:
1. `pisstvpp2_image.h` → `slowframe_image.h`
2. `pisstvpp2_sstv.h` → `slowframe_sstv.h`
3. `pisstvpp2_audio_encoder.h` → `slowframe_audio_encoder.h`
4. `pisstvpp2_config.h` → `slowframe_config.h`
5. `pisstvpp2_context.h` → `slowframe_context.h`
6. `pisstvpp2_mmsstv_adapter.h` → `slowframe_mmsstv_adapter.h`

**Commands:**
```bash
cd src/include/
for f in pisstvpp2_*.h; do
  mv "$f" "${f/pisstvpp2/slowframe}"
done
```
**Impact:** File system only  
**Risk:** Low

### STEP 4: Update Include Statements in Source Files
**Files to update:**
- All 7 `.c` files in `src/`
- Update pattern: `#include "pisstvpp2_` → `#include "slowframe_`

**Example changes:**
```c
// Before:
#include "pisstvpp2_image.h"
#include "pisstvpp2_sstv.h"

// After:
#include "slowframe_image.h"
#include "slowframe_sstv.h"
```

**Risk:** High if done incorrectly - will break compilation  
**Verification:** Must compile after this step

### STEP 5: Update Header Guard Macros
**Files to update:** All 6 header files  
**Pattern:**
```c
// Before:
#ifndef PISSTVPP2_IMAGE_H
#define PISSTVPP2_IMAGE_H
...
#endif /* PISSTVPP2_IMAGE_H */

// After:
#ifndef SLOWFRAME_IMAGE_H
#define SLOWFRAME_IMAGE_H
...
#endif /* SLOWFRAME_IMAGE_H */
```

**Risk:** High - critical for compilation  
**Verification:** Must compile after this step

### STEP 6: Update Enum/Constant Definitions
**Files to update:** All header files  
**Pattern:**
```c
// Before:
enum PisstvppStatus {
    PISSTVPP2_OK = 0,
    PISSTVPP2_ERR_IMAGE_LOAD = 81,
    ...
};

// After:
enum SlowframeStatus {
    SLOWFRAME_OK = 0,
    SLOWFRAME_ERR_IMAGE_LOAD = 81,
    ...
};
```

**Critical constants to update:**
- `PISSTVPP2_OK` → `SLOWFRAME_OK`
- `PISSTVPP2_ERR_*` → `SLOWFRAME_ERR_*`
- All other `PISSTVPP2_*` defines

**Risk:** High - code will reference old constants  
**Verification:** Must compile after this step

### STEP 7: Update Function Names
**Pattern:**
```c
// Before:
int pisstvpp2_image_load(ImageBuffer *buf, const char *filename);

// After:
int slowframe_image_load(ImageBuffer *buf, const char *filename);
```

**Scope:** All function declarations and definitions  
**Risk:** Very High - widespread usage  
**Verification:** Must compile and all references must update

### STEP 8: Update Structure Names
**Pattern:**
```c
// Before:
typedef struct {
    ImageBuffer *image;
    ...
} PisstvppContext;

// After:
typedef struct {
    ImageBuffer *image;
    ...
} SlowframeContext;
```

**Risk:** High - requires updating all usage sites

### STEP 9: Update Comments and Documentation
**Patterns to update:**
- File headers: `@file pisstvpp2.c` → `@file slowframe.c`
- Brief descriptions: `@brief PiSSTVpp2` → `@brief SlowFrame`
- Function documentation with examples
- Author lines: `@author PiSSTVpp2 Contributors` → `@author SlowFrame Contributors`
- Example commands in comments

**Risk:** Low - doesn't affect compilation  
**Verification:** Manual review

### STEP 10: Update Makefile
**Changes needed:**
1. Change `TARGET = $(BIN_DIR)/pisstvpp2` → `TARGET = $(BIN_DIR)/slowframe`
2. Change all source file references
3. Change all object file references
4. Update target names (`TARGET_LIBGD`, `TARGET_SAN`)
5. Update clean targets

**Risk:** High - build will fail if incorrect  
**Verification:** Must build successfully

### STEP 11: Update Test Scripts
**Python test files:**
- Update executable paths from `bin/pisstvpp2` → `bin/slowframe`
- Update temporary directory prefixes
- Update project references in output strings

**Bash test scripts:**
- Update executable variable names and values
- Update test description strings

**Risk:** Medium - tests may fail but won't break compilation  
**Verification:** Tests must run and pass

### STEP 12: Update Test Executable Name
- Rename `tests/pisstvpp2` → `tests/slowframe`
- Update any references to this path

**Risk:** Low  
**Verification:** Test scripts must find executable

### STEP 13: Update Error Code References in Tests
- Update test_error_codes.py to use new `SLOWFRAME_ERR_*` constants
- Update expected error messages

**Risk:** Medium - tests may fail  
**Verification:** Error code tests must pass

### STEP 14: Update Legacy Build Script
- Update `src/legacy/build_legacy.sh` with appropriate references
- Consider: legacy code might intentionally keep old names

**Risk:** Low - only affects legacy compatibility testing  
**Verification:** Legacy script should still work or be marked deprecated

### STEP 15: Update Documentation Files
**Priority files:**
1. `README.md` - Update title, feature descriptions, examples
2. `CONTRIBUTING.md` - Update project name, setup instructions
3. `ARCHITECTURE.md` - Update project references
4. `BUILD.md` - Update build instructions and examples
5. All other docs in `/docs/` directory

**Risk:** Low - doesn't affect compilation  
**Verification:** Documentation quality review

### STEP 16: Final Verification
1. **Full rebuild:**
   ```bash
   make clean
   make all
   ```

2. **Run test suite:**
   ```bash
   make test
   ```

3. **Check for remaining references:**
   ```bash
   grep -r "pisstvpp2" . --exclude-dir=.git --exclude-dir=bin
   ```

4. **Verify binary execution:**
   ```bash
   ./bin/slowframe --help
   ```

---

## Implementation Order Rationale

The sequence above is optimized to:

1. **Minimize compilation errors** - File renames first, then #includes, then constants, then functions
2. **Enable early verification** - Build after each category of changes
3. **Track changes clearly** - Each step is discrete and testable
4. **Prevent cascading failures** - Update dependencies before dependents
5. **Maintain partial functionality** - Can roll back at each step if needed

---

## Validation Procedures

### Checkpoint 1: After File Renames (Step 3)
- All files renamed
- File system structure correct
- No code changes yet

### Checkpoint 2: After Include Updates (Step 5)
- Project compiles
- No include errors
- Linker may still fail

### Checkpoint 3: After Constant Updates (Step 6)
- Project compiles
- All references resolve
- Some runtime issues may exist

### Checkpoint 4: After Function Updates (Step 7)
- Project compiles
- Functions resolve correctly
- Ready for build verification

### Checkpoint 5: After Makefile (Step 10)
- Clean build successful
- Executable generated at new location
- `make test` passes

### Checkpoint 6: After All Updates
- Full test suite passes
- No "pisstvpp2" references remain
- Documentation updated
- Binary executable works correctly

---

## Rollback Plan

If issues are encountered:

```bash
# Return to pre-rename state
git reset --hard HEAD~N  # Where N is number of commits made
git checkout main
```

Or manual rollback:
```bash
# Rename files back
cd src/
for f in slowframe*.c slowframe.c; do
  mv "$f" "${f/slowframe/pisstvpp2}"
done

# Revert code changes via git
git checkout -- .
```

---

## Risk Assessment

| Step | Risk | Mitigation |
|------|------|-----------|
| 1-3 (File renames) | Low | Just file system operations |
| 4-5 (Includes/guards) | High | Compile immediately after |
| 6-7 (Constants/functions) | Very High | Systematic replacement required |
| 8 (Structures) | High | Check all usage sites |
| 9 (Comments) | Low | Informational only |
| 10 (Makefile) | High | Test build immediately |
| 11-13 (Tests) | Medium | Run tests thoroughly |
| 14-15 (Docs) | Low | Quality review only |

---

## Estimated Timeline

- **File operations:** 5 minutes
- **Code updates:** 30-45 minutes (systematic replacement)
- **Build testing:** 10 minutes
- **Test suite:** 15-20 minutes
- **Documentation:** 20-30 minutes
- **Final validation:** 10 minutes
- **Total:** ~2-2.5 hours

---

## Success Criteria

✓ All pisstvpp2 files renamed to slowframe  
✓ All header files renamed with new guards  
✓ All constants renamed (PISSTVPP2_* → SLOWFRAME_*)  
✓ All functions renamed (pisstvpp2_* → slowframe_*)  
✓ All structures renamed (Pisstvpp* → Slowframe*)  
✓ Makefile updated and builds successfully  
✓ Test scripts updated and all tests pass  
✓ Documentation updated  
✓ No remaining "pisstvpp2" references (except in comments about history)  
✓ Executable runs and functions correctly  

---

## Notes

- **Git History:** Using a feature branch preserves the ability to see what changed
- **Legacy Code:** `src/legacy/pisstvpp.c` can stay unchanged (it's legacy)
- **Comments about History:** It's okay to mention "formerly PiSSTVpp2" in documentation
- **Test Data:** Test outputs with old paths can be manually updated after validation
- **Backwards Compatibility:** Not required - this is an internal rename, not a library API

