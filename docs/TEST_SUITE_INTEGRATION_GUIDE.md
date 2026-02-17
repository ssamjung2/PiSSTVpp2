# Test Suite Integration Guide
## How to Use Week 1 Infrastructure in Your Test Suite

Quick reference for integrating `TestReportGenerator` and `HumanVerifier` into existing or new test suites.

---

## 5-Minute Integration

### Step 1: Import the modules

```python
from test_report_generator import TestReportGenerator
from human_verification_framework import HumanVerifier
from test_metadata import TestMetadata
```

### Step 2: Create test metadata

```python
metadata = TestMetadata(
    test_id="my_test",
    test_number=1,
    title="My Test Title",
    category="text_overlay",  # or "aspect_ratio", "image_format"
    purpose="What this test validates",
    expected_outcome="What should happen",
    input_image="input.png",
    input_width=1920,
    input_height=1080,
    output_width=320,
    output_height=256,
    # verification_checklist auto-generated based on category!
)
```

### Step 3: Initialize reporter

```python
generator = TestReportGenerator("My Test Suite Name")
```

### Step 4: Run test and add result

```python
# Run your test
command = "./bin/slowframe -i input.png -o output.wav"
status = run_test(command)  # Your existing test logic

# Add to report
generator.add_test_result(
    test_id="T001",
    name="my_test",
    suite="my_suite",
    status="passed" if status else "failed",
    command=command,
    metadata=metadata,
    output_files={
        "Input": "input.png",
        "Output": "output.png"
    },
    automated_checks={
        "File exists": os.path.exists("output.png"),
        "Correct dimensions": check_dimensions("output.png")
    },
    execution_time=elapsed_time
)
```

### Step 5: Generate reports

```python
generator.generate_html_report("my_test_report.html")
generator.generate_json_report("my_test_report.json")
```

**That's it!** You now have professional HTML and JSON reports.

---

## Adding Human Verification (Optional)

### Step 1: Initialize verifier

```python
verifier = HumanVerifier(
    session_file="my_verification_session.json",
    auto_preview=True,  # Auto-open images
    verifier_name="your_name"  # Optional
)
```

### Step 2: Add verification to your test loop

```python
for test in tests:
    # ... run test ...
    
    # Human verification
    verification = verifier.verify_test(
        test_id=test.id,
        test_name=test.name,
        metadata=test.metadata,
        images={"Output": test.output_file},
        checklist=test.metadata.verification_checklist,
        command=test.command
    )
    
    # Update report with verification
    result.human_verified = True
    result.human_verdict = verification.verdict
    result.human_notes = verification.notes
```

### Step 3: Save session

```python
verifier.save_session()
verifier.print_summary()
```

---

## Complete Example: Text Overlay Test

```python
#!/usr/bin/env python3
"""Example: Integrating new infrastructure into text overlay tests"""

import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent / "util"))

from test_report_generator import TestReportGenerator
from human_verification_framework import HumanVerifier
from test_metadata import TestMetadata


def main():
    # Initialize
    generator = TestReportGenerator("Text Overlay Tests")
    verifier = HumanVerifier(session_file="tests/overlay_verification.json")
    
    # Define tests
    tests = [
        {
            "metadata": TestMetadata(
                test_id="overlay_white_bottom",
                test_number=1,
                title="White Text at Bottom",
                category="text_overlay",
                purpose="Verify white text renders at bottom",
                expected_outcome="White text visible at bottom of image",
                input_image="test.png",
                input_width=1920,
                input_height=1080,
                output_width=320,
                output_height=256
            ),
            "command": "./bin/slowframe -i test.png -t 'N0CALL' -tc white -tp bottom -o output.wav"
        }
    ]
    
    # Run tests
    for i, test in enumerate(tests, 1):
        test_id = f"T{i:03d}"
        metadata = test["metadata"]
        command = test["command"]
        
        # Run test (your existing logic)
        status = run_slowframe_test(command)
        
        # Add to report
        result = generator.add_test_result(
            test_id=test_id,
            name=metadata.test_id,
            suite="text_overlay",
            status="passed" if status else "failed",
            command=command,
            metadata=metadata,
            output_files={
                "Output": f"tests/test_outputs/overlay/{test_id}_output.png"
            },
            automated_checks={
                "No errors": status,
                "Output exists": os.path.exists(f"tests/test_outputs/overlay/{test_id}_output.png")
            }
        )
        
        # Human verification
        verification = verifier.verify_test(
            test_id=test_id,
            test_name=metadata.title,
            metadata=metadata,
            images={"Output": f"tests/test_outputs/overlay/{test_id}_output.png"},
            checklist=metadata.verification_checklist,
            command=command
        )
        
        # Update result
        result.human_verified = True
        result.human_verdict = verification.verdict
        result.human_notes = verification.notes
    
    # Generate final reports
    generator.generate_html_report("tests/overlay_test_report.html")
    generator.generate_json_report("tests/overlay_test_report.json")
    
    # Summary
    verifier.print_summary()


if __name__ == "__main__":
    main()
```

---

## Command-Line Flags Pattern

Add these flags to your test suite for flexibility:

```python
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--html-report", default="report.html", help="HTML report path")
parser.add_argument("--json-report", default="report.json", help="JSON report path")
parser.add_argument("--verify", action="store_true", help="Enable human verification")
parser.add_argument("--auto-preview", action="store_true", help="Auto-open images for verification")
args = parser.parse_args()

# Use in code
if args.verify:
    verifier = HumanVerifier(auto_preview=args.auto_preview)
    # ... verification workflow ...

generator.generate_html_report(args.html_report)
generator.generate_json_report(args.json_report)
```

**Usage:**
```bash
# Just run tests and generate reports
python3 my_test_suite.py

# Run tests with human verification
python3 my_test_suite.py --verify

# Custom report paths
python3 my_test_suite.py --html-report custom_report.html

# Auto-open images during verification
python3 my_test_suite.py --verify --auto-preview
```

---

## Checklist Auto-Generation

The `TestMetadata` class automatically generates verification checklists based on the `category` field:

| Category | Auto-Generated Checklist Includes |
|----------|-----------------------------------|
| `aspect_ratio` | Dimension checks, padding verification (mode-specific), centering, image quality |
| `text_overlay` | Text rendering, color accuracy, placement, readability, artifacts, styling |
| `image_format` | Load success, color preservation, dimension preservation, artifacts |
| Other | Generic quality checks |

**Override if needed:**
```python
metadata = TestMetadata(
    ...
    category="text_overlay",
    verification_checklist=[
        "Custom check 1",
        "Custom check 2"
    ]  # Overrides auto-generation
)
```

---

## Image Annotation Example

Annotate output images with test information:

```python
generator.annotate_image(
    image_path="output.png",
    output_path="output_annotated.png",
    test_id="T001",
    annotations={
        "Mode": "PAD",
        "Protocol": "M1",
        "Dimensions": "320x256"
    }
)

# Use annotated image in report
generator.add_test_result(
    ...
    output_files={
        "Output": "output_annotated.png"
    }
)
```

---

## Session Persistence

The `HumanVerifier` automatically saves progress:

```python
verifier = HumanVerifier(session_file="my_session.json")

# Verify tests
for test in tests:
    verifier.verify_test(...)
    # Automatically saves after each test

# If interrupted (Ctrl+C or 'q'), session is saved
# Resume later:
verifier = HumanVerifier(session_file="my_session.json")
# Loads previous progress
# Only unverified tests will be prompted
```

---

## Best Practices

### 1. **Consistent Test IDs**
```python
# Good: Sequential, zero-padded
test_id = f"T{i:03d}"  # T001, T002, ...

# Bad: Inconsistent
test_id = f"test{i}"   # test1, test2, ...
```

### 2. **Descriptive Metadata**
```python
# Good
metadata = TestMetadata(
    title="PAD Mode with Wide Image - M1 Protocol",
    purpose="Verify PAD mode adds black letterbox bars for wide images",
    expected_outcome="Image centered with black bars on top/bottom"
)

# Bad
metadata = TestMetadata(
    title="Test 1",
    purpose="Testing",
    expected_outcome="Works"
)
```

### 3. **Include All Output Files**
```python
# Good: Include input, output, annotated
output_files={
    "Input": "input.png",
    "Output": "output.png",
    "Annotated": "output_annotated.png"
}

# Okay: Just output
output_files={"Output": "output.png"}
```

### 4. **Meaningful Automated Checks**
```python
# Good: Specific, actionable
automated_checks={
    "Output dimensions 320x256": width == 320 and height == 256,
    "Padding color is black": padding_color == (0, 0, 0),
    "No encoding errors": exit_code == 0
}

# Bad: Too vague
automated_checks={
    "Test passed": True
}
```

### 5. **Use Verification for Visual Tests**
```python
# Text overlay, image quality → Human verification required
if test.category in ["text_overlay", "image_format"]:
    verifier.verify_test(...)

# Error codes, dimensions → Automated sufficient
else:
    # Skip human verification
```

---

## Troubleshooting

### Images don't open for preview
- **macOS:** Ensure `open` command works: `open test.png`
- **Linux:** Ensure `xdg-open` is installed: `apt-get install xdg-utils`
- **Workaround:** Set `auto_preview=False` and open manually

### HTML report images not showing
- Check file paths are correct
- Ensure images exist before calling `add_test_result()`
- Check browser console for errors

### Session file not saving
- Ensure parent directory exists: `Path(session_file).parent.mkdir(exist_ok=True)`
- Check file permissions

### Checklists not generating
- Ensure `category` is set correctly
- Valid categories: `"aspect_ratio"`, `"text_overlay"`, `"image_format"`
- Override with custom checklist if needed

---

## FAQ

**Q: Do I need to use both TestReportGenerator and HumanVerifier?**  
A: No. `TestReportGenerator` is standalone for automated reporting. Add `HumanVerifier` only for tests requiring visual verification.

**Q: Can I use just the HTML reports without JSON?**  
A: Yes. Just call `generate_html_report()` and skip `generate_json_report()`.

**Q: How do I integrate with existing test suites?**  
A: Wrap your existing test execution in the new reporting calls. The core test logic doesn't change.

**Q: Can I customize the HTML styling?**  
A: Currently, styling is built-in. For custom styling, modify `_generate_html_content()` in `test_report_generator.py`.

**Q: What if I want to verify tests after running them?**  
A: Save test results to JSON, then run verification separately using the saved results.

---

## Support & Examples

- **Demo:** `tests/util/week1_infrastructure_demo.py`
- **Full Documentation:** `docs/WEEK1_INFRASTRUCTURE_COMPLETE.md`
- **Example Reports:** `tests/demo_report.html`, `tests/demo_integrated_report.html`
- **Test Metadata:** `tests/util/test_metadata.py` (includes all checklist logic)

---

**Ready to integrate?** Start with the 5-minute integration above, then refer back to this guide for advanced features.
