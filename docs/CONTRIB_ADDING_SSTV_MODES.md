# Contributor Playbook: Adding SSTV Modes to SlowFrame

**Step-by-step guide for implementing new native SSTV modes**

---

## Overview

This playbook walks you through the complete process of adding a new native SSTV mode to SlowFrame v2.1. We'll use a real example: implementing **Wraase SC1-32** mode.

**Time required**: 2-4 hours (first time), 1-2 hours (subsequent modes)

**Prerequisites**:
- Basic C programming knowledge
- Understanding of SSTV signal format
- SlowFrame development environment set up
- Familiarity with [DEVELOPER_WALKTHROUGH.md](DEVELOPER_WALKTHROUGH.md)

---

## Table of Contents

1. [Before You Start](#before-you-start)
2. [Step 1: Research Mode Specifications](#step-1-research-mode-specifications)
3. [Step 2: Create Mode Definition](#step-2-create-mode-definition)
4. [Step 3: Implement Encoding Function](#step-3-implement-encoding-function)
5. [Step 4: Register Mode](#step-4-register-mode)
6. [Step 5: Test Implementation](#step-5-test-implementation)
7. [Step 6: Write Documentation](#step-6-write-documentation)
8. [Step 7: Submit Contribution](#step-7-submit-contribution)
9. [Troubleshooting](#troubleshooting)
10. [Examples](#examples)

---

## Before You Start

### Check if Mode Already Exists

```bash
# List all modes
./bin/slowframe -L

# Search for specific mode
./bin/slowframe -L | grep -i "wraase"
```

**If mode exists**:
- Native (first section) → Already implemented ✅
- MMSSTV (second section) → Available via MMSSTV library ✅
- Not listed → Proceed with implementation! 🚀

### Gather Mode Specifications

You'll need:
- **VIS code** (0-127)
- **Resolution** (width × height)
- **Scan timing** (sync, porch, pixel durations)
- **Color mode** (RGB, YUV, monochrome)
- **Scan order** (line-by-line RGB, interlaced, etc.)

**Resources**:
- [SSTV Modes Research](SSTV_MODES_RESEARCH.md)
- [SSTV Handbook (W5BAA)](http://www.barberdsp.com/downloads/Dayton%20Paper.pdf)
- [MMSSTV Mode Reference](MMSSTV_MODE_REFERENCE.md)
- Mode specification documents

---

## Step 1: Research Mode Specifications

### Example: Wraase SC1-32

**Find official specification**:

```
Mode Name: Wraase SC1-32
VIS Code: 93 (0x5D)
Resolution: 320×256 pixels
Transmission Time: ~32 seconds
Color System: RGB (sequential)
Scan Order: R-G-B per line

Timing (per line):
- Horizontal sync: 1200 Hz, 5ms
- Porch: 1500 Hz, 0.5ms  
- R scan: 320 pixels, 30ms (93.75 µs/pixel)
- G scan: 320 pixels, 30ms (93.75 µs/pixel)
- B scan: 320 pixels, 30ms (93.75 µs/pixel)

Line time: 5 + 0.5 + 30 + 30 + 30 = 95.5ms
Total time: 256 lines × 95.5ms = 24.45 seconds
(Plus VIS header ~7s = ~31.5s total)
```

**Document your research** in a markdown file: `docs/modes/mode_wraase_sc1_32_spec.md`

---

## Step 2: Create Mode Definition

### Choose Mode Family

SlowFrame organizes modes by family:
- `src/sstv/modes_martin.c` - Martin modes
- `src/sstv/modes_scottie.c` - Scottie modes
- `src/sstv/modes_robot.c` - Robot modes

**For new families**: Create new file `src/sstv/modes_wraase.c`

### Define Mode Structure

Create `src/sstv/modes_wraase.c`:

```c
#include "mode_registry.h"
#include "../slowframe_sstv.h"
#include <stddef.h>

// Mode definition
static mode_definition_t wraase_sc1_32 = {
    .name = "sc1_32",
    .vis_code = 93,
    .width = 320,
    .height = 256,
    .duration = 31.5,
    .family = "Wraase SC1",
    .color_mode = "color",
    .encode = sstv_encode_wraase_sc1_32,  // Function we'll implement
    .user_data = NULL
};

// Family registration function
void modes_wraase_register(void) {
    mode_registry_add(&wraase_sc1_32);
}
```

### Create Header File

Create `src/sstv/modes_wraase.h`:

```c
#ifndef MODES_WRAASE_H
#define MODES_WRAASE_H

/**
 * Register Wraase SC1 family modes with the mode registry
 */
void modes_wraase_register(void);

#endif // MODES_WRAASE_H
```

---

## Step 3: Implement Encoding Function

### Add to slowframe_sstv.c

Open `src/slowframe_sstv.c` and add the encoding function:

```c
/**
 * Wraase SC1-32 encoding
 * VIS: 93, 320×256, RGB sequential, ~32s
 */
static int buildaudio_sc1_32(
    VipsImage* image,
    int sample_rate,
    audio_encoder_t* encoder
) {
    const int width = 320;
    const int height = 256;
    
    // Timing constants (in seconds)
    const double sync_duration = 0.005;      // 5ms
    const double porch_duration = 0.0005;    // 0.5ms
    const double scan_duration = 0.030;      // 30ms (per color)
    const double pixel_duration = scan_duration / width;  // 93.75µs
    
    // SSTV tone frequencies
    const double sync_freq = 1200.0;
    const double porch_freq = 1500.0;
    
    // Extract pixel data
    int stride;
    const uint8_t* pixels = vips_image_get_data(image, &stride);
    if (!pixels) {
        return ERROR_IMAGE_INVALID_DATA;
    }
    
    // Encode each scan line
    for (int row = 0; row < height; row++) {
        const uint8_t* line = pixels + (row * stride);
        
        // 1. Horizontal sync pulse (1200 Hz, 5ms)
        if (!generate_tone_to_encoder(sync_freq, sync_duration, sample_rate, encoder)) {
            return ERROR_ENCODING_FAILED;
        }
        
        // 2. Porch (1500 Hz, 0.5ms)
        if (!generate_tone_to_encoder(porch_freq, porch_duration, sample_rate, encoder)) {
            return ERROR_ENCODING_FAILED;
        }
        
        // 3. Red scan (30ms for 320 pixels)
        for (int col = 0; col < width; col++) {
            const uint8_t* pixel = line + (col * 3);  // RGB format
            uint8_t red = pixel[0];
            double freq = pixel_to_frequency(red);
            if (!generate_tone_to_encoder(freq, pixel_duration, sample_rate, encoder)) {
                return ERROR_ENCODING_FAILED;
            }
        }
        
        // 4. Green scan (30ms for 320 pixels)
        for (int col = 0; col < width; col++) {
            const uint8_t* pixel = line + (col * 3);
            uint8_t green = pixel[1];
            double freq = pixel_to_frequency(green);
            if (!generate_tone_to_encoder(freq, pixel_duration, sample_rate, encoder)) {
                return ERROR_ENCODING_FAILED;
            }
        }
        
        // 5. Blue scan (30ms for 320 pixels)
        for (int col = 0; col < width; col++) {
            const uint8_t* pixel = line + (col * 3);
            uint8_t blue = pixel[2];
            double freq = pixel_to_frequency(blue);
            if (!generate_tone_to_encoder(freq, pixel_duration, sample_rate, encoder)) {
                return ERROR_ENCODING_FAILED;
            }
        }
    }
    
    return ERROR_SUCCESS;
}

/**
 * Public wrapper function for mode registry
 */
int sstv_encode_wraase_sc1_32(
    VipsImage* image,
    int sample_rate,
    audio_encoder_t* encoder,
    const slowframe_config_t* config
) {
    return buildaudio_sc1_32(image, sample_rate, encoder);
}
```

### Add Function Declaration

In `src/include/slowframe_sstv.h`, add:

```c
/**
 * Encode image in Wraase SC1-32 format
 */
int sstv_encode_wraase_sc1_32(
    VipsImage* image,
    int sample_rate,
    audio_encoder_t* encoder,
    const slowframe_config_t* config
);
```

---

## Step 4: Register Mode

### Update Mode Registry Initialization

In `src/sstv/mode_registry.c`, find `mode_registry_init()` and add:

```c
#include "sstv/modes_wraase.h"  // At top of file

void mode_registry_init(void) {
    // ... existing registrations ...
    modes_martin_register();
    modes_scottie_register();
    modes_robot_register();
    modes_wraase_register();  // NEW
}
```

### Update Makefile

In `makefile`, add new source file to `SOURCES`:

```makefile
SOURCES = \
    src/slowframe.c \
    src/slowframe_config.c \
    # ... existing files ...
    src/sstv/modes_martin.c \
    src/sstv/modes_scottie.c \
    src/sstv/modes_robot.c \
    src/sstv/modes_wraase.c \
    # ... rest of files ...
```

---

## Step 5: Test Implementation

### Compile

```bash
make clean && make

# Check for errors
echo "Exit code: $?"  # Should be 0
```

### Verify Mode Registration

```bash
# List modes
./bin/slowframe -L | grep -i sc1

# Expected output:
# sc1_32 - Wraase SC1-32    (VIS 93,  320x256,   31.5s, color)
```

### Test Encoding

```bash
# Create test image (320×256)
convert -size 320x256 gradient:blue-red test_sc1.png

# Encode
./bin/slowframe -i test_sc1.png -p sc1_32 -o test_sc1.wav

# Verify output
file test_sc1.wav
# Output: RIFF (little-endian) data, WAVE audio, Microsoft PCM, 16 bit, mono 22050 Hz

# Check duration (should be ~31-32s)
ffprobe test_sc1.wav 2>&1 | grep Duration
# Output: Duration: 00:00:31.xx
```

### Validate Signal

```bash
# Inspect waveform
ffplay test_sc1.wav

# Check for:
# 1. VIS header at start (~7s of tones)
# 2. Repeating pattern (sync + scan tones)
# 3. Correct total duration
# 4. No silence/gaps
```

### Create Test Suite

Create `tests/util/test_sc1_32_mode.py`:

```python
#!/usr/bin/env python3
"""Test suite for Wraase SC1-32 mode implementation"""

import unittest
import subprocess
import os

class TestWraaseSC1_32(unittest.TestCase):
    
    def setUp(self):
        self.slowframe = "./bin/slowframe"
        self.test_image = "tests/images/test_320x256.png"
        self.output = "/tmp/test_sc1_32.wav"
    
    def test_mode_registered(self):
        """Verify sc1_32 mode is registered"""
        result = subprocess.run(
            [self.slowframe, "-L"],
            capture_output=True,
            text=True
        )
        self.assertIn("sc1_32", result.stdout)
        self.assertIn("VIS 93", result.stdout)
    
    def test_encoding_success(self):
        """Test basic encoding"""
        result = subprocess.run(
            [self.slowframe, "-i", self.test_image, 
             "-p", "sc1_32", "-o", self.output],
            capture_output=True
        )
        self.assertEqual(result.returncode, 0)
        self.assertTrue(os.path.exists(self.output))
    
    def test_output_duration(self):
        """Verify transmission duration (~31-32s)"""
        subprocess.run(
            [self.slowframe, "-i", self.test_image,
             "-p", "sc1_32", "-o", self.output],
            check=True
        )
        
        # Get duration
        result = subprocess.run(
            ["ffprobe", "-v", "error", "-show_entries",
             "format=duration", "-of", "default=noprint_wrappers=1:nokey=1",
             self.output],
            capture_output=True,
            text=True
        )
        duration = float(result.stdout.strip())
        
        # Allow ±2s tolerance (VIS header variation)
        self.assertGreater(duration, 29.0)
        self.assertLess(duration, 34.0)
    
    def test_vis_code(self):
        """Verify VIS code 93 in mode info"""
        result = subprocess.run(
            [self.slowframe, "-L"],
            capture_output=True,
            text=True
        )
        # Look for "sc1_32" line with "VIS 93"
        for line in result.stdout.split('\n'):
            if 'sc1_32' in line:
                self.assertIn('93', line)
                break

if __name__ == "__main__":
    unittest.main()
```

**Run tests**:
```bash
cd tests/util
python3 test_sc1_32_mode.py

# Expected: OK (4 tests passed)
```

---

## Step 6: Write Documentation

### Update Mode List

In `docs/SSTV_MODES_RESEARCH.md`, add to "Implemented Modes" section:

```markdown
### Wraase SC1 Family (1 mode)

- **Wraase SC1-32** (`sc1_32`)
  - VIS: 93
  - Resolution: 320×256
  - Duration: ~32s
  - Color: RGB sequential
  - Status: ✅ **Native implementation**
```

### Create Mode Documentation

Create `docs/modes/WRAASE_SC1_32.md`:

```markdown
# Wraase SC1-32 Mode

## Specifications

- **Mode Name**: Wraase SC1-32
- **VIS Code**: 93 (0x5D)
- **Resolution**: 320×256 pixels
- **Transmission Time**: ~32 seconds
- **Color System**: RGB (sequential scan)

## Timing

| Component | Frequency | Duration | 
|-----------|-----------|----------|
| Horizontal Sync | 1200 Hz | 5.0 ms |
| Porch | 1500 Hz | 0.5 ms |
| Red Scan | Variable | 30.0 ms |
| Green Scan | Variable | 30.0 ms |
| Blue Scan | Variable | 30.0 ms |
| **Line Time** | - | **95.5 ms** |
| **Total (256 lines)** | - | **24.45 s** |
| **With VIS** | - | **~31.5 s** |

## Implementation

**Files**:
- `src/sstv/modes_wraase.c` - Mode definition
- `src/slowframe_sstv.c` - `buildaudio_sc1_32()` encoding function

**Status**: ✅ Implemented in SlowFrame v2.1

## Usage

```bash
./bin/slowframe -i photo.jpg -p sc1_32 -o output.wav
```

## References

- [SSTV Specification Document](#)
- [Wraase SC1 Technical Details](#)
```

---

## Step 7: Submit Contribution

### Pre-submission Checklist

- [ ] Code compiles without warnings
- [ ] Mode appears in `slowframe -L` output
- [ ] Encoding produces valid WAV file
- [ ] Test suite passes (all tests)
- [ ] No regressions (existing tests still pass)
- [ ] Code follows SlowFrame conventions
- [ ] Documentation updated

### Run Full Test Suite

```bash
# Backward compatibility
cd tests/util
python3 test_suite.py

# Modernization
python3 test_modernization.py

# MMSSTV integration
python3 test_mmsstv_integration.py

# New mode
python3 test_sc1_32_mode.py

# All should pass!
```

### Create Git Commit

```bash
# Stage changes
git add src/sstv/modes_wraase.c
git add src/sstv/modes_wraase.h
git add src/slowframe_sstv.c
git add src/include/slowframe_sstv.h
git add src/sstv/mode_registry.c
git add makefile
git add tests/util/test_sc1_32_mode.py
git add docs/modes/WRAASE_SC1_32.md
git add docs/SSTV_MODES_RESEARCH.md

# Commit with descriptive message
git commit -m "Add Wraase SC1-32 mode support

- Implement native encoding for SC1-32 (VIS 93)
- 320×256 resolution, RGB sequential, ~32s
- Add comprehensive test suite (4 tests, all passing)
- Update documentation with mode specifications

Resolves #123"
```

### Create Pull Request

1. Push to your fork:
```bash
git push origin feature/add-wraase-sc1-32
```

2. Open PR on GitHub:
   - Title: `Add Wraase SC1-32 mode support`
   - Description:
     ```markdown
     ## Summary
     Implements native support for Wraase SC1-32 SSTV mode.
     
     ## Changes
     - Added `modes_wraase.c/h` for Wraase SC1 family
     - Implemented `buildaudio_sc1_32()` encoding function
     - Created test suite with 4 comprehensive tests
     - Updated documentation
     
     ## Testing
     - ✅ All existing tests pass (no regressions)
     - ✅ New mode test suite: 4/4 passing
     - ✅ Manual verification with real transmission
     
     ## References
     - [SSTV Specification](#)
     - Closes #123
     ```

3. Wait for review and address feedback

---

## Troubleshooting

### Issue: Mode not appearing in list

**Check**:
```bash
# Verify registration called
grep -n "modes_wraase_register" src/sstv/mode_registry.c

# Rebuild
make clean && make

# Re-check
./bin/slowframe -L | grep sc1_32
```

**Solution**: Ensure `modes_wraase_register()` is called in `mode_registry_init()`

---

### Issue: Compilation errors

**Common errors**:

**"Undefined reference to sstv_encode_wraase_sc1_32"**
- Add function declaration to `src/include/slowframe_sstv.h`

**"modes_wraase.c: No such file"**
- Add file to `SOURCES` in `makefile`

**"Conflicting types for..."**
- Check function signature matches between `.c` and `.h`

---

### Issue: Encoding produces invalid output

**Debug**:
```bash
# Enable verbose output
./bin/slowframe -v -i test.png -p sc1_32 -o test.wav 2>&1 | tee debug.log

# Check debug log for errors
grep -i error debug.log
```

**Common causes**:
- Wrong pixel access (stride issues)
- Incorrect timing constants
- Missing tone generation calls
- Wrong frequency calculations

**Solution**:
```c
// Add debug output
fprintf(stderr, "DEBUG: Row %d/%d, sync_freq=%.1f, scan_duration=%.6f\n",
        row, height, sync_freq, scan_duration);

// Verify pixel access
const uint8_t* pixel = line + (col * 3);  // RGB = 3 bytes
fprintf(stderr, "Pixel[%d,%d]: R=%d G=%d B=%d\n",
        row, col, pixel[0], pixel[1], pixel[2]);
```

---

### Issue: Duration incorrect

**Expected**: ~32s  
**Actual**: 45s

**Check timing constants**:
```c
// Verify calculations
const double line_time = sync_duration + porch_duration + 
                         (scan_duration * 3);  // R+G+B
const double total_time = line_time * height;
fprintf(stderr, "Calculated duration: %.2fs\n", total_time);
```

**Common mistakes**:
- Scan duration per color vs. total
- Forgot to account for sync/porch
- Wrong width used in calculations

---

## Examples

### Simple Monochrome Mode

**Example**: Robot B&W 8 (8-second, monochrome)

```c
static int buildaudio_robot_bw8(
    VipsImage* image,
    int sample_rate,
    audio_encoder_t* encoder
) {
    const int width = 160;
    const int height = 120;
    const double scan_pixel_duration = 0.000069;  // 69µs per pixel
    
    for (int row = 0; row < height; row++) {
        // Sync + porch
        generate_tone_to_encoder(1200, 0.009, sample_rate, encoder);
        generate_tone_to_encoder(1500, 0.003, sample_rate, encoder);
        
        // Y (luminance) scan only
        for (int col = 0; col < width; col++) {
            uint8_t luma = get_pixel_luminance(image, row, col);
            double freq = pixel_to_frequency(luma);
            generate_tone_to_encoder(freq, scan_pixel_duration, sample_rate, encoder);
        }
    }
    
    return ERROR_SUCCESS;
}
```

---

### YUV Color Mode

**Example**: Robot 36 (YUV encoding)

```c
static int buildaudio_robot36(
    VipsImage* image,
    int sample_rate,
    audio_encoder_t* encoder
) {
    for (int row = 0; row < height; row++) {
        // Even lines: Y0, U, Y1, V
        // Odd lines: Y0, V, Y1, U
        
        bool even_line = (row % 2 == 0);
        
        // Y0 scan (first half of line)
        scan_channel_yuv(image, row, 0, width/2, 'Y', encoder);
        
        // U or V scan
        if (even_line) {
            scan_channel_yuv(image, row, 0, width, 'U', encoder);
        } else {
            scan_channel_yuv(image, row, 0, width, 'V', encoder);
        }
        
        // Y1 scan (second half of line)
        scan_channel_yuv(image, row, width/2, width, 'Y', encoder);
        
        // V or U scan
        if (even_line) {
            scan_channel_yuv(image, row, 0, width, 'V', encoder);
        } else {
            scan_channel_yuv(image, row, 0, width, 'U', encoder);
        }
    }
}
```

---

### High-Resolution Mode

**Example**: FAX480 (640×480)

```c
static int buildaudio_fax480(
    VipsImage* image,
    int sample_rate,
    audio_encoder_t* encoder
) {
    const int width = 640;
    const int height = 480;
    const double pixel_duration = 0.000182;  // 182µs per pixel
    
    // FAX uses grayscale
    VipsImage* gray = convert_to_grayscale(image);
    
    for (int row = 0; row < height; row++) {
        // Start tone
        generate_tone_to_encoder(1900, 0.005, sample_rate, encoder);
        
        // Scan full line (640 pixels)
        for (int col = 0; col < width; col++) {
            uint8_t luma = get_pixel_gray(gray, row, col);
            double freq = pixel_to_frequency(luma);
            generate_tone_to_encoder(freq, pixel_duration, sample_rate, encoder);
        }
        
        // Stop tone
        generate_tone_to_encoder(1500, 0.005, sample_rate, encoder);
    }
    
    g_object_unref(gray);
    return ERROR_SUCCESS;
}
```

---

## Quick Reference

### Essential Functions

```c
// Tone generation
bool generate_tone_to_encoder(
    double frequency,
    double duration,
    int sample_rate,
    audio_encoder_t* encoder
);

// Pixel to frequency (1500-2300 Hz)
double pixel_to_frequency(uint8_t pixel_value);

// Get pixel data
const uint8_t* vips_image_get_data(VipsImage* image, int* stride);

// RGB pixel access (3 bytes per pixel)
const uint8_t* pixel = line + (col * 3);
uint8_t red = pixel[0];
uint8_t green = pixel[1];
uint8_t blue = pixel[2];
```

### Timing Formulas

```c
// Pixels per second
double pps = 1.0 / pixel_duration;

// Line time
double line_time = sync + porch + (pixel_duration * width * channels);

// Total transmission time
double total_time = (line_time * height) + vis_header_time;
```

### Common VIS Codes

- Martin 1: 44, Martin 2: 40
- Scottie 1: 60, Scottie 2: 56
- Robot 36: 8, Robot 72: 12
- PD: 90-99 range
- Wraase: 88-95 range

Reserve unused VIS codes for new modes!

---

## Resources

- [ARCHITECTURE.md](ARCHITECTURE.md) - System architecture
- [DEVELOPER_WALKTHROUGH.md](DEVELOPER_WALKTHROUGH.md) - Code walkthrough
- [SSTV_MODES_RESEARCH.md](SSTV_MODES_RESEARCH.md) - Mode catalog
- [SSTV Specification (W5BAA)](http://www.barberdsp.com/downloads/Dayton%20Paper.pdf)
- [MMSSTV Mode Reference](MMSSTV_MODE_REFERENCE.md)

---

**Good luck with your contribution!** 🚀

*Last Updated: February 16, 2026*  
*SlowFrame v2.1.0*
