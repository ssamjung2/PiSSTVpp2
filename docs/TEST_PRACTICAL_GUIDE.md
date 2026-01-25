# PiSSTVpp Testing - Practical Guide

This guide shows how to use the test suite in real-world scenarios.

## Scenario 1: First-Time Testing After Build

You just compiled the program and want to verify it works:

```bash
# Option A: Quick bash test (2-3 minutes)
./test_suite_bash.sh
# or
make test-quick

# Option B: Full Python test (5-8 minutes)
python3 test_suite.py
# or
make test
```

**Expected output**: All tests should PASS

```
✓ PASSED: 58
✗ FAILED: 0
```

## Scenario 2: Regular CI/CD Pipeline

You want to run tests in your CI/CD system:

```bash
# In your CI configuration (GitHub Actions, GitLab CI, Jenkins)
make test-ci

# This will:
# 1. Build pisstvpp2
# 2. Run full test suite
# 3. Generate JSON report
# 4. Exit with appropriate code (0=success, 1=failure)
```

**Output**: JSON report in `test_outputs/test_results_*.json`

```json
{
  "summary": {
    "passed": 58,
    "failed": 0,
    "total": 58
  }
}
```

## Scenario 3: Testing After Making Changes

You modified the source code and want to verify nothing broke:

```bash
# Rebuild and test
make clean && make test

# Or if only testing (already built)
make test-python

# Check results
ls -lh test_outputs/
cat test_outputs/test_results_*.json | grep -E '"passed"|"failed"'
```

## Scenario 4: Testing Specific Features

You want to test only certain functionality:

```bash
# Test only protocols
python3 << 'EOF'
from test_suite import TestSuite
suite = TestSuite()
suite.test_protocols()
suite.print_summary()
EOF

# Test only CW signatures
python3 << 'EOF'
from test_suite import TestSuite
suite = TestSuite()
suite.test_cw_signatures()
suite.test_cw_without_callsign()
suite.print_summary()
EOF

# Test only error handling
python3 << 'EOF'
from test_suite import TestSuite
suite = TestSuite()
suite.test_missing_input()
suite.test_nonexistent_input()
suite.test_invalid_sample_rates()
suite.test_invalid_cw_parameters()
suite.print_summary()
EOF
```

## Scenario 5: Performance Benchmarking

You want to measure encoding time for each protocol:

```bash
#!/bin/bash
# benchmark.sh

echo "Protocol Benchmarks"
echo "===================="

for protocol in m1 m2 s1 s2 sdx r36 r72; do
    echo -n "$protocol: "
    time ./pisstvpp2 -i test_320x240.png -p "$protocol" -o /tmp/bench_$protocol.wav
done
```

## Scenario 6: Testing Different Image Formats

You want to verify the program works with various image types:

```bash
#!/bin/bash
# test_formats.sh

for img in test*.{jpg,png,gif,bmp}; do
    echo "Testing $img..."
    ./pisstvpp2 -i "$img" -o /tmp/test_${img##*/}.wav -v
    if [ -f "/tmp/test_${img##*/}.wav" ]; then
        echo "  ✓ Success"
    else
        echo "  ✗ Failed"
    fi
done
```

## Scenario 7: Stress Testing with Batch Processing

You want to encode many images to find issues:

```bash
#!/bin/bash
# batch_test.sh

test_dir="batch_test"
mkdir -p "$test_dir/output"

# Create diverse test images
python3 << 'EOF'
from PIL import Image, ImageDraw, ImageFont
import os

sizes = [(320, 240), (320, 256), (400, 300)]
colors = ['red', 'green', 'blue', 'white', 'black']

for size in sizes:
    for color in colors:
        img = Image.new('RGB', size, color=color)
        filename = f'batch_test/test_{size[0]}x{size[1]}_{color}.png'
        img.save(filename)
        print(f"Created {filename}")
EOF

# Test encoding of all images
for img in batch_test/test_*.png; do
    output="batch_test/output/$(basename ${img%.png}).wav"
    if ./pisstvpp2 -i "$img" -o "$output"; then
        echo "✓ $img"
    else
        echo "✗ $img"
    fi
done
```

## Scenario 8: Validation After Updates

You updated dependencies and want to ensure compatibility:

```bash
# Rebuild with latest dependencies
make clean
make pisstvpp2

# Run full test suite with verbose output
make test-full

# Review specific results
python3 << 'EOF'
import json
from pathlib import Path

# Find most recent test results
results_file = max(Path("test_outputs").glob("test_results_*.json"))
with open(results_file) as f:
    results = json.load(f)

# Show any failures
failures = [r for r in results['results'] if r['status'] == 'FAILED']
if failures:
    print("FAILURES DETECTED:")
    for f in failures:
        print(f"  - {f['test']}: {f.get('reason', 'Unknown')}")
else:
    print("All tests passed!")
EOF
```

## Scenario 9: Debugging a Specific Test Failure

You got a failure and need to debug:

```bash
#!/bin/bash
# debug_test.sh - Detailed test of Robot 72 protocol

echo "Debugging Robot 72..."

# Create minimal test image
convert -size 320x240 gradient: debug_test.png

# Run with maximum verbosity
./pisstvpp2 -i debug_test.png -p r72 -o debug_r72.wav -v

# Analyze output
echo ""
echo "Output file analysis:"
file debug_r72.wav
ls -lh debug_r72.wav

# Check audio properties
echo ""
echo "Audio properties:"
python3 << 'EOF'
import wave
with wave.open('debug_r72.wav', 'rb') as f:
    print(f"  Channels: {f.getnchannels()}")
    print(f"  Sample rate: {f.getframerate()} Hz")
    print(f"  Frames: {f.getnframes()}")
    print(f"  Duration: {f.getnframes() / f.getframerate():.1f} seconds")
EOF
```

## Scenario 10: Continuous Testing During Development

You want to automatically test after each edit:

```bash
#!/bin/bash
# watch_test.sh - Re-run tests on source changes

while true; do
    # Wait for source file changes
    inotifywait -e modify pisstvpp2.c
    
    # Rebuild and test
    echo "Source changed, rebuilding and testing..."
    make clean && make pisstvpp2 && make test-quick
    
    echo "Done. Waiting for next change..."
done
```

Or using git hooks:

```bash
# .git/hooks/post-commit
#!/bin/bash
# Auto-test after commits

echo "Running test suite..."
make test-quick

if [ $? -ne 0 ]; then
    echo "Tests failed! Review changes."
    exit 1
fi
```

## Scenario 11: Generating Test Reports for Documentation

You want to create a summary of test results:

```bash
#!/bin/bash
# generate_report.sh

echo "# PiSSTVpp Test Report" > test_report.md
echo "Generated: $(date)" >> test_report.md
echo "" >> test_report.md

# Run tests
python3 test_suite.py > /dev/null 2>&1

# Parse results
python3 << 'EOF'
import json
from pathlib import Path

results_file = max(Path("test_outputs").glob("test_results_*.json"))
with open(results_file) as f:
    results = json.load(f)

summary = results['summary']
print(f"\n## Summary\n")
print(f"- Passed: {summary['passed']}")
print(f"- Failed: {summary['failed']}")
print(f"- Total: {summary['total']}")
print(f"- Success Rate: {100*summary['passed']/summary['total']:.1f}%")

# Show failed tests
failures = [r for r in results['results'] if r['status'] == 'FAILED']
if failures:
    print(f"\n## Failed Tests\n")
    for f in failures:
        print(f"- {f['test']}")
EOF >> test_report.md

cat test_report.md
```

## Scenario 12: Testing With Custom Executable Path

You have multiple versions of pisstvpp2:

```bash
# Test with development version
python3 test_suite.py --exe ./pisstvpp2_dev

# Test with optimized version
python3 test_suite.py --exe ./pisstvpp2_O3

# Test with sanitizer version
python3 test_suite.py --exe ./pisstvpp2_san

# Compare results
for exe in pisstvpp2*; do
    echo "Testing $exe..."
    python3 test_suite.py --exe "./$exe" 2>&1 | grep -E "PASSED|FAILED" | tail -1
done
```

## Scenario 13: Testing on Raspberry Pi

You want to test on actual hardware:

```bash
# SSH to Raspberry Pi
ssh pi@192.168.1.100

# Navigate to project
cd ~/projects/PiSSTVpp

# Build
make clean && make pisstvpp2

# Run quick test (uses less resources)
make test-quick

# Or run specific protocol test
./pisstvpp2 -i test_320x240.png -p r72 -o test_r72.wav -v
```

## Scenario 14: Testing Edge Cases

You want to test boundary conditions:

```bash
#!/bin/bash
# edge_case_tests.sh

echo "Edge Case Testing"
echo "================="

# Minimum sample rate
echo "Min sample rate (8000 Hz):"
./pisstvpp2 -i test_320x240.png -r 8000 -o /tmp/edge_min_rate.wav

# Maximum sample rate
echo "Max sample rate (48000 Hz):"
./pisstvpp2 -i test_320x240.png -r 48000 -o /tmp/edge_max_rate.wav

# Very long callsign (max 31 chars)
echo "Max callsign length:"
./pisstvpp2 -i test_320x240.png -C "A1B2C3D4E5F6G7H8I9" -o /tmp/edge_call_long.wav

# Edge WPM values
echo "Min WPM (1):"
./pisstvpp2 -i test_320x240.png -C "TEST" -W 1 -o /tmp/edge_wpm_min.wav

echo "Max WPM (50):"
./pisstvpp2 -i test_320x240.png -C "TEST" -W 50 -o /tmp/edge_wpm_max.wav

# Edge tone frequencies
echo "Min tone (400 Hz):"
./pisstvpp2 -i test_320x240.png -C "TEST" -T 400 -o /tmp/edge_tone_min.wav

echo "Max tone (2000 Hz):"
./pisstvpp2 -i test_320x240.png -C "TEST" -T 2000 -o /tmp/edge_tone_max.wav
```

## Quick Command Reference

```bash
# Build and test
make clean && make test

# Test only
make test-python        # Full Python suite
make test-bash          # Quick bash suite
make test-quick         # Alias for bash
make test-full          # Python with verbose
make test-ci            # CI/CD mode
make test-clean         # Clean outputs

# Manual testing
./pisstvpp2 -h          # Show help
./test_suite_bash.sh    # Run bash tests

# Python suite options
python3 test_suite.py --verbose       # Detailed output
python3 test_suite.py --exe ./custom  # Custom executable
python3 test_suite.py --keep-outputs  # Keep files
```

## Troubleshooting Test Failures

| Problem | Solution |
|---------|----------|
| "No test images found" | Create with: `convert -size 320x240 gradient: test_320x240.png` |
| "Executable not found" | Run `make pisstvpp2` first |
| "WAV file validation failed" | Check file isn't truncated: `file test_outputs/*.wav` |
| "Python test fails" | Verify Python 3.6+: `python3 --version` |
| "Permission denied" | Make executable: `chmod +x test_suite_bash.sh` |

## Performance Expectations

| Operation | Expected Time |
|-----------|---------------|
| Martin 1 encoding | 30-60 seconds |
| Robot 72 encoding | 30-60 seconds |
| Full Python suite | 5-8 minutes |
| Quick bash suite | 2-3 minutes |
| Single protocol | 30-120 seconds |

