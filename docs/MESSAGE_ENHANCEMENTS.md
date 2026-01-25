# PiSSTVpp User Message Enhancements

## Summary
Comprehensive refactoring of user-facing messages throughout the SSTV encoder to provide clear progress tracking, meaningful error messages, and better configuration visibility.

## Changes Made

### 1. Startup Configuration Display
**Before:**
```
PiSSTVpp - SSTV Audio Encoder
```

**After:**
```
PiSSTVpp - SSTV Audio Encoder
Input  file: test.png
Output file: test_m1.wav (wav format, 22050 Hz)
Protocol: m1 (VIS code 44)
Image dimensions: 320x256 pixels (assumed by SSTV spec)
```
- Shows input/output file details
- Displays selected protocol with VIS code
- Shows audio configuration (format, sample rate)
- Provides context about image dimensions

### 2. Progress Indicators with [1/4]-[4/4] Steps

#### [1/4] Image Loading
```
[1/4] Loading image...
   » Detected: 320x256, 3-band image
   » Buffering pixel data...
   ✓ Image buffer ready: 320x256 RGB (245760 bytes)
   ✓ Image loaded successfully
```
- Progress step indicator
- Intermediate status messages with arrows (»)
- Success checkmarks (✓)
- Detailed buffer information (size in bytes)

#### [2/4] Output File Opening
```
[2/4] Opening output file...
   ✓ Output file ready
```

#### [3/4] SSTV Encoding
```
[3/4] Encoding image as SSTV audio...
Adding VIS header to audio data.
   ✓ VIS header added
   » Processing pixels...
   » Encoding row 64/256 (25%)
   » Encoding row 128/256 (50%)
   » Encoding row 192/256 (75%)
 done
   ✓ Image encoded
   ✓ VIS trailer added
```
- Encoding step with progress tracking
- VIS header/trailer confirmation
- Row-by-row progress (every 25%) showing:
  - Current row number and total
  - Percentage completion
- For Robot 36, shows row pairs instead

#### [4/4] File Writing
```
[4/4] Writing WAV file...
   » Format: WAV
   » Sample rate: 22050 Hz
   ✓ File written
```
- Output file format confirmation
- Audio configuration details
- Write completion confirmation

### 3. Success Message
```
✅ SUCCESS!
   Output: test_m1.wav
   Time elapsed: 0 seconds
   Ready to transmit via SSB radio!
```
- Clear success indicator (✅)
- Output filename reference
- Transmission readiness message

### 4. Enhanced Error Messages
**File Not Found Example:**
```
[1/4] Loading image...
   ✗ Failed to load 'nonexistent.png'
   Details: VipsForeignLoad: file "nonexistent.png" does not exist

❌ ERROR: Failed to load image 'nonexistent.png'
   Supported formats: PNG, JPEG, GIF, BMP, TIFF, WebP
   Check file exists and is a valid image file
```

**Memory Allocation Error:**
```
   ✗ Memory allocation failed
   Details: Unable to allocate 245760 bytes for image data
   Hint: Image is 320x256 (960 bytes/row)
```

**Buffering Error:**
```
   ✗ Image buffering failed
   Details: Unable to access pixel data from image
```

### 5. Protocol-Specific Progress Tracking

#### Martin/Scottie Modes (M1, M2, S1, S2, SDX)
Progress every 64 rows out of 256:
```
   » Encoding row 64/256 (25%)
   » Encoding row 128/256 (50%)
   » Encoding row 192/256 (75%)
```

#### Robot 36 Mode
Progress every 32 row pairs out of 240 (processes 2 rows at a time):
```
   » Encoding row pair 32/240 (13%)
   » Encoding row pair 64/240 (26%)
   » Encoding row pair 96/240 (40%)
   » Encoding row pair 128/240 (53%)
   » Encoding row pair 160/240 (66%)
   » Encoding row pair 192/240 (80%)
   » Encoding row pair 224/240 (93%)
```

## Code Modifications

### Functions Updated

1. **main()** (Lines 200-310)
   - Configuration summary display
   - [1/4]-[4/4] progress indicators
   - Status checkmarks for each step
   - Success/error messages with context

2. **load_image_with_vips()** (Lines 350-450)
   - Image format detection with dimensions
   - Buffering progress indicators
   - Buffer size confirmation
   - Detailed error messages with vips context

3. **buildaudio_m()** (Lines 590-643)
   - Progress indicator every 64 rows
   - Removed redundant "Adding image" messages
   - Removed redundant "Done" messages

4. **buildaudio_s()** (Lines 648-706)
   - Progress indicator every 64 rows
   - Removed redundant messages

5. **buildaudio_r36()** (Lines 707-796)
   - Progress indicator every 32 row pairs
   - Row pair numbering instead of single rows
   - Removed redundant messages

6. **addvisheader()** (Lines 540-589)
   - Removed internal printf statements
   - Messages now handled in main()

7. **addvistrailer()** (Lines 798-810)
   - Removed internal printf statements
   - Messages now handled in main()

8. **writefile_wav()** (Lines 882-952)
   - Removed internal printf statements
   - Message details handled in main()

9. **writefile_aiff()** (Lines 816-877)
   - Removed internal printf statements
   - Message details handled in main()

## Message Symbols Used

- **✓** - Checkmark: Step completed successfully
- **✗** - X mark: Error or failure
- **✅** - Green checkmark emoji: Overall success
- **❌** - Red X emoji: Overall failure
- **»** - Bullet arrow: Progress/intermediate step
- **[1/4]** - Step indicators: Current step / Total steps

## Test Results

All 6 SSTV modes tested successfully with enhanced messaging:
- ✓ **m1** (Martin 1): 5,179,156 bytes
- ✓ **m2** (Martin 2): 2,699,418 bytes
- ✓ **s1** (Scottie 1): 4,973,788 bytes
- ✓ **s2** (Scottie 2): 3,274,388 bytes
- ✓ **sdx** (Scottie DX): 11,996,822 bytes
- ✓ **r36** (Robot 36): 1,726,560 bytes

Compilation: **Clean** (zero warnings with `-Wall -Wextra -Wpedantic`)

## User Experience Improvements

1. **Better Progress Visibility**
   - Users see which stage the encoding is at
   - Progress percentage displayed during encoding
   - Clear indication when complete

2. **Meaningful Feedback**
   - Intermediate status messages show what's happening
   - Buffer sizes and image dimensions confirmed
   - File formats and sample rates displayed

3. **Clearer Error Messages**
   - Error type clearly identified with ✗
   - Root cause from underlying libraries included
   - Helpful hints provided (supported formats, file existence, etc.)

4. **Professional Appearance**
   - Structured output with consistent formatting
   - Status symbols for quick scanning
   - Transmission readiness confirmation at completion
