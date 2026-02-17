# MMSSTV Setup Guide for SlowFrame v2.1

**Complete user guide for enabling 43 additional SSTV modes**

---

## Quick Start

**Want 51 SSTV modes instead of 8?** Follow these steps:

### 1. Check Current Status

```bash
./bin/slowframe -L
```

**Look for the library status line:**
- ✅ **"MMSSTV library detected"** → You're all set! Enjoy 51 modes.
- ⚠️ **"MMSSTV library not found"** → Continue to Step 2

### 2. Obtain MMSSTV Library

**Option A: Download Pre-built** (Easiest)
```bash
# macOS
curl -O https://example.com/libsstv_encoder.1.0.0.dylib
sudo cp libsstv_encoder.1.0.0.dylib /usr/local/lib/

# Linux
curl -O https://example.com/libsstv_encoder.so.1.0.0
sudo cp libsstv_encoder.so.1.0.0 /usr/local/lib/
```

**Option B: Build from Source** (See [Building MMSSTV](#building-mmsstv-library) section)

### 3. Verify Installation

```bash
# Re-check
./bin/slowframe -L

# You should now see:
# MMSSTV library detected: /usr/local/lib/libsstv_encoder.1.0.0.dylib
# 
# Available SSTV modes:
# Native modes (8):
#   ...
# MMSSTV modes (43):
#   ...
```

### 4. Start Using MMSSTV Modes

```bash
# PD90 mode (popular for fast color)
./bin/slowframe -i photo.jpg -p pd90 -o output.wav

# Robot 24 (MMSSTV Robot24, different from native bw24)
./bin/slowframe -i photo.jpg -p robot24 -o output.wav

# FAX480 (high resolution)
./bin/slowframe -i photo.jpg -p fax480 -o output.wav
```

**Done!** 🎉

---

## Understanding MMSSTV Integration

### What is MMSSTV?

MMSSTV (Makoto Mori's SSTV) is a popular Windows SSTV application that includes a comprehensive encoding library with 43 additional SSTV modes beyond SlowFrame's 8 native modes.

### How SlowFrame Uses MMSSTV

**Runtime Detection**:
- SlowFrame searches for `libsstv_encoder` library at startup
- If found → Loads 43 additional modes dynamically
- If not found → Continues with 8 native modes

**No Compile-Time Dependency**:
- SlowFrame compiles and runs without MMSSTV
- Library is purely optional
- Zero impact on native functionality

**Graceful Fallback**:
- Missing library = warning + native modes only
- No crashes, no errors
- Seamless experience either way

---

## Complete Mode List

### Native Modes (Always Available)

| Mode | Protocol | VIS | Resolution | Duration | Color |
|------|----------|-----|------------|----------|-------|
| m1   | Martin 1 | 44  | 320×256    | 114s     | RGB   |
| m2   | Martin 2 | 40  | 320×256    | 58s      | RGB   |
| s1   | Scottie 1| 60  | 320×256    | 110s     | RGB   |
| s2   | Scottie 2| 56  | 320×256    | 71s      | RGB   |
| sdx  | Scottie DX | 76 | 320×256   | 269s     | RGB   |
| r36  | Robot 36 | 8   | 320×240    | 36s      | YUV   |
| r72  | Robot 72 | 12  | 320×240    | 72s      | YUV   |
| bw24 | Robot B&W 24 | 9 | 320×240  | 24s      | Mono  |

### MMSSTV Modes (43 modes, requires library)

**Martin Family** (2 modes):
- `m3`, `m4`

**PD Family** (7 modes):
- `pd50`, `pd90`, `pd120`, `pd160`, `pd180`, `pd240`, `pd290`

**Scottie Family** (2 modes):
- `s3`, `s4`

**Robot Family** (6 modes):
- `robot8bw`, `robot12bw`, `robot24`, `robot24bw`, `robot36`, `robot72`

**Wraase SC2 Family** (4 modes):
- `sc2_30`, `sc2_60`, `sc2_120`, `sc2_180`

**MP Family** (3 modes):
- `mp73n`, `mp115n`, `mp140n`

**FAX Family** (1 mode):
- `fax480`

**P Family** (3 modes):
- `p3`, `p5`, `p7`

**Pasokon Family** (3 modes):
- `pasokon_p3`, `pasokon_p5`, `pasokon_p7`

**MR Family** (5 modes):
- `mr73`, `mr90`, `mr115`, `mr140`, `mr175`

**ML Family** (4 modes):
- `ml180`, `ml240`, `ml280`, `ml320`

**AVT Family** (4 modes):
- `avt24`, `avt90`, `avt94`, `avt125`, `avt188`

**Total**: **51 modes** (8 native + 43 MMSSTV)

---

## Installation Methods

### Method 1: Standard Installation (Recommended)

**macOS:**
```bash
# Download library
curl -O https://example.com/libsstv_encoder.1.0.0.dylib

# Install system-wide
sudo cp libsstv_encoder.1.0.0.dylib /usr/local/lib/

# Update library cache (if necessary)
sudo update_dyld_shared_cache
```

**Linux:**
```bash
# Download library
curl -O https://example.com/libsstv_encoder.so.1.0.0

# Install system-wide
sudo cp libsstv_encoder.so.1.0.0 /usr/local/lib/

# Create symbolic link
sudo ln -s /usr/local/lib/libsstv_encoder.so.1.0.0 /usr/local/lib/libsstv_encoder.so.1

# Update library cache
sudo ldconfig
```

**Homebrew (macOS):**
```bash
# If available from tap
brew install mmsstv-encoder
```

### Method 2: Local Installation (No Root)

Place library in SlowFrame directory:

```bash
cd /path/to/SlowFrame
curl -O https://example.com/libsstv_encoder.1.0.0.dylib
# Library is automatically detected in ./ directory
./bin/slowframe -L
```

### Method 3: Custom Location

Use environment variable to specify path:

```bash
# Point to custom location
export SLOWFRAME_MMSSTV_LIB="/custom/path/libsstv_encoder.1.0.0.dylib"

# Run SlowFrame
./bin/slowframe -L
```

**Permanent Setup** (add to `~/.bashrc` or `~/.zshrc`):
```bash
echo 'export SLOWFRAME_MMSSTV_LIB="/custom/path/libsstv_encoder.1.0.0.dylib"' >> ~/.zshrc
source ~/.zshrc
```

---

## Building MMSSTV Library

### Prerequisites

**macOS:**
```bash
brew install cmake
```

**Linux:**
```bash
sudo apt-get install cmake build-essential
```

### Build Steps

```bash
# Clone MMSSTV source (placeholder URL)
git clone https://github.com/example/mmsstv-encoder.git
cd mmsstv-encoder

# Configure build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Compile
make -j$(nproc)

# Install
sudo make install

# Verify
ls -l /usr/local/lib/libsstv_encoder*
```

### Build Options

```bash
# Custom install prefix
cmake -DCMAKE_INSTALL_PREFIX=/opt/mmsstv ..

# Static library (if supported)
cmake -DBUILD_SHARED_LIBS=OFF ..

# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

---

## Verification & Troubleshooting

### Verify Library Detection

```bash
# List modes
./bin/slowframe -L

# Verbose mode (shows library path)
./bin/slowframe -v -L
```

**Expected Output:**
```
SlowFrame v2.1.0
MMSSTV library detected: /usr/local/lib/libsstv_encoder.1.0.0.dylib

Available SSTV modes:

Native modes (8):
  m1    - Martin 1           (VIS 44,  320x256,  114.0s, color)
  m2    - Martin 2           (VIS 40,  320x256,   58.0s, color)
  ...

MMSSTV modes (43):
  m3    - Martin 3           (VIS 36,  320x256,   57.0s, color)
  pd90  - PD 90              (VIS 99,  320x256,   90.0s, color)
  ...

Total: 51 modes
```

### Common Issues

#### Issue: "MMSSTV library not found"

**Diagnosis:**
```bash
# Check library exists
ls -l /usr/local/lib/libsstv_encoder*

# Check library dependencies (macOS)
otool -L /usr/local/lib/libsstv_encoder.1.0.0.dylib

# Check library dependencies (Linux)
ldd /usr/local/lib/libsstv_encoder.so.1.0.0
```

**Solutions:**
1. **Library missing**: Install library (see [Installation Methods](#installation-methods))
2. **Wrong architecture**: Download correct version (x86_64, arm64, etc.)
3. **Permission issues**: Check library is readable (`chmod 644 lib...`)
4. **Path issues**: Use `SLOWFRAME_MMSSTV_LIB` environment variable

#### Issue: Library found but modes not appearing

**Diagnosis:**
```bash
# Enable verbose logging
./bin/slowframe -v -L 2>&1 | grep -i mmsstv
```

**Solutions:**
1. **API version mismatch**: Update library to compatible version
2. **Symbol resolution failure**: Check library wasn't stripped
3. **Initialization failure**: Check library dependencies are installed

#### Issue: Encoding fails with MMSSTV mode

**Example:**
```bash
./bin/slowframe -i test.jpg -p pd90
# Error: Failed to encode with MMSSTV mode pd90
```

**Solutions:**
1. **Image format incompatible**: Try different input image
2. **Memory issue**: Check system has enough RAM
3. **Library crash**: Report bug with details

### Debug Logging

```bash
# Enable maximum verbosity
./bin/slowframe -v -i test.jpg -p pd90 2>&1 | tee debug.log

# Check for MMSSTV-specific messages
grep -i mmsstv debug.log
```

---

## Usage Examples

### Basic MMSSTV Encoding

```bash
# PD90 - Fast color mode (popular)
./bin/slowframe -i photo.jpg -p pd90 -o transmission.wav

# FAX480 - High resolution
./bin/slowframe -i detailed.png -p fax480 -o hires.wav

# Martin 3 - Extended Martin family
./bin/slowframe -i landscape.jpg -p m3 -o output.wav
```

### With Text Overlay

```bash
# PD90 with callsign overlay
./bin/slowframe -i photo.jpg -p pd90 \
    -T "text:K9ABC|size:24|pos:LT|bg:yes" \
    -o transmission.wav

# Multiple overlays
./bin/slowframe -i photo.jpg -p pd120 \
    -T "text:K9ABC|pos:LT|size:20" \
    -T "text:FN20xg|pos:RB|size:16" \
    -o transmission.wav
```

### With CW Identification

```bash
# PD90 with CW ID
./bin/slowframe -i photo.jpg -p pd90 -C "K9ABC" -o transmission.wav

# Custom CW parameters
./bin/slowframe -i photo.jpg -p pd90 \
    -C "K9ABC" --cw-wpm 20 --cw-tone 800 \
    -o transmission.wav
```

### Multi-Format Output

```bash
# WAV (default)
./bin/slowframe -i photo.jpg -p pd90 -f wav -o output.wav

# OGG Vorbis (compressed)
./bin/slowframe -i photo.jpg -p pd90 -f ogg -o output.ogg

# AIFF (Apple)
./bin/slowframe -i photo.jpg -p pd90 -f aiff -o output.aiff
```

### Batch Processing

```bash
#!/bin/bash
# Encode all JPEGs in directory with PD90

for img in *.jpg; do
    base=$(basename "$img" .jpg)
    ./bin/slowframe -i "$img" -p pd90 -o "${base}_pd90.wav"
    echo "Encoded: $img → ${base}_pd90.wav"
done
```

---

## Mode Selection Guide

### By Speed (Fastest to Slowest)

**Fast** (< 30s):
- `bw24` (24s, mono)
- `robot8bw` (8s, BW)
- `robot12bw` (12s, BW)

**Medium** (30-90s):
- `r36` (36s, color)
- `m2` (58s, color)
- `s2` (71s, color)
- `r72` (72s, color)
- `pd90` (90s, color) ✨ **Popular**

**Slow** (90-180s):
- `s1` (110s, color)
- `m1` (114s, color)
- `pd120` (120s, color)
- `pd160` (160s, color)

**Very Slow** (> 180s):
- `pd240` (240s, color)
- `sdx` (269s, color)
- `pd290` (290s, color)

### By Resolution

**Standard** (320×256):
- Martin, Scottie, PD, most MMSSTV modes

**Robot** (320×240):
- Robot family (native and MMSSTV)

**High Resolution** (640×480+):
- `fax480` (FAX mode)

### By Color Mode

**Full Color** (RGB/YUV):
- Martin, Scottie, PD, Robot 36/72, most MMSSTV

**Monochrome**:
- `bw24` (native Robot B&W 24)
- `robot8bw`, `robot12bw`, `robot24bw` (MMSSTV)

### Popular Choices

**Field Day / Events**:
- `pd90` - Good balance of speed and quality
- `r36` - Fast, good quality
- `m2` - Classic, reliable

**DX / Contest**:
- `m1` - High quality, well-supported
- `s1` - Scottie popular in some regions

**Experimental / High Quality**:
- `fax480` - Maximum detail
- `pd240` - Premium quality

---

## FCC Part 97 Compliance

### Station Identification

**Use Text Overlay** for FCC § 97.119 compliance:

```bash
# Overlay callsign on transmission
./bin/slowframe -i photo.jpg -p pd90 \
    -T "text:K9ABC|size:24|pos:LB|bg:yes" \
    -o transmission.wav
```

**Or use CW identification**:

```bash
# CW ID at end of transmission
./bin/slowframe -i photo.jpg -p pd90 -C "K9ABC" -o transmission.wav
```

**Best Practice**: Use both for redundancy:

```bash
./bin/slowframe -i photo.jpg -p pd90 \
    -T "text:K9ABC|size:20|pos:LT|bg:yes" \
    -C "K9ABC" \
    -o transmission.wav
```

### Content Guidelines

Per FCC § 97.113, transmissions must not include:
- Obscene or indecent material
- Music (except incidental in satellite retransmissions)
- Encrypted messages (except control commands)
- Commercial content

**Recommended Content**:
- Station photos
- Equipment photos
- Event coverage
- Weather/propagation data
- Scenic views
- Technical information

---

## Performance Considerations

### Encoding Times (macOS M1, approximate)

| Mode Type | Time | Notes |
|-----------|------|-------|
| Native    | 5-15ms | Extremely fast |
| MMSSTV    | 50-200ms | Still very fast |
| Overlay   | +5ms | Minimal impact |

**Example**:
```bash
# Measure encoding time
time ./bin/slowframe -i large.jpg -p pd90 -o output.wav

# Typical: 0.12s total (120ms)
```

### File Size Estimation

**Formula**: `size_mb ≈ duration_sec × sample_rate × 2 / 1024 / 1024`

**Examples** (44.1kHz, 16-bit mono):
- 36s mode → ~3.1 MB
- 90s mode → ~7.8 MB
- 120s mode → ~10.4 MB

**Reduce File Size**:
```bash
# Use OGG format (typically 10-20% of WAV size)
./bin/slowframe -i photo.jpg -p pd90 -f ogg -o output.ogg

# Or compress post-encoding
flac -8 output.wav  # Lossless, ~50% size
```

### Memory Usage

**Typical**:
- Base: 2-5 MB
- Image processing: +image file size
- MMSSTV library: +50 MB (when loaded)

**Large Images**:
```bash
# SlowFrame automatically scales down
# No memory concerns for normal use
./bin/slowframe -i huge_20000x15000.jpg -p pd90  # Works fine
```

---

## Advanced Configuration

### Environment Variables

```bash
# Specify library location
export SLOWFRAME_MMSSTV_LIB="/custom/path/libsstv_encoder.dylib"

# Disable MMSSTV (use native only)
export SLOWFRAME_NO_MMSSTV=1

# Debug library loading
export SLOWFRAME_DEBUG_MMSSTV=1
```

### Multiple Installations

```bash
# Project-specific library
cd ~/sstv/project1
export SLOWFRAME_MMSSTV_LIB="./libsstv_encoder_v1.dylib"
./bin/slowframe -L

# Different project
cd ~/sstv/project2
export SLOWFRAME_MMSSTV_LIB="./libsstv_encoder_v2.dylib"
./bin/slowframe -L
```

### Library Version Management

```bash
# Check library version
strings /usr/local/lib/libsstv_encoder.1.0.0.dylib | grep -i version

# Test compatibility
./bin/slowframe -v -L 2>&1 | grep -i "mmsstv.*version"
```

---

## FAQ

### Q: Do I need MMSSTV to use SlowFrame?

**A**: No! SlowFrame works perfectly with 8 native modes. MMSSTV is purely optional for 43 additional modes.

### Q: Will SlowFrame work on Linux / Raspberry Pi?

**A**: Yes! MMSSTV library availability may vary, but native modes work everywhere.

### Q: Can I use MMSSTV modes without the library?

**A**: No. MMSSTV modes require the library. Use native modes as an alternative.

### Q: Why not compile MMSSTV into SlowFrame?

**A**: Licensing, portability, and flexibility. Runtime loading keeps SlowFrame lightweight and flexible.

### Q: Which modes are most popular?

**A**: PD90, Robot 36, Martin 1, Scottie 1 are widely used. PD90 is especially popular for its speed/quality balance.

### Q: Can I contribute new native modes?

**A**: Yes! See [CONTRIBUTING.md](CONTRIBUTING.md) and [CONTRIB_ADDING_SSTV_MODES.md](CONTRIB_ADDING_SSTV_MODES.md).

### Q: Does MMSSTV library work on macOS ARM (M1/M2)?

**A**: Depends on library build. Check for arm64/universal binary. Native modes work natively on ARM.

### Q: How do I report issues with MMSSTV integration?

**A**: File a GitHub issue with:
- SlowFrame version (`./bin/slowframe -v`)
- Library path (from `-L` output)
- Error messages
- Steps to reproduce

---

## Resources

### Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md) - Technical architecture
- [MMSSTV_INTEGRATION.md](MMSSTV_INTEGRATION.md) - Technical integration details
- [MMSSTV_MODE_REFERENCE.md](MMSSTV_MODE_REFERENCE.md) - Complete mode specifications
- [BUILD.md](BUILD.md) - Building SlowFrame
- [README.md](../README.md) - General documentation

### External Links

- [MMSSTV Official](http://mmhamsoft.amateur-radio.ca/) - Original Windows application
- [SSTV Signal Specification](http://www.barberdsp.com/files/Dayton%20Paper.pdf) - Technical specs
- [FCC Part 97](https://www.ecfr.gov/current/title-47/chapter-I/subchapter-D/part-97) - Amateur radio regulations

### Community

- GitHub Issues: Report bugs, request features
- Ham Radio Forums: Discuss SSTV modes
- SSTV Activity: Monitor 14.230 MHz USB (20m), 7.173 MHz LSB (40m)

---

## Quick Reference Card

### Essential Commands

```bash
# List available modes
./bin/slowframe -L

# Basic encoding (native mode)
./bin/slowframe -i photo.jpg -p m1 -o output.wav

# MMSSTV mode
./bin/slowframe -i photo.jpg -p pd90 -o output.wav

# With identification
./bin/slowframe -i photo.jpg -p pd90 -C "K9ABC" -o output.wav

# With text overlay
./bin/slowframe -i photo.jpg -p pd90 -T "text:K9ABC|pos:LT" -o output.wav
```

### Library Management

```bash
# Check status
./bin/slowframe -L | head -1

# Custom library path
export SLOWFRAME_MMSSTV_LIB="/path/to/libsstv_encoder.dylib"

# Disable MMSSTV
export SLOWFRAME_NO_MMSSTV=1
```

### Popular Modes

- **`pd90`** - Fast color (90s)
- **`r36`** - Robot 36 (36s, color)
- **`m1`** - Martin 1 (114s, high quality)
- **`bw24`** - Robot B&W (24s, monochrome)

---

**73 de SlowFrame!** 📡

*Last Updated: February 16, 2026*  
*SlowFrame v2.1.0*
