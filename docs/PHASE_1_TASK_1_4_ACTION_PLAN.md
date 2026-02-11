# Phase 1 Task 1.4 - Action Plan: Update All Modules to Use Error Codes

**Purpose:** Complete the final 25-30% of Phase 1 by standardizing error handling across all modules

**Scope:** 8 modules, ~150 lines total changes, 18-24 hours effort

**Deadline:** Estimated 2-3 business days of focused work

---

## Critical Success Factors

1. ✅ Error code vocabulary already defined (see error.h)
2. ✅ `error_log()` function ready and working
3. ✅ Config and context modules complete
4. ✅ Test infrastructure (249 tests) ready to validate changes
5. ❌ Need to  apply consistently across remaining modules

---

## Module-by-Module Action Plan

### Module 1: pisstvpp2.c (Main Program) - CRITICAL PATH

**Priority:** 🔴 **HIGHEST** - This is the main application entry point

**Current Problem:**
```c
// Lines 149-161: Old VIPS_CALL macro pattern
#define VIPS_CALL(func, ...) do { \
    if (func(__VA_ARGS__)) { \
        fprintf(stderr, "[ERROR] %s failed\n", #func); \
        vips_error_clear(); \
        error_code = 2;              // ← Generic "runtime error"
        goto cleanup; \
    } \
} while(0)
```

**Issues:**
- Uses generic `error_code = 2` instead of specific codes
- Uses `fprintf()` instead of `error_log()`
- Uses `goto cleanup` pattern instead of direct returns
- Error context is lost

**Solution:**

**Step 1A:** Map VIPS operations to error codes

Create a helper function to replace VIPS_CALL:

```c
/**
 * @brief Check VIPS operation result and log error
 * @param result VIPS operation return value (0=success, non-0=error)
 * @param error_code Specific PISSTVPP2_ERR_* code to use on failure
 * @param operation Description of what operation was being done
 * @return The error_code if operation failed, PISSTVPP2_OK if succeeded
 */
static int check_vips_op(int result, int error_code, const char *operation) {
    if (result != 0) {
        error_log(error_code, operation, "libvips: %s", vips_error_buffer());
        vips_error_clear();
        return error_code;
    }
    return PISSTVPP2_OK;
}
```

**Step 1B:** Remove VIPS_CALL macro and replace all call sites

Find all VIPS_CALL usages (about 10-15) and replace:

**Before:**
```c
VIPS_CALL(vips_resize, image, &resized, scale, NULL);
// Continues to cleanup on error
```

**After:**
```c
int ret = check_vips_op(
    vips_resize(image, &resized, scale, NULL),
    PISSTVPP2_ERR_IMAGE_PROCESS,
    "Image resize operation"
);
if (ret != PISSTVPP2_OK) {
    // Handle error - cleanup if needed
    g_object_unref(image);
    return ret;
}
```

**Step 1C:** Refactor main() error handling

**Before:**
```c
int error_code = 0;
// ... code ...
if (result != 0) {
    error_code = result;
    goto cleanup;
}
// ... more code ...
cleanup:
    if (error_code != 0) {
        fprintf(stderr, "Error: %d\n", error_code);
    }
    return (error_code != 0) ? 1 : 0;
```

**After:**
```c
int result;

// ... code ...
if (result != PISSTVPP2_OK) {
    error_log(result, "Command-line option validation");
    // Cleanup specific resources
    pisstvpp_config_cleanup(&config);
    return result;
}

// ... more code ...

// Proper cleanup with error checking
pisstvpp_config_cleanup(&config);
pisstvpp_context_cleanup(&ctx);

if (result != PISSTVPP2_OK) {
    // Error already logged by error_log()
    return 1;  // Exit with error status
}
return 0;  // Success
```

**Files to Modify:** `src/pisstvpp2.c`

**Expected Changes:**
- Remove VIPS_CALL macro definition (12 lines)
- Add check_vips_op() helper function (15 lines)
- Replace 10-15 VIPS_CALL instances (~30 lines changed)
- Update error handling in main() (~20 lines)
- Net: ~50-60 lines modified/added

**Testing After:**
```bash
./bin/pisstvpp2 -i test_image.jpg              # Should work
./bin/pisstvpp2 -i /tmp/nonexistent.jpg        # Should show error code 501
./bin/pisstvpp2 -i test_image.jpg -p invalid_mode  # Should show error code 106
```

**Estimated Time:** 4-5 hours

---

### Module 2: pisstvpp2_image.c (Image Processing)

**Priority:** 🟠 **HIGH** - Core functionality, blocks Phase 2

**Current Problems:**
```c
// Line 302: return -1 instead of error
fprintf(stderr, "   [ERROR] vips_crop failed: %s\n", vips_error_buffer());
return -1;  // ← MUST CHANGE

// Line 314: Same pattern
fprintf(stderr, "   [ERROR] vips_resize failed: %s\n", vips_error_buffer());
return -1;  // ← MUST CHANGE

// Line 351, 381: repeat pattern
fprintf(stderr, "   [ERROR] vips_embed failed: %s\n", ...);
return -1;  // ← MUST CHANGE
```

**Solution - Search and Replace Pattern:**

**Pattern 1 - vips operation failures:**
```c
// FIND:
fprintf(stderr, "   [ERROR] vips_crop failed: %s\n", vips_error_buffer());
return -1;

// REPLACE WITH:
int err = PISSTVPP2_ERR_IMAGE_ASPECT_CORRECTION;  // or appropriate code
error_log(err, "Aspect ratio cropping", "libvips: %s", vips_error_buffer());
vips_error_clear();
return err;
```

**Pattern 2 - vips operation general failures:**
```c
// FIND:
fprintf(stderr, "   [ERROR] vips_resize failed: %s\n", vips_error_buffer());
return -1;

// REPLACE WITH:
error_log(PISSTVPP2_ERR_IMAGE_PROCESS, "Image resize", "libvips: %s", vips_error_buffer());
vips_error_clear();
return PISSTVPP2_ERR_IMAGE_PROCESS;
```

**Error Code Mapping for Image Module:**
- Image loading failures → `PISSTVPP2_ERR_IMAGE_LOAD` (201)
- Format unsupported → `PISSTVPP2_ERR_IMAGE_FORMAT_UNSUPPORTED` (202)
- Dimension issues → `PISSTVPP2_ERR_IMAGE_DIMENSIONS_INVALID` (203)
- General processing → `PISSTVPP2_ERR_IMAGE_PROCESS` (204)
- Aspect correction → `PISSTVPP2_ERR_IMAGE_ASPECT_CORRECTION` (205)
- Memory issues → `PISSTVPP2_ERR_IMAGE_MEMORY` (206)

**Occurrences to Replace:**
- `return -1;` (4 instances: lines 302, 314, 351, 381)
- `fprintf(stderr, "   [ERROR]...` (8-10 instances)

**Testing After:**
```bash
./bin/pisstvpp2 -i test_pattern_320x240.jpg    # Should work
./bin/pisstvpp2 -i /nonexistent.jpg            # error_code 201
./bin/pisstvpp2 -i /etc/passwd                 # error_code 202 (bad format)
```

**Files to Modify:** `src/pisstvpp2_image.c`

**Expected Changes:** ~20 lines modified

**Estimated Time:** 2-3 hours

---

### Module 3: pisstvpp2_sstv.c (SSTV Encoding)

**Priority:** 🟠 **HIGH** - Core encoding functionality

**Current Problems:**
```c
// Line 112: fprintf for warnings
fprintf(stderr, "[WARNING] Audio buffer overflow at sample %u\n", g_sstv.samples);

// Line 698: fprintf for errors
fprintf(stderr, "[ERROR] SSTV module not initialized\n");
```

**Solution:**

**For Warnings:**
```c
// FIND:
fprintf(stderr, "[WARNING] Audio buffer overflow at sample %u\n", g_sstv.samples);

// REPLACE WITH:
if (verbose) {
    fprintf(stderr, "[WARNING] Audio buffer overflow at sample %u\n", g_sstv.samples);
}
// Note: Warnings can use fprintf if debug/verbose, but errors must use error_log
```

**For Errors:**
```c
// FIND:
fprintf(stderr, "[ERROR] SSTV module not initialized\n");

// REPLACE WITH:
error_log(PISSTVPP2_ERR_SSTV_INIT, "SSTV encoding", "Module must be initialized before use");
```

**Error Code Mapping for SSTV Module:**
- Encoding failures → `PISSTVPP2_ERR_SSTV_ENCODE` (301)
- Mode not found → `PISSTVPP2_ERR_SSTV_MODE_NOT_FOUND` (302)
- Initialization → `PISSTVPP2_ERR_SSTV_INIT` (303)
- Registry errors → `PISSTVPP2_ERR_SSTV_REGISTRY` (304)
- VIS encoding → `PISSTVPP2_ERR_SSTV_VIS` (306)
- CW encoding → `PISSTVPP2_ERR_SSTV_CW` (307)

**Files to Modify:** `src/pisstvpp2_sstv.c`

**Expected Changes:** ~8-10 lines modified

**Estimated Time:** 2-3 hours

---

### Module 4: audio_encoder_wav.c (WAV Format Encoder)

**Priority:** 🟡 **MEDIUM** - Important for core functionality

**Current Problem:**
All error handling uses `return -1` pattern:

```c
// Example from lines 80-134:
if (fputs("RIFF", state->fp) == EOF) return -1;
if (fputc((file_size & 0x000000ff), state->fp) == EOF) return -1;
// ... many repeats of this pattern
```

**Solution - Unified Approach:**

Create a helper:
```c
/**
 * @brief Check if file write succeeded, log error if not
 */
static int check_file_write(int result, const char *operation) {
    if (result == EOF) {
        error_log(PISSTVPP2_ERR_FILE_WRITE, "WAV file encoding", 
                 "Failed to write %s: %s", operation, strerror(errno));
        return PISSTVPP2_ERR_FILE_WRITE;
    }
    return PISSTVPP2_OK;
}
```

**Then replace patterns:**
```c
// BEFORE:
if (fputs("RIFF", state->fp) == EOF) return -1;
if (fputc((file_size & 0x000000ff), state->fp) == EOF) return -1;

// AFTER:
int ret;
ret = check_file_write(fputs("RIFF", state->fp) == EOF ? EOF : 0, "RIFF header");
if (ret != PISSTVPP2_OK) return ret;

ret = check_file_write((fputc((file_size & 0x000000ff), state->fp) == EOF) ? EOF : 0, 
                       "file size byte 0");
if (ret != PISSTVPP2_OK) return ret;
// ... etc
```

**Statistics:**
- Total `return -1` instances: 25-30
- Total `fprintf` instances: 3-5

**Error Code Mapping for Audio/WAV:**
- General audio error → `PISSTVPP2_ERR_AUDIO_ENCODE` (401)
- Format unsupported → `PISSTVPP2_ERR_AUDIO_FORMAT_UNSUPPORTED` (402)
- Sample rate unsupported → `PISSTVPP2_ERR_AUDIO_SAMPLE_RATE_UNSUPPORTED` (403)
- Memory issues → `PISSTVPP2_ERR_AUDIO_MEMORY` (404)
- WAV-specific → `PISSTVPP2_ERR_AUDIO_WAV` (405)
- File write errors → `PISSTVPP2_ERR_FILE_WRITE` (503)

**Files to Modify:** `src/audio_encoder_wav.c`

**Expected Changes:** 40-50 lines modified

**Estimated Time:** 2-3 hours

---

### Module 5: audio_encoder_aiff.c (AIFF Format Encoder)

**Priority:** 🟡 **MEDIUM** - Same format as WAV but different structure

**Current Problem:**
Similar to WAV - all `return -1`

**Solution:**
Use same approach as WAV encoder - create helper and replace all `return -1`

**Statistics:**
- Total `return -1` instances: 40-50 (more complex AIFF format)
- Total error patterns: Very similar to WAV

**Helper Function:**
```c
// Same check_file_write() as WAV encoder
static int check_file_write(int result, const char *operation) {
    if (result == EOF) {
        error_log(PISSTVPP2_ERR_AUDIO_AIFF, "AIFF file encoding", 
                 "Failed to write %s: %s", operation, strerror(errno));
        return PISSTVPP2_ERR_AUDIO_AIFF;
    }
    return PISSTVPP2_OK;
}
```

**Used Error Codes:**
- `PISSTVPP2_ERR_AUDIO_AIFF` (406) for AIFF-specific failures
- `PISSTVPP2_ERR_FILE_WRITE` (503) for general file I/O

**Files to Modify:** `src/audio_encoder_aiff.c`

**Expected Changes:** 50-60 lines modified

**Estimated Time:** 2-3 hours

---

### Module 6: audio_encoder_ogg.c (OGG Vorbis Encoder)

**Priority:** 🟡 **MEDIUM** - Optional but should be consistent

**Current Problem:**
```c
// Lines 49, 54, 59: Early returns without context
if (encoder == NULL) return -1;
if (encoder->state.vorbis_state == NULL) return -1;
if (encoder->output_file == NULL) return -1;

// Line 367: Fallback with fprintf
fprintf(stderr, "[ERROR] OGG Vorbis support not compiled in...");
```

**Solution:**

**For init failures:**
```c
// BEFORE:
if (encoder == NULL) return -1;

// AFTER:
if (encoder == NULL) {
    error_log(PISSTVPP2_ERR_AUDIO_FORMAT_UNSUPPORTED, "OGG Vorbis encoding",
             "Encoder initialization failed");
    return PISSTVPP2_ERR_AUDIO_FORMAT_UNSUPPORTED;
}
```

**For compilation fallback:**
```c
// BEFORE:
fprintf(stderr, "[ERROR] OGG Vorbis support not compiled in. Install libvorbis-dev and rebuild.\n");

// AFTER:
error_log(PISSTVPP2_ERR_AUDIO_OGG, "OGG Vorbis encoding",
         "OGG Vorbis not compiled in. Install libvorbis-dev and rebuild.");
```

**Error Codes:**
- `PISSTVPP2_ERR_AUDIO_FORMAT_UNSUPPORTED` (402) - if library not available
- `PISSTVPP2_ERR_AUDIO_OGG` (407) - OGG-specific failures

**Files to Modify:** `src/audio_encoder_ogg.c`

**Expected Changes:** ~10 lines modified

**Estimated Time:** 1-2 hours

---

### Module 7: pisstvpp2_audio_encoder.c (Base Encoder)

**Priority:** 🟡 **MEDIUM** - Factory and base functionality

**Current Status:**
- Minimal error handling
- Mostly factory functions

**What to Review:**
- Audio encoder creation path
- Format validation
- Error propagation from factory functions

**Solution:**
Review all error returns and ensure they use PISSTVPP2_ERR_AUDIO_* codes

**Files to Modify:** `src/pisstvpp2_audio_encoder.c`

**Expected Changes:** ~5-10 lines

**Estimated Time:** 1-2 hours

---

### Module 8: pisstvpp2_mmsstv_adapter.c (MMSSTV Library Integration)

**Priority:** 🟡 **MEDIUM** - Phase 4 blocker, but needed for dynamic mode system

**Current Problem:**
All library detection and loading uses `return -1`:

```c
// Many instances throughout:
if (!mmsstv_lib_path) return -1;
if (dlopen(...) == NULL) return -1;
if (dlsym(...) == NULL) return -1;
// ... etc (20+ total)
```

**Solution:**

Create helper for library loading:
```c
/**
 * @brief Check if dynamic library symbol loaded successfully
 * @return PISSTVPP2_OK if found, PISSTVPP2_ERR_MMSSTV_INIT if failed
 */
static int check_mmsstv_symbol(void *symbol, const char *symbol_name) {
    if (symbol == NULL) {
        error_log(PISSTVPP2_ERR_MMSSTV_INIT, "MMSSTV library loading",
                 "Failed to load symbol: %s", symbol_name);
        return PISSTVPP2_ERR_MMSSTV_INIT;
    }
    return PISSTVPP2_OK;
}
```

**Error Code Mapping for MMSSTV:**
- Library not found → `PISSTVPP2_ERR_MMSSTV_NOT_FOUND` (701)
- Initialization failed → `PISSTVPP2_ERR_MMSSTV_INIT` (702)
- Mode allocation failed → Use `PISSTVPP2_ERR_MEMORY_ALLOC` (601)

**Search/Replace Pattern:**
```c
// BEFORE:
if (dlsym(...) == NULL) return -1;

// AFTER:
symbol = dlsym(...);
int ret = check_mmsstv_symbol(symbol, "function_name");
if (ret != PISSTVPP2_OK) {
    return ret;
}
```

**Files to Modify:** `src/pisstvpp2_mmsstv_adapter.c`

**Expected Changes:** 40-50 lines modified

**Estimated Time:** 2-3 hours

---

## Execution Plan (Recommended Order)

### Phase 1A - Critical Path (5-6 hours)
1. **pisstvpp2.c** - Remove VIPS_CALL, add proper error handling (4-5 hrs)
2. **pisstvpp2_image.c** - Replace return -1, add error_log (2-3 hrs)

✅ **Stop here and test** - Verify audio output quality unchanged

### Phase 1B - Core Encoders (6-8 hours)
3. **audio_encoder_wav.c** - Helper + replace all (2-3 hrs)
4. **audio_encoder_aiff.c** - Helper + replace all (2-3 hrs)
5. **audio_encoder_ogg.c** - Quick fixes (1-2 hrs)

✅ **Test** - Verify WAV/AIFF/OGG encoding works

### Phase 1C - Remaining Modules (4-6 hours)
6. **pisstvpp2_sstv.c** - Update error handling (2-3 hrs)
7. **audio_encoder_base** - Review factory functions (1-2 hrs)
8. **pisstvpp2_mmsstv_adapter.c** - Full update (2-3 hrs)

✅ **Final Test** - Run all 249 tests

---

## Testing Checklist

After making changes to each module:

```bash
# Rebuild
make clean && make all

# Run full test suite
python3 tests/test_suite.py

# Check specific scenarios
./bin/pisstvpp2 -i tests/images/test_pattern_320x240.jpg              # Success case
./bin/pisstvpp2 -i /nonexistent_file.jpg                             # File not found
./bin/pisstvpp2 -i tests/images/test_pattern_320x240.jpg -f invalid   # Format error
./bin/pisstvpp2 -i tests/images/test_pattern_320x240.jpg -p invalid   # Mode error

# Validation
echo "Check that all error messages are clear and helpful"
```

---

## Code Review Checklist

Before committing each module update:

- [ ] All `return -1` replaced with specific error codes
- [ ] All `fprintf(stderr, ...)` for errors replaced with `error_log()`
- [ ] Error codes match the PISSTVPP2_ERR_* definitions
- [ ] No unhandled error codes
- [ ] `vips_error_clear()` called after VIPS operations
- [ ] `errno` preserved for file I/O errors
- [ ] No goto statements (except where required by existing pattern)
- [ ] All error paths cleaned up properly
- [ ] No compiler warnings
- [ ] All 249 tests still pass

---

## Expected Outcomes When Complete

✅ **Code Quality:**
- Consistent error handling across all modules
- Clear error messages for all failure modes
- Proper resource cleanup on error paths
- No scattered return codes

✅ **Testing:**
- All 249 tests passing
- No compiler warnings
- Binary size unchanged (~138 KB)
- Audio output quality unchanged

✅ **Documentation:**
- Error code usage clear throughout codebase
- Contribution guide updated for error handling
- Master plan Phase 1 marked as COMPLETE

✅ **Git History:**
- Clear commits showing each module update
- Each commit logical and reviewable
- Master plan references in commit messages

---

**Last Updated:** February 11, 2026  
**Status:** Action Plan Ready  
**Next Step:** Begin with Module 1 (pisstvpp2.c)  
**Estimated Completion:** 2-3 business days
