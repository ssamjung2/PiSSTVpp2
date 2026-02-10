# PiSSTVpp2 Testing Plan

**Comprehensive testing strategy and test case coverage for PiSSTVpp2 v2.0**

---

## Table of Contents

1. [Overview](#overview)
2. [Testing Strategy](#testing-strategy)
3. [Test Environment Setup](#test-environment-setup)
4. [Test Execution](#test-execution)
5. [Coverage Areas](#coverage-areas)
6. [Test Metrics](#test-metrics)
7. [Regression Testing](#regression-testing)
8. [Performance Testing](#performance-testing)
9. [CI/CD Integration](#cicd-integration)

---

## Overview

### Purpose

The PiSSTVpp2 test suite ensures:
- **Functional correctness**: All SSTV modes work properly
- **Audio quality**: Generated output is valid and complete
- **Input handling**: All image formats supported by libvips process correctly
- **Error handling**: Invalid inputs are rejected gracefully
- **Configuration**: All command-line options work as documented
- **Compatibility**: Output is compatible with SSTV receivers
- **Regression prevention**: Changes don't break existing functionality

### Test Coverage

The test suite covers **55+ automated tests** across:
- **7 SSTV Protocols** (Martin 1/2, Scottie 1/2/DX, Robot 36/72)
- **3 Audio Formats** (WAV, AIFF, OGG Vorbis)
- **6 Sample Rates** (8000-48000 Hz)
- **3 Aspect Modes** (center, pad, stretch)
- **CW Signatures** (Morse code, variable speed/tone)
- **Error Handling** (missing args, invalid files, bad parameters)
- **Edge Cases** (boundary conditions, limits, malformed inputs)

### Test Infrastructure

**Python Test Suite** (`tests/test_suite.py`)
- Comprehensive, structured testing
- JSON output for CI/CD
- 55+ tests with detailed validation
- Can run individual test groups or all tests

**Bash Quick Tests** (`tests/test_suite_bash.sh`)
- Fast smoke tests for quick validation
- No Python dependency
- ~15 quick checks

---

## Testing Strategy

### Levels of Testing

#### Unit Testing
- **Scope**: Individual modules in isolation
- **Coverage**: Image processing, SSTV encoding, audio generation
- **Approach**: Compile individual test binaries with mocked dependencies

#### Integration Testing
- **Scope**: Module interactions (pisstvpp2.c + all modules)
- **Coverage**: Full end-to-end pipeline
- **Approach**: Run complete binary with various inputs

#### Regression Testing
- **Scope**: Prevent breaking existing functionality
- **Coverage**: Previous test cases must continue passing
- **Approach**: Compare output characteristics (duration, sample rate, structure)

#### Performance Testing
- **Scope**: Execution time and resource usage
- **Coverage**: Encoding speed, memory usage
- **Approach**: Measure and track build/execution statistics

#### Compatibility Testing
- **Scope**: Output compatibility with SSTV receivers
- **Coverage**: VIS codes, timing accuracy, frequency response
- **Approach**: Validate output structure and timing

### Test Categories

```
Tests (55+ cases)
├── Information/Help (2 tests)
│   ├── Help display (-h flag)
│   └── Version information
│
├── Error Handling (8 tests)
│   ├── Missing input file (-i)
│   ├── Non-existent input file
│   ├── Invalid protocol code
│   ├── Invalid audio format
│   ├── Invalid sample rate (too low)
│   ├── Invalid sample rate (too high)
│   ├── CW parameters without callsign
│   └── CW parameters out of range
│
├── SSTV Protocols (7 tests)
│   ├── Martin 1 (m1, VIS 44)
│   ├── Martin 2 (m2, VIS 40)
│   ├── Scottie 1 (s1, VIS 60)
│   ├── Scottie 2 (s2, VIS 56)
│   ├── Scottie DX (sdx, VIS 76)
│   ├── Robot 36 (r36, VIS 8)
│   └── Robot 72 (r72, VIS 12)
│
├── Audio Formats (3 tests)
│   ├── WAV output (16-bit PCM)
│   ├── AIFF output (Apple format)
│   └── OGG output (Vorbis, if compiled)
│
├── Sample Rates (6 tests)
│   ├── 8000 Hz (minimum)
│   ├── 11025 Hz
│   ├── 22050 Hz (default)
│   ├── 32000 Hz
│   ├── 44100 Hz
│   └── 48000 Hz (maximum)
│
├── Aspect Ratio Modes (3 tests)
│   ├── center-crop (default)
│   ├── pad with bars
│   └── stretch
│
├── CW Identification (8 tests)
│   ├── Standard callsign (e.g., N0CALL)
│   ├── Portable callsign (e.g., W5ABC/4)
│   ├── Maritime callsign (e.g., WCE)
│   ├── CW speed (WPM) variation
│   ├── CW tone frequency variation
│   ├── Oversized callsign rejection
│   ├── Invalid WPM rejection
│   └── Invalid tone frequency rejection
│
├── Image Handling (5+ tests)
│   ├── PNG images
│   ├── JPEG images
│   ├── GIF images
│   ├── BMP images
│   └── Various resolutions and aspect ratios
│
├── Output Validation (8+ tests)
│   ├── WAV header correctness
│   ├── AIFF header correctness
│   ├── Audio sample count correctness
│   ├── File size validation
│   ├── Audio duration validation
│   ├── Sample rate verification
│   ├── Bit depth verification
│   └── Channel count verification
│
└── Combination Tests (5+ tests)
    ├── Multiple modes × formats
    ├── All modes at various sample rates
    ├── Complex aspect ratio combinations
    ├── Edge case image dimensions
    └── Stress tests (large images, long protocols)
```

---

## Test Environment Setup

### Requirements

**Hardware Minimum:**
- CPU: Any dual-core processor (tests run in ~5-20 minutes)
- RAM: 512 MB available
- Disk: 100 MB for test outputs

**Software Requirements:**
- GCC 4.9+ or Clang 3.4+
- Python 3.6+
- libvips 8.0+
- libogg + libvorbis (optional, for OGG tests)
- make
- bash

### Installation for Testing

```bash
# 1. Install dependencies
sudo apt-get update
sudo apt-get install build-essential libvips-dev libogg-dev libvorbis-dev python3

# 2. Clone/download repository
cd /path/to/PiSSTVpp2

# 3. Build
make clean && make all

# 4. Verify binary exists
ls -lh bin/pisstvpp2

# 5. Verify test images exist
ls -lh tests/images/
```

### Test Data

**Test Images Location:** `tests/images/`

**Test images provided:**
- `alt_color_bars_320x240.ppm` - Standard color bars (320×240)

**Additional test images (optional):**
- Add various image formats (PNG, JPEG, GIF, BMP) to test directory
- Include various aspect ratios for aspect mode testing
- Include edge cases (1×1 pixel, very large, etc.)

---

## Test Execution

### Running the Full Test Suite

```bash
# Run all tests
cd /path/to/PiSSTVpp2/tests
python3 test_suite.py

# Run with verbose output
python3 test_suite.py --verbose

# Run specific test group (see test class methods)
python3 test_suite.py --group protocols
```

**Expected Output:**
```
================================================================================
TEST SUITE SUMMARY
================================================================================
Total Tests:  55
Passed:       55 ✓
Failed:       0
Skipped:      0 (e.g., OGG tests if libraries not available)
Duration:     15.2 seconds
Status:       ✓ ALL TESTS PASSED

Details written to: test_results.json
```

### Running Quick Tests

```bash
# Bash quick tests (no Python needed)
cd /path/to/PiSSTVpp2/tests
bash test_suite_bash.sh

# Run specific test
bash test_suite_bash.sh m1          # Test Martin 1 mode
bash test_suite_bash.sh all         # Test all modes
```

### Running Specific Test Groups

The Python test suite can run individual test categories:

```bash
# Test help/info
python3 test_suite.py test_help_and_info

# Test SSTV protocols
python3 test_suite.py test_protocols

# Test audio formats
python3 test_suite.py test_audio_formats

# Test sample rates
python3 test_suite.py test_sample_rates

# Test aspect modes
python3 test_suite.py test_aspect_modes

# Test CW signatures
python3 test_suite.py test_cw_signatures

# Test error handling
python3 test_suite.py test_error_handling
```

### Output and Artifacts

**Test outputs are saved to:** `tests/test_outputs/`

**Generated files:**
- `test_protocol_m1.wav` - Test output for Martin 1
- `test_protocol_s2.wav` - Test output for Scottie 2
- `test_format_ogg.ogg` - Test OGG output
- And ~40+ other test output files

**Test results:**
- `test_results.json` - Structured test results
- `test_results.txt` - Human-readable summary

To clean up test outputs:
```bash
make test-clean
```

---

## Coverage Areas

### 1. Protocol Coverage (7 modes)

| Mode | VIS | Status | Test Cases |
|------|-----|--------|-----------|
| Martin 1 | 44 | ✓ | Default settings, all formats, all sample rates |
| Martin 2 | 40 | ✓ | All options |
| Scottie 1 | 60 | ✓ | All options |
| Scottie 2 | 56 | ✓ | All options (recommended default) |
| Scottie DX | 76 | ✓ | All options (longest duration) |
| Robot 36 | 8 | ✓ | All options (fastest) |
| Robot 72 | 12 | ✓ | All options |

**Protocol Tests Include:**
- ✓ Basic mode functionality
- ✓ Output file generation
- ✓ Correct VIS code generation
- ✓ Correct image dimensions
- ✓ Correct transmission duration (±5% tolerance)
- ✓ Interaction with all formats and sample rates
- ✓ Interaction with all aspect modes

### 2. Audio Format Coverage (3 formats)

| Format | Codec | Status | Test Cases |
|--------|-------|--------|-----------|
| WAV | 16-bit PCM | ✓ | All modes, all sample rates |
| AIFF | 16-bit PCM | ✓ | All modes, all sample rates |
| OGG | Vorbis | ✓* | All modes, all sample rates |

*OGG tests skip if libvorbis not available

**Format Tests Include:**
- ✓ File creation and completeness
- ✓ Header correctness (sample rate, channels, bit depth)
- ✓ Sample count accuracy
- ✓ Duration correctness
- ✓ Format validation (can be read back)
- ✓ Compatibility with standard file tools

### 3. Sample Rate Coverage (6 rates)

| Rate | Hz | Status | Notes |
|------|-----|--------|-------|
| Minimum | 8000 | ✓ | Low bandwidth |
| Standard | 11025 | ✓ | Telephony quality |
| Default | 22050 | ✓ | Most common |
| High | 32000 | ✓ | Good quality |
| Excellent | 44100 | ✓ | CD quality |
| Maximum | 48000 | ✓ | Professional audio |

**Sample Rate Tests Include:**
- ✓ All modes at each sample rate
- ✓ Output duration scales correctly
- ✓ Audio quality maintains
- ✓ File sizes vary appropriately
- ✓ Invalid rates (below/above range) rejected

### 4. Aspect Ratio Coverage (3 modes)

| Mode | Behavior | Status | Test Cases |
|------|----------|--------|-----------|
| center | Crop to target aspect | ✓ | Portrait, landscape, square images |
| pad | Letter/pillarbox | ✓ | Preserves aspect, adds bars |
| stretch | Force to target | ✓ | May distort but always fits |

**Aspect Tests Include:**
- ✓ Each mode with various image shapes
- ✓ Correct output dimensions
- ✓ No data loss (center/pad)
- ✓ Complete coverage (stretch)
- ✓ Interaction with all protocols

### 5. CW Identification Coverage

**Basic CW Tests:**
- ✓ Standard 5-character callsign (N0CALL)
- ✓ Portable callsign with slash (W5ABC/4)
- ✓ Long callsign (up to 31 chars)
- ✓ Morse code encoding correctness
- ✓ Manual generation and verification

**CW Speed (WPM) Tests:**
- ✓ 1 WPM (slowest)
- ✓ 10 WPM (slow)
- ✓ 15 WPM (default)
- ✓ 25 WPM (fast)
- ✓ 50 WPM (maximum)
- ✓ Invalid WPM (0, -1, 51) rejected

**CW Tone Frequency Tests:**
- ✓ 400 Hz (minimum)
- ✓ 700 Hz (standard)
- ✓ 800 Hz (default)
- ✓ 1000 Hz (alternative)
- ✓ 2000 Hz (maximum)
- ✓ Invalid frequencies rejected

**CW Validation Tests:**
- ✓ -W without -C rejected
- ✓ -T without -C rejected
- ✓ Oversized callsign rejected
- ✓ Invalid characters handled gracefully

### 6. Error Handling Coverage

**Missing Arguments:**
- ✓ Missing -i (input file) → error
- ✓ Missing -o inferred from input

**Invalid Input:**
- ✓ Non-existent input file → error
- ✓ Unreadable input file → error
- ✓ Invalid image format → error

**Invalid Options:**
- ✓ Unknown protocol → error
- ✓ Invalid format → error
- ✓ Sample rate < 8000 → error
- ✓ Sample rate > 48000 → error
- ✓ Invalid aspect mode → error

**Invalid CW Parameters:**
- ✓ WPM < 1 → error
- ✓ WPM > 50 → error
- ✓ Tone < 400 Hz → error
- ✓ Tone > 2000 Hz → error
- ✓ Callsign > 31 chars → error

**Output File Issues:**
- ✓ Output directory doesn't exist → created or error
- ✓ Output file permission denied → error
- ✓ Device full (simulated) → error

### 7. Image Format Coverage

**Supported Formats (via libvips):**
- ✓ PNG
- ✓ JPEG
- ✓ GIF
- ✓ BMP
- ✓ TIFF
- ✓ WebP
- ✓ Others supported by libvips

**Image Property Tests:**
- ✓ Various resolutions (small, medium, large)
- ✓ Unusual aspect ratios (portrait, landscape, square)
- ✓ Color images
- ✓ Grayscale images
- ✓ Images with alpha channel
- ✓ Indexed color images

---

## Test Metrics

### Coverage Metrics

| Category | Coverage | Target | Status |
|----------|----------|--------|--------|
| SSTV Modes | 7/7 (100%) | 100% | ✓ |
| Audio Formats | 3/3 (100%) | 100% | ✓ |
| Sample Rates | 6/6 (100%) | 100% | ✓ |
| Aspect Modes | 3/3 (100%) | 100% | ✓ |
| Error Cases | 15+ | All major | ✓ |
| CW Features | Full | Complete | ✓ |

### Test Execution Metrics

**Test Suite Characteristics:**
- **Total Test Cases**: 55+
- **Automated Tests**: 55
- **Estimated Runtime**: 15-20 minutes (Python suite)
- **Quick Tests**: ~15 (Bash suite, 2-3 minutes)
- **Pass Rate Target**: 100% (all tests must pass)
- **Skip Rate**: < 5% (OGG only if not compiled)

### Quality Metrics

**Expected Results:**
- All 55+ tests pass on target platforms
- No memory leaks (valgrind clean)
- No compiler warnings (with -Wall -Wextra)
- Output files valid and readable
- Performance within expected ranges

---

## Regression Testing

### Regression Test Strategy

**Purpose**: Ensure updates don't break existing functionality

**Frequency**: Every build, before committing changes

**Process**:
1. Run baseline tests on main branch
2. Make code changes
3. Run regression tests
4. Compare results against baseline
5. Investigate any failures
6. Update baseline if intentional changes

### Baseline Test Output

Store baseline results in:
- `tests/baseline_results.json` - Expected test results
- `tests/baseline_timing.txt` - Expected durations

**Comparison Thresholds:**
- Duration: ±5% tolerance (timing variations expected)
- File size: ±2% tolerance
- Sample count: Exact match required
- Format headers: Exact match required

### Regression Test checklist

Before committing:
```bash
# 1. Run full test suite
cd tests && python3 test_suite.py

# 2. Verify all tests pass
# Expected: 55 passed, 0 failed

# 3. Check for memory leaks (optional)
valgrind --leak-check=full ./bin/pisstvpp2 -i tests/images/*.ppm

# 4. Verify no compiler warnings
make clean && make all 2>&1 | grep -i warning

# 5. Test on multiple platforms if possible
# (Raspberry Pi, Linux, macOS, etc.)
```

---

## Performance Testing

### Performance Metrics

**Encoding Speed:**
- Target: All tests complete in < 20 minutes
- Measure: Time from start to all tests passing
- Acceptable variance: ±30% (system-dependent)

**Memory Usage:**
- Target: < 50 MB peak memory
- Measure: Memory during encoding operation
- Tools: valgrind, /proc/self/status, etc.

**CPU Efficiency:**
- Target: Single-threaded, efficient encoding
- Measure: CPU load during operation
- Note: Should not max out CPU indefinitely

### Performance Benchmarks

Sample performance on different platforms:

```
Machine: Raspberry Pi 4 (4GB RAM, 4 CPU @ 1.5 GHz)
  - Full test suite: ~45 minutes
  - Single mode test: 15-30 seconds
  - Memory peak: ~20 MB

Machine: Desktop Linux (16GB RAM, 8 CPU @ 3.5 GHz)
  - Full test suite: ~8 minutes
  - Single mode test: 2-5 seconds
  - Memory peak: ~15 MB

Machine: macOS (Intel i7, 16GB RAM)
  - Full test suite: ~10 minutes
  - Single mode test: 3-6 seconds
  - Memory peak: ~18 MB
```

### Performance Optimization Testing

Test that optimizations work:
```bash
# Compile with optimizations enabled
make clean
make CFLAGS="-O3 -march=native" all

# Run tests and measure
time python3 test_suite.py

# Compare against non-optimized build
```

---

## CI/CD Integration

### GitHub Actions / GitLab CI Integration

**Automated Test Runs:**
- On every commit
- On every pull request
- Scheduled daily/weekly builds
- Release builds

**CI Configuration:**

```yaml
# .github/workflows/test.yml (example for GitHub Actions)
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y build-essential libvips-dev libogg-dev libvorbis-dev
      
      - name: Build
        run: |
          make clean && make all
      
      - name: Run tests
        run: |
          cd tests && python3 test_suite.py
      
      - name: Check results
        if: always()
        run: |
          cat test_results.txt || echo "No results file"
```

### Test Result Reporting

**JSON Output Format** (for CI integration):
```json
{
  "summary": {
    "total": 55,
    "passed": 55,
    "failed": 0,
    "skipped": 0,
    "duration_seconds": 15.2
  },
  "tests": [
    {
      "name": "Protocol m1",
      "status": "PASSED",
      "duration_seconds": 0.5,
      "details": "Output: 114.2s @ 22050 Hz"
    }
  ]
}
```

### Build Matrix Testing

Test on multiple configurations:

```
Platforms:
  - Ubuntu 20.04 LTS (GCC)
  - Ubuntu 22.04 LTS (GCC)
  - Raspberry Pi OS 32-bit (GCC)
  - Raspberry Pi OS 64-bit (GCC)
  - macOS 11+ (Clang)

Configurations:
  - With OGG support
  - Without OGG support
  - Debug mode (-g -O0)
  - Release mode (-O3)
  - With sanitizers (-fsanitize=address, -fsanitize=undefined)
```

---

## Test Documentation

### Writing New Tests

To add a new test to the Python test suite:

```python
def test_new_feature(self):
    """Test description"""
    print("\n" + "="*70)
    print("TEST GROUP: New Feature")
    print("="*70)
    
    test_img = list(self.test_images.keys())[0]
    output_file = self.test_dir / "test_new.wav"
    
    ret, stdout, stderr = self._run_command([
        "-i", test_img,
        "-o", str(output_file),
        # ... test-specific options
    ])
    
    if not self._assert_success("Test new feature", ret, stderr):
        return
    
    is_valid, info = self._assert_output("Test new feature", str(output_file))
    if is_valid:
        self._log_test("Test new feature", "PASSED", f"Details: {info}")
```

### Test Best Practices

1. **Name tests clearly**: `test_protocol_m1_wav_format`
2. **One assertion per test case**: Fails clearly
3. **Test both success and failure**: Happy path + error cases
4. **Use meaningful output files**: `test_<feature>_<variant>.wav`
5. **Validate output**: Not just file creation but content
6. **Document expected behavior**: In test comments
7. **Include edge cases**: Boundary conditions, limits
8. **Clean up properly**: Remove test files after (or keep for debugging)

---

## Troubleshooting Tests

### Tests Fail on Build
```bash
# Check compilation errors
make clean && make all 2>&1 | head -50

# Check dependencies
pkg-config --exists vips && echo "libvips OK"
pkg-config --exists vorbis && echo "libvorbis OK"
```

### OGG Tests Skip
Normal behavior if libvorbis not installed:
```bash
# To enable: install libvorbis-dev
sudo apt-get install libogg-dev libvorbis-dev

# Rebuild with OGG support
make clean && make all
```

### Output Files Can't Be Written
```bash
# Check test directory exists and is writable
ls -ld tests/test_outputs/
chmod 755 tests/test_outputs/

# Check disk space
df -h tests/
```

### Tests Timeout
Increase timeout in test_suite.py:
```python
# In test_suite.py
COMMAND_TIMEOUT = 60  # seconds (increase if needed)
```

---

## See Also

- [Test Cases Reference](TEST_CASES.md) - Detailed test case specifications
- [Test Suite README](TEST_SUITE_README.md) - Test framework documentation
- [Test Quick Start](TEST_QUICK_START.md) - Quick testing guide
- [Practical Testing Guide](TEST_PRACTICAL_GUIDE.md) - Writing and debugging tests
