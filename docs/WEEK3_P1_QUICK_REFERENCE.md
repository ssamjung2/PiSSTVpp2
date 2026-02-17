# Week 3 P1 Validators - Quick Reference

## Run Format Validator

### Basic Usage
```bash
cd tests/util
python3 format_validator.py
```

### With Reporting and Verification
```bash
python3 format_validator.py --report --verify
```

### Sample Output
```
Testing PNG format...
✓ PNG format validation passed

Testing JPEG format...
✓ JPEG format validation passed

Testing GIF format...
✓ GIF format validation passed

Testing BMP format...
✓ BMP format validation passed

Report: format_validation_20250212_143050.html
```

## Run Aspect Ratio Validator

### Basic Usage (CENTER mode)
```bash
cd tests/util
python3 aspect_validator.py test_output.png center 2000 1600 320 256
```

### With Source Image Analysis
```bash
python3 aspect_validator.py test_output.png pad 1024 768 320 256 \
  --source source_input.png --report --verify
```

### All Available Modes
```bash
# CENTER: Crops to match aspect ratio
python3 aspect_validator.py output_center.png center 2000 1600 320 256

# PAD: Adds black padding to preserve aspect
python3 aspect_validator.py output_pad.png pad 1024 768 320 256

# STRETCH: Direct resize without aspect preservation
python3 aspect_validator.py output_stretch.png stretch 800 600 320 256
```

### With All Flags
```bash
python3 aspect_validator.py output.png center 2000 1600 320 256 \
  --source input.png \
  --report \
  --verify \
  --verbose
```

## Generated Reports

Both validators generate:

### HTML Report
- **Location**: Current directory
- **Name Pattern**: `{validator}_validation_YYYYMMDD_HHMMSS.html`
- **Contains**:
  - Test results with metadata
  - Embedded images (base64)
  - Verification checklist
  - Automated checks status
  - Professional formatting

### JSON Report
- **Location**: Same as HTML
- **Name Pattern**: `{validator}_validation_YYYYMMDD_HHMMSS.json`
- **Contains**:
  - Test data in structured format
  - Programmatic access to results
  - Verification tracking
  - Automation-friendly format

## Verification Workflow

When you run with `--verify`:

1. **Interactive Prompt** appears for each test
2. **Image Preview** shows automatically (opens in appropriate viewer)
3. **Checklist Items** presented one at a time
4. **Verdict Recording** saves your approval/rejection
5. **Session File** persists state for resumption

### Verification Checklists

**Format Validator (4 items)**:
1. Image loaded successfully
2. Colors appear correct (no corruption)
3. No visual artifacts from format conversion
4. Dimensions preserved

**Aspect Validator (5 items)**:
1. Output dimensions are exactly target (320x256)
2. Image quality is acceptable (no severe artifacts)
3. Image is centered (equal cropping on all sides)
4. Aspect ratio transformation matches mode
5. No unexpected padding or distortion

## Session Files

Verification sessions saved to:
- **Format Validator**: `format_verification_session.json`
- **Aspect Validator**: `aspect_verification_session.json`

These files allow you to:
- Resume interrupted verification sessions
- Track which tests have been approved
- Review previous verdicts
- Export verification results

## Sample Commands for Testing

### Quick Validation without Reports
```bash
python3 format_validator.py
python3 aspect_validator.py output.png center 2000 1600 320 256
```

### Full Validation with All Features
```bash
# Format
python3 format_validator.py --report --verify

# Aspect (all modes)
for mode in center pad stretch; do
  python3 aspect_validator.py output_${mode}.png $mode 1024 768 320 256 --report --verify
done
```

### Automated Report Generation (CI/CD)
```bash
# Format validator
python3 format_validator.py --report

# Move report to results directory
mv format_validation_*.html /path/to/results/

# Aspect validator
python3 aspect_validator.py output.png pad 1024 768 320 256 --report --source input.png
mv aspect_validation_*.html /path/to/results/
```

## Interpreting Results

### Passed Test
```
✓ PNG format validation passed
```
- Test completed successfully
- Image meets all validation criteria
- No errors or warnings

### Failed Test
```
✗ Format validation failed
  Details: {error details here}
```
- Test did not meet validation criteria
- Check Details section for specific failure reason
- May need to fix image generation

## Troubleshooting

### Image Preview Not Opening
The validators automatically detect your system:
- **macOS**: Uses `open -a Preview`
- **Linux**: Uses `xdg-open`
- **Other**: Shows file path for manual viewing

If preview fails:
1. Verify image file exists
2. Check file permissions
3. Use `--verbose` flag for debug info
4. Manually open image file in your image viewer

### Session File Issues
If verification gets stuck:
1. Delete corresponding `*_verification_session.json`
2. Run validator again (will create fresh session)
3. Results will start from beginning

### Report Generation Issues
If report generation fails:
1. Check write permissions in current directory
2. Ensure `test_report_generator.py` is present
3. Check for space issues in filename
4. Run with `--verbose` for detailed error

## Integration with CI/CD

### Automated Report Collection
```bash
#!/bin/bash
REPORT_DIR="test_reports"
mkdir -p $REPORT_DIR

# Run validators
python3 format_validator.py --report
python3 aspect_validator.py output.png center 2000 1600 320 256 --report --source input.png

# Collect reports
mv *.html $REPORT_DIR/
mv *.json $REPORT_DIR/

# Archive
tar -czf $REPORT_DIR.tar.gz $REPORT_DIR/
```

## Next Steps (Week 4)

These enhanced validators will be:
1. **Unified** in Master Test Orchestrator
2. **Aggregated** in Testing Dashboard
3. **Automated** in CI/CD pipeline
4. **Tracked** for historical trend analysis

---

**Created**: Week 3 P1 Enhancement
**Status**: Ready for Production ✅
**Part of**: Week 3 Testing Infrastructure Roadmap
