# PiSSTVpp Test Suite - Complete Package

## What's Included

### Test Executables
1. **test_suite.py** - Comprehensive Python test framework (60+ tests)
2. **test_suite_bash.sh** - Lightweight bash test suite (~35 tests)

### Documentation
1. **TEST_SUITE_README.md** - Complete reference manual
2. **TEST_QUICK_START.md** - Quick reference guide
3. **TEST_PRACTICAL_GUIDE.md** - Real-world usage scenarios
4. **TEST_MAINTENANCE_CHECKLIST.md** - Maintenance and extension guide
5. **TEST_IMPLEMENTATION_SUMMARY.md** - Technical overview
6. **This file** - Package contents

### Build Integration
- **makefile** - Enhanced with test targets

## Getting Started (5 Minutes)

### Option 1: Quick Test (2-3 minutes)
```bash
cd PiSSTVpp
make clean && make pisstvpp2
make test-quick
```

### Option 2: Full Test (5-8 minutes)
```bash
cd PiSSTVpp
make clean && make pisstvpp2
make test
```

### Option 3: Manual Python Test
```bash
cd PiSSTVpp
make pisstvpp2
python3 test_suite.py
```

### Option 4: Manual Bash Test
```bash
cd PiSSTVpp
make pisstvpp2
./test_suite_bash.sh
```

## What Gets Tested

### Protocols (7 modes)
✓ Martin 1, Martin 2  
✓ Scottie 1, Scottie 2, Scottie DX  
✓ Robot 36, Robot 72

### Audio Formats
✓ WAV (16-bit PCM)  
✓ AIFF

### Sample Rates
✓ Valid: 8000, 11025, 22050, 32000, 44100, 48000 Hz  
✓ Invalid: Rejection testing

### Features
✓ Aspect ratio modes (4:3, fit, stretch)  
✓ CW callsign signatures  
✓ Error handling and validation  
✓ File I/O and naming  
✓ Combined option scenarios

### Total: 60+ test cases

## Output

### Test Files
Generated in `test_outputs/` directory:
- Protocol test files (7 WAV files)
- Sample rate test files (6 WAV files)
- Format test files (2 WAV/AIFF files)
- Aspect mode test files (3 WAV files)
- CW signature test files (4+ WAV files)
- Combined test files (3+ WAV files)

### Test Report
- JSON format: `test_results_YYYYMMDD_HHMMSS.json`
- Contains: Pass/fail counts, detailed results, timestamps

### Example Report
```json
{
  "summary": {
    "passed": 58,
    "failed": 0,
    "skipped": 0,
    "total": 58
  },
  "results": [
    {
      "test": "Protocol m1 (Martin 1)",
      "status": "PASSED",
      "details": "Output: 114.5s @ 22050 Hz"
    }
  ]
}
```

## Make Targets

```bash
make test          # Run full Python test suite
make test-python   # Python tests only
make test-bash     # Bash tests only
make test-quick    # Quick bash test (alias)
make test-full     # Full with verbose output
make test-ci       # CI/CD-ready tests
make test-clean    # Clean test outputs
```

## Documentation Quick Links

| Document | Purpose | Read Time |
|----------|---------|-----------|
| TEST_QUICK_START.md | One-command testing | 2 min |
| TEST_SUITE_README.md | Complete reference | 15 min |
| TEST_PRACTICAL_GUIDE.md | Usage scenarios | 10 min |
| TEST_MAINTENANCE_CHECKLIST.md | Extending tests | 10 min |
| TEST_IMPLEMENTATION_SUMMARY.md | Technical details | 10 min |

## Requirements

### Minimal
- Compiled `pisstvpp2` executable
- Test images in working directory
- Bash 4.0+ (for bash suite)

### For Python Suite
- Python 3.6+
- Standard library modules (no external dependencies)

### For Building
- gcc compiler
- libvips development files
- make

## Features

✓ **Comprehensive** - 60+ test cases  
✓ **Automated** - Runs without user interaction  
✓ **Validated** - Checks audio file properties  
✓ **CI/CD Ready** - JSON output, exit codes  
✓ **Fast** - 5-8 minutes for full suite  
✓ **Lightweight** - Bash suite with no Python needed  
✓ **Documented** - 5 detailed guides included  
✓ **Maintainable** - Easy to extend with new tests  
✓ **Cross-platform** - Works on Linux, macOS, Raspberry Pi  

## Success Criteria

All tests should pass when:

✓ `pisstvpp2` compiles without errors  
✓ All 7 SSTV protocols encode correctly  
✓ Audio output is valid WAV/AIFF  
✓ Sample rates (8-48 kHz) are supported  
✓ Aspect ratio modes handle images  
✓ CW signatures generate correctly  
✓ Error handling rejects invalid input  

## Common Tasks

### Run tests after making changes
```bash
make clean && make test
```

### Test specific feature
```bash
python3 test_suite.py --verbose
```

### CI/CD integration
```bash
make test-ci
```

### Debug a failure
```bash
./test_suite_bash.sh 2>&1 | grep FAIL
```

### Keep only test results
```bash
make test-clean
make test  # Generates only JSON
```

## File Organization

```
PiSSTVpp/
├── test_suite.py                    (NEW) Python suite
├── test_suite_bash.sh               (NEW) Bash suite
├── TEST_SUITE_README.md             (NEW) Full docs
├── TEST_QUICK_START.md              (NEW) Quick ref
├── TEST_PRACTICAL_GUIDE.md          (NEW) Usage guide
├── TEST_MAINTENANCE_CHECKLIST.md    (NEW) Maintenance
├── TEST_IMPLEMENTATION_SUMMARY.md   (NEW) Tech overview
├── makefile                         (UPDATED) Test targets
├── pisstvpp2.c                      (Source)
├── README.md                        (Project doc)
└── test_outputs/                    (AUTO) Generated tests
    ├── test_protocol_m1.wav
    ├── test_protocol_m2.wav
    ├── test_rate_8000.wav
    ├── test_format_wav.wav
    └── test_results_*.json
```

## Integration Examples

### GitHub Actions
```yaml
name: Tests
on: [push]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - run: sudo apt-get install -y gcc libvips-dev
      - run: make test-ci
```

### GitLab CI
```yaml
test:
  script:
    - apt-get update
    - apt-get install -y gcc libvips-dev
    - make test-ci
```

### Local Pre-commit Hook
```bash
#!/bin/bash
# .git/hooks/pre-commit
make test-quick || exit 1
```

## Performance

| Operation | Time |
|-----------|------|
| Full Python suite | 5-8 min |
| Bash quick suite | 2-3 min |
| Single protocol | 30-120 sec |
| Full build + test | 10-15 min |

## Next Steps

1. **Read** TEST_QUICK_START.md (2 min)
2. **Run** `make test-quick` (3 min)
3. **Review** test_outputs/ results (2 min)
4. **Integrate** into your workflow

## Support

For issues:

1. Check TEST_SUITE_README.md
2. Review TEST_PRACTICAL_GUIDE.md
3. Run with `--verbose` flag
4. Check test_outputs/ logs
5. Verify pisstvpp2 works: `./pisstvpp2 -h`

## Statistics

| Metric | Count |
|--------|-------|
| Test files created | 2 |
| Documentation files | 6 |
| Test cases | 60+ |
| Protocols tested | 7 |
| Sample rates tested | 11 |
| Format tests | 2 |
| Error cases | 15+ |
| CW signature variations | 4+ |
| Lines of test code | 1000+ |

## Version

- **Version**: 1.0
- **Created**: January 24, 2024
- **Status**: Complete and ready for use

## License

Test suite provided under the same license as PiSSTVpp.

---

## Quick Start Commands

```bash
# One-time setup
cd PiSSTVpp
make clean && make pisstvpp2

# Quick test (2-3 min)
make test-quick

# Full test (5-8 min)
make test

# CI/CD test
make test-ci

# Clean up
make test-clean
```

## Key Documents

- **Start here**: TEST_QUICK_START.md
- **Full reference**: TEST_SUITE_README.md
- **Usage examples**: TEST_PRACTICAL_GUIDE.md
- **Extending**: TEST_MAINTENANCE_CHECKLIST.md

---

**Happy Testing!** 🧪📡

