# SlowFrame v2.1

**Modern SSTV (Slow Scan Television) Encoder for Amateur Radio**

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![Tests](https://img.shields.io/badge/tests-167%2F169-brightgreen)]()
[![License](https://img.shields.io/badge/license-GPL--3.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-linux%20%7C%20raspberry%20pi%20%7C%20macOS-lightgrey)]()

Convert images to SSTV audio signals for amateur radio transmission. **51 SSTV modes** (8 native + 43 MMSSTV), text overlay, multiple audio formats, and CW identification.

---

## What's New in v2.1 🚀

✨ **51 SSTV Modes** (up from 7!)
- **8 native modes**: Martin 1/2, Scottie 1/2/DX, Robot 36/72, **Robot B&W 24** (NEW!)
- **43 MMSSTV modes**: PD, extended Martin/Scottie, Wraase SC2, MP, FAX, MR, ML, AVT families
- Runtime library detection with graceful fallback

📝 **Text Overlay System** (NEW!)
- FCC Part 97 compliant station identification
- Customizable text, fonts, colors, positioning
- Background bars for visibility
- Multiple overlays per image

🏗️ **Modern Architecture**
- Extensible mode registry system
- Comprehensive error handling
- Modular design with clean separation
- 6× mode expansion from v2.0

🧪 **Enhanced Testing**
- **167/169 tests passing (98.8%)**
- Comprehensive test coverage
- Backward compatibility validated
- No regressions

---

## Quick Start

### Installation

```bash
# Clone repository
git clone https://github.com/yourusername/SlowFrame.git
cd SlowFrame

# Install dependencies (Debian/Ubuntu)
sudo apt-get update
sudo apt-get install build-essential libvips-dev libogg-dev libvorbis-dev libcairo2-dev

# Build
make clean && make all

# Verify installation
./bin/slowframe -h

# List available modes
./bin/slowframe -L
```

### Basic Usage

```bash
# Encode image with default settings (Martin 1, WAV output)
./bin/slowframe -i photo.jpg -o transmission.wav

# Use PD90 mode (popular, fast, MMSSTV)
./bin/slowframe -i photo.jpg -p pd90 -o transmission.wav

# Robot B&W 24 - fast monochrome mode (NEW!)
./bin/slowframe -i photo.jpg -p bw24 -o transmission.wav

# Add callsign overlay for FCC compliance
./bin/slowframe -i photo.jpg -p m1 \
    -T "text:K9ABC|size:24|pos:LT|bg:yes" \
    -o transmission.wav

# Multiple overlays (callsign + grid square)
./bin/slowframe -i photo.jpg -p pd90 \
    -T "text:K9ABC|pos:LT|size:20" \
    -T "text:FN20xg|pos:RB|size:16" \
    -o transmission.wav

# With CW identification
./bin/slowframe -i photo.jpg -p m1 -C "K9ABC" -o transmission.wav

# High quality Robot 72 at 44.1kHz
./bin/slowframe -i photo.jpg -p r72 -r 44100 -o transmission.wav
```

---

## MMSSTV Integration

SlowFrame v2.1 supports **43 additional modes** via the MMSSTV library:

### Quick Setup

1. **Check current status**:
   ```bash
   ./bin/slowframe -L
   # Look for "MMSSTV library detected" or "not found"
   ```

2. **Install MMSSTV library** (if needed):
   ```bash
   # Download and install (example)
   sudo cp libsstv_encoder.1.0.0.dylib /usr/local/lib/
   ```

3. **Verify**:
   ```bash
   ./bin/slowframe -L
   # Should now show 51 total modes (8 native + 43 MMSSTV)
   ```

**See [MMSSTV Setup Guide](docs/MMSSTV_SETUP_GUIDE.md) for complete instructions**

### Available MMSSTV Modes

When library is installed, you get access to:
- **PD Family** (7): PD50, PD90, PD120, PD160, PD180, PD240, PD290
- **Martin** (2): M3, M4
- **Scottie** (2): S3, S4
- **Robot** (6): 8BW, 12BW, 24, 24BW, 36, 72
- **Wraase SC2** (4): SC2-30, SC2-60, SC2-120, SC2-180
- **And more**: MP, FAX, P, Pasokon, MR, ML, AVT families

**Total: 51 modes!**

---

## Documentation

� **[Complete Documentation Index](docs/DOCUMENTATION_INDEX.md)** - Find everything here!

### Getting Started

- **[MMSSTV Setup Guide](docs/MMSSTV_SETUP_GUIDE.md)** ⭐ NEW! - Enable 43 additional modes
- [Quick Start Guide](docs/QUICK_START.md) - Get started in 5 minutes
- [User Guide](docs/USER_GUIDE.md) - Complete usage reference
- [Build Guide](docs/BUILD.md) - Platform-specific installation

### For Users

- **[Mode Reference](docs/MODE_REFERENCE.md)** - All 51 SSTV modes explained
- **[MMSSTV Mode Reference](docs/MMSSTV_MODE_REFERENCE.md)** - MMSSTV mode specifications
- [Text Overlay Guide](docs/TEXT_OVERLAY_GUIDE.md) - FCC compliance and styling
- [CLI Reference](docs/CLI_REFERENCE.md) - Complete command-line options

### For Developers

- **[Developer Walkthrough](docs/DEVELOPER_WALKTHROUGH.md)** ⭐ NEW! - Code navigation guide
- **[Architecture](docs/ARCHITECTURE.md)** - Updated for v2.1
- [MMSSTV Integration](docs/MMSSTV_INTEGRATION.md) - Technical details
- [Contributing](CONTRIBUTING.md) - How to contribute

### Contributor Playbooks

- **[Adding SSTV Modes](docs/CONTRIB_ADDING_SSTV_MODES.md)** ⭐ NEW!
- **[Fixing Bugs](docs/CONTRIB_FIXING_BUGS.md)** ⭐ NEW!
- [Code Style Guide](docs/CODE_STYLE.md)
- [Testing Guide](docs/TESTING.md)

### Progress & Status

- [v2.1 Progress Status](docs/V2_1_PROGRESS_STATUS_FEB16_2026.md) - Current development status
- [Changelog](CHANGELOG.md) - Version history
- [Roadmap](docs/ROADMAP.md) - Future plans
- [Test Suite Guide](docs/TEST_SUITE_README.md) - Running tests
- [Testing Plan](docs/TESTING_PLAN.md) - Comprehensive test strategy
- [Test Cases](docs/TEST_CASES.md) - Detailed test specifications
- [Test Results](docs/TEST_RESULTS.md) - Latest test execution results
- [Test Execution Summary](docs/TEST_EXECUTION_SUMMARY.md) - Quick test overview

🚀 **Advanced Topics**
- [MMSSTV Integration](docs/MMSSTV_INTEGRATION.md) - Extended modes (v2.1)
- [MMSSTV Mode Reference](docs/MMSSTV_MODE_REFERENCE.md) - 43+ future modes
- [Contributing](CONTRIBUTING.md) - How to contribute

---

## Command Line Options

### Required

```
-i <file>       Input image file (PNG, JPEG, GIF, BMP, TIFF, WebP)
```

### Optional

```
-o <file>       Output audio file (default: input_file.wav)
-p <protocol>   SSTV protocol (default: m1)
                Native modes (8):
                  m1     - Martin 1 (VIS 44, 320×256, 114s)
                  m2     - Martin 2 (VIS 40, 320×256, 58s)
                  s1     - Scottie 1 (VIS 60, 320×256, 110s)
                  s2     - Scottie 2 (VIS 56, 320×256, 71s)
                  sdx    - Scottie DX (VIS 76, 320×256, 269s)
                  r36    - Robot 36 (VIS 8, 320×240, 36s)
                  r72    - Robot 72 (VIS 12, 320×240, 72s)
                  bw24   - Robot B&W 24 (VIS 9, 320×240, 24s) ⭐ NEW!
                
                MMSSTV modes (43, when library available):
                  pd90   - PD 90 (popular, fast)
                  pd120  - PD 120 (good quality)
                  fax480 - FAX 480 (high resolution)
                  ... and 40 more (see -L for full list)

-L              List all available modes
-f <fmt>        Output format: wav, aiff, ogg (default: wav)
-r <rate>       Sample rate in Hz (default: 22050, range: 8000-48000)
-a <mode>       Aspect ratio: center, pad, stretch (default: center)
-v              Enable verbose output
-h              Display help message
```

### Text Overlay Options ⭐ NEW!

```
-T <spec>       Add text overlay (can specify multiple times)
                Format: "key:value|key:value|..."
                
                Keys:
                  text     - Text to display (required)
                  size     - Font size 8-72 (default: 20)
                  color    - RGB color "R,G,B" (default: "255,255,255")
                  pos      - Position L/C/R + T/M/B (default: "CM")
                             LT=top-left, CM=center-middle, RB=bottom-right
                  font     - Font family (default: "Sans")
                  opacity  - Text opacity 0.0-1.0 (default: 1.0)
                  bg       - Background bar yes/no (default: no)
                  bgheight - Bar height in pixels (default: auto)
                  bgcolor  - Bar color "R,G,B" (default: "0,0,0")
                  bgopacity- Bar opacity 0.0-1.0 (default: 0.7)
                
                Examples:
                  -T "text:K9ABC"
                  -T "text:K9ABC|size:24|pos:LT|bg:yes"
                  -T "text:Field Day 2024|size:16|color:255,255,0|pos:CB"
```

### CW Signature Options

```
-C <callsign>   Add CW signature (max 31 characters)
--cw-wpm <wpm>  CW speed in WPM (default: 15, range: 1-50)
--cw-tone <hz>  CW tone frequency in Hz (default: 800, range: 400-2000)
```

### VIS Header Options

```
--no-vis        Suppress VIS header (for non-standard use)
```

---

## Examples

### Basic Encoding

```bash
# Martin 1 (most compatible)
./bin/slowframe -i photo.jpg -p m1 -o transmission.wav

# PD90 (popular MMSSTV mode - fast, good quality)
./bin/slowframe -i photo.jpg -p pd90 -o transmission.wav

# Robot B&W 24 (NEW! - fast monochrome)
./bin/slowframe -i photo.jpg -p bw24 -o transmission.wav

# Scottie 2 (good quality/speed balance)
./bin/slowframe -i photo.jpg -p s2 -o transmission.wav

# Robot 72 (highest quality, longest transmission)
./bin/slowframe -i photo.jpg -p r72 -o transmission.wav
```

### Text Overlays (FCC Part 97 Compliance)

```bash
# Simple callsign overlay
./bin/slowframe -i photo.jpg -p m1 \
    -T "text:K9ABC" \
    -o transmission.wav

# Callsign with background bar (more visible)
./bin/slowframe -i photo.jpg -p m1 \
    -T "text:K9ABC|size:24|pos:LT|bg:yes" \
    -o transmission.wav

# Multiple overlays (callsign + grid square)
./bin/slowframe -i photo.jpg -p pd90 \
    -T "text:K9ABC|pos:LT|size:20|bg:yes" \
    -T "text:FN20xg|pos:RB|size:16|color:0,255,0" \
    -o transmission.wav

# Event overlay
./bin/slowframe -i event_photo.jpg -p s1 \
    -T "text:Field Day 2024|size:20|pos:CT|bg:yes|bgcolor:0,0,128" \
    -T "text:W1AW/4|size:18|pos:LB" \
    -o transmission.wav
```

### Combined Features

```bash
# Text overlay + CW identification
./bin/slowframe -i photo.jpg -p m1 \
    -T "text:K9ABC|size:24|pos:LT|bg:yes" \
    -C "K9ABC" \
    -o transmission.wav

# High quality with all features
./bin/slowframe -i photo.jpg -p pd120 -r 44100 -f wav \
    -T "text:K9ABC|size:20|pos:LT|bg:yes" \
    -T "text:FN20xg|size:16|pos:RB" \
    -C "K9ABC" --cw-wpm 20 \
    -o field_day.wav
```

### Aspect Ratio Handling

```bash
# Center-crop to 4:3 aspect (default)
./bin/slowframe -i photo.jpg -a center -o output.wav

# Add black bars to preserve aspect
./bin/slowframe -i photo.jpg -a pad -o output.wav

# Stretch to fit (may distort)
./bin/slowframe -i photo.jpg -a stretch -o output.wav
```

### CW Identification

```bash
# Add callsign signature
./bin/slowframe -i photo.jpg -C "W1AW" -o transmission.wav

# Fast CW (25 WPM)
./bin/slowframe -i photo.jpg -C "N0CALL" -W 25 -o transmission.wav

# Custom CW tone (1000 Hz)
./bin/slowframe -i photo.jpg -C "K0ABC/P" -T 1000 -o transmission.wav

# Full custom CW signature
./bin/slowframe -i photo.jpg -C "VE3XYZ" -W 18 -T 850 -o transmission.wav
```

### High Quality Output

```bash
# OGG format with high sample rate
./bin/slowframe -i photo.jpg -p s1 -f ogg -r 44100 -o output.ogg

# Robot 72 at maximum quality
./bin/slowframe -i photo.jpg -p r72 -r 48000 -o output.wav

# Scottie DX (highest quality RGB mode)
./bin/slowframe -i photo.jpg -p sdx -r 44100 -o output.wav
```

---

## SSTV Mode Reference

| Mode | VIS | Resolution | Duration | Color Space | Use Case |
|------|-----|------------|----------|-------------|----------|
| Martin 1 | 44 | 320×256 | 114s | RGB | Most compatible |
| Martin 2 | 40 | 320×256 | 58s | RGB | Faster Martin |
| Scottie 1 | 60 | 320×256 | 110s | RGB | High quality |
| Scottie 2 | 56 | 320×256 | 71s | RGB | Good balance |
| Scottie DX | 76 | 320×256 | 269s | RGB | Best RGB quality |
| Robot 36 | 8 | 320×240 | 36s | YUV 4:2:0 | Fast transmission |
| Robot 72 | 12 | 320×240 | 72s | YUV 4:2:2 | High quality YUV |

**Recommendation:**
- **Quick transmission**: Robot 36 (36s)
- **Best compatibility**: Martin 1
- **Best quality/time**: Scottie 2
- **Highest quality**: Scottie DX or Robot 72

---

## Testing

```bash
# Run full test suite (55 tests)
cd tests
python test_suite.py --exe ../bin/slowframe

# Run quick tests
make test-quick

# View test results
cat tests/test_outputs/test_results_*.json
```

**Test Coverage (55 Tests, 100% Pass Rate):**
- ✅ All 7 SSTV protocols (m1, m2, s1, s2, sdx, r36, r72)
- ✅ All 3 audio formats (WAV, AIFF, OGG Vorbis)
- ✅ 6 sample rates (8000, 11025, 22050, 32000, 44100, 48000 Hz)
- ✅ 3 aspect ratio modes (center-crop, pad, stretch)
- ✅ CW signature generation with speed/tone control
- ✅ Comprehensive error handling and validation
- ✅ Multi-image processing and output file naming
- ✅ Callsign validation (including portable/maritime formats)

**Latest Test Results:** See [TEST_EXECUTION_SUMMARY.md](docs/TEST_EXECUTION_SUMMARY.md) for detailed results.

---

## System Requirements

### Minimum Requirements
- **OS**: Linux (any distribution), Raspberry Pi OS, macOS
- **CPU**: ARM (Raspberry Pi) or x86_64
- **RAM**: 64 MB
- **Storage**: 10 MB (installed)

### Recommended
- **Raspberry Pi 3B+** or newer
- **128 MB RAM** for high-resolution modes
- **SSD storage** for faster image loading

### Dependencies
- **libvips** (≥8.0) - Image processing
- **libogg, libvorbis** (optional) - OGG support
- **GCC** or **Clang** - Compilation

---

## Building from Source

### Prerequisites

**Debian/Ubuntu:**
```bash
sudo apt-get install build-essential libvips-dev libogg-dev libvorbis-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc make vips-devel libogg-devel libvorbis-devel
```

**macOS (Homebrew):**
```bash
brew install vips libogg libvorbis
```

**Raspberry Pi OS:**
```bash
sudo apt-get install build-essential libvips-dev libogg-dev libvorbis-dev
```

### Compilation

```bash
# Clean build
make clean && make all

# Build without OGG support
make clean && make all NO_OGG=1

# Install to /usr/local/bin
sudo make install

# Uninstall
sudo make uninstall
```

### Build Targets

```
make all          # Build main binary
make clean        # Remove build artifacts
make test         # Run full test suite
make test-quick   # Run quick tests
make install      # Install to system
make uninstall    # Remove from system
```

---

## Performance

**Encoding Speed (Raspberry Pi 4B, 1.5GHz):**
- Martin 1: ~2-3 seconds
- Scottie 2: ~2-3 seconds  
- Robot 72: ~3-4 seconds

**Memory Usage:**
- Base: ~20 MB
- Peak (with image): ~50 MB
- Audio buffer: ~57 MB (maximum, 600s @ 48kHz)

**CPU Usage:**
- Encoding: 5-15% (single core)
- Idle: <1%

---

## Troubleshooting

### Common Issues

**1. "Cannot find libvips"**
```bash
# Install libvips development package
sudo apt-get install libvips-dev
```

**2. "OGG format not supported"**
```bash
# Install OGG/Vorbis libraries
sudo apt-get install libogg-dev libvorbis-dev
# Rebuild
make clean && make all
```

**3. "Image file not found"**
```bash
# Verify file exists and path is correct
ls -la your-image.jpg
# Use absolute path
./bin/slowframe -i /full/path/to/image.jpg -o output.wav
```

**4. "Audio buffer overflow"**
```bash
# Use lower sample rate
./bin/slowframe -i photo.jpg -r 22050 -o output.wav
# Or shorter transmission mode
./bin/slowframe -i photo.jpg -p r36 -o output.wav
```

See [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) for more solutions.

---

## Version History

### v2.0.0 (January 2026) - Current
**Developer:** Andrew Blessing (KW9D)

- ✅ Complete modular architecture refactor (7 source files)
- ✅ Migrated from libgd to libvips for better image handling
- ✅ Added Robot 72 mode (7 modes total vs 6 in v1)
- ✅ **Fixed Robot 36/72 YUV encoding** - Proper 4:2:0/4:2:2 chroma subsampling with per-line encoding
- ✅ Added aspect ratio correction (center/pad/stretch modes)
- ✅ Added CW identification system with speed/tone control
- ✅ Added OGG Vorbis audio format support
- ✅ Configurable sample rates (8000-48000 Hz)
- ✅ Increased audio buffer capacity (600s @ 48kHz)
- ✅ Fixed integer overflow in tone generation
- ✅ Comprehensive test suite (55 tests, 100% pass rate)
- ✅ Improved error handling and graceful validation
- ✅ Complete documentation suite (21 markdown files)
- ✅ CLI comparison documentation (v1 vs v2.0)

### v1.x (Legacy)
- Original implementation with 6 modes
- libgd-based image processing
- WAV output only

---

## Roadmap

### v2.1 (Q1 2026) - MMSSTV Integration
- [ ] Integration with mmsstv-portable library
- [ ] Support for 40+ additional SSTV modes
- [ ] PD modes (PD120, PD180, PD240, etc.)
- [ ] Extended mode reference documentation

### v2.2 (Q2 2026) - Enhanced Features
- [ ] Real-time transmission monitoring
- [ ] Audio preview functionality
- [ ] Batch processing support
- [ ] GUI frontend (optional)

### Future
- [ ] Hardware-accelerated encoding (GPU)
- [ ] Network streaming support
- [ ] Plugin architecture for custom modes

---

## Project Structure

```
SlowFrame/
├── README.md              # Project overview
├── CONTRIBUTING.md        # Contribution guidelines
├── makefile              # Build configuration
│
├── bin/                  # Built binary (generated)
│   └── slowframe
│
├── src/                  # Source code
│   ├── include/          # Header files
│   │   ├── logging.h
│   │   ├── mmsstv_stub.h
│   │   ├── slowframe_audio_encoder.h
│   │   ├── slowframe_image.h
│   │   ├── slowframe_mmsstv_adapter.h
│   │   └── slowframe_sstv.h
│   │
│   ├── slowframe.c                    # Main program
│   ├── slowframe_image.c              # Image processing
│   ├── slowframe_sstv.c               # SSTV encoding
│   ├── slowframe_audio_encoder.c      # Audio encoder
│   ├── slowframe_mmsstv_adapter.c     # MMSSTV adapter (v2.1)
│   │
│   ├── audio_encoder_wav.c            # WAV encoder
│   ├── audio_encoder_aiff.c           # AIFF encoder
│   ├── audio_encoder_ogg.c            # OGG encoder
│   │
│   ├── pifm_sstv.c                    # Legacy PiFM code
│   └── pisstvpp.c                     # Legacy code
│
├── docs/                 # Documentation (21 markdown files)
│   ├── DOCUMENTATION_INDEX.md         # Complete doc navigation
│   │
│   ├── QUICK_START.md                 # 5-minute intro
│   ├── USER_GUIDE.md                  # Complete usage guide
│   ├── BUILD.md                       # Build instructions
│   ├── ARCHITECTURE.md                # System design
│   ├── MODE_REFERENCE.md              # v2.0 SSTV modes
│   ├── CLI_COMPARISON.md              # v1 vs v2.0 comparison
│   │
│   ├── TESTING_PLAN.md                # Test strategy
│   ├── TEST_CASES.md                  # Test specifications
│   ├── TEST_RESULTS.md                # Detailed results
│   ├── TEST_EXECUTION_SUMMARY.md      # Quick overview
│   ├── TEST_SUITE_README.md           # Testing guide
│   ├── TEST_QUICK_START.md            # Quick tests
│   ├── TEST_PRACTICAL_GUIDE.md        # Test writing
│   │
│   ├── MMSSTV_INTEGRATION.md          # v2.1 plan
│   ├── MMSSTV_MODE_REFERENCE.md       # 43+ future modes
│   ├── SLOWFRAME_v2_0_MASTER_PLAN.md  # Complete roadmap
│   ├── DOCUMENTATION_UPDATE_SUMMARY.md # Doc changes
│   │
│   ├── LICENSE.md                      # GPL-3.0 license
│   └── *.pdf                           # SSTV specifications
│
├── tests/                # Test suite
│   ├── test_suite.sh                  # Bash runner
│   ├── test_suite.py                  # Python runner
│   ├── images/                        # Test images
│   ├── test_outputs/                  # Generated files
│   └── test_results/                  # Test reports
│
└── util/                 # Utilities
    ├── sstvcam.sh                     # Webcam capture
    ├── test_aspect_modes.sh           # Aspect tests
    ├── test_dimension_verification.sh # Dimension tests
    └── test_visual_comparison.sh      # Visual tests
```

---

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

**How to Contribute:**
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Run tests (`make test`)
5. Commit changes (`git commit -m 'Add amazing feature'`)
6. Push to branch (`git push origin feature/amazing-feature`)
7. Open a Pull Request

**Areas needing help:**
- Additional SSTV mode implementations
- Performance optimization
- Documentation improvements
- Platform testing (various Linux distributions)
- Bug reports and fixes

---

## License

This project is licensed under the **GNU General Public License v3.0** - see [LICENSE](LICENSE) for details.

### Credits

**SlowFrame v2.0 (2026)**
- **KW9D** (Andrew Blessing) - Complete v2.0 refactoring, architecture redesign, comprehensive testing suite, and documentation

**Original PiSSTVpp (2013-2014)**
- **KI4MCW** (Robert Marshall) - Original SSTV implementation
- **PA3BYA** (Gerrit Polder, AgriVision) - VIS header fixes and enhancements
- **KM4EQR** (Don Gi Min) - Additional protocols and option handling

**Third-Party Components:**
- **libvips** - Image processing library
- **libogg/libvorbis** - OGG audio encoding
- **MMSSTV** (Makoto Mori, JE3HHT) - Mode specifications (future integration)

---

## Support

- **Issues**: [GitHub Issues](https://github.com/yourusername/SlowFrame/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/SlowFrame/discussions)
- **Documentation**: [docs/](docs/)

---

## Acknowledgments

This project stands on the shoulders of giants in the amateur radio community. Special thanks to:
- The SSTV amateur radio community
- MMSSTV author Makoto Mori (JE3HHT)
- All contributors and testers

---

**73 de KW9D** 📡✨

*Happy SSTV transmissions!*
