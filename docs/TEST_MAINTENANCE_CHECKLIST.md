# PiSSTVpp Test Suite - Maintenance Checklist

Use this checklist to maintain and extend the test suite.

## Initial Setup Checklist

- [ ] Clone or download PiSSTVpp repository
- [ ] Verify test images exist:
  - [ ] `test_320x240.png`
  - [ ] `test_320x256.png`
  - [ ] At least one additional image (`.jpg`, `.png`, `.bmp`)
- [ ] Compile the program: `make pisstvpp2`
- [ ] Verify executable works: `./pisstvpp2 -h`
- [ ] Install Python 3.6+ (if using Python suite)
- [ ] Run quick test: `make test-quick`
- [ ] Review results in `test_outputs/`

## Pre-Release Checklist

Before releasing a new version:

- [ ] Run full test suite: `make test`
- [ ] Verify all tests PASS
- [ ] Check JSON report: `test_outputs/test_results_*.json`
- [ ] Run stress test with multiple images
- [ ] Test on Raspberry Pi (if targeting RPi)
- [ ] Test on target platform (Linux, macOS)
- [ ] Verify no performance regressions
- [ ] Document any new features tested
- [ ] Update version in test output if needed

## Feature Addition Checklist

When adding a new feature:

1. **Create test cases**
   - [ ] Normal operation test
   - [ ] Edge case test
   - [ ] Error case test
   - [ ] Integration test with existing features

2. **Update documentation**
   - [ ] Add test description to appropriate section
   - [ ] Update test count in summary
   - [ ] Add example usage if applicable
   - [ ] Document expected results

3. **Implement tests**
   - [ ] Add method to `test_suite.py`
   - [ ] Add equivalent bash test if applicable
   - [ ] Ensure deterministic results
   - [ ] Verify test independence

4. **Validation**
   - [ ] Run new tests standalone
   - [ ] Run full suite to ensure no conflicts
   - [ ] Check file cleanup after tests
   - [ ] Verify JSON output is valid

5. **Documentation**
   - [ ] Update TEST_SUITE_README.md
   - [ ] Update test statistics
   - [ ] Add to practical guide if applicable

## Bug Fix Checklist

When fixing a bug:

- [ ] Create test case that reproduces bug
- [ ] Verify test FAILS before fix
- [ ] Apply fix to source code
- [ ] Verify test PASSES after fix
- [ ] Run full suite to ensure no regressions
- [ ] Document in test comments
- [ ] Keep test in suite as regression prevention

## Protocol Addition Checklist

When adding a new SSTV protocol:

- [ ] Create test protocol: `test_protocol_XX`
- [ ] Verify encoding works
- [ ] Check output duration is reasonable
- [ ] Validate audio format
- [ ] Test all sample rates with protocol
- [ ] Test all aspect ratio modes
- [ ] Add to TEST_SUITE_README.md
- [ ] Update protocol count

## Sample Rate Addition Checklist

When modifying sample rate support:

- [ ] Update `test_sample_rates()` method
- [ ] Test boundary values (8000, 48000)
- [ ] Test invalid edge cases
- [ ] Run full protocol suite at new rates
- [ ] Update documentation
- [ ] Check for performance impact

## Maintenance Schedule

### Weekly
- [ ] Run test suite once: `make test`
- [ ] Review any test failures
- [ ] Check test output directory size

### Monthly
- [ ] Run full test suite with verbose output
- [ ] Review JSON results for patterns
- [ ] Check test execution time trends
- [ ] Update test documentation
- [ ] Review and organize test output files

### Before Release
- [ ] Run stress tests
- [ ] Test on multiple platforms
- [ ] Verify all edge cases
- [ ] Test with various image sizes
- [ ] Performance profile if major changes

## Performance Monitoring

Track test execution time to detect regressions:

```bash
# Run suite and save timing
time make test > test_log_$(date +%Y%m%d).txt 2>&1

# Compare with previous runs
diff test_log_*.txt
```

### Expected Baselines

| Suite | Expected | Alert if > |
|-------|----------|-----------|
| Bash quick | 2-3 min | 5 min |
| Python full | 5-8 min | 15 min |
| Single protocol | 30-120 sec | 3 min |

## Documentation Updates

Keep documentation current:

- [ ] TEST_SUITE_README.md - Primary reference
- [ ] TEST_QUICK_START.md - Quick commands
- [ ] TEST_PRACTICAL_GUIDE.md - Usage scenarios
- [ ] This file - Maintenance guide
- [ ] makefile - Build and test targets

## Test Coverage Analysis

Ensure adequate coverage:

- [ ] All command-line options tested
- [ ] All protocols tested
- [ ] All error conditions tested
- [ ] All audio formats tested
- [ ] All sample rates tested
- [ ] Edge cases covered

**Current Coverage:**
- Protocols: 7/7 (100%)
- Formats: 2/2 (100%)
- Sample rates: 6 valid + invalid edge cases
- Aspect modes: 3/3 (100%)
- CW features: Enabled + all validation
- Error cases: ~15 tests

## Known Limitations

Track known test limitations:

- [ ] Some tests require specific image dimensions
- [ ] CW signature timing may vary on slow systems
- [ ] AIFF validation is basic (doesn't verify all headers)
- [ ] No real-time audio verification
- [ ] No spectral analysis of output

## Future Test Enhancements

Consider these improvements:

- [ ] Audio quality metrics (THD, SNR)
- [ ] Memory usage profiling
- [ ] Tone frequency verification
- [ ] VIS code validation
- [ ] Sync pulse detection
- [ ] Performance benchmarking
- [ ] Stress testing (1000+ images)
- [ ] Multi-threaded encoding tests
- [ ] Signal analysis validation
- [ ] Comparison with reference encoders

## Common Issues and Solutions

### Test Execution Problems

**Problem: Tests timeout**
- Check system resources
- Run single protocol test
- Verify I/O performance
- Check for system processes hogging CPU

**Problem: File creation fails**
- Check disk space: `df -h`
- Verify permissions: `chmod 755 test_outputs/`
- Check for file locks
- Verify test_outputs directory exists

**Problem: Audio validation fails**
- Verify WAV headers: `file test_outputs/*.wav`
- Check sample rate: `python3 -c "import wave; w=wave.open(...)" `
- Verify channel count (should be 1)
- Check bit depth (should be 16-bit)

### Platform-Specific Issues

**macOS**
- Use `stat -f%z` for file size (not `-c%s`)
- Verify libvips from Homebrew
- Check `/opt/homebrew` paths in makefile

**Linux**
- Use `stat -c%s` for file size
- Verify dependencies: `pkg-config --cflags --libs vips`
- Check for glib version conflicts

**Raspberry Pi**
- Use lightweight bash suite instead of Python
- Expect longer execution time
- Monitor memory usage
- Check CPU temperature

## Test Validation Checklist

Before committing test changes:

- [ ] All tests PASS
- [ ] No test files left behind in root directory
- [ ] JSON output is valid: `python3 -m json.tool test_results_*.json`
- [ ] No hardcoded paths (use relative paths)
- [ ] Code is readable and commented
- [ ] Error messages are clear
- [ ] Test methods are independent
- [ ] No dependencies on external tools (except pisstvpp2)

## Debugging Failed Tests

Steps to debug:

1. **Identify failing test**
   ```bash
   grep "FAILED" test_results_*.json
   ```

2. **Run test individually**
   ```python
   from test_suite import TestSuite
   suite = TestSuite()
   suite.test_protocols()  # Run specific test group
   ```

3. **Enable verbose output**
   ```bash
   python3 test_suite.py --verbose
   ```

4. **Check generated files**
   ```bash
   ls -lh test_outputs/
   file test_outputs/*.wav
   ```

5. **Verify executable**
   ```bash
   ./pisstvpp2 -h
   ```

6. **Review error messages**
   ```bash
   ./pisstvpp2 -i test_320x240.png -p m1 -v
   ```

## Version Control

Recommended `.gitignore` entries:

```
test_outputs/
*.wav
*.aiff
*.o
pisstvpp2
pisstvpp2_*
batch_test/
debug_test.*
test_log_*.txt
.pytest_cache/
__pycache__/
```

## Contact and Support

For test suite issues:

1. Check TEST_SUITE_README.md
2. Review TEST_PRACTICAL_GUIDE.md
3. Check this maintenance file
4. Verify pisstvpp2 compiles: `make clean && make pisstvpp2`
5. Check test images exist
6. Review error messages carefully

## Sign-Off

Test suite validation completed:

- Date: ________________
- Tested by: ________________
- Platform: ________________
- Result: [ ] PASS [ ] FAIL [ ] PARTIAL
- Notes: ________________________________________________

