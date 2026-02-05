# PiSSTVpp2 User Guide

**Complete reference for all features and capabilities**

Version 2.0 | Updated: 2024

---

## Table of Contents

1. [Introduction](#introduction)
2. [Installation](#installation)
3. [Basic Usage](#basic-usage)
4. [SSTV Modes](#sstv-modes)
5. [Command-Line Options](#command-line-options)
6. [Audio Formats](#audio-formats)
7. [Image Handling](#image-handling)
8. [CW Identification](#cw-identification)
9. [VIS Header](#vis-header)
10. [Advanced Features](#advanced-features)
11. [Performance Tuning](#performance-tuning)
12. [Troubleshooting](#troubleshooting)
13. [Best Practices](#best-practices)
14. [Technical Details](#technical-details)

---

## Introduction

### What is PiSSTVpp2?

PiSSTVpp2 is a modern SSTV (Slow Scan Television) encoder that converts images into audio signals. Originally designed for Raspberry Pi, it works on any Linux system and macOS.

**Key Features:**
- 7 SSTV modes (Martin, Scottie, Robot families)
- 3 audio formats (WAV, AIFF, OGG Vorbis)
- Automatic aspect ratio correction
- VIS header for mode identification
- CW (Morse code) identification
- Sample rates from 8-48 kHz
- libvips-based image processing

### Version 2.0 Changes

PiSSTVpp2 v2.0 is a complete rewrite with:

**New Features:**
- OGG Vorbis output format
- Advanced aspect ratio handling (center, pad, stretch)
- Enhanced CW envelope shaping
- Improved VIS header generation
- Comprehensive test suite (55 tests)

**Under the Hood:**
- Migrated from libgd to libvips
- Modular architecture (image, SSTV, audio modules)
- Error handling improvements
- Memory safety enhancements
- Better logging and diagnostics

**Compatibility:**
- All v1.x command-line options supported
- Same output quality and timing
- Drop-in replacement for v1.x

---

## Installation

### System Requirements

**Operating Systems:**
- Linux (any distribution)
- Raspberry Pi OS (32-bit or 64-bit)
- macOS 10.12+
- Other UNIX-like systems

**Dependencies:**
- C11 compiler (GCC 4.9+ or Clang 3.4+)
- libvips 8.0+ (image processing)
- libogg + libvorbis (for OGG support, optional)
- make

**Disk Space:**
- Source: ~500 KB
- Build: ~2 MB
- Runtime: Minimal (audio files size varies)

### Installing Dependencies

#### Debian/Ubuntu/Raspberry Pi OS
```bash
sudo apt-get update
sudo apt-get install build-essential libvips-dev libogg-dev libvorbis-dev
```

#### Fedora/RHEL/CentOS
```bash
sudo dnf install gcc make vips-devel libogg-devel libvorbis-devel
```

#### macOS (Homebrew)
```bash
brew install vips libogg libvorbis
```

#### macOS (MacPorts)
```bash
sudo port install vips libogg libvorbis
```

### Building from Source

```bash
# Navigate to source directory
cd /path/to/PiSSTVpp2

# Clean previous builds
make clean

# Build
make all

# Verify build
./bin/pisstvpp2 -h
```

**Build Output:**
- Binary: `bin/pisstvpp2`
- No installation required (self-contained)

### Optional: System-Wide Installation

```bash
# Install to /usr/local/bin
sudo cp bin/pisstvpp2 /usr/local/bin/

# Verify
pisstvpp2 -h
```

---

## Basic Usage

### Command Structure

```bash
pisstvpp2 -i <input-image> -o <output-audio> [options]
```

**Required:**
- `-i`: Input image file
- `-o`: Output audio file

**Optional:**
- `-p`: SSTV mode (default: m1)
- `-f`: Audio format (default: wav)
- `-r`: Sample rate (default: 22050)
- `-a`: Aspect mode (default: center)
- `-C`: CW callsign
- And more...

### Simplest Example

```bash
pisstvpp2 -i photo.jpg -o transmission.wav
```

This creates a Martin 1 mode WAV file with default settings.

### Typical Usage

```bash
pisstvpp2 -i photo.jpg -o transmission.wav -p s2 -C "N0CALL" -v
```

This creates:
- Scottie 2 mode transmission
- WAV format
- With CW identification "N0CALL"
- Verbose output showing progress

---

## SSTV Modes

### Available Modes

PiSSTVpp2 v2.0 supports 7 SSTV modes:

| Mode Code | Full Name | Duration | Resolution | Color | Quality |
|-----------|-----------|----------|------------|-------|---------|
| **m1** | Martin 1 | 114s | 320×256 | RGB | Very Good |
| **m2** | Martin 2 | 58s | 320×256 | RGB | Good |
| **s1** | Scottie 1 | 110s | 320×256 | RGB | Excellent |
| **s2** | Scottie 2 | 71s | 320×256 | RGB | Very Good |
| **sdx** | Scottie DX | 269s | 320×256 | RGB | Best |
| **r36** | Robot 36 | 36s | 320×240 | YCrCb | Good |
| **r72** | Robot 72 | 72s | 320×240 | YCrCb | Excellent |

### Mode Selection Guide

**Choose by Priority:**

**Speed (fastest to slowest):**
1. Robot 36 (r36) - 36 seconds
2. Martin 2 (m2) - 58 seconds
3. Scottie 2 (s2) - 71 seconds
4. Robot 72 (r72) - 72 seconds
5. Scottie 1 (s1) - 110 seconds
6. Martin 1 (m1) - 114 seconds
7. Scottie DX (sdx) - 269 seconds

**Quality (best to good):**
1. Scottie DX (sdx) - Best (slowest)
2. Robot 72 (r72) - Excellent
3. Scottie 1 (s1) - Excellent
4. Martin 1 (m1) - Very Good
5. Scottie 2 (s2) - Very Good
6. Martin 2 (m2) - Good
7. Robot 36 (r36) - Good (fastest)

**Compatibility (most to least):**
1. Martin 1 (m1) - Universal standard
2. Scottie 1 (s1) - Very common
3. Martin 2 (m2) - Common
4. Scottie 2 (s2) - Common
5. Robot 36 (r36) - Common
6. Robot 72 (r72) - Less common
7. Scottie DX (sdx) - Rare

**Recommendations:**

- **General Use**: Scottie 2 (s2) - Best balance of speed/quality
- **Maximum Compatibility**: Martin 1 (m1) - Works everywhere
- **Quick Tests**: Robot 36 (r36) - Fast turnaround
- **High Quality**: Robot 72 (r72) - Excellent quality, reasonable speed
- **Best Quality**: Scottie DX (sdx) - When time doesn't matter
- **Contests**: Martin 1 (m1) or Scottie 1 (s1) - Standard modes

### Mode Technical Details

#### Martin Modes (m1, m2)

**Characteristics:**
- RGB color model (separate red, green, blue scans)
- Horizontal sync pulses between lines
- 320×256 resolution
- VIS codes: 44 (M1), 40 (M2)

**Timing:**
- M1: 114.286 seconds per image
- M2: 58.286 seconds per image

**Best For:**
- Standard SSTV operations
- Maximum compatibility
- When everyone needs to decode

#### Scottie Modes (s1, s2, sdx)

**Characteristics:**
- RGB color model
- Separate sync pulse
- 320×256 resolution
- VIS codes: 60 (S1), 56 (S2), 76 (SDX)

**Timing:**
- S1: 110 seconds per image
- S2: 71 seconds per image
- SDX: 269 seconds per image

**Best For:**
- High-quality transmissions
- When quality matters most
- DX (long-distance) contacts with SDX

#### Robot Modes (r36, r72)

**Characteristics:**
- YCrCb color space (luminance + chroma)
- Different color encoding
- 320×240 resolution
- VIS codes: 8 (R36), 12 (R72)

**Timing:**
- R36: 36 seconds per image
- R72: 72 seconds per image

**Best For:**
- Fast transmissions
- Better color representation for some images
- Quick turnaround needed

---

## Command-Line Options

### Complete Reference

#### Input/Output

```
-i, --input <file>
    Input image file path (required)
    Formats: JPEG, PNG, GIF, BMP, TIFF, WebP, etc.
    Example: -i /path/to/photo.jpg

-o, --output <file>
    Output audio file path (required)
    Extension determines format if -f not specified
    Example: -o transmission.wav
```

#### SSTV Configuration

```
-p, --protocol <mode>
    SSTV mode to use (default: m1)
    Options: m1, m2, s1, s2, sdx, r36, r72
    Example: -p s2

-n, --no-vis
    Disable VIS header (not recommended)
    Use only if receiver doesn't support VIS
    Example: -n
```

#### Audio Settings

```
-f, --format <format>
    Audio output format (default: wav)
    Options: wav, aiff, ogg
    Example: -f ogg

-r, --sample-rate <rate>
    Audio sample rate in Hz (default: 22050)
    Range: 8000-48000 Hz
    Common: 11025, 22050, 44100, 48000
    Example: -r 44100
```

#### Image Handling

```
-a, --aspect <mode>
    Aspect ratio correction mode (default: center)
    
    Options:
      center  - Center-crop to 4:3 (no distortion, may crop)
      pad     - Add black bars to preserve aspect
      stretch - Force to 4:3 (may distort)
    
    Example: -a pad
```

#### CW Identification

```
-C, --cw-id <callsign>
    Add CW identification after SSTV image
    Format: Letters, numbers, and "/" only
    Example: -C "N0CALL"

-W, --wpm <speed>
    CW speed in words per minute (default: 18)
    Range: 5-40 WPM
    Standard: 18 WPM
    Example: -W 25

-T, --cw-tone <frequency>
    CW tone frequency in Hz (default: 850)
    Range: 400-1200 Hz
    Standard: 850 Hz
    Example: -T 1000
```

#### Diagnostic Options

```
-v, --verbose
    Enable verbose logging
    Shows detailed progress and diagnostics
    Example: -v

-h, --help
    Display help message and exit
    Shows all options with brief descriptions
    Example: -h

--version
    Display version information and exit
    Example: --version
```

### Option Combinations

**Maximum Quality:**
```bash
pisstvpp2 -i photo.jpg -o output.wav -p sdx -r 48000 -f wav
```

**Minimum File Size:**
```bash
pisstvpp2 -i photo.jpg -o output.ogg -p r36 -r 11025 -f ogg
```

**Contest Standard:**
```bash
pisstvpp2 -i photo.jpg -o output.wav -p m1 -C "N0CALL" -r 44100
```

**Quick Test:**
```bash
pisstvpp2 -i photo.jpg -o test.wav -p r36 -v
```

---

## Audio Formats

### WAV Format

**Description:** Uncompressed PCM audio (WAVE)

**Characteristics:**
- Lossless, uncompressed
- 16-bit signed integer samples
- Standard RIFF WAVE format
- Universal compatibility

**File Size:** Large (depends on sample rate and duration)

**When to Use:**
- Maximum compatibility required
- Quality is critical
- File size not a concern
- Transmitting immediately

**Example:**
```bash
pisstvpp2 -i photo.jpg -o transmission.wav -f wav
```

**File Size Examples:**
- Robot 36 @ 22050 Hz: ~3.2 MB
- Martin 1 @ 22050 Hz: ~10 MB
- Scottie DX @ 22050 Hz: ~24 MB

### AIFF Format

**Description:** Audio Interchange File Format (Apple)

**Characteristics:**
- Lossless, uncompressed
- 16-bit signed integer samples
- Apple/macOS standard format
- Similar to WAV but different structure

**File Size:** Same as WAV

**When to Use:**
- macOS/iOS ecosystem
- Apple software compatibility
- Similar use cases as WAV

**Example:**
```bash
pisstvpp2 -i photo.jpg -o transmission.aiff -f aiff
```

### OGG Vorbis Format

**Description:** Compressed lossy audio

**Characteristics:**
- Lossy compression
- Quality factor: 0.4 (good quality)
- Open source codec
- 60-80% smaller than WAV

**File Size:** 20-40% of equivalent WAV

**When to Use:**
- Storage space limited
- Archiving transmissions
- Quality not absolutely critical
- Internet distribution

**Example:**
```bash
pisstvpp2 -i photo.jpg -o transmission.ogg -f ogg
```

**File Size Examples:**
- Robot 36 @ 22050 Hz: ~1 MB (vs 3.2 MB WAV)
- Martin 1 @ 22050 Hz: ~3 MB (vs 10 MB WAV)
- Scottie DX @ 22050 Hz: ~7 MB (vs 24 MB WAV)

**Quality Note:** OGG quality is excellent for SSTV. The compression is designed to preserve the tones used in SSTV signaling.

### Format Selection Guide

| Use Case | Recommended Format | Reason |
|----------|-------------------|--------|
| Live transmission | WAV or AIFF | Uncompressed, direct playback |
| Archive/storage | OGG | Smaller files, good quality |
| Maximum compatibility | WAV | Universal support |
| macOS/iOS use | AIFF | Native format |
| Web sharing | OGG | Smaller downloads |
| Professional use | WAV | Lossless |

---

## Image Handling

### Supported Input Formats

PiSSTVpp2 uses libvips, supporting:

**Common Formats:**
- JPEG (.jpg, .jpeg)
- PNG (.png)
- GIF (.gif)
- BMP (.bmp)
- TIFF (.tiff, .tif)

**Advanced Formats:**
- WebP (.webp)
- SVG (.svg) - Vector graphics
- PDF (.pdf) - First page
- HEIF (.heif, .heic) - If libvips compiled with support
- RAW camera formats - If libvips compiled with support

### Image Processing Pipeline

1. **Load**: Image loaded via libvips
2. **Color Space**: Convert to RGB (if needed)
3. **Aspect Correction**: Applied based on `-a` mode
4. **Resize**: Scaled to mode resolution (320×256 or 320×240)
5. **Scan**: Read line-by-line for SSTV encoding

### Aspect Ratio Handling

SSTV modes expect 4:3 aspect ratio images. Different input aspect ratios need correction.

#### Mode: `center` (Default)

**What it does:**
- Crops image from center to 4:3 aspect ratio
- No distortion
- May lose edges of image

**Best for:**
- Landscape photos (already close to 4:3)
- When center content is most important
- Avoiding distortion

**Example:**
```bash
pisstvpp2 -i portrait.jpg -o output.wav -a center
```

**Result:** Portrait image cropped horizontally from center, losing left/right edges.

#### Mode: `pad`

**What it does:**
- Adds black bars (pillarbox/letterbox) to reach 4:3
- Preserves entire image
- No distortion, but bars visible in transmission

**Best for:**
- Portrait images
- Square images
- When entire image must be visible
- Artistic frames

**Example:**
```bash
pisstvpp2 -i square.jpg -o output.wav -a pad
```

**Result:** Square image with black bars on left and right sides.

#### Mode: `stretch`

**What it does:**
- Forces image to 4:3 by stretching
- May distort image (squash or elongate)
- No cropping, no bars

**Best for:**
- Images already close to 4:3
- When distortion is acceptable
- Avoiding black bars and cropping

**Example:**
```bash
pisstvpp2 -i wide.jpg -o output.wav -a stretch
```

**Result:** Wide image compressed horizontally to fit 4:3.

### Aspect Ratio Examples

**Input: 16:9 landscape (1920×1080)**
- `center`: Crops top and bottom, keeps width
- `pad`: Black bars on top and bottom
- `stretch`: Image compressed vertically (looks squished)

**Input: 9:16 portrait (1080×1920)**
- `center`: Crops left and right sides
- `pad`: Black bars on left and right
- `stretch`: Image compressed horizontally (looks squashed)

**Input: 1:1 square (1000×1000)**
- `center`: Crops top and bottom
- `pad`: Black bars on left and right
- `stretch`: Image compressed horizontally

**Input: 4:3 already (800×600)**
- All modes: Same result (no correction needed)

### Resolution Handling

**Internal Processing:**
- Martin modes: 320×256
- Scottie modes: 320×256
- Robot modes: 320×240

**Input Resolution:**
- Any resolution accepted
- Automatically scaled to mode resolution
- Maintains quality during resize

**Recommendations:**
- Input >= 640×480: Best quality (downscaled)
- Input 320×240 to 640×480: Good quality (minimal scaling)
- Input < 320×240: Quality loss (upscaled)

**Example Inputs:**
```bash
# High-res photo (downscaled, excellent quality)
pisstvpp2 -i 4000x3000.jpg -o output.wav

# Small image (upscaled, may show artifacts)
pisstvpp2 -i 160x120.jpg -o output.wav

# Perfect size (minimal processing)
pisstvpp2 -i 640x480.jpg -o output.wav
```

---

## CW Identification

### Overview

CW (Continuous Wave) identification adds a Morse code signature after the SSTV image. Common in amateur radio for station identification.

**Format:**
```
"SSTV DE <callsign>"
```

Where:
- "SSTV" = Identifies transmission type
- "DE" = Morse abbreviation for "from"
- `<callsign>` = Your station callsign

### Basic Usage

```bash
pisstvpp2 -i photo.jpg -o output.wav -C "N0CALL"
```

**Result:**
1. SSTV image transmitted
2. Brief silence
3. CW: "SSTV DE N0CALL" in Morse code

### CW Parameters

#### Callsign (-C)

**Format Requirements:**
- Letters: A-Z (converted to uppercase)
- Numbers: 0-9
- Slash: "/" (for portable operations)
- Examples: "N0CALL", "W1AW/5", "VE3ABC"

**Invalid Characters:**
- Spaces (ignored)
- Special characters except "/"
- Lowercase (auto-converted)

#### Speed (-W)

**Range:** 5-40 WPM (words per minute)

**Typical Speeds:**
- 5 WPM: Very slow, beginners
- 13 WPM: Standard CW test speed
- 18 WPM: Default, good balance
- 25 WPM: Fast, experienced operators
- 40 WPM: Very fast

**Example:**
```bash
# Slow CW for easy copying
pisstvpp2 -i photo.jpg -C "N0CALL" -W 13 -o output.wav

# Fast CW for quick ID
pisstvpp2 -i photo.jpg -C "N0CALL" -W 25 -o output.wav
```

#### Tone Frequency (-T)

**Range:** 400-1200 Hz

**Common Frequencies:**
- 600 Hz: Low, mellow tone
- 800 Hz: Standard, good for QRM
- 850 Hz: Default, pleasant tone
- 1000 Hz: Higher, cuts through
- 1200 Hz: Very high, distinctive

**Example:**
```bash
# Low tone
pisstvpp2 -i photo.jpg -C "N0CALL" -T 600 -o output.wav

# High tone
pisstvpp2 -i photo.jpg -C "N0CALL" -T 1200 -o output.wav
```

### Complete CW Examples

**Standard ID:**
```bash
pisstvpp2 -i photo.jpg -o output.wav -C "N0CALL"
```
- 18 WPM
- 850 Hz tone
- "SSTV DE N0CALL"

**Slow, Low Tone:**
```bash
pisstvpp2 -i photo.jpg -o output.wav -C "W1AW" -W 13 -T 600
```
- 13 WPM (slower)
- 600 Hz (deeper tone)

**Fast, High Tone:**
```bash
pisstvpp2 -i photo.jpg -o output.wav -C "K0ABC" -W 25 -T 1000
```
- 25 WPM (faster)
- 1000 Hz (higher pitch)

**Portable Operation:**
```bash
pisstvpp2 -i photo.jpg -o output.wav -C "N0CALL/5"
```
- Callsign with portable indicator

### CW Envelope Shaping

v2.0 includes improved CW envelope shaping to prevent clicks and key chirps.

**Features:**
- Smooth rise/fall times (5ms attack/release)
- Click-free keying
- Professional-sounding CW
- Reduced audio artifacts

**Technical Details:**
- Attack time: 5 ms (0-100%)
- Decay time: 5 ms (100%-0%)
- Shape: Raised cosine (smooth)
- Dit duration: 1200/WPM milliseconds

**No Configuration Required:** Envelope shaping is automatic.

### Without CW ID

If you don't need CW identification:

```bash
# No -C option = no CW
pisstvpp2 -i photo.jpg -o output.wav
```

**Result:** Only SSTV image, no CW signature.

---

## VIS Header

### What is VIS?

VIS (Vertical Interval Signaling) is a code sent before the SSTV image to identify the transmission mode.

**Purpose:**
- Tells receiving software which mode to use
- Enables automatic mode detection
- Ensures correct decoding

**Structure:**
- Leader tone: 1900 Hz, ~300ms
- Break: 1200 Hz, 10ms
- Start bit: 1900 Hz, 30ms
- 8 data bits: 1100 Hz (0) or 1300 Hz (1)
- Parity bit
- Stop bit: 1200 Hz, 30ms

**Total Duration:** ~640ms

### VIS Codes by Mode

| Mode | VIS Code | Binary |
|------|----------|--------|
| Martin 1 | 44 | 0010 1100 |
| Martin 2 | 40 | 0010 1000 |
| Scottie 1 | 60 | 0011 1100 |
| Scottie 2 | 56 | 0011 1000 |
| Scottie DX | 76 | 0100 1100 |
| Robot 36 | 8 | 0000 1000 |
| Robot 72 | 12 | 0000 1100 |

### Default Behavior

VIS header is **enabled by default**:

```bash
pisstvpp2 -i photo.jpg -o output.wav
# VIS header included automatically
```

### Disabling VIS Header

Use `-n` or `--no-vis` to disable:

```bash
pisstvpp2 -i photo.jpg -o output.wav -n
```

**When to Disable:**
- Receiver doesn't support VIS (rare)
- Manual mode selection required
- Testing specific scenarios
- Compatibility with old hardware

**Warning:** Disabling VIS means receivers must manually select the mode. Not recommended for general use.

### VIS Transmission Order

1. **Leader Tone**
   - Frequency: 1900 Hz
   - Duration: ~300ms
   - Purpose: Alert receiver, AGC settling

2. **VIS Code**
   - Duration: ~340ms
   - Purpose: Mode identification

3. **Sync Pulse**
   - Mode-specific timing
   - Signals start of image data

4. **Image Data**
   - Line-by-line transmission
   - Mode-specific duration

### Verifying VIS Header

**Method 1: Listen**
- First ~300ms: Steady tone (1900 Hz)
- Next ~340ms: Clicking/beeping sounds (VIS bits)
- Then: Image data (varying tones)

**Method 2: Spectral Analysis**
```bash
# Use FFT analysis tool
sox output.wav -n spectrogram -o spectrogram.png
# Look for VIS pattern at start
```

**Method 3: Decode**
- Play file into SSTV receiver
- Check if mode auto-detected
- If yes: VIS working correctly

---

## Advanced Features

### Verbose Logging

Enable detailed diagnostic output:

```bash
pisstvpp2 -i photo.jpg -o output.wav -v
```

**Output Includes:**
- Image loading details (format, size, color space)
- Aspect ratio corrections applied
- Resolution scaling information
- SSTV mode and timing details
- VIS header generation
- Sample generation progress
- CW identification details (if enabled)
- Audio encoding information
- File size and duration

**Example Output:**
```
[INFO] PiSSTVpp2 v2.0 SSTV Encoder
[INFO] Loading image: photo.jpg
[INFO] Image format: JPEG, size: 1920x1080, bands: 3
[INFO] Applying aspect correction: center (crop)
[INFO] Scaling to 320x256
[INFO] SSTV mode: Martin 1 (m1), VIS code: 44
[INFO] Sample rate: 22050 Hz, format: WAV
[INFO] Generating VIS header...
[INFO] Encoding image data (256 lines)...
[INFO] Progress: 128/256 lines (50%)
[INFO] Progress: 256/256 lines (100%)
[INFO] Image encoding complete (114.286 seconds)
[INFO] Adding CW identification: SSTV DE N0CALL
[INFO] CW speed: 18 WPM, tone: 850 Hz
[INFO] Writing audio file: output.wav
[INFO] Output: 2522880 samples, duration: 114.48s, size: 4.8 MB
[INFO] Encoding complete!
```

**When to Use:**
- Troubleshooting issues
- Verifying settings
- Understanding processing steps
- Debugging problems
- Learning how encoder works

### Chaining with Audio Tools

PiSSTVpp2 outputs standard audio files that can be processed with other tools.

#### Normalize Audio Levels

```bash
# Generate SSTV
pisstvpp2 -i photo.jpg -o transmission.wav

# Normalize with sox
sox transmission.wav normalized.wav gain -n

# Normalize with ffmpeg
ffmpeg -i transmission.wav -af loudnorm output.wav
```

#### Convert Formats

```bash
# SSTV to WAV
pisstvpp2 -i photo.jpg -o sstv.wav

# Convert to MP3
ffmpeg -i sstv.wav -b:a 192k transmission.mp3

# Convert to FLAC
ffmpeg -i sstv.wav transmission.flac
```

#### Concatenate Multiple Transmissions

```bash
# Generate multiple images
pisstvpp2 -i photo1.jpg -o tx1.wav
pisstvpp2 -i photo2.jpg -o tx2.wav
pisstvpp2 -i photo3.jpg -o tx3.wav

# Concatenate with sox
sox tx1.wav tx2.wav tx3.wav combined.wav

# Add silence between
sox tx1.wav -p pad 0 5 : tx2.wav -p pad 0 5 : tx3.wav combined.wav
```

#### Add Audio Effects

```bash
# Generate base transmission
pisstvpp2 -i photo.jpg -o base.wav

# Add echo effect
sox base.wav echo.wav echo 0.8 0.9 1000 0.3

# Add reverb
sox base.wav reverb.wav reverb 50

# Apply lowpass filter (simulate radio)
sox base.wav filtered.wav lowpass 3000
```

#### Analyze Audio

```bash
# Generate transmission
pisstvpp2 -i photo.jpg -o transmission.wav

# Show statistics
sox transmission.wav -n stat

# Generate spectrogram
sox transmission.wav -n spectrogram -o spectrogram.png

# Analyze frequency content
sox transmission.wav -n stat -freq
```

### Batch Processing

Process multiple images automatically:

#### Simple Bash Loop

```bash
#!/bin/bash
# Process all JPEGs in directory

for img in *.jpg; do
    basename="${img%.jpg}"
    pisstvpp2 -i "$img" -o "${basename}.wav" -p s2 -v
done
```

#### With Callsign

```bash
#!/bin/bash
CALLSIGN="N0CALL"

for img in photos/*.jpg; do
    basename=$(basename "$img" .jpg)
    pisstvpp2 -i "$img" -o "sstv/${basename}.wav" \
        -p s2 -C "$CALLSIGN" -v
done
```

#### Different Modes

```bash
#!/bin/bash
# Generate same image in multiple modes

INPUT="photo.jpg"
MODES=("m1" "m2" "s1" "s2" "sdx" "r36" "r72")

for mode in "${MODES[@]}"; do
    pisstvpp2 -i "$INPUT" -o "transmission_${mode}.wav" -p "$mode"
done
```

#### Organized Output

```bash
#!/bin/bash
# Organized batch processing

INPUT_DIR="source_images"
OUTPUT_DIR="sstv_output"
MODE="s2"
CALLSIGN="N0CALL"

mkdir -p "$OUTPUT_DIR"

for img in "$INPUT_DIR"/*.{jpg,png}; do
    [ -f "$img" ] || continue
    basename=$(basename "$img")
    name="${basename%.*}"
    
    echo "Processing: $basename"
    pisstvpp2 -i "$img" \
        -o "$OUTPUT_DIR/${name}.wav" \
        -p "$MODE" \
        -C "$CALLSIGN" \
        -v
done

echo "Batch processing complete!"
```

### Integration with Other Tools

#### Transmission via PiFM

```bash
# Generate SSTV
pisstvpp2 -i photo.jpg -o transmission.wav -p s2

# Transmit on Raspberry Pi (requires pifm)
# WARNING: Check local regulations before transmitting!
sudo ./pifm transmission.wav 145.500 22050
```

#### Fldigi Integration

```bash
# Generate SSTV transmission
pisstvpp2 -i photo.jpg -o transmission.wav -p m1

# Play into Fldigi (virtual audio cable or direct)
aplay -D pulse transmission.wav
```

#### Remote Operation

```bash
# Generate on local machine
pisstvpp2 -i photo.jpg -o transmission.wav

# Copy to remote station
scp transmission.wav user@station:/tmp/

# Play on remote station
ssh user@station 'aplay /tmp/transmission.wav'
```

---

## Performance Tuning

### Encoding Speed

**Factors Affecting Speed:**
1. Image size (larger = slower loading)
2. Sample rate (higher = more samples)
3. SSTV mode (complexity)
4. System CPU speed
5. Disk I/O speed

**Typical Encoding Times:**
- Raspberry Pi 3: 2-5 seconds
- Raspberry Pi 4: 1-3 seconds
- Modern x86 laptop: <1 second
- Desktop workstation: <0.5 seconds

**Note:** Encoding is much faster than transmission duration. A 114-second Martin 1 transmission encodes in 1-2 seconds.

### Memory Usage

**Typical Memory:**
- Base program: ~5 MB
- Image buffer: ~1 MB (320×256 RGB)
- Audio buffer: Variable (depends on mode/sample rate)
- Total: ~10-20 MB

**Large Files:**
- Input images: Loaded and immediately scaled (efficient)
- Output audio: Written incrementally (low memory)

**Low-Memory Systems:**
- PiSSTVpp2 runs well on 256 MB+ RAM
- Suitable for Raspberry Pi Zero

### Disk I/O

**Read Performance:**
- Image loading: Fast (libvips is efficient)
- JPEG decode: Hardware-accelerated on some systems

**Write Performance:**
- WAV: Fast (minimal processing)
- AIFF: Fast (similar to WAV)
- OGG: Slower (encoding overhead)

**SSD vs HDD:**
- SSD: Minimal difference (small files)
- SD Card (Pi): Use quality cards for best performance

### Sample Rate Selection

**Trade-offs:**

| Sample Rate | File Size | Quality | Compatibility | Encoding Speed |
|-------------|-----------|---------|---------------|----------------|
| 8000 Hz | Smallest | Adequate | Universal | Fastest |
| 11025 Hz | Small | Good | Very Good | Fast |
| 22050 Hz | Medium | Very Good | Excellent | Fast |
| 44100 Hz | Large | Excellent | Excellent | Moderate |
| 48000 Hz | Largest | Best | Good | Slower |

**Recommendations:**
- **Default (22050 Hz)**: Best balance for most uses
- **Low bandwidth (11025 Hz)**: Smaller files, still good quality
- **Archival (44100 Hz)**: Maximum quality, standard CD rate
- **Professional (48000 Hz)**: Studio quality, larger files

**Quality Note:** SSTV signal bandwidth is ~3 kHz. Sample rates above 8 kHz are sufficient for perfect reproduction. Higher rates mainly affect audio quality and future processing.

### Optimization Tips

**For Faster Encoding:**
```bash
# Use lower sample rate
pisstvpp2 -i photo.jpg -o output.wav -r 11025

# Use faster mode
pisstvpp2 -i photo.jpg -o output.wav -p r36

# Avoid verbose logging in batch
# (omit -v flag)
```

**For Smaller Files:**
```bash
# Use OGG format
pisstvpp2 -i photo.jpg -o output.ogg -f ogg -r 11025

# Fast mode + OGG
pisstvpp2 -i photo.jpg -o output.ogg -p r36 -f ogg -r 11025
```

**For Best Quality:**
```bash
# High sample rate + best mode
pisstvpp2 -i photo.jpg -o output.wav -p sdx -r 48000

# Lossless format
pisstvpp2 -i photo.jpg -o output.wav -f wav -r 44100
```

---

## Troubleshooting

### Common Issues and Solutions

#### "Cannot load image file"

**Symptoms:**
```
[ERROR] Failed to load image: /path/to/photo.jpg
```

**Causes:**
- File doesn't exist
- Wrong file path
- Unsupported format
- Corrupted file
- Permission denied

**Solutions:**
```bash
# Verify file exists
ls -l /path/to/photo.jpg

# Check file type
file photo.jpg

# Use absolute path
pisstvpp2 -i /full/path/to/photo.jpg -o output.wav

# Check permissions
chmod 644 photo.jpg

# Try different format
convert photo.jpg photo.png
pisstvpp2 -i photo.png -o output.wav
```

#### "Cannot create output file"

**Symptoms:**
```
[ERROR] Failed to create audio file: output.wav
```

**Causes:**
- No write permission
- Disk full
- Invalid path
- Directory doesn't exist

**Solutions:**
```bash
# Check disk space
df -h

# Check directory exists
mkdir -p output/

# Check permissions
ls -ld output/

# Use different location
pisstvpp2 -i photo.jpg -o /tmp/output.wav
```

#### "OGG format not supported"

**Symptoms:**
```
[ERROR] OGG Vorbis support not compiled
```

**Cause:**
- PiSSTVpp2 built without OGG libraries

**Solution:**
```bash
# Install OGG libraries
sudo apt-get install libogg-dev libvorbis-dev

# Rebuild
make clean && make all

# Verify OGG support
./bin/pisstvpp2 -h | grep -i ogg
```

#### Output file size is huge

**Symptoms:**
- WAV files larger than expected
- Disk filling up

**Causes:**
- High sample rate
- Long transmission mode
- Multiple files

**Solutions:**
```bash
# Use lower sample rate
pisstvpp2 -i photo.jpg -o output.wav -r 11025

# Use compression
pisstvpp2 -i photo.jpg -o output.ogg -f ogg

# Clean up old files
rm old_transmissions/*.wav
```

#### Audio has clicks or pops

**Symptoms:**
- Audible clicks in output
- Especially in CW identification

**Cause:**
- Usually not a problem in v2.0 (envelope shaping improved)
- May occur if audio played through low-quality DAC

**Solutions:**
```bash
# Regenerate file
pisstvpp2 -i photo.jpg -o output.wav -C "CALL"

# Test with different player
aplay output.wav
# vs
ffplay output.wav

# Check CW settings
pisstvpp2 -i photo.jpg -o output.wav -C "CALL" -W 18 -T 850
```

#### Image looks wrong when decoded

**Symptoms:**
- Image slanted, skewed, or distorted
- Colors incorrect
- Sync issues

**Causes:**
- Wrong mode selected in receiver
- VIS header not recognized
- Sample rate mismatch
- Timing drift

**Solutions:**
```bash
# Ensure VIS header enabled
pisstvpp2 -i photo.jpg -o output.wav
# (no -n flag)

# Try different mode
pisstvpp2 -i photo.jpg -o output.wav -p m1

# Use standard sample rate
pisstvpp2 -i photo.jpg -o output.wav -r 22050

# Check aspect ratio
pisstvpp2 -i photo.jpg -o output.wav -a center

# Manually select mode in receiver
# Match mode to -p setting
```

#### Aspect ratio not working as expected

**Symptoms:**
- Image still distorted with `-a` flag
- Unexpected cropping or padding

**Cause:**
- Misunderstanding of aspect modes
- Image already 4:3 (no change needed)

**Solutions:**
```bash
# Try all modes to compare
pisstvpp2 -i photo.jpg -o center.wav -a center
pisstvpp2 -i photo.jpg -o pad.wav -a pad
pisstvpp2 -i photo.jpg -o stretch.wav -a stretch

# Decode all three and compare results

# Check original image aspect ratio
identify photo.jpg
# or
file photo.jpg
```

#### Verbose output too much information

**Symptoms:**
- Terminal flooded with messages
- Hard to see important info

**Solutions:**
```bash
# Remove -v flag
pisstvpp2 -i photo.jpg -o output.wav

# Redirect to file
pisstvpp2 -i photo.jpg -o output.wav -v > log.txt 2>&1

# Filter output
pisstvpp2 -i photo.jpg -o output.wav -v 2>&1 | grep ERROR
```

### Debugging Steps

**Step 1: Verify Installation**
```bash
# Check binary exists
which pisstvpp2

# Check version
pisstvpp2 --version

# Check help
pisstvpp2 -h
```

**Step 2: Test with Known-Good Image**
```bash
# Use simple test image
convert -size 320x256 xc:blue test.png

# Encode
pisstvpp2 -i test.png -o test.wav -v
```

**Step 3: Check Dependencies**
```bash
# Check libvips
pkg-config --modversion vips

# Check OGG (if using)
pkg-config --modversion vorbis

# Check linking
ldd bin/pisstvpp2
```

**Step 4: Enable Verbose Logging**
```bash
# Run with -v for diagnostics
pisstvpp2 -i photo.jpg -o output.wav -v
```

**Step 5: Test Output File**
```bash
# Check file created
ls -lh output.wav

# Verify audio file
file output.wav

# Play back
aplay output.wav
```

---

## Best Practices

### Image Preparation

**1. Aspect Ratio**
- Prepare images at 4:3 ratio before encoding
- Avoids cropping or distortion
- Recommended: 800×600, 1024×768, 1600×1200

**2. Content**
- High contrast works best
- Avoid very detailed images (may encode poorly)
- Bright colors transmit well
- Text should be large and bold

**3. Format**
- Use JPEG for photos
- Use PNG for graphics/text
- Avoid GIF (limited colors)

**4. Resolution**
- Minimum: 320×256 (native SSTV resolution)
- Recommended: 640×480 or higher
- Maximum: Any (will be downscaled)

### Mode Selection

**1. For Maximum Compatibility**
- Use Martin 1 (m1)
- Most receivers support it
- Standard for contests

**2. For Best Quality/Speed Balance**
- Use Scottie 2 (s2)
- Good quality, reasonable speed
- Widely supported

**3. For Quick Tests**
- Use Robot 36 (r36)
- Fast turnaround
- Good for testing equipment

**4. For Best Quality**
- Use Scottie DX (sdx) or Robot 72 (r72)
- When time is available
- Excellent results

### Audio Settings

**1. Sample Rate**
- 22050 Hz: Default, works everywhere
- 44100 Hz: If quality critical
- 11025 Hz: If file size important

**2. Format**
- WAV: For immediate transmission
- OGG: For storage/archiving
- AIFF: For macOS ecosystem

**3. Quality**
- Use lossless (WAV/AIFF) for transmission
- Compress (OGG) for archival
- Keep originals if disk space available

### CW Identification

**1. Callsign**
- Always use correct callsign
- Include portable indicator if needed (/P, /M, /5, etc.)
- Uppercase is automatic

**2. Speed**
- 18 WPM: Standard, good default
- 13 WPM: If recipient may be beginner
- 25 WPM: If both experienced

**3. Tone**
- 850 Hz: Good default
- Adjust if specific frequency needed
- Keep in 600-1000 Hz range for best copy

### Transmission

**1. Pre-Transmission**
- Decode your own transmission first
- Verify image quality
- Check mode and settings
- Test audio levels

**2. During Transmission**
- Monitor transmission
- Watch for sync issues
- Keep audio levels consistent
- Avoid clipping/distortion

**3. Post-Transmission**
- Verify receipt if possible
- Get signal reports
- Archive transmitted files
- Note any issues

### File Management

**1. Organization**
```
sstv/
├── originals/       # Original images
├── prepared/        # Pre-processed images (4:3)
├── transmissions/   # Generated audio files
└── received/        # Decoded images from others
```

**2. Naming**
```
# Descriptive names
sunset-martin1.wav
contest-entry-scottie2.wav
portable-ops-robot36.wav

# Include metadata
20240115-N0CALL-m1.wav
field-day-s2-44100.wav
```

**3. Archival**
```bash
# Compress old transmissions
for f in transmissions/*.wav; do
    ffmpeg -i "$f" "${f%.wav}.ogg"
done

# Backup
tar czf sstv-backup-$(date +%Y%m%d).tar.gz sstv/
```

### Workflow Example

**Professional Workflow:**

```bash
# 1. Prepare image (4:3 aspect, good content)
convert raw-photo.jpg -resize 800x600^ -gravity center -extent 800x600 prepared.jpg

# 2. Generate transmission
pisstvpp2 -i prepared.jpg -o transmission.wav \
    -p s2 -C "N0CALL" -r 44100 -v

# 3. Verify quality
aplay transmission.wav
# Decode with SSTV software

# 4. Archive
cp transmission.wav archive/$(date +%Y%m%d)-prepared-s2.wav
pisstvpp2 -i prepared.jpg -o archive/$(date +%Y%m%d)-prepared-s2.ogg -f ogg

# 5. Transmit
# (via radio/online/etc.)

# 6. Clean up working files
rm prepared.jpg transmission.wav
```

---

## Technical Details

### Audio Signal Specifications

**Sample Format:**
- Type: 16-bit signed integer
- Byte order: Little-endian
- Channels: Mono (1 channel)
- Range: -32768 to +32767

**Frequency Generation:**
- Method: Direct Digital Synthesis (DDS)
- Phase accumulator: Double precision
- Sample accuracy: Excellent (<0.01 Hz at 22050 Hz)

**Tone Frequencies:**
Mode-dependent, typically:
- Sync: 1200 Hz
- Black: 1500 Hz
- White: 2300 Hz
- Range: 1500-2300 Hz (800 Hz bandwidth)

### SSTV Encoding Details

**Line Scan Order:**
- Top to bottom
- Left to right pixels within line
- Interlacing: None (progressive)

**Color Encoding:**

Martin/Scottie modes (RGB):
1. Green scan line
2. Blue scan line
3. Red scan line
4. Sync pulse
5. Repeat for next line

Robot modes (YCrCb):
1. Y (luminance) scan
2. Cr (red difference) scan
3. Cb (blue difference) scan
4. Sync pulse
5. Repeat

**Timing Precision:**
- Sync pulses: ±0.1ms
- Scan lines: ±0.5ms
- Overall duration: <0.01% error

### Module Architecture

**Code Structure:**

```
pisstvpp2_image.c/.h
├── Image loading (libvips)
├── Color space conversion
├── Aspect ratio correction
└── Scaling/resizing

pisstvpp2_sstv.c/.h
├── VIS header generation
├── Line-by-line encoding
├── Frequency generation
└── Sample buffer management

pisstvpp2_audio_encoder.c/.h
├── Format handlers (WAV/AIFF/OGG)
├── Sample writing
└── File I/O

audio_encoder_*.c
├── WAV encoder
├── AIFF encoder
└── OGG encoder

pisstvpp2.c
├── Main program
├── Argument parsing
├── CW generation
└── Module coordination
```

**Data Flow:**

```
Image File
    ↓
[Image Module]
- Load image
- Convert to RGB
- Apply aspect correction
- Scale to mode resolution
    ↓
[SSTV Module]
- Generate VIS header
- Scan image line-by-line
- Generate audio samples
    ↓
[Audio Encoder Module]
- Format audio data
- Write to file
    ↓
Output File
```

### Performance Characteristics

**CPU Usage:**
- Encoding: 5-20% (single core, varies by system)
- I/O bound more than CPU bound
- Scales with sample rate

**Memory Profile:**
- Heap allocations: Minimal
- Stack usage: Low
- No memory leaks (valgrind verified)

**File I/O:**
- Sequential writes (efficient)
- Buffered I/O (system optimized)
- No random access

### Compatibility

**Standards Compliance:**
- SSTV timing: Per mode specifications
- VIS codes: Standard codes used
- Audio formats: Industry-standard formats

**Tested With:**
- QSSTV (Linux)
- MMSSTV (Windows via Wine)
- Robot36 (Android/iOS)
- Fldigi (Linux/macOS/Windows)
- Various online decoders

**Platform Support:**
- Linux: All distributions
- macOS: 10.12+
- Raspberry Pi: All models (including Zero)
- BSD: Likely works (untested)
- Windows: Via WSL or Cygwin

---

## Appendices

### Appendix A: Frequency Tables

**Martin Modes:**
- Sync: 1200 Hz, 4.862 ms
- Porch: 1500 Hz, 0.572 ms
- Black: 1500 Hz
- White: 2300 Hz
- Scan: 0.457 ms per pixel (M1), 0.229 ms (M2)

**Scottie Modes:**
- Sync: 1200 Hz, 9 ms
- Porch: 1500 Hz, 1.5 ms
- Black: 1500 Hz
- White: 2300 Hz
- Scan: Variable by mode

**Robot Modes:**
- Sync: 1200 Hz, 9 ms
- Porch: 1900 Hz, 3 ms
- Black: 1500 Hz
- White: 2300 Hz
- Luma/Chroma encoding

### Appendix B: File Size Calculator

**Formula:**
```
Size (bytes) = Duration (s) × Sample Rate (Hz) × 2 bytes
```

**Examples:**

Robot 36 @ 11025 Hz:
```
36s × 11025 Hz × 2 = 793,800 bytes ≈ 775 KB
```

Martin 1 @ 22050 Hz:
```
114s × 22050 Hz × 2 = 5,027,400 bytes ≈ 4.8 MB
```

Scottie DX @ 44100 Hz:
```
269s × 44100 Hz × 2 = 23,725,800 bytes ≈ 22.6 MB
```

**OGG Compression:**
Typically 20-30% of WAV size at quality 0.4.

### Appendix C: Command Quick Reference

```bash
# Basic encoding
pisstvpp2 -i input.jpg -o output.wav

# All options
pisstvpp2 -i input.jpg -o output.wav -p s2 -f wav -r 22050 \
    -a center -C "CALL" -W 18 -T 850 -v

# Fast mode, small file
pisstvpp2 -i input.jpg -o output.ogg -p r36 -f ogg -r 11025

# Best quality
pisstvpp2 -i input.jpg -o output.wav -p sdx -r 48000

# Portrait with padding
pisstvpp2 -i portrait.jpg -o output.wav -a pad

# No VIS header
pisstvpp2 -i input.jpg -o output.wav -n

# Batch processing
for img in *.jpg; do
    pisstvpp2 -i "$img" -o "${img%.jpg}.wav" -p s2
done
```

### Appendix D: Glossary

**AIFF**: Audio Interchange File Format (Apple audio format)

**Aspect Ratio**: Proportional relationship between width and height

**CW**: Continuous Wave (Morse code)

**DDS**: Direct Digital Synthesis (tone generation method)

**Interlacing**: Scanning odd/even lines separately (not used in SSTV)

**libvips**: Fast image processing library

**OGG Vorbis**: Open-source lossy audio compression

**PCM**: Pulse Code Modulation (uncompressed audio)

**SSTV**: Slow Scan Television (image transmission mode)

**VIS**: Vertical Interval Signaling (mode identification code)

**WPM**: Words Per Minute (CW speed measurement)

**YCrCb**: Luminance-chrominance color space

---

## Version History

### v2.0 (2024)
- Complete rewrite
- Migrated to libvips
- Added OGG Vorbis support
- Enhanced aspect ratio handling
- Improved CW envelope
- Better VIS header
- Comprehensive test suite
- Modular architecture

### v1.x (Previous)
- Original version
- libgd-based
- Basic functionality
- WAV output only

---

## License

PiSSTVpp2 is licensed under the GNU General Public License v3.0 or later.

See [docs/LICENSE.md](LICENSE.md) for full license text.

---

## Getting Help

- **Quick Start**: [docs/QUICK_START.md](QUICK_START.md)
- **Documentation**: This guide
- **Issues**: GitHub Issues (if available)
- **Testing**: [docs/TEST_SUITE_README.md](TEST_SUITE_README.md)

---

**73 de PiSSTVpp2!** 📡🖼️📻

*End of User Guide*
