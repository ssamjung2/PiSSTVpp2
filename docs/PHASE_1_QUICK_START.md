# Phase 1: Unified Error Handling - Quick Start

**Estimated Duration:** 10-18 hours  
**Target Completion:** 1-2 days (concentrated work)  
**Success Gate:** All 55 tests passing, consistent error handling throughout

---

## Overview

Phase 1 modernizes how the codebase handles errors. Instead of scattered error codes (-1, 0, 1, 2) and direct `fprintf(stderr, ...)` calls, we'll implement:

1. **Unified error code system** - Named constants for all errors
2. **Config management module** - Centralized configuration handling
3. **Context management** - Proper state encapsulation
4. **Consistent error propagation** - All modules use the new system

**Result:** Cleaner code, better error messages, easier debugging.

---

## Tasks in Phase 1

### Task 1.1: Implement Error Code System (4-6 hours) ⭐ START HERE

**Files to Create:**
- `src/util/error.h` (header file)
- `src/util/error.c` (implementation)

**What to Include:**

```c
// error.h - Define error codes and functions

#ifndef PISSTVPP2_ERROR_H
#define PISSTVPP2_ERROR_H

#include <stdio.h>

// Error code constants
#define PISSTVPP2_OK                0

// Argument/config errors
#define PISSTVPP2_ERR_ARGS          101
#define PISSTVPP2_ERR_IMAGE_PATH    102
#define PISSTVPP2_ERR_OUTPUT_PATH   103
#define PISSTVPP2_ERR_MODE          104
#define PISSTVPP2_ERR_SR            105
#define PISSTVPP2_ERR_TC            106

// Image processing errors
#define PISSTVPP2_ERR_IMAGE_LOAD    111
#define PISSTVPP2_ERR_IMAGE_CONVERT 112
#define PISSTVPP2_ERR_IMAGE_RESIZE  113

// SSTV encoding errors
#define PISSTVPP2_ERR_SSTV_INIT     121
#define PISSTVPP2_ERR_SSTV_ENCODE   122

// Audio format errors
#define PISSTVPP2_ERR_AUDIO_INIT    131
#define PISSTVPP2_ERR_AUDIO_WRITE   132

// File I/O errors
#define PISSTVPP2_ERR_FILE_CREATE   141
#define PISSTVPP2_ERR_FILE_WRITE    142
#define PISSTVPP2_ERR_FILE_READ     143

// Memory errors
#define PISSTVPP2_ERR_MEMORY        151

// Generic/unspecified errors
#define PISSTVPP2_ERR_UNKNOWN       199

// Function declarations
const char* error_string(int error_code);
void error_log(int error_code, const char *context, const char *details);

#endif // PISSTVPP2_ERROR_H
```

**Checklist:**
- [ ] Create src/util/ directory if missing
- [ ] Create error.h with all error codes defined
- [ ] Implement error_string() function with message for each code
- [ ] Implement error_log() function that prints error with context
- [ ] Test: Each error code maps to correct message
- [ ] Test: error_log() output is properly formatted
- [ ] Test: All 55 existing tests still pass

**Testing:**
```bash
# Compile with the new files
make clean && make all

# Run full test suite
./tests/test_suite.py

# Expected: 55/55 passing (same as before)
```

---

### Task 1.2: Create Config Management Module (3-4 hours)

**Files to Create:**
- `src/core/pisstvpp2_config.h` (header)
- `src/core/pisstvpp2_config.c` (implementation)

**What to Include:**

```c
// Refactor existing config_t structure
// Implement config initialization, validation, access functions

// New functions:
config_t* config_init(void);           // Create with defaults
void config_free(config_t *cfg);       // Cleanup
int config_validate(config_t *cfg);    // Validation (returns error code)
void config_print(config_t *cfg);      // Debug output

// Getter functions:
const char* config_get_input(config_t *cfg);
const char* config_get_output(config_t *cfg);
const char* config_get_mode(config_t *cfg);
// ... etc for all fields
```

**Checklist:**
- [ ] Move `config_t` structure to config.h
- [ ] Create config_init() with sensible defaults
- [ ] Create config_validate() that checks combinations
- [ ] Create config_print() for debugging
- [ ] Create getter functions for each field
- [ ] Test: All option combinations parse correctly
- [ ] Test: Invalid combinations properly rejected
- [ ] Test: All 55 tests still pass

---

### Task 1.3: Create Context/State Management (3-4 hours)

**Files to Create:**
- `src/core/pisstvpp2_context.h` (header)
- `src/core/pisstvpp2_context.c` (implementation)

**What to Include:**

```c
// Define context structure containing:
typedef struct {
    config_t *config;
    image_buffer_t *image;
    audio_samples_t *audio;
    // ... other state
} pisstvpp2_context_t;

// New functions:
pisstvpp2_context_t* context_create(config_t *cfg);
void context_free(pisstvpp2_context_t *ctx);
void context_reset(pisstvpp2_context_t *ctx);
```

**Checklist:**
- [ ] Define pisstvpp2_context_t structure (with comment explaining ownership)
- [ ] Create context_create() function
- [ ] Create context_free() function
- [ ] Create context_reset() function
- [ ] Document pointer ownership in comments
- [ ] Test: No memory leaks on create/destroy
- [ ] Test: Works through encode cycle
- [ ] Test: All 55 tests still pass

---

### Task 1.4: Update All Modules to Use Error Codes (6-8 hours)

**Files to Modify:**
- `src/pisstvpp2.c` (main)
- `src/pisstvpp2_image.c` (image processing)
- `src/pisstvpp2_sstv.c` (SSTV encoding)
- `src/pisstvpp2_audio_encoder.c` (audio base)
- `src/audio_encoder_wav.c` (WAV specific)
- `src/audio_encoder_aiff.c` (AIFF specific)
- `src/audio_encoder_ogg.c` (OGG specific)

**Pattern Change:**

**Before (scattered errors):**
```c
if (vips_image_write_to_file(...) != 0) {
    fprintf(stderr, "Error writing file\n");
    return -1;
}
```

**After (unified error handling):**
```c
if (vips_image_write_to_file(...) != 0) {
    error_log(PISSTVPP2_ERR_FILE_WRITE, "encode", "Failed to write output file");
    return PISSTVPP2_ERR_FILE_WRITE;
}
```

**Checklist per file:**
- [ ] Replace `fprintf(stderr, ...)` with `error_log()`
- [ ] Replace error returns (-1, 1, 2) with `PISSTVPP2_*` codes
- [ ] Update error checking pattern from `if (result != 0)` to `if (result != PISSTVPP2_OK)`
- [ ] Add error context to error_log() calls
- [ ] Propagate error codes up call stack
- [ ] Test: All 55 tests pass
- [ ] Test: Each error condition still detected
- [ ] Test: Error messages helpful and consistent

---

## Workflow for Phase 1

### Step 1: Setup
```bash
# Start from clean state
cd /Users/ssamjung/Desktop/WIP/PiSSTVpp2

# Verify current state
./tests/test_suite.py
# Expected: 55/55 passing

# Create feature branch
git checkout -b feature/phase-1-error-codes
```

### Step 2: Task 1.1 (Error Code System)
```bash
# Create util directory
mkdir -p src/util

# Create error.h and error.c
# (See template above)

# Compile and test
make clean && make all
./tests/test_suite.py

# If all pass:
git add src/util/error.{h,c}
git commit -m "feat(error): Implement unified error code system

- Define PISSTVPP2_OK and ERR_* constants (15+ error codes)
- Implement error_string() for human-readable messages
- Implement error_log() for consistent logging with context
- All 55 existing tests still passing

Task 1.1 completion"
```

### Step 3: Task 1.2 (Config Module)
```bash
# Create core directory if missing
mkdir -p src/core

# Create config.h and config.c
# Refactor existing config_t structure

# Test
make clean && make all
./tests/test_suite.py

# Commit when tests pass
git commit -m "feat(config): Extract config management module

- Create src/core/pisstvpp2_config.h/c
- Move config_t structure definition
- Implement config_init(), config_free(), config_validate()
- Create getter functions for each field
- All 55 tests still passing

Task 1.2 completion"
```

### Step 4: Task 1.3 (Context Management)
```bash
# Create context.h and context.c
# Define pisstvpp2_context_t with proper ownership documentation

# Test
make clean && make all
./tests/test_suite.py

# Commit
git commit -m "feat(context): Implement context/state management

- Create src/core/pisstvpp2_context.h/c
- Define pisstvpp2_context_t with config, image, audio, state
- Implement context_create(), context_free(), context_reset()
- Document pointer ownership clearly
- All 55 tests still passing

Task 1.3 completion"
```

### Step 5: Task 1.4 (Update All Modules)
```bash
# Update each file in order:
# 1. src/pisstvpp2.c
# 2. src/pisstvpp2_image.c
# 3. src/pisstvpp2_sstv.c
# 4. src/pisstvpp2_audio_encoder.c
# 5. src/audio_encoder_wav.c
# 6. src/audio_encoder_aiff.c
# 7. src/audio_encoder_ogg.c

# After each file:
make clean && make all
./tests/test_suite.py
# Verify: 55/55 still passing

# Commit after each file with context
git commit -m "refactor(error): Update pisstvpp2.c to use unified error codes

- Replace fprintf(stderr) with error_log()
- Replace scattered error returns with PISSTVPP2_* codes
- Propagate error codes through call stack
- All 55 tests passing

Context: Task 1.4 (part 1)"

# After all files done:
git commit --amend -m "refactor(error): Update all modules to unified error codes

- Update src/pisstvpp2.c (main program)
- Update src/pisstvpp2_image.c (image processing)
- Update src/pisstvpp2_sstv.c (SSTV encoding)
- Update src/pisstvpp2_audio_encoder.c (audio base)
- Update all audio_encoder_*.c (WAV, AIFF, OGG)
- All error returns use PISSTVPP2_* codes
- All error messages via error_log()
- All 55 tests still passing

Task 1.4 completion"
```

### Step 6: Final Validation
```bash
# Run full test suite one more time
make clean && make all
./tests/test_suite.py

# Expected: 55/55 passing ✓

# Check code compiles with no warnings
make clean && make all

# Verify git history is clean
git log --oneline origin..HEAD

# Results: 4-5 clear commits for review
```

---

## Expected Challenges & Solutions

| Challenge | Solution |
|-----------|----------|
| "How do I know which error codes to use?" | Scan existing code for error messages, map to categories (111-151 range) |
| "Do I need to change function signatures?" | Mostly no - return error codes instead of -1/0/1/2 |
| "Will tests break?" | They might during refactoring - fix issues and keep testing |
| "How much refactoring is needed?" | Replace ~30-40 fprintf/return statements, straightforward |

---

## Success Criteria for Phase 1

**All of the following must be true:**

- [x] `make clean && make all` succeeds with no errors
- [x] No compiler warnings
- [x] `./tests/test_suite.py` shows 55/55 passing
- [x] Error messages are human-readable
- [x] Error handling is consistent throughout
- [x] Code follows existing style
- [x] Git history shows 4-5 focused commits
- [x] No regressions in functionality
- [x] Ready to proceed to Phase 2

---

## Files Reference

### Key Files in This Phase
- New: `src/util/error.h` and `src/util/error.c`
- New: `src/core/pisstvpp2_config.h` and `src/core/pisstvpp2_config.c`
- New: `src/core/pisstvpp2_context.h` and `src/core/pisstvpp2_context.c`
- Modified: All files listed in Task 1.4

### Files NOT Modified in Phase 1
- Tests (they remain the same)
- Image processing logic (same functionality, better error handling)
- Audio encoding logic (same output, better error handling)

---

## Estimated Time Breakdown

| Task | Duration | Notes |
|------|----------|-------|
| Task 1.1: Error codes | 4-6 hrs | Most straightforward |
| Task 1.2: Config module | 3-4 hrs | Organize existing code |
| Task 1.3: Context mgmt | 3-4 hrs | Define structures |
| Task 1.4: Update modules | 6-8 hrs | Largest refactoring |
| Testing & fixes | 2-3 hrs | Handle any issues |
| **Phase 1 Total** | **10-18 hrs** | **1-2 days intensive** |

---

## Next Phase (Phase 2) Preview

Once Phase 1 is complete and all tests passing, Phase 2 begins:

**Phase 2: Image Module Refactoring (24-26 hours)**
- Extract image loader
- Extract image processor  
- Extract aspect ratio correction
- **Implement text overlay & color bars (FCC compliance)**

---

## Quick Reference Commands

```bash
# Essential commands
make clean && make all      # Build executable
./tests/test_suite.py       # Run all tests
git add .                   # Stage changes
git commit -m "..."         # Commit changes
git log --oneline           # View history

# During development
make clean                  # Remove build artifacts
gcc -c src/file.c -Iinclude # Test individual file compilation
```

---

## Questions to Answer Before Starting

- [ ] Do you understand why error codes matter?
- [ ] Can you see the pattern of how errors are currently handled?
- [ ] Do you know what the 55 tests are checking?
- [ ] Can you read the master plan's Task 1.1 section?
- [ ] Is your development environment set up (compiler, make, Python)?

**If you answered yes to all, you're ready to start!**

---

**Last Updated:** February 10, 2026  
**Status:** ✅ READY TO BEGIN  
**Estimated Start:** Now  
**Estimated Completion:** 1-2 days
