# PiSSTVpp Test Suite - Complete Index

## Quick Navigation

**New to the test suite?** Start here:
- [TEST_SUITE_SUMMARY.md](TEST_SUITE_SUMMARY.md) - 2 minute overview
- [TEST_QUICK_START.md](TEST_QUICK_START.md) - Run tests in 5 minutes

**Need details?**
- [TEST_SUITE_README.md](TEST_SUITE_README.md) - Complete reference manual
- [TEST_PRACTICAL_GUIDE.md](TEST_PRACTICAL_GUIDE.md) - Real-world examples and scenarios

**Extending or maintaining?**
- [TEST_MAINTENANCE_CHECKLIST.md](TEST_MAINTENANCE_CHECKLIST.md) - Adding new tests
- [TEST_IMPLEMENTATION_SUMMARY.md](TEST_IMPLEMENTATION_SUMMARY.md) - Technical details

**See it in action?**
- [SAMPLE_TEST_OUTPUT.md](SAMPLE_TEST_OUTPUT.md) - Example test output

**Package overview?**
- [TEST_SUITE_PACKAGE.md](TEST_SUITE_PACKAGE.md) - What's included and why

---

## File Guide

### Test Executables

| File | Type | Purpose | When to Use |
|------|------|---------|------------|
| `test_suite.py` | Python | Full test suite (60+ tests) | Detailed testing, CI/CD |
| `test_suite_bash.sh` | Bash | Quick tests (~35 tests) | Fast feedback, no Python needed |

### Documentation Files

| File | Pages | Purpose | Read Time |
|------|-------|---------|-----------|
| TEST_SUITE_SUMMARY.md | 2 | User-focused overview | 2 min |
| TEST_QUICK_START.md | 1 | One-command testing | 2 min |
| TEST_SUITE_PACKAGE.md | 3 | Complete package info | 5 min |
| TEST_SUITE_README.md | 15+ | Comprehensive manual | 15 min |
| TEST_PRACTICAL_GUIDE.md | 10 | Usage scenarios (14 examples) | 10 min |
| TEST_MAINTENANCE_CHECKLIST.md | 8 | Extending/maintaining | 10 min |
| TEST_IMPLEMENTATION_SUMMARY.md | 5 | Technical overview | 10 min |
| SAMPLE_TEST_OUTPUT.md | 5 | Example outputs | 5 min |

### Configuration Files

| File | Change | Purpose |
|------|--------|---------|
| makefile | Updated | Added 7 test targets |

---

## Getting Started (3 Steps)

### Step 1: Build (1 minute)
```bash
cd PiSSTVpp
make clean && make pisstvpp2
```

### Step 2: Run Tests (5 minutes)
```bash
make test    # Full suite
# or
make test-quick  # Quick version
```

### Step 3: Review Results
```bash
cat test_outputs/test_results_*.json
```

---

## Make Targets Reference

```bash
make test          # Full Python test suite (recommended) - 5-8 min
make test-python   # Python tests only - 5-8 min
make test-bash     # Bash tests only - 2-3 min
make test-quick    # Quick bash suite (alias) - 2-3 min
make test-full     # Full with verbose output - 5-8 min
make test-ci       # CI/CD-ready tests - 5-8 min
make test-clean    # Clean test outputs
```

---

## Test Coverage Summary

| Category | Count | Documentation |
|----------|-------|----------------|
| Protocols | 7 | SSTV modes M1/M2, S1/S2/SDX, R36/R72 |
| Formats | 2 | WAV and AIFF |
| Sample rates | 11 | 6 valid (8k-48k) + 5 invalid |
| Aspect modes | 4 | 3 modes + invalid rejection |
| CW signatures | 4+ | Various callsigns and parameters |
| Error cases | 15+ | Invalid inputs properly rejected |
| Combined | 3+ | Multiple options together |
| **Total** | **60+** | Comprehensive coverage |

---

## Documentation by Use Case

### I want to...

**...run tests immediately**
→ [TEST_QUICK_START.md](TEST_QUICK_START.md)

**...understand what's tested**
→ [TEST_SUITE_SUMMARY.md](TEST_SUITE_SUMMARY.md)

**...integrate into CI/CD**
→ [TEST_PRACTICAL_GUIDE.md](TEST_PRACTICAL_GUIDE.md) → Scenario 2

**...debug a failure**
→ [TEST_PRACTICAL_GUIDE.md](TEST_PRACTICAL_GUIDE.md) → Scenario 13

**...add new tests**
→ [TEST_MAINTENANCE_CHECKLIST.md](TEST_MAINTENANCE_CHECKLIST.md)

**...understand technical details**
→ [TEST_IMPLEMENTATION_SUMMARY.md](TEST_IMPLEMENTATION_SUMMARY.md)

**...see example output**
→ [SAMPLE_TEST_OUTPUT.md](SAMPLE_TEST_OUTPUT.md)

**...get complete reference**
→ [TEST_SUITE_README.md](TEST_SUITE_README.md)

---

## Key Features

✅ **60+ automated tests** covering all functionality  
✅ **Two implementations**: Python (comprehensive) and Bash (lightweight)  
✅ **Validated output**: Checks audio format, sample rate, duration  
✅ **CI/CD ready**: JSON output, appropriate exit codes  
✅ **Well documented**: 8 comprehensive guides totaling 60+ pages  
✅ **Easy to use**: `make test` is all you need  
✅ **Easy to extend**: Clear structure for adding tests  
✅ **Cross-platform**: Linux, macOS, Raspberry Pi  
✅ **Fast**: Full suite in 5-8 minutes  

---

## Quick Command Reference

```bash
# Build and test (one command)
make clean && make test

# Just run tests (already built)
make test-python        # Full suite
make test-quick         # Quick version
make test-full -v       # Verbose output

# Manual testing
python3 test_suite.py                    # Full suite
./test_suite_bash.sh                     # Bash suite
./pisstvpp2 -i test_320x240.png -p m1   # Single test

# View results
ls -lh test_outputs/                     # Generated files
cat test_outputs/test_results_*.json     # JSON report
```

---

## File Statistics

| Aspect | Value |
|--------|-------|
| Test Python code | 500+ lines |
| Test Bash code | 350+ lines |
| Documentation | 60+ pages |
| Test cases | 60+ |
| Make targets | 7 new |
| Total files created/modified | 11 |

---

## Recommended Reading Order

1. **First**: [TEST_SUITE_SUMMARY.md](TEST_SUITE_SUMMARY.md) (2 min)
2. **Quick start**: [TEST_QUICK_START.md](TEST_QUICK_START.md) (2 min)
3. **Run tests**: `make test` (5-8 min)
4. **Deep dive**: [TEST_SUITE_README.md](TEST_SUITE_README.md) (15 min)
5. **Examples**: [TEST_PRACTICAL_GUIDE.md](TEST_PRACTICAL_GUIDE.md) (10 min)

---

## Support Resources

| Need | Resource |
|------|----------|
| Quick overview | TEST_SUITE_SUMMARY.md |
| How to run | TEST_QUICK_START.md |
| Complete guide | TEST_SUITE_README.md |
| Real examples | TEST_PRACTICAL_GUIDE.md |
| Extending tests | TEST_MAINTENANCE_CHECKLIST.md |
| Technical details | TEST_IMPLEMENTATION_SUMMARY.md |
| Example output | SAMPLE_TEST_OUTPUT.md |
| Package contents | TEST_SUITE_PACKAGE.md |

---

## What's Next?

1. **Try it**: `make test-quick` (3 minutes)
2. **Review output**: Check `test_outputs/` directory
3. **Read one guide**: Pick based on your need from table above
4. **Integrate**: Add to your workflow or CI/CD pipeline

---

## Success Indicators

When tests run successfully, you should see:

```
✓ PASSED: 58
✗ FAILED: 0
```

All 7 SSTV protocols working, all sample rates supported, all audio formats valid.

---

## Document Sizes

| File | Size |
|------|------|
| test_suite.py | ~15 KB |
| test_suite_bash.sh | ~12 KB |
| TEST_SUITE_README.md | ~18 KB |
| TEST_PRACTICAL_GUIDE.md | ~20 KB |
| TEST_MAINTENANCE_CHECKLIST.md | ~12 KB |
| TEST_IMPLEMENTATION_SUMMARY.md | ~12 KB |
| TEST_SUITE_PACKAGE.md | ~10 KB |
| Other docs | ~25 KB |
| **Total** | **~124 KB** |

---

**Happy Testing!** 🧪

For any specific questions, find your use case in the "I want to..." section above.

