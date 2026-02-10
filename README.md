# PiSSTVpp2 v2.0

**Modern SSTV (Slow Scan Television) Encoder for Raspberry Pi and Linux**

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![Tests](https://img.shields.io/badge/tests-55%2F55-brightgreen)]()
[![License](https://img.shields.io/badge/license-GPL--3.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-linux%20%7C%20raspberry%20pi%20%7C%20macOS-lightgrey)]()

Convert images to SSTV audio signals for amateur radio transmission. Supports 7 SSTV modes, multiple audio formats, and CW identification.

---

## Features

✨ **7 SSTV Modes**
- Martin 1, Martin 2
- Scottie 1, Scottie 2, Scottie DX
- Robot 36 Color, Robot 72 Color

🎵 **Multiple Audio Formats**
- WAV (16-bit PCM)
- AIFF (Apple format)
- OGG Vorbis (compressed)

🖼️ **Smart Image Handling**
- Auto-scaling and aspect ratio correction
- Support for PNG, JPEG, GIF, BMP
- Three aspect modes: center, pad, stretch

📡 **CW Identification**
- Morse code signature with callsign
- Configurable speed (1-50 WPM)
- Custom tone frequency (400-2000 Hz)

⚡ **Performance**
- Sample rates: 8000-48000 Hz
- Optimized for Raspberry Pi
- Low memory footprint

🧪 **Robust Testing**
- 55 comprehensive tests
- 100% test pass rate
- Validated output formats

---

## Quick Start

### Installation

```bash
# Clone repository
git clone https://github.com/yourusername/PiSSTVpp2.git
cd PiSSTVpp2

# Install dependencies (Debian/Ubuntu)
sudo apt-get update
sudo apt-get install build-essential libvips-dev libogg-dev libvorbis-dev

# Build
make clean && make all

# Verify installation
./bin/pisstvpp2 -h
```

### Basic Usage

```bash
# Encode image with default settings (Martin 1, WAV output)
./bin/pisstvpp2 -i photo.jpg -o transmission.wav

# Use Scottie 2 mode with OGG output
./bin/pisstvpp2 -i photo.jpg -p s2 -f ogg -o transmission.ogg

# Add CW signature with callsign
./bin/pisstvpp2 -i photo.jpg -p m1 -C "N0CALL" -o transmission.wav

# High quality Robot 72 at 44.1kHz
./bin/pisstvpp2 -i photo.jpg -p r72 -r 44100 -o transmission.wav
```

---

## Documentation

� **[Complete Documentation Index](docs/DOCUMENTATION_INDEX.md)** - Find everything here!

📚 **Getting Started**
- [Quick Start Guide](docs/QUICK_START.md) - Get started in 5 minutes
- [User Guide](docs/USER_GUIDE.md) - Complete usage reference
- [Build Guide](docs/BUILD.md) - Platform-specific installation

🔧 **Technical Documentation**
- [Architecture](docs/ARCHITECTURE.md) - Code structure and design
- [Mode Reference](docs/MODE_REFERENCE.md) - SSTV mode specifications
- [CLI Comparison](docs/CLI_COMPARISON.md) - v1 vs v2.0 feature comparison

🧪 **Testing Documentation**
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
-i <file>       Input image file (PNG, JPEG, GIF, or BMP)
```

### Optional

```
-o <file>       Output audio file (default: input_file.wav)
-p <protocol>   SSTV protocol (default: m1)
                  m1     - Martin 1 (VIS 44)
                  m2     - Martin 2 (VIS 40)
                  s1     - Scottie 1 (VIS 60)
                  s2     - Scottie 2 (VIS 56)
                  sdx    - Scottie DX (VIS 76)
                  r36    - Robot 36 Color (VIS 8)
                  r72    - Robot 72 Color (VIS 12)

-f <fmt>        Output format: wav, aiff, ogg (default: wav)
-r <rate>       Sample rate in Hz (default: 22050, range: 8000-48000)
-a <mode>       Aspect ratio: center, pad, stretch (default: center)
-v              Enable verbose output
-h              Display help message
```

### CW Signature Options

```
-C <callsign>   Add CW signature (max 31 characters)
-W <wpm>        CW speed in WPM (default: 15, range: 1-50)
-T <freq>       CW tone frequency in Hz (default: 800, range: 400-2000)
```

---

## Examples

### Basic Encoding

```bash
# Martin 1 (most compatible)
./bin/pisstvpp2 -i photo.jpg -p m1 -o transmission.wav

# Scottie 2 (good quality/speed balance)
./bin/pisstvpp2 -i photo.jpg -p s2 -o transmission.wav

# Robot 72 (highest quality, longest transmission)
./bin/pisstvpp2 -i photo.jpg -p r72 -o transmission.wav
```

### Aspect Ratio Handling

```bash
# Center-crop to 4:3 aspect (default)
./bin/pisstvpp2 -i photo.jpg -a center -o output.wav

# Add black bars to preserve aspect
./bin/pisstvpp2 -i photo.jpg -a pad -o output.wav

# Stretch to fit (may distort)
./bin/pisstvpp2 -i photo.jpg -a stretch -o output.wav
```

### CW Identification

```bash
# Add callsign signature
./bin/pisstvpp2 -i photo.jpg -C "W1AW" -o transmission.wav

# Fast CW (25 WPM)
./bin/pisstvpp2 -i photo.jpg -C "N0CALL" -W 25 -o transmission.wav

# Custom CW tone (1000 Hz)
./bin/pisstvpp2 -i photo.jpg -C "K0ABC/P" -T 1000 -o transmission.wav

# Full custom CW signature
./bin/pisstvpp2 -i photo.jpg -C "VE3XYZ" -W 18 -T 850 -o transmission.wav
```

### High Quality Output

```bash
# OGG format with high sample rate
./bin/pisstvpp2 -i photo.jpg -p s1 -f ogg -r 44100 -o output.ogg

# Robot 72 at maximum quality
./bin/pisstvpp2 -i photo.jpg -p r72 -r 48000 -o output.wav

# Scottie DX (highest quality RGB mode)
./bin/pisstvpp2 -i photo.jpg -p sdx -r 44100 -o output.wav
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
python test_suite.py --exe ../bin/pisstvpp2

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
./bin/pisstvpp2 -i /full/path/to/image.jpg -o output.wav
```

**4. "Audio buffer overflow"**
```bash
# Use lower sample rate
./bin/pisstvpp2 -i photo.jpg -r 22050 -o output.wav
# Or shorter transmission mode
./bin/pisstvpp2 -i photo.jpg -p r36 -o output.wav
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
PiSSTVpp2/
├── README.md              # Project overview
├── CONTRIBUTING.md        # Contribution guidelines
├── makefile              # Build configuration
│
├── bin/                  # Built binary (generated)
│   └── pisstvpp2
│
├── src/                  # Source code
│   ├── include/          # Header files
│   │   ├── logging.h
│   │   ├── mmsstv_stub.h
│   │   ├── pisstvpp2_audio_encoder.h
│   │   ├── pisstvpp2_image.h
│   │   ├── pisstvpp2_mmsstv_adapter.h
│   │   └── pisstvpp2_sstv.h
│   │
│   ├── pisstvpp2.c                    # Main program
│   ├── pisstvpp2_image.c              # Image processing
│   ├── pisstvpp2_sstv.c               # SSTV encoding
│   ├── pisstvpp2_audio_encoder.c      # Audio encoder
│   ├── pisstvpp2_mmsstv_adapter.c     # MMSSTV adapter (v2.1)
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
│   ├── PISSTVPP2_v2_0_MASTER_PLAN.md  # Complete roadmap
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

**PiSSTVpp2 v2.0 (2026)**
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

- **Issues**: [GitHub Issues](https://github.com/yourusername/PiSSTVpp2/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/PiSSTVpp2/discussions)
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
