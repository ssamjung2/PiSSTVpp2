# Legacy Components Build Guide

**Build instructions for PiSSTVpp v1.0 and pifm_sstv**

---

## Table of Contents

1. [Overview](#overview)
2. [Platform Support](#platform-support)
3. [Quick Start](#quick-start)
4. [Platform-Specific Instructions](#platform-specific-instructions)
5. [Troubleshooting](#troubleshooting)
6. [Using the Built Binaries](#using-the-built-binaries)

---

## Overview

The legacy components directory contains two original SSTV transmission tools:

| Component | Purpose | Platform | Language |
|-----------|---------|----------|----------|
| **pisstvpp** | Convert images to SSTV audio | Linux, macOS, RPi | C |
| **pifm** | Transmit on RPi FM with audio | Raspberry Pi only | C++ |

Both are self-contained and independent of the main pisstvpp2 codebase.

---

## Platform Support

### pisstvpp (Image to SSTV Audio)

| Platform | Support | Dependencies | Tested |
|----------|---------|--------------|--------|
| Raspberry Pi OS (ARM) | ✓ Full | libgd-dev, libmagic-dev | Yes |
| Ubuntu/Debian (x86_64) | ✓ Full | libgd-dev, libmagic-dev | Yes |
| Ubuntu/Debian (ARM64) | ✓ Full | libgd-dev, libmagic-dev | Partial |
| macOS (Intel) | ✓ Full | gd via Homebrew | Yes |
| macOS (Apple Silicon) | ✓ Full | gd via Homebrew | Yes |

### pifm (RPi FM Transmitter)

| Platform | Support | Notes |
|----------|---------|-------|
| Raspberry Pi 1-4 | ✓ Full | Requires root, BCM2835+ hardware |
| Raspberry Pi 5+ | ⚠ Limited | May require porting to new hardware |
| Other Linux (ARM) | ✗ UNO | No /dev/mem access framework |
| macOS / Intel Linux | ✗ None | Raspberry Pi hardware specific |

---

## Quick Start

### For Raspberry Pi

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install build-essential libgd-dev libmagic-dev g++

# Build
cd src/legacy
make -f Makefile.legacy all

# Test pisstvpp
./../../bin/pisstvpp -h

# Test pifm (requires RPi hardware and root)
sudo ./../../bin/pifm --help
```

### For macOS

```bash
# Install dependencies
brew install gd libmagic

# Build
cd src/legacy
make -f Makefile.legacy pisstvpp

# Test
./../../bin/pisstvpp -h
```

### For Linux (Ubuntu/Debian)

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install build-essential libgd-dev libmagic-dev

# Build (note: pifm requires RPi hardware)
cd src/legacy
make -f Makefile.legacy pisstvpp

# Test
./../../bin/pisstvpp -h
```

---

## Platform-Specific Instructions

### Raspberry Pi OS

#### Prerequisites

```bash
sudo apt-get update
sudo apt-get install build-essential gcc g++
sudo apt-get install libgd-dev libmagic-dev
```

#### Building

```bash
cd /path/to/PiSSTVpp2/src/legacy
make -f Makefile.legacy all
```

This will build both `pisstvpp` and `pifm`.

#### Verifying Install

```bash
# Check pisstvpp
../../bin/pisstvpp -h

# Check pifm (won't transmit without further setup)
sudo ../../bin/pifm --help
```

---

### Ubuntu / Debian (x86_64 or ARM64)

#### Prerequisites

```bash
sudo apt-get update
sudo apt-get install build-essential gcc
sudo apt-get install libgd-dev libmagic-dev
```

#### Building

```bash
cd /path/to/PiSSTVpp2/src/legacy
make -f Makefile.legacy pisstvpp
```

**Note:** `pifm` will not build on non-RPi systems due to direct hardware access.

#### Verifying Install

```bash
../../bin/pisstvpp -h
```

---

### macOS (Intel or Apple Silicon)

#### Prerequisites

**Using Homebrew:**

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install gd libmagic pkg-config
```

**Verify Installation:**

```bash
brew list gd libmagic
pkg-config --list-all | grep -E 'gd|magic'
```

#### Building

```bash
cd /path/to/PiSSTVpp2/src/legacy
make -f Makefile.legacy pisstvpp
```

**Note:** `pifm` is not applicable on macOS (requires RPi hardware).

#### Verifying Install

```bash
../../bin/pisstvpp -h
```

---

## Troubleshooting

### pisstvpp Build Errors

#### Error: "cannot find -lgd"

**Solution:** Install libgd

**macOS:**
```bash
brew install gd
# Verify:
pkg-config --cflags --libs gdlib
```

**Linux:**
```bash
sudo apt-get install libgd-dev
# Verify:
pkg-config --cflags --libs gdlib
```

---

#### Error: "cannot find -lmagic"

**Solution:** Install libmagic

**macOS:**
```bash
brew install libmagic
# Verify:
pkg-config --cflags --libs libmagic
```

**Linux:**
```bash
sudo apt-get install libmagic-dev
# Verify:
pkg-config --cflags --libs libmagic
```

---

#### Error: "magic.h: No such file or directory"

**macOS Solution:**
```bash
brew install libmagic
# If still not found, check:
brew --prefix libmagic
# Add to environment:
export CFLAGS="-I$(brew --prefix libmagic)/include"
export LDFLAGS="-L$(brew --prefix libmagic)/lib"
```

---

### pifm Build Errors

#### Error: "C++ compiler not found"

**Solution:** Install G++

**Raspberry Pi:**
```bash
sudo apt-get install g++ build-essential
```

---

#### Error: "cannot open source file 'stdio.h'"

**Solution:** Install build tools

**Raspberry Pi:**
```bash
sudo apt-get install build-essential
```

---

#### Error: "unresolved reference to 'malloc'"

This is a linking error. Ensure flags are correct:

```bash
make -f Makefile.legacy clean
make -f Makefile.legacy pifm
```

---

### Runtime Issues

#### pisstvpp: "Input image too small"

The image must meet minimum dimensions based on the protocol:

- **Martin 1/2, Scottie 1/2, Scottie DX:** 320×256 minimum
- **Robot 36:** 320×240 minimum  

**Solution:** Resize your image

```bash
# Using ImageMagick
convert input.jpg -resize 320x256 output.jpg

# Using ffmpeg
ffmpeg -i input.jpg -vf scale=320:256 output.jpg
```

---

#### pifm: "can't open /dev/mem" or "Permission denied"

pifm requires root access to the hardware.

**Solution:** Use sudo

```bash
sudo ../../bin/pifm sound.wav 105.5 22050
```

**Alternative:** Grant capabilities (advanced)

```bash
sudo setcap cap_sys_admin+ep ../../bin/pifm
```

This allows running without sudo, but is a security risk.

---

#### pifm: Segmentation fault on startup

Possible causes:
1. Running on non-RPi hardware
2. BCM2835 memory mapping failed
3. Corrupt WAV file

**Solutions:**

1. **Verify RPi hardware:**
   ```bash
   cat /proc/cpuinfo | grep Model
   # Should show Raspberry Pi
   ```

2. **Verify WAV file:**
   ```bash
   file sound.wav
   ffprobe sound.wav
   ```

3. **Check system:**
   ```bash
   dmesg | tail -20  # Check for kernel errors
   free -h            # Check memory availability
   ```

---

## Using the Built Binaries

### pisstvpp Usage

```bash
./bin/pisstvpp -p [protocol] -r [rate] <image_file>

Options:
  -p  SSTV protocol (default: m1)
      m1   - Martin 1
      m2   - Martin 2
      s1   - Scottie 1
      s2   - Scottie 2
      sdx  - Scottie DX
      r36  - Robot 36
      
  -r  Sample rate in Hz (default: 11025, max: 22050)
  -h  Show help
```

**Examples:**

```bash
# Create Martin 1 SSTV audio from image
./bin/pisstvpp -p m1 -r 11025 photo.jpg

# Creates: photo.jpg.wav

# Robot 36 at higher quality
./bin/pisstvpp -p r36 -r 22050 image.png

# Creates: image.png.wav
```

**Output Files:**

- `.wav` - WAV audio format (16-bit mono, configurable sample rate)
- `.aiff` - AIFF audio format (if compiled with AUDIO_AIFF)

---

### pifm Usage

**Warning:** pifm requires Raspberry Pi hardware and root access. It will transmit an RF signal.

```bash
sudo ./bin/pifm <wavfile> [freq] [rate] [bandwidth] [timing] [stereo]

Arguments:
  wavfile    - Input WAV file (16-bit, mono or stereo)
               Use '-' to read from stdin
               
  freq       - Transmission frequency in MHz (default: 144.5)
  rate       - Sample rate in Hz (default: 22050)
  bandwidth  - Bandwidth parameter (default: 1)
  timing     - Timing correction (default: 1414.0)
  stereo     - 'stereo' for stereo input (default: mono)
```

**Examples:**

```bash
# Transmit mono WAV on 105.5 MHz
sudo ./bin/pifm sound.wav 105.5

# Transmit stereo WAV on SSTV frequency
sudo ./bin/pifm stereo_music.wav 144.5 22050

# Pipe from another program
ffmpeg -i input.mp3 -f wav - | sudo ./bin/pifm - 105.5
```

**Safety:**

- **FM frequencies:** 88-108 MHz (broadcast), 430-440 MHz (amateur radio)
- **Legal use:** Check local regulations before transmitting
- **Signal strength:** Very weak (PLL noise), short range only
- **Interference:** May affect nearby wireless devices

---

## Build System Details

### Makefile.legacy Structure

The build system consists of:

```
src/legacy/
├── Makefile.legacy      # Standalone build for legacy components
├── pisstvpp.c           # Original pisstvpp (requires: libgd, libmagic)
├── pifm_sstv.c          # pifm transmitter (requires: C++, RPi hardware)
```

### Environment Variables

Override default compiler:

```bash
# Use clang instead of gcc
CC=clang make -f Makefile.legacy pisstvpp

# Use different C++ compiler
CXX=clang++ make -f Makefile.legacy pifm

# Add custom flags
CFLAGS="-O3 -march=native" make -f Makefile.legacy pisstvpp
```

---

## Maintenance Notes

### Code Quality

Legacy components have been updated with:

- ✓ Fixed buffer overflow vulnerabilities
- ✓ Fixed array bounds errors (off-by-one)
- ✓ Improved error handling
- ✓ Added missing header includes
- ✓ Fixed pointer arithmetic for 64-bit systems
- ✓ Added documentation

See [LEGACY_CODE_ANALYSIS.md](../LEGACY_CODE_ANALYSIS.md) for detailed information.

---

## See Also

- [LEGACY_CODE_ANALYSIS.md](../LEGACY_CODE_ANALYSIS.md) - Code issues and fixes
- [LEGACY_COMPONENTS.md](./LEGACY_COMPONENTS.md) - Component overview
- [USER_GUIDE.md](../USER_GUIDE.md) - Main PiSSTVpp2 documentation

