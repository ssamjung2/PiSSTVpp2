# PiSSTVpp Test Suite Documentation

This document describes the comprehensive automated test suite for the PiSSTVpp SSTV audio encoder.

## Overview

Two test suites are available:

1. **Python Test Suite** (`test_suite.py`) - Comprehensive, detailed testing with JSON output
2. **Bash Test Suite** (`test_suite_bash.sh`) - Quick smoke tests without dependencies

## Python Test Suite

### Features

- **60+ automated tests** covering all major functionality
- **Structured test groups** for easy organization
- **JSON output** for CI/CD integration
- **Detailed validation** including audio file verification
- **Error case testing** to ensure proper error handling
- **Combination testing** with multiple options

### Requirements

- Python 3.6+
- `pisstvpp2` executable in current directory or specified path
- Test images in current directory

### Usage

```bash
# Run all tests with default executable
python3 test_suite.py

# Specify custom executable path
python3 test_suite.py --exe ./pisstvpp2_custom

# Verbose output
python3 test_suite.py --verbose

# Keep test output files (normally cleaned up)
python3 test_suite.py --keep-outputs
```

### Test Categories

#### 1. Help and Information
- Verifies `-h` help flag works
- Confirms help text is displayed

#### 2. Error Handling - Missing Arguments
- Missing input file (`-i`)
- Invalid or non-existent input file

#### 3. SSTV Protocols
Tests all 7 supported modes:
- Martin 1 (`m1`, VIS code 44, ~114 sec)
- Martin 2 (`m2`, VIS code 40, ~58 sec)
- Scottie 1 (`s1`, VIS code 60, ~110 sec)
- Scottie 2 (`s2`, VIS code 56, ~71 sec)
- Scottie DX (`sdx`, VIS code 76, ~269 sec)
- Robot 36 (`r36`, VIS code 8, ~36 sec)
- Robot 72 (`r72`, VIS code 12, ~72 sec)

#### 4. Audio Output Formats
- WAV format (16-bit PCM)
- AIFF format

#### 5. Audio Sample Rates
Tests valid rates: 8000, 11025, 22050, 32000, 44100, 48000 Hz
Tests invalid rates for rejection: 7999, 4000, 48001, 96000, 0, -1

#### 6. Aspect Ratio Modes
- `4:3` - Center-crop to aspect ratio
- `fit` - Letterbox with black bars
- `stretch` - Non-uniform scaling

#### 7. CW Callsign Signatures
Tests Morse code CW signatures with:
- Various callsigns (standard, portable with `/`, maritime)
- WPM range: 1-50 WPM
- Tone frequency: 400-2000 Hz
- Message format: "SSTV de [CALLSIGN]"

#### 8. Error Handling - CW Validation
- Rejecting `-W` without `-C`
- Rejecting `-T` without `-C`
- Oversized callsigns (>31 chars)

#### 9. Invalid CW Parameters
- WPM outside 1-50 range
- Tone frequency outside 400-2000 Hz range

#### 10. Audio Format Validation
- Rejection of invalid formats (e.g., `mp3`)
- Format auto-detection from extension

#### 11. Protocol Validation
- Rejection of unrecognized protocol codes

#### 12. Combined Options
Complex test scenarios combining:
- Different protocols with various sample rates
- Aspect ratios with formats
- CW signatures with protocols

#### 13. Multiple Test Images
Tests with various image files found in directory

#### 14. Output File Naming
- Automatic extension addition
- Explicit filename specification
- Directory handling

### Output

Test results are saved to JSON in `test_outputs/` directory:

```json
{
  "summary": {
    "passed": 45,
    "failed": 0,
    "skipped": 2,
    "total": 47,
    "timestamp": "2024-01-24T14:30:00"
  },
  "results": [
    {
      "test": "Protocol m1 (Martin 1)",
      "status": "PASSED",
      "details": "Output: 114.5s @ 22050 Hz, 5046750 bytes"
    }
  ]
}
```

## Bash Test Suite

### Features

- Lightweight, no Python dependency
- Quick smoke tests for basic functionality
- Colored output for easy reading
- ~35 core tests

### Usage

```bash
# Make script executable
chmod +x test_suite_bash.sh

# Run with default executable path
./test_suite_bash.sh

# Specify custom executable
./test_suite_bash.sh ./pisstvpp2_custom
```

### Test Groups

1. **Basic Functionality** - Help, missing input, error messages
2. **Protocol Tests** - All 7 SSTV modes
3. **Sample Rate Tests** - Common rates (8k to 48k Hz)
4. **Format Tests** - WAV and AIFF output
5. **Aspect Ratio Tests** - All 3 modes
6. **CW Signature Tests** - Multiple callsign scenarios
7. **Error Handling** - Invalid options rejection
8. **Combined Options** - Multi-option scenarios

### Output

Test output with color-coded results:
- ✓ `PASS` in green
- ✗ `FAIL` in red
- Summary statistics at end

## Test Images

Required test images (place in project root):

- `test_320x240.png` - Recommended for Robot modes
- `test_320x256.png` - Recommended for Martin/Scottie modes
- `test.jpg`, `test.png`, `test.bmp` - General tests
- Any valid image format (JPEG, PNG, BMP, GIF, TIFF, WebP, etc.)

Generate test images if needed:

```bash
# Using ImageMagick
convert -size 320x240 xc:red test_320x240.png
convert -size 320x256 xc:blue test_320x256.png

# Using Python PIL
python3 << 'EOF'
from PIL import Image, ImageDraw
img = Image.new('RGB', (320, 240), color='red')
img.save('test_320x240.png')
EOF
```

## Output Directory Structure

```
test_outputs/
├── test_protocol_m1.wav
├── test_protocol_m2.wav
├── test_protocol_s1.wav
├── test_protocol_s2.wav
├── test_protocol_sdx.wav
├── test_protocol_r36.wav
├── test_protocol_r72.wav
├── test_rate_8000.wav
├── test_rate_11025.wav
├── test_rate_22050.wav
├── test_rate_44100.wav
├── test_rate_48000.wav
├── test_format_wav.wav
├── test_format_aiff.aiff
├── test_aspect_4_3.wav
├── test_aspect_fit.wav
├── test_aspect_stretch.wav
├── test_cw_standard.wav
├── test_cw_portable.wav
├── test_cw_maritime.wav
├── test_combined_*.wav
└── test_results_20240124_143000.json
```

## Integration with CI/CD

### GitHub Actions Example

```yaml
name: PiSSTVpp Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y gcc libvips-dev
      
      - name: Build
        run: make pisstvpp2
      
      - name: Run Python test suite
        run: python3 test_suite.py
      
      - name: Upload results
        if: always()
        uses: actions/upload-artifact@v2
        with:
          name: test-results
          path: test_outputs/
```

### GitLab CI Example

```yaml
stages:
  - build
  - test

build:
  stage: build
  script:
    - sudo apt-get update
    - sudo apt-get install -y gcc libvips-dev
    - make pisstvpp2
  artifacts:
    paths:
      - pisstvpp2

test:python:
  stage: test
  dependencies:
    - build
  script:
    - python3 test_suite.py
  artifacts:
    reports:
      junit: test_outputs/test_results_*.json
```

## Troubleshooting

### No test images found
```bash
# Create sample test images
convert -size 320x240 gradient: test_320x240.png
convert -size 320x256 gradient: test_320x256.png
```

### Executable not found
```bash
# Make sure pisstvpp2 is compiled and in PATH
./pisstvpp2 -h  # Should display help
```

### Python test suite fails to find output files
```bash
# Ensure WAV/AIFF headers are valid
file test_outputs/*.wav
```

### Audio file validation fails
```bash
# Verify using sox or ffprobe
sox test_outputs/test_protocol_m1.wav -n stat
ffprobe test_outputs/test_protocol_m1.wav
```

## Test Statistics

| Aspect | Count |
|--------|-------|
| Total test cases | 60+ |
| Protocol tests | 7 |
| Sample rate tests | 11 (5 valid + 6 invalid) |
| Format tests | 2 |
| Aspect mode tests | 3 |
| CW signature tests | 4 |
| Error handling tests | 10+ |
| Combined tests | 3 |

## Performance

Approximate test execution times:

| Suite | Time | Notes |
|-------|------|-------|
| Full Python suite | 5-8 minutes | Includes all validations |
| Quick bash suite | 2-3 minutes | Basic functionality only |
| Single test | 30-120 seconds | Depends on protocol |

## Advanced Usage

### Running specific test category

```python
suite = TestSuite()
suite.test_protocols()  # Run only protocol tests
suite.test_cw_signatures()  # Run only CW tests
```

### Custom test scenarios

Add to `test_suite.py`:

```python
def test_custom_scenario(self):
    """Your custom test"""
    print("\n" + "="*70)
    print("TEST GROUP: Custom Scenario")
    print("="*70)
    
    # Your test logic here
    ret, stdout, stderr = self._run_command([...])
    if ret == 0:
        self._log_test("Custom test", "PASSED")
    else:
        self._log_test("Custom test", "FAILED", stderr)
```

## Contributing

To add new tests:

1. Follow existing test naming convention: `test_<category>_<feature>`
2. Use `_log_test()` for consistent result recording
3. Document test purpose in docstring
4. Ensure test is deterministic and reproducible
5. Clean up test files or document output

## Version History

- **v1.0** (2024-01-24) - Initial comprehensive test suite
  - 60+ tests across all major functionality
  - Python and Bash implementations
  - JSON output for CI/CD
  - Full error case coverage

## License

These test suites are provided under the same license as PiSSTVpp.

## Support

For issues with the test suite:
1. Verify `pisstvpp2` compiles and runs
2. Ensure test images exist and are valid
3. Check Python version (3.6+)
4. Review test output in `test_outputs/`
5. Run individual tests for debugging

