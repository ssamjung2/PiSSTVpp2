# WEEK 2 QUICK START: Enhanced Text Overlay Tests

## Overview

The text overlay comprehensive test suite has been enhanced with Week 1 infrastructure:
- **TestReportGenerator**: Creates professional HTML/JSON reports
- **HumanVerifier**: Interactive verification workflow with session persistence
- **TestMetadata**: Auto-generates verification checklists

## Quick Examples

### 1. Run Tests and Generate HTML Report

```bash
cd /Users/ssamjung/Desktop/WIP/SlowFrame

# Generate HTML report with embedded debug images
python3 tests/util/test_text_overlay_comprehensive.py --report

# Output: tests/test_outputs/text_overlay_comprehensive/text_overlay_report_YYYYMMDD_HHMMSS.html
```

**What you get**:
- Professional HTML with all 22 test results
- Debug images embedded (base64)
- Test specifications and commands
- Automated checks results
- Human verification checklist (6 items per test)

### 2. Run Tests with Interactive Verification

```bash
# Enable human verification workflow
python3 tests/util/test_text_overlay_comprehensive.py --verify

# For each test, you'll be prompted:
# Verify test T001 (XY Positioning)? [a/r/s/q]: 
# [a] = approve, [r] = reject, [s] = skip, [d] = defer, [q] = quit
```

**What happens**:
- Tests run and collect results
- After all tests, interactive verification starts
- For each test:
  - Display test name and ID
  - Show verification checklist items
  - Accept your verdict (approve/reject/defer)
- Session saved to: `overlay_verification_session.json`

### 3. Run With Both Reports and Verification

```bash
# Generate report AND run interactive verification
python3 tests/util/test_text_overlay_comprehensive.py --report --verify

# Creates HTML report, then prompts for verification
```

### 4. View Generated Reports

```bash
# HTML Report
open tests/test_outputs/text_overlay_comprehensive/text_overlay_report_*.html

# JSON Report (programmatic access)
cat tests/test_outputs/text_overlay_comprehensive/comprehensive_results_*.json
```

---

## What to Look For (Verification Checklist)

When verifying text overlay tests, use this auto-generated checklist:

1. **Text Rendering** - Is text visible on the image?
2. **Color Match** - Does text color match what was specified?
3. **Placement** - Is text in the correct position (top/bottom/center/xy)?
4. **Readability** - Is text clear and easy to read?
5. **Artifacts** - Are there any clipping or distortion artifacts?
6. **Styling** - Is background/padding/borders correct if specified?

---

## Integration Pattern (For Developers)

If you want to understand how it works:

```python
# 1. Create enhanced test suite
suite = TextOverlayComprehensiveTests(
    executable_path="bin/slowframe",
    enable_reports=True,      # Enable HTML/JSON reports
    enable_verification=True  # Enable interactive verification
)

# 2. Run tests
# - Each test creates TestMetadata with category="text_overlay"
# - Metadata auto-generates 6-item verification checklist
# - Results collected in TestReportGenerator

# 3. Generate reports
# - HTML report with embedded images (images/base64)
# - JSON report with structured data

# 4. Interactive verification
# - Reviews each test one-by-one
# - Presents checklist items
# - Records approval/rejection
# - Saves session for later review
```

---

## Report Contents

### HTML Report Includes

- ✅ Test summary (total, passed, failed)
- ✅ Individual test sections with:
  - Test ID and name
  - Status badge (passed/failed/skipped)
  - Command executed
  - Input/output images (embedded)
  - Automated checks (executed, no crashes, output generated)
  - Verification checklist (6 items with empty checkboxes)
  - Human verdict section (if verified)
- ✅ Professional styling (responsive, color-coded)
- ✅ Timestamp and suite information

### JSON Report Includes

- test_id, name, suite
- status, command, execution_time
- input_image, output_files
- automated_checks (dictionary)
- verification_checklist (array of 6 items)
- human_verified, human_verdict, human_notes
- error details if applicable

---

## Demo

To see the integration in action without running all 22 tests:

```bash
# Run Week 2 integration demo (2 minute demo)
python3 tests/util/week2_overlay_integration_demo.py

# Shows:
# - Demo 1: Basic test with report generation
# - Demo 2: Category-specific checklists  
# - Demo 3: Integration pattern with 3 sample tests
# 
# Generates: demo_overlay_integration_report.json (sample)
```

---

## File Locations

| What | Where |
|---|---|
| Enhanced test suite | `tests/util/test_text_overlay_comprehensive.py` |
| Integration demo | `tests/util/week2_overlay_integration_demo.py` |
| Generated HTML reports | `tests/test_outputs/text_overlay_comprehensive/text_overlay_report_*.html` |
| Generated JSON reports | `tests/test_outputs/text_overlay_comprehensive/comprehensive_results_*.json` |
| Verification sessions | `tests/test_outputs/text_overlay_comprehensive/overlay_verification_session.json` |
| Demo sample report | `tests/util/demo_overlay_integration_report.json` |

---

## Command Cheat Sheet

```bash
# Basic run (no reports)
python3 tests/util/test_text_overlay_comprehensive.py

# With HTML report
python3 tests/util/test_text_overlay_comprehensive.py --report

# With interactive verification
python3 tests/util/test_text_overlay_comprehensive.py --verify

# With both
python3 tests/util/test_text_overlay_comprehensive.py --report --verify

# With custom executable path
python3 tests/util/test_text_overlay_comprehensive.py --exe /path/to/slowframe --report

# With verbose output
python3 tests/util/test_text_overlay_comprehensive.py --verbose --report

# Run demo to see integration examples
python3 tests/util/week2_overlay_integration_demo.py
```

---

## Auto-Generated Verification Checklist

Text overlay tests automatically get a 6-item checklist:

```
□ Text is rendered and visible
□ Text color matches specification
□ Text placement is correct (top/bottom/custom)
□ Text is readable and clear
□ No clipping or artifacts around text
□ Background/styling applied correctly if specified
```

This appears in:
- **HTML Report**: Human-readable checklist section
- **JSON Report**: As `verification_checklist` array
- **Interactive Verification**: Item-by-item review

---

## What's Different from Week 1?

### Week 1 (Foundation)
- TestReportGenerator: Generic HTML/JSON report generation
- HumanVerifier: Generic verification framework
- TestMetadata: Enhanced with auto-checklist generation
- Demo shows pattern with 3 sample tests

### Week 2 (Text Overlay Integration)
- All 22 overlay tests now use TestReportGenerator
- All tests now support interactive verification
- All tests get auto-generated text_overlay checklist
- Command-line flags integrated into test suite
- Demo shows actual integration with overlay tests

### Week 3+ (Planned)
- Same integration for Format Validator tests
- Same integration for Aspect Ratio tests
- Unified reporting across all test categories

---

## Troubleshooting

### "ImportError: No module named 'test_report_generator'"
Solution: Run from the SlowFrame root directory

```bash
cd /Users/ssamjung/Desktop/WIP/SlowFrame
python3 tests/util/test_text_overlay_comprehensive.py --report
```

### HTML Report Not Generated
Check:
1. Test suite completed without exception
2. Use `--report` flag
3. Check `tests/test_outputs/text_overlay_comprehensive/` directory

### Verification Session Not Saved
Check:
1. Used `--verify` flag
2. Tests ran to completion
3. Session file location: `tests/test_outputs/text_overlay_comprehensive/overlay_verification_session.json`

### Images Not Embedded in HTML Report
Check:
1. Debug images were generated during test run
2. Images are in the test_outputs directory
3. No file path errors in the report

---

## Next Steps

After verifying text overlay tests:

1. **Review findings**: Check what tests passed/failed
2. **Debug failures**: Use debug images in HTML report
3. **Document results**: Save HTML report as baseline
4. **Proceed to Week 3**: Apply same pattern to other test suites

---

**Status**: ✅ Week 2 Text Overlay Enhancement Ready to Use
