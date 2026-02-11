# Legacy Components README

**PiSSTVpp v1.0 and pifm_sstv - Original SSTV Transmission Tools**

---

## Quick Overview

This directory contains the original SSTV transmission tools from before the PiSSTVpp2 modernization:

- **pisstvpp** - Convert images (JPEG/PNG) to SSTV audio format
- **pifm_sstv** - Transmit audio via FM on Raspberry Pi

Both tools are fully functional and cross-platform compatible.

---

## Building

### Simple Build (All Platforms)

```bash
cd /path/to/PiSSTVpp2/src/legacy

# Build pisstvpp (all platforms)
make -f Makefile.legacy pisstvpp

# Build pifm (RPi compatible only, works on other platforms with caveats)
make -f Makefile.legacy pifm

# Build both
make -f Makefile.legacy all

# Verify builds
./build_legacy.sh
```

### Dependencies

**macOS:**
```bash
brew install gd libmagic
```

**Linux (Debian/Ubuntu/Raspberry Pi OS):**
```bash
sudo apt-get install build-essential libgd-dev libmagic-dev g++
```

---

## Usage

### pisstvpp: Image to SSTV Audio

```bash
# Basic usage
./../../bin/pisstvpp -p m1 image.jpg

# With options
./../../bin/pisstvpp -p r36 -r 22050 photo.png

# Help
./../../bin/pisstvpp -h
```

**Output:** Audio file (image.jpg.wav or image.png.wav)

### pifm: RPi FM Transmission

```bash
# Basic usage (requires RPi hardware and root)
sudo ./../../bin/pifm audio.wav 105.5

# With options
sudo ./../../bin/pifm sound.wav 145.5 22050 1 1414.0 stereo

# Help
./../../bin/pifm -h
```

**Requirements:** 
- Raspberry Pi (RPi 1-4)
- Root privileges
- Audio file (16-bit WAV)

---

## Documentation

Comprehensive guides available in `docs/`:

| Document | Purpose |
|----------|---------|
| [LEGACY_BUILD.md](../docs/LEGACY_BUILD.md) | Detailed build instructions |
| [LEGACY_COMPONENTS.md](../docs/LEGACY_COMPONENTS.md) | Component overview & workflows |
| [LEGACY_CODE_ANALYSIS.md](../docs/LEGACY_CODE_ANALYSIS.md) | Technical analysis & fixes |

---

## Build System

### Makefile.legacy

Standalone build system with:
- Automatic dependency checking
- Platform detection
- Clear error messages
- Configurable compile options

**Targets:**
- `make -f Makefile.legacy pisstvpp` - Build image-to-audio tool
- `make -f Makefile.legacy pifm` - Build RF transmitter
- `make -f Makefile.legacy all` - Build all available
- `make -f Makefile.legacy clean` - Clean artifacts
- `make -f Makefile.legacy help` - Show help

### build_legacy.sh

Shell script for build verification:
- Checks dependencies
- Tests each component
- Provides detailed output
- Green/red status indicators

```bash
./build_legacy.sh          # Full verification
./build_legacy.sh -c pisstvpp  # Test one component
./build_legacy.sh -v       # Verbose output
./build_legacy.sh -h       # Help
```

---

## Platform Support

| Platform | pisstvpp | pifm | Notes |
|----------|---------|------|-------|
| **Raspberry Pi OS** | ✓ | ✓ | Full support |
| **Linux (x86_64)** | ✓ | ✗ | pifm requires RPi hardware |
| **Linux (ARM64)** | ✓ | ✗ | pifm requires RPi hardware |
| **macOS (Intel)** | ✓ | ✓ | pifm compiles but won't run |
| **macOS (Apple Silicon)** | ✓ | ✓ | pifm compiles but won't run |

---

## Code Quality

Recent improvements include:
- ✓ Fixed buffer overflow vulnerabilities
- ✓ Fixed array bounds errors
- ✓ Added missing header includes
- ✓ Made 64-bit safe
- ✓ Improved error handling
- ✓ Platform-specific fixes (malloc.h, etc.)

See [LEGACY_CODE_ANALYSIS.md](../docs/LEGACY_CODE_ANALYSIS.md) for details.

---

## File Structure

```
src/legacy/
├── README.md              ← You are here
├── pisstvpp.c             ← Image to SSTV audio
├── pifm_sstv.c            ← RPi FM transmitter
├── Makefile.legacy        ← Build system
└── build_legacy.sh        ← Build verification script
```

---

## Examples

### Convert Image to SSTV Audio

```bash
# Create Martin 1 SSTV from JPEG
./../../bin/pisstvpp -p m1 photo.jpg
# Output: photo.jpg.wav

# Create Robot 36 at higher quality
./../../bin/pisstvpp -p r36 -r 22050 image.png
# Output: image.png.wav

# Try all protocols
for proto in m1 m2 s1 s2 sdx r36; do
  ./../../bin/pisstvpp -p $proto -r 11025 image.jpg
done
```

### Transmit on Raspberry Pi

```bash
# Transmit on 105.5 FM (check local regulations!)
sudo ./../../bin/pifm sound.wav 105.5

# Transmit stereo
sudo ./../../bin/pifm stereo.wav 145.5 22050 1 1414.0 stereo

# Stream from another program
ffmpeg -i input.mp3 -f wav - | sudo ./../../bin/pifm - 105.5
```

---

## Troubleshooting

### Build Issues

**"libgd not found"**
```bash
# macOS
brew install gd

# Linux
sudo apt-get install libgd-dev
```

**"libmagic not found"**
```bash
# macOS
brew install libmagic

# Linux
sudo apt-get install libmagic-dev
```

**"g++ not found"** (for pifm)
```bash
# macOS
brew install gcc

# Linux
sudo apt-get install g++
```

### Runtime Issues

**pisstvpp: "Input image too small"**
- Images must be at least 320×256 (or 320×240 for Robot 36)
- Use `ffmpeg` or ImageMagick to resize

**pifm: "can't open /dev/mem"**
- Use `sudo`: `sudo ./in/pifm sound.wav 105.5`
- Running on non-Raspberry Pi?

**pifm: Segmentation fault**
- Check you're on Raspberry Pi: `cat /proc/cpuinfo | grep Model`
- Verify WAV file: `file sound.wav`

See [LEGACY_BUILD.md](../docs/LEGACY_BUILD.md) for comprehensive troubleshooting.

---

## See Also

- [PiSSTVpp2 Documentation](../docs/README.md) - Modern version
- [Main README](../../README.md) - Project overview
- [User Guide](../docs/USER_GUIDE.md) - PiSSTVpp2 usage

---

## License

See LICENSE.md in repository root.

Original authors:
- **pisstvpp:** Robert Marshall (KI4MCW), Gerrit Polder (PA3BYA), Don Gi Min (KM4EQR)
- **pifm:** Oliver Mattos, Oskar Weigl, adaptations by Gerrit Polder (PA3BYA)

---

## Getting Help

1. **Build Help:** Run `./build_legacy.sh`
2. **Usage Help:** Run `./../../bin/pisstvpp -h`
3. **Documentation:** See [LEGACY_BUILD.md](../docs/LEGACY_BUILD.md)
4. **Report Issues:** Check existing issues or create new one

---

**Last Updated:** February 10, 2026  
**Status:** Fully Functional & Documented

