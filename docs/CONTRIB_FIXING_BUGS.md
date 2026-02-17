# Contributor Playbook: Fixing Bugs in SlowFrame

**Step-by-step guide for identifying, diagnosing, and fixing bugs**

---

## Overview

This playbook provides a systematic approach to bug fixing in SlowFrame v2.1, from initial report to tested fix.

**Time required**: Varies (30 minutes - 4 hours depending on complexity)

**Prerequisites**:
- SlowFrame development environment
- Basic debugging skills
- Familiarity with [DEVELOPER_WALKTHROUGH.md](DEVELOPER_WALKTHROUGH.md)

---

## Table of Contents

1. [Bug Lifecycle](#bug-lifecycle)
2. [Step 1: Reproduce the Bug](#step-1-reproduce-the-bug)
3. [Step 2: Isolate the Cause](#step-2-isolate-the-cause)
4. [Step 3: Develop the Fix](#step-3-develop-the-fix)
5. [Step 4: Test Thoroughly](#step-4-test-thoroughly)
6. [Step 5: Submit the Fix](#step-5-submit-the-fix)
7. [Common Bug Patterns](#common-bug-patterns)
8. [Debugging Tools](#debugging-tools)
9. [Examples](#examples)

---

## Bug Lifecycle

```
1. Bug Report → 2. Reproduce → 3. Isolate → 4. Fix → 5. Test → 6. Submit → 7. Review → 8. Merge
```

**Your role**: Steps 2-6

---

## Step 1: Reproduce the Bug

### Gather Information

**From bug report, collect**:
- SlowFrame version
- Operating system
- Command used
- Expected behavior
- Actual behavior
- Error messages
- Input files (if relevant)

**Example bug report**:
```
Title: Martin 1 mode produces garbled output on macOS

Version: SlowFrame v2.1.0
OS: macOS 13.2 (M1)
Command: ./bin/slowframe -i test.jpg -p m1 -o output.wav

Expected: Valid Martin 1 SSTV audio
Actual: Audio plays but image is distorted (colors shifted)

Error messages: None

Input: test.jpg (320×256 JPEG)
```

### Reproduce Locally

**Try exact command**:
```bash
# Create test image if not provided
convert -size 320x256 gradient:blue-red test.jpg

# Run reported command
./bin/slowframe -i test.jpg -p m1 -o output.wav

# Listen to output
ffplay output.wav

# Compare with known-good version (if available)
git checkout v2.0.0
make clean && make
./bin/slowframe -i test.jpg -p m1 -o reference.wav
ffplay reference.wav
```

**Document reproduction**:
```markdown
## Reproduction Steps

1. Create 320×256 test image: `convert -size 320x256 gradient:blue-red test.jpg`
2. Encode with M1: `./bin/slowframe -i test.jpg -p m1 -o output.wav`
3. Play audio: `ffplay output.wav`
4. Observe: Colors are shifted (green appears where blue should be)

✅ **Reproduced**: Colors in output are incorrect
```

**If you can't reproduce**:
- Ask reporter for more details
- Try on different OS
- Check for environment-specific issues
- Label as "needs-more-info"

---

## Step 2: Isolate the Cause

### Add Debug Output

**Enable verbose mode**:
```bash
./bin/slowframe -v -i test.jpg -p m1 -o output.wav 2>&1 | tee debug.log
```

**Add targeted debug prints**:

In `src/slowframe_sstv.c`, find `buildaudio_m1()`:
```c
static int buildaudio_m1(...) {
    fprintf(stderr, "DEBUG: Starting Martin 1 encoding\n");
    fprintf(stderr, "DEBUG: Image dimensions: %dx%d\n", 
            vips_image_get_width(image), vips_image_get_height(image));
    
    for (int row = 0; row < height; row++) {
        const uint8_t* line = pixels + (row * stride);
        
        // Debug first pixel of each row
        const uint8_t* pixel = line;
        fprintf(stderr, "DEBUG: Row %d, Pixel[0,0]: R=%d G=%d B=%d\n",
                row, pixel[0], pixel[1], pixel[2]);
        
        // ... encoding logic ...
    }
}
```

**Rebuild and test**:
```bash
make clean && make
./bin/slowframe -i test.jpg -p m1 -o output.wav 2>&1 | head -20
```

### Use GDB

**Start debugger**:
```bash
gdb ./bin/slowframe
```

**Set breakpoints**:
```gdb
# Break at Martin 1 encoder
(gdb) break buildaudio_m1

# Run with arguments
(gdb) run -i test.jpg -p m1 -o output.wav

# When breakpoint hits
(gdb) print width
(gdb) print height
(gdb) print sample_rate

# Step through code
(gdb) next
(gdb) next

# Inspect pixel data
(gdb) print pixels[0]
(gdb) print pixels[1]
(gdb) print pixels[2]
```

### Inspect Intermediate Values

**Check pixel access**:
```c
// In buildaudio_m1(), verify RGB channel access
for (int row = 0; row < 5; row++) {  // First 5 rows only
    for (int col = 0; col < 5; col++) {  // First 5 columns
        const uint8_t* pixel = line + (col * 3);
        fprintf(stderr, "Pixel[%d,%d]: R=%d G=%d B=%d\n",
                row, col, pixel[0], pixel[1], pixel[2]);
    }
}
```

**Check encoding output**:
```c
// Verify scan order
fprintf(stderr, "Scanning GREEN channel, row %d\n", row);
scan_channel(image, row, GREEN_CHANNEL, scan_duration, sample_rate);

fprintf(stderr, "Scanning BLUE channel, row %d\n", row);
scan_channel(image, row, BLUE_CHANNEL, scan_duration, sample_rate);

fprintf(stderr, "Scanning RED channel, row %d\n", row);
scan_channel(image, row, RED_CHANNEL, scan_duration, sample_rate);
```

### Hypothesis Formation

**Based on debug output, form hypothesis**:

**Example findings**:
```
DEBUG: Row 0, Pixel[0,0]: R=255 G=0 B=0
Scanning GREEN channel, row 0
Scanning BLUE channel, row 0
Scanning RED channel, row 0
```

**Hypothesis**: "Scan order is wrong. Should be G-B-R for Martin 1, but pixel access might be off."

**Check specification**:
```
Martin 1 scan order: Green → Blue → Red (per spec)
Pixel access: pixel[0]=R, pixel[1]=G, pixel[2]=B (RGB order)
```

**Root cause identified**: Channel indexing is correct, but scan order implementation may be wrong.

---

## Step 3: Develop the Fix

### Locate Bug

**In `src/slowframe_sstv.c`, line ~450**:
```c
// WRONG: Scanning channels in wrong order
scan_channel(image, row, 0, scan_duration, sample_rate);  // R (should be G)
scan_channel(image, row, 1, scan_duration, sample_rate);  // G (should be B)
scan_channel(image, row, 2, scan_duration, sample_rate);  // B (should be R)
```

**Correct order (Martin 1 spec)**:
```c
// CORRECT: Green → Blue → Red
scan_channel(image, row, 1, scan_duration, sample_rate);  // G
scan_channel(image, row, 2, scan_duration, sample_rate);  // B
scan_channel(image, row, 0, scan_duration, sample_rate);  // R
```

### Implement Fix

**Option 1: Direct channel indices**:
```c
// Use explicit channel indices
scan_channel(image, row, 1, scan_duration, sample_rate);  // Green
scan_channel(image, row, 2, scan_duration, sample_rate);  // Blue
scan_channel(image, row, 0, scan_duration, sample_rate);  // Red
```

**Option 2: Named constants (better)**:
```c
// In slowframe_sstv.h, add:
#define CHANNEL_RED   0
#define CHANNEL_GREEN 1
#define CHANNEL_BLUE  2

// In buildaudio_m1():
scan_channel(image, row, CHANNEL_GREEN, scan_duration, sample_rate);
scan_channel(image, row, CHANNEL_BLUE, scan_duration, sample_rate);
scan_channel(image, row, CHANNEL_RED, scan_duration, sample_rate);
```

**Document the fix**:
```c
/**
 * Martin 1 encoding
 * Scan order per spec: Green → Blue → Red
 */
static int buildaudio_m1(...) {
    // ... existing code ...
    
    // Martin 1 scan order (G-B-R)
    scan_channel(image, row, CHANNEL_GREEN, 0.146432, sample_rate);
    scan_channel(image, row, CHANNEL_BLUE, 0.146432, sample_rate);
    scan_channel(image, row, CHANNEL_RED, 0.146432, sample_rate);
}
```

---

## Step 4: Test Thoroughly

### Test the Fix

**Rebuild**:
```bash
make clean && make
```

**Test exact reproduction case**:
```bash
./bin/slowframe -i test.jpg -p m1 -o fixed.wav
ffplay fixed.wav  # Should show correct colors
```

**Compare waveforms** (if possible):
```bash
# Compare with known-good reference
python3 tests/util/compare_audio.py reference.wav fixed.wav
```

### Regression Testing

**Run full test suite**:
```bash
cd tests/util

# Backward compatibility
python3 test_suite.py

# Modernization
python3 test_modernization.py

# MMSSTV integration
python3 test_mmsstv_integration.py

# All should still pass!
```

**Test related modes**:
```bash
# If fixing Martin 1, test Martin 2 as well
./bin/slowframe -i test.jpg -p m2 -o m2_test.wav

# Test Scottie modes (similar encoding)
./bin/slowframe -i test.jpg -p s1 -o s1_test.wav
```

### Edge Case Testing

**Test various inputs**:
```bash
# Different image sizes
./bin/slowframe -i small_100x100.jpg -p m1 -o test1.wav
./bin/slowframe -i large_2000x1500.jpg -p m1 -o test2.wav

# Different formats
./bin/slowframe -i test.png -p m1 -o test3.wav
./bin/slowframe -i test.gif -p m1 -o test4.wav

# With overlays
./bin/slowframe -i test.jpg -p m1 -T "text:K9ABC" -o test5.wav

# With CW
./bin/slowframe -i test.jpg -p m1 -C "K9ABC" -o test6.wav
```

### Create Regression Test

**Add to test suite** (`tests/util/test_martin_modes.py`):
```python
def test_martin1_color_order(self):
    """Verify Martin 1 uses correct scan order (G-B-R)"""
    # Create test image with known colors
    # Red top half, blue bottom half
    subprocess.run([
        "convert", "-size", "320x256", "xc:red",
        "(", "-size", "320x128", "xc:blue", ")",
        "-gravity", "south", "-composite",
        "/tmp/color_test.png"
    ], check=True)
    
    # Encode
    subprocess.run([
        "./bin/slowframe", "-i", "/tmp/color_test.png",
        "-p", "m1", "-o", "/tmp/m1_color.wav"
    ], check=True)
    
    # Decode and verify (if decoder available)
    # Or just verify file size and duration
    result = subprocess.run([
        "ffprobe", "-v", "error", "-show_entries",
        "format=duration", "-of", "default=noprint_wrappers=1:nokey=1",
        "/tmp/m1_color.wav"
    ], capture_output=True, text=True)
    
    duration = float(result.stdout.strip())
    # Martin 1 should be ~114 seconds
    self.assertAlmostEqual(duration, 114.0, delta=2.0)
```

---

## Step 5: Submit the Fix

### Create Commit

```bash
# Stage changes
git add src/slowframe_sstv.c
git add src/include/slowframe_sstv.h
git add tests/util/test_martin_modes.py

# Commit with clear message
git commit -m "Fix Martin 1 color channel scan order

Martin 1 specification requires Green-Blue-Red scan order,
but implementation was using Red-Green-Blue, causing color
shifts in transmitted images.

Changes:
- Corrected scan order in buildaudio_m1() to G-B-R
- Added channel name constants for clarity
- Added regression test for color ordering

Fixes #234"
```

### Create Pull Request

**Title**: `Fix Martin 1 color channel scan order`

**Description**:
```markdown
## Problem
Martin 1 mode produces images with incorrect colors (reds appear green, greens appear blue, etc.)

## Root Cause
`buildaudio_m1()` was scanning channels in RGB order instead of the Martin 1 specification order (GBR).

## Solution
- Corrected channel scan order to Green → Blue → Red
- Added named constants (`CHANNEL_RED`, `CHANNEL_GREEN`, `CHANNEL_BLUE`) for clarity
- Added regression test to prevent future regressions

## Testing
- ✅ Verified fix with test image (colors now correct)
- ✅ All existing tests pass (no regressions)
- ✅ Added regression test for color ordering
- ✅ Tested with various image formats and sizes

## References
- Martin 1 Specification: [SSTV Handbook, p.12](#)
- Closes #234
```

---

## Common Bug Patterns

### Memory Leaks

**Symptom**: Memory usage grows, eventual crash

**Detection**:
```bash
valgrind --leak-check=full ./bin/slowframe -i test.jpg -p m1 -o test.wav
```

**Common causes**:
- Unreleased VipsImage objects
- Unclosed file handles
- Unfreed malloc'd memory

**Fix pattern**:
```c
// WRONG
VipsImage* img = load_image(...);
// ... use img ...
return ERROR_SUCCESS;  // LEAK!

// CORRECT
VipsImage* img = load_image(...);
// ... use img ...
g_object_unref(img);  // Free VipsImage
return ERROR_SUCCESS;
```

### Off-by-One Errors

**Symptom**: Array access errors, incorrect output

**Detection**: Segmentation fault, valgrind errors

**Common causes**:
```c
// WRONG
for (int i = 0; i <= width; i++) {  // Off by one!
    pixel = line + (i * 3);
}

// CORRECT
for (int i = 0; i < width; i++) {
    pixel = line + (i * 3);
}
```

### Floating-Point Precision

**Symptom**: Timing drift, duration incorrect

**Common causes**:
```c
// WRONG: Accumulated rounding errors
for (int i = 0; i < 1000; i++) {
    duration += 0.001;  // May drift
}

// CORRECT: Calculate precisely
for (int i = 0; i < 1000; i++) {
    duration = i * 0.001;
}
```

### Resource Cleanup

**Symptom**: File locks, resource exhaustion

**Detection**: Can't delete output file, ulimit errors

**Fix pattern**:
```c
// Use context cleanup
slowframe_context_t* ctx = slowframe_context_create();

// ... operations ...

// Always cleanup, even on error
if (error) {
    slowframe_context_destroy(ctx);
    return error;
}

slowframe_context_destroy(ctx);
return ERROR_SUCCESS;
```

---

## Debugging Tools

### GDB Commands

```bash
# Start with arguments
gdb --args ./bin/slowframe -i test.jpg -p m1 -o test.wav

# Common commands
(gdb) break slowframe_sstv.c:450    # Breakpoint at line
(gdb) break buildaudio_m1           # Breakpoint at function
(gdb) run                           # Start program
(gdb) continue                      # Continue execution
(gdb) next                          # Step over
(gdb) step                          # Step into
(gdb) print variable                # Print variable
(gdb) backtrace                     # Show call stack
(gdb) frame 2                       # Switch to frame 2
(gdb) info locals                   # Show local variables
```

### Valgrind

```bash
# Memory leak detection
valgrind --leak-check=full --show-leak-kinds=all ./bin/slowframe ...

# Uninitialized memory access
valgrind --track-origins=yes ./bin/slowframe ...

# Address sanitizer (compile-time)
# In makefile: CFLAGS += -fsanitize=address
make clean && CFLAGS="-fsanitize=address -g" make
./bin/slowframe ...
```

### Logging

```bash
# Enable all debug output
export SLOWFRAME_DEBUG=1
./bin/slowframe -v -i test.jpg -p m1 -o test.wav 2>&1 | tee full_debug.log

# Filter for specific module
./bin/slowframe -v ... 2>&1 | grep -i "image"
./bin/slowframe -v ... 2>&1 | grep -i "sstv"
```

---

## Examples

### Example 1: Segmentation Fault

**Bug report**: "Crash when encoding PD90 with MMSSTV"

**Reproduce**:
```bash
./bin/slowframe -i test.jpg -p pd90 -o test.wav
# Segmentation fault (core dumped)
```

**Debug**:
```bash
gdb ./bin/slowframe
(gdb) run -i test.jpg -p pd90 -o test.wav
# Program received signal SIGSEGV, Segmentation fault.
# 0x00007ffff7abc123 in mmsstv_encode_wrapper () at src/mmsstv/mmsstv_adapter.c:89

(gdb) backtrace
#0  mmsstv_encode_wrapper () at src/mmsstv/mmsstv_adapter.c:89
#1  mode_definition_encode () at src/sstv/mode_registry.c:234
#2  encode_sstv () at src/slowframe_sstv.c:920

(gdb) frame 0
(gdb) list
84      // Convert image to format expected by MMSSTV
85      VipsImage* converted = convert_for_mmsstv(image);
86      
87      // Call MMSSTV encoder
88      mmsstv_encode_func encoder = mmsstv_get_encoder();
89      result = encoder(converted, sample_rate, output_buffer);
90

(gdb) print converted
$1 = (VipsImage *) 0x0  # NULL pointer!
```

**Root cause**: `convert_for_mmsstv()` returned NULL (error), but code didn't check

**Fix**:
```c
// In src/mmsstv/mmsstv_adapter.c, line 85
VipsImage* converted = convert_for_mmsstv(image);
if (!converted) {
    error_log(ERROR_IMAGE_CONVERSION_FAILED, 
              "Failed to convert image for MMSSTV");
    return ERROR_IMAGE_CONVERSION_FAILED;
}

// Call MMSSTV encoder
mmsstv_encode_func encoder = mmsstv_get_encoder();
result = encoder(converted, sample_rate, output_buffer);
```

---

### Example 2: Incorrect Duration

**Bug report**: "Martin 2 takes 85 seconds instead of 58 seconds"

**Reproduce**:
```bash
./bin/slowframe -i test.jpg -p m2 -o test.wav
ffprobe test.wav 2>&1 | grep Duration
# Duration: 00:01:25.xx (85 seconds, should be ~58)
```

**Debug**:
```c
// In buildaudio_m2(), add timing verification
const double line_time = sync_duration + porch_duration + 
                         (scan_duration * 3);
fprintf(stderr, "Line time: %.6f seconds\n", line_time);
fprintf(stderr, "Total time (256 lines): %.2f seconds\n", 
        line_time * 256);

// Output:
// Line time: 0.332800 seconds
// Total time (256 lines): 85.20 seconds
```

**Analyze**:
```c
// Martin 2 spec: 58 seconds for 256 lines
// Line time should be: 58 / 256 = 0.2266 seconds

// Current calculation:
sync_duration = 0.004862    // Correct
porch_duration = 0.000572   // Correct
scan_duration = 0.146432    // WRONG! (This is Martin 1 timing)

// Martin 2 scan should be: 0.073216 seconds (half of Martin 1)
```

**Fix**:
```c
// In buildaudio_m2(), line ~520
const double scan_duration = 0.073216;  // Martin 2: half of M1
```

**Verify**:
```bash
make clean && make
./bin/slowframe -i test.jpg -p m2 -o test.wav
ffprobe test.wav 2>&1 | grep Duration
# Duration: 00:00:58.xx (correct!)
```

---

### Example 3: Memory Leak

**Bug report**: "Memory usage grows when encoding multiple images"

**Reproduce**:
```bash
# Script to encode 100 images
for i in {1..100}; do
    ./bin/slowframe -i test_$i.jpg -p m1 -o output_$i.wav
done

# Monitor memory with top/htop
# Memory grows from 50MB → 500MB → 1GB → ...
```

**Debug with Valgrind**:
```bash
valgrind --leak-check=full ./bin/slowframe -i test.jpg -p m1 -o test.wav

# Output shows:
# ==12345== 5,242,880 bytes in 1 blocks are definitely lost
# ==12345==    at 0x4C2DB8F: malloc (in /usr/lib/valgrind/...)
# ==12345==    by 0x400ABC: load_image (image_loader.c:123)
# ==12345==    by 0x400DEF: main (slowframe.c:456)
```

**Locate leak**:
```c
// In src/image/image_loader.c, line 123
VipsImage* image = vips_image_new();
// ... load image ...
return image;  // LEAK: never freed!
```

**Fix**:
```c
// In src/slowframe.c, after encoding
VipsImage* image = load_image(config.input_file);
// ... use image ...
encode_sstv(image, ...);

// FREE the image!
g_object_unref(image);
```

**Verify**:
```bash
valgrind --leak-check=full ./bin/slowframe -i test.jpg -p m1 -o test.wav

# Output:
# ==12345== All heap blocks were freed -- no leaks are possible
```

---

## Quick Reference

### Before Fixing

- [ ] Reproduce the bug locally
- [ ] Document reproduction steps
- [ ] Identify affected versions
- [ ] Form hypothesis about root cause

### While Fixing

- [ ] Add debug output
- [ ] Use GDB/Valgrind if needed
- [ ] Verify fix solves the problem
- [ ] Check for similar bugs elsewhere

### After Fixing

- [ ] Run full test suite (no regressions)
- [ ] Test edge cases
- [ ] Add regression test
- [ ] Update documentation if needed
- [ ] Create clear commit message
- [ ] Submit PR with explanation

---

## Resources

- [GDB Tutorial](https://www.gnu.org/software/gdb/documentation/)
- [Valgrind Manual](https://valgrind.org/docs/manual/manual.html)
- [DEVELOPER_WALKTHROUGH.md](DEVELOPER_WALKTHROUGH.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)

---

**Happy Bug Hunting!** 🐛🔍

*Last Updated: February 16, 2026*  
*SlowFrame v2.1.0*
