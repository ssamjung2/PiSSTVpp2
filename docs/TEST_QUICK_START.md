# PiSSTVpp Test Suite - Quick Start Guide

## One-Command Testing

### Python Suite (Comprehensive)
```bash
python3 test_suite.py
```

### Bash Suite (Quick)
```bash
chmod +x test_suite_bash.sh
./test_suite_bash.sh
```

## What Gets Tested

### Protocols (7 modes)
- Martin 1, Martin 2
- Scottie 1, Scottie 2, Scottie DX  
- Robot 36, Robot 72

### Audio Formats
- WAV (16-bit PCM)
- AIFF

### Sample Rates
- 8000, 11025, 22050, 32000, 44100, 48000 Hz
- Invalid rates are rejected

### Aspect Ratios
- 4:3 (crop)
- fit (letterbox)
- stretch (non-uniform)

### CW Signatures
- Morse code generation
- Callsigns (standard, portable, maritime)
- WPM range: 1-50
- Tones: 400-2000 Hz

### Error Handling
- Missing/invalid inputs
- Out-of-range parameters
- Invalid options

## Output

- **Python**: JSON report in `test_outputs/test_results_*.json`
- **Bash**: Color-coded terminal output + summary
- **Files**: Generated test audio files in `test_outputs/`

## Expected Results

✓ All core functionality tests should PASS
✓ Error handling tests should reject invalid input
✓ All output files should be valid WAV/AIFF

## Troubleshooting

No test images? Create them:
```bash
# Using ImageMagick
convert -size 320x240 gradient: test_320x240.png
convert -size 320x256 gradient: test_320x256.png
```

Executable not found? Build it:
```bash
make clean && make pisstvpp2
```

## Key Test Files

| File | Purpose | Runtime |
|------|---------|---------|
| `test_suite.py` | Full comprehensive testing | 5-8 min |
| `test_suite_bash.sh` | Quick smoke tests | 2-3 min |
| `TEST_SUITE_README.md` | Detailed documentation | - |

## Sample Test Commands

```bash
# Test single protocol
python3 -c "from test_suite import *; TestSuite().test_protocols()"

# Test error handling only
python3 -c "from test_suite import *; TestSuite().test_invalid_sample_rates()"

# Run bash suite with custom executable
./test_suite_bash.sh /path/to/pisstvpp2
```

## CI/CD Integration

Python suite outputs JSON that works with:
- GitHub Actions
- GitLab CI
- Jenkins
- Travis CI
- Any CI system that reads JSON

## Success Criteria

- ✓ All 7 protocols encode successfully
- ✓ All sample rates (8k-48k Hz) work
- ✓ Both WAV and AIFF formats produce valid files
- ✓ All aspect ratios handle image resizing
- ✓ CW signatures generate without errors
- ✓ Invalid options are properly rejected
- ✓ Output files have correct duration/size

