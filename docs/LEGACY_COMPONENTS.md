# Legacy Components Overview

**PiSSTVpp v1.0 and pifm_sstv - Original SSTV Transmission Tools**

---

## Quick Reference

| Component | Purpose | Status | Maintenance |
|-----------|---------|--------|-------------|
| **pisstvpp** | Image → SSTV audio encoding | Active | Stable |
| **pifm_sstv** | Audio → RPi FM transmission | Legacy | Limited |

---

## Components

### 1. PiSSTVpp (Original v1.0)

**What it does:**
Converts images (JPEG/PNG) to Slow-Scan Television (SSTV) audio format suitable for radio transmission.

**Input:** Image file (JPEG or PNG)  
**Output:** Audio file (WAV or AIFF)

**Supported SSTV Modes:**
- Martin 1 (M1)
- Martin 2 (M2)
- Scottie 1 (S1)
- Scottie 2 (S2)
- Scottie DX (SDX)
- Robot 36 (R36)

**Key Features:**
- Configurable sample rate (11025 - 22050 Hz)
- Automatic image size validation
- Multiple color modes (RGB for Martin/Scottie, YUV for Robot 36)
- Stable, well-tested implementation

**Dependencies:**
- **libgd** - image loading and processing
- **libmagic** - image file type detection

**Status:** Stable, actively maintained

### 2. pifm_sstv (Raspberry Pi FM Broadcaster)

**What it does:**
Transmits audio as FM signal using Raspberry Pi's GPIO and clock circuits. Originally designed to broadcast SSTV audio from pisstvpp.

**Input:** Audio file (16-bit WAV, mono or stereo)  
**Output:** FM signal at specified frequency

**Key Features:**
- Direct hardware modulation (no external hardware needed)
- Configurable frequency, sample rate, and timing
- Stereo support with RDS encoding capability
- Very low RF power output (milliwatts)

**Hardware Requirements:**
- **Raspberry Pi** (Models 1-4 confirmed working)
- BCM2835/BCM2836 SoC
- GPIO access (requires root)
- /proc/self/pagemap access

**Status:** Legacy, RPi-specific, minimal updates

---

## Architecture

### Legacy Code Separation

The legacy components are completely independent of PiSSTVpp2:

```
PiSSTVpp2 (v2.0)
├── Modern architecture with libvips
├── Multiple audio codecs (WAV/AIFF/OGG)
├── Cross-platform support
└── Active development

src/legacy/
├── pisstvpp.c        (v1.0 - uses libgd/libmagic)
└── pifm_sstv.c       (RPi hardware specific)
    └── No dependencies on PiSSTVpp2
```

**Rationale for Separation:**

1. **Different Dependencies:** PiSSTVpp2 uses libvips, legacy uses libgd
2. **Different Architectures:** PiSSTVpp2 is modular, legacy is monolithic
3. **Different Target Audiences:** Legacy tools used independently
4. **Maintenance Isolation:** Changes to one don't affect the other
5. **Historical Reference:** Shows original implementation approach

---

## Typical Workflows

### Workflow 1: Image to SSTV Audio (Any Platform)

```bash
# Create SSTV audio from image (Linux/macOS/RPi)
./bin/pisstvpp -p m1 -r 11025 photo.jpg

# Output: photo.jpg.wav
# Can then:
# - Play through audio interface on radio transceiver
# - Send to soundmodem, MMSSTV, or other SSTV software
```

### Workflow 2: Audio to FM Broadcast (RPi Only)

```bash
# On Raspberry Pi:
sudo ./bin/pifm audio.wav 145.5 22050

# Broadcasts on 145.5 MHz (check local regulations!)
```

### Workflow 3: Image to SSTV to FM (RPi Only)

```bash
# Complete pipeline (requires both tools on RPi)
./bin/pisstvpp -p r36 -r 22050 photo.jpg
sudo ./bin/pifm photo.jpg.wav 145.5 22050
```

---

## Improvements Made

### Code Quality Fixes

| Issue | Severity | Fixed |
|-------|----------|-------|
| Missing `<getopt.h>` | High | ✓ |
| Buffer overflow in path handling | High | ✓ |
| Off-by-one in audio loops | High | ✓ |
| Unsafe pointer arithmetic (32-bit assumptions) | High | ✓ |
| Missing error handling | Medium | ✓ |
| Unsafe signal handlers | Medium | ✓ |
| Memory leaks (pifm) | Low | Documented |
| Deprecated dependencies (libgd) | Low | Noted |

### Platform Support

- ✓ Raspberry Pi OS (ARM)
- ✓ Linux (x86_64, ARM64)
- ✓ macOS (Intel, Apple Silicon)
- ✓ 32-bit and 64-bit systems
- ✓ GCC and Clang compilers

---

## Comparison: Legacy vs. Modern

### PiSSTVpp (Legacy v1.0) vs. PiSSTVpp2 (v2.0)

| Aspect | Legacy (v1.0) | Modern (v2.0) |
|--------|---------------|---------------|
| **Image Library** | libgd (outdated) | libvips (modern) |
| **Audio Support** | WAV, AIFF only | WAV, AIFF, OGG |
| **Code Structure** | Monolithic | Modular |
| **Architecture** | Single file | Multi-file, separated concerns |
| **Protocols** | 6 SSTV modes | All SSTV modes |
| **Error Handling** | Minimal | Comprehensive |
| **CW ID** | No | Yes |
| **Platform Support** | Cross-platform | Cross-platform (better) |
| **Maintenance** | Minimal | Active |
| **Build System** | Simple | Comprehensive |
| **Performance** | Good | Optimized |

**Recommendation:** Use **PiSSTVpp2** for new projects. Legacy tools are retained for:
- Historical reference
- Backward compatibility
- Specific use cases where original tools preferred

---

## Known Limitations

### pisstvpp Limitations

1. **Image Format Support:** Limited to JPEG/PNG (libgd constraints)
2. **AIFF Sample Rate:** Hard-coded for 11.025 kHz in AIFF format
3. **Color Accuracy:** RGB to frequency conversion is simplified
4. **Aspect Ratio:** Limited correction options
5. **Outdated Dependencies:** libgd is unmaintained, libmagic has known issues

### pifm Limitations

1. **Raspberry Pi Only:** Direct hardware access to BCM2835
2. **Requires root:** Must run as sudo (security concern)
3. **Low Power:** mW output only (short range, interference risk)
4. **Signal Quality:** Pure square-wave modulation (high harmonics)
5. **Limited Documentation:** Original code minimally commented

---

## Building from Source

### Quick Build

```bash
cd src/legacy
make -f Makefile.legacy all
```

### Detailed Instructions

See [LEGACY_BUILD.md](../docs/LEGACY_BUILD.md) for:
- Platform-specific dependencies
- Troubleshooting guide
- Verification steps
- Environment variables

### Building Specific Components

```bash
# Build only pisstvpp (all platforms)
make -f Makefile.legacy pisstvpp

# Build only pifm (RPi only)
make -f Makefile.legacy pifm

# Clean
make -f Makefile.legacy clean
```

---

## Security Considerations

### pisstvpp

- **Standard security:** Validates image files, bounds checking on buffers
- **Vulnerability Status:** Fixed known issues (see LEGACY_CODE_ANALYSIS.md)
- **Best Practices:** Validate all input files

### pifm

- **Root access required:** Inherent security risk
- **Hardware access:** Direct /dev/mem access can crash system
- **Capabilities:** Consider CAP_SYS_ADMIN if available
- **Risk Level:** High - only use on controlled systems

---

## Maintenance Status

### pisstvpp
- **Status:** Stable
- **Updates:** Bug fixes only
- **Support:** Limited (legacy tool)
- **Bugs:** Known limitations documented

### pifm_sstv
- **Status:** Legacy
- **Updates:** Minimal
- **Support:** Community (not actively maintained)
- **Bugs:** See LEGACY_CODE_ANALYSIS.md

---

## See Also

- [LEGACY_CODE_ANALYSIS.md](./LEGACY_CODE_ANALYSIS.md) - Detailed code analysis
- [LEGACY_BUILD.md](./LEGACY_BUILD.md) - Build instructions
- [ARCHITECTURE.md](./ARCHITECTURE.md) - PiSSTVpp2 modern architecture
- [USER_GUIDE.md](./USER_GUIDE.md) - PiSSTVpp2 usage guide

---

## Contributing

To contribute fixes to legacy components:

1. **Report Issues:** Use the project issue tracker
2. **Code Changes:**
   - Submit PR against `src/legacy/`
   - Update LEGACY_CODE_ANALYSIS.md
   - Update LEGACY_BUILD.md if dependencies change
3. **Testing:** Verify builds on multiple platforms
4. **Documentation:** Update relevant .md files

---

## License

See LICENSE.md in repository root for project licensing.

The legacy components (pisstvpp, pifm) retain their original authorship:
- **pisstvpp:** Robert Marshall (KI4MCW), Gerrit Polder (PA3BYA), Don Gi Min (KM4EQR)
- **pifm:** Oliver Mattos, Oskar Weigl, adaptations by Gerrit Polder (PA3BYA)

---

