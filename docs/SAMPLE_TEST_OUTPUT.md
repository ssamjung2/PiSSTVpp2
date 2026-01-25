# Sample Test Run Output

This document shows example output from running the test suites.

## Sample Python Test Suite Output

```
======================================================================
PiSSTVpp Comprehensive Test Suite
Start time: 2024-01-24 14:35:22
Executable: ./pisstvpp2
Available test images: 5
======================================================================

======================================================================
TEST GROUP: Help and Information
======================================================================
Testing: Help output (-h) ... PASSED

======================================================================
TEST GROUP: Error Handling - Missing Arguments
======================================================================
Testing: Missing input file (-i) ... PASSED
Testing: Non-existent input file ... PASSED

======================================================================
TEST GROUP: SSTV Protocols
======================================================================
Testing: Protocol m1 (Martin 1) ... PASSED
Testing: Protocol m2 (Martin 2) ... PASSED
Testing: Protocol s1 (Scottie 1) ... PASSED
Testing: Protocol s2 (Scottie 2) ... PASSED
Testing: Protocol sdx (Scottie DX) ... PASSED
Testing: Protocol r36 (Robot 36) ... PASSED
Testing: Protocol r72 (Robot 72) ... PASSED

======================================================================
TEST GROUP: Audio Output Formats
======================================================================
Testing: Output format WAV ... PASSED
Testing: Output format AIFF ... PASSED

======================================================================
TEST GROUP: Audio Sample Rates
======================================================================
Testing: Sample rate 8000 Hz ... PASSED
Testing: Sample rate 11025 Hz ... PASSED
Testing: Sample rate 22050 Hz ... PASSED
Testing: Sample rate 32000 Hz ... PASSED
Testing: Sample rate 44100 Hz ... PASSED
Testing: Sample rate 48000 Hz ... PASSED

======================================================================
TEST GROUP: Error Handling - Invalid Sample Rates
======================================================================
Testing: Invalid sample rate 7999 Hz (rejection) ... PASSED
Testing: Invalid sample rate 4000 Hz (rejection) ... PASSED
Testing: Invalid sample rate 48001 Hz (rejection) ... PASSED
Testing: Invalid sample rate 96000 Hz (rejection) ... PASSED
Testing: Invalid sample rate 0 Hz (rejection) ... PASSED
Testing: Invalid sample rate -1 Hz (rejection) ... PASSED

======================================================================
TEST GROUP: Aspect Ratio Modes
======================================================================
Testing: Aspect ratio mode '4:3' ... PASSED
Testing: Aspect ratio mode 'fit' ... PASSED
Testing: Aspect ratio mode 'stretch' ... PASSED

======================================================================
TEST GROUP: Error Handling - Invalid Aspect Modes
======================================================================
Testing: Invalid aspect mode rejection ... PASSED

======================================================================
TEST GROUP: CW Signatures
======================================================================
Testing: CW Signature: Standard K4ABC ... PASSED
Testing: CW Signature: Slow N0CALL 10 WPM ... PASSED
Testing: CW Signature: Fast W5ZZZ 25 WPM ... PASSED
Testing: CW Signature: Maritime with slash ... PASSED

======================================================================
TEST GROUP: Error Handling - CW Validation
======================================================================
Testing: CW -W without -C (error check) ... PASSED
Testing: CW -T without -C (error check) ... PASSED

======================================================================
TEST GROUP: Error Handling - Invalid CW Parameters
======================================================================
Testing: Invalid WPM 0 (rejection) ... PASSED
Testing: Invalid WPM 51 (rejection) ... PASSED
Testing: Invalid WPM -1 (rejection) ... PASSED
Testing: Invalid WPM 100 (rejection) ... PASSED
Testing: Invalid tone 300 Hz (rejection) ... PASSED
Testing: Invalid tone 2100 Hz (rejection) ... PASSED
Testing: Invalid tone 399 Hz (rejection) ... PASSED
Testing: Invalid tone 0 Hz (rejection) ... PASSED
Testing: Invalid tone -1 Hz (rejection) ... PASSED

======================================================================
TEST GROUP: Error Handling - Callsign Validation
======================================================================
Testing: Oversized callsign rejection ... PASSED

======================================================================
TEST GROUP: Error Handling - Invalid Formats
======================================================================
Testing: Invalid format rejection ... PASSED

======================================================================
TEST GROUP: Error Handling - Invalid Protocols
======================================================================
Testing: Invalid protocol rejection ... PASSED

======================================================================
TEST GROUP: Combined Options
======================================================================
Testing: Combined: Robot 72 at 44100 Hz with CW ... PASSED
Testing: Combined: Martin 2 with AIFF and custom aspect ... PASSED
Testing: Combined: Scottie DX with fit mode and verbose ... PASSED

======================================================================
TEST GROUP: Multiple Test Images
======================================================================
Testing: Test image: test_320x240.png ... PASSED
Testing: Test image: test_320x256.png ... PASSED
Testing: Test image: test.jpg ... PASSED

======================================================================
TEST GROUP: Output File Naming
======================================================================
Testing: Auto .wav extension ... PASSED
Testing: Explicit .aiff filename ... PASSED

======================================================================
Test Summary
======================================================================
✓ PASSED: 58
✗ FAILED: 0
⊘ SKIPPED: 0
======================================================================

Results saved to: test_outputs/test_results_20240124_143522.json
```

## Sample JSON Output

```json
{
  "summary": {
    "passed": 58,
    "failed": 0,
    "skipped": 0,
    "total": 58,
    "timestamp": "2024-01-24T14:35:22"
  },
  "results": [
    {
      "test": "Help output (-h)",
      "status": "PASSED"
    },
    {
      "test": "Missing input file (-i)",
      "status": "PASSED",
      "details": "Correctly rejected: Error: Input file (-i) is required"
    },
    {
      "test": "Protocol m1 (Martin 1)",
      "status": "PASSED",
      "details": "Output: 114.5s @ 22050 Hz, 5046750 bytes"
    },
    {
      "test": "Protocol m2 (Martin 2)",
      "status": "PASSED",
      "details": "Output: 57.8s @ 22050 Hz, 2546100 bytes"
    },
    {
      "test": "Protocol s1 (Scottie 1)",
      "status": "PASSED",
      "details": "Output: 110.2s @ 22050 Hz, 4858800 bytes"
    },
    {
      "test": "Protocol s2 (Scottie 2)",
      "status": "PASSED",
      "details": "Output: 70.8s @ 22050 Hz, 3125600 bytes"
    },
    {
      "test": "Protocol sdx (Scottie DX)",
      "status": "PASSED",
      "details": "Output: 269.4s @ 22050 Hz, 11894400 bytes"
    },
    {
      "test": "Protocol r36 (Robot 36)",
      "status": "PASSED",
      "details": "Output: 36.1s @ 22050 Hz, 1592200 bytes"
    },
    {
      "test": "Protocol r72 (Robot 72)",
      "status": "PASSED",
      "details": "Output: 72.2s @ 22050 Hz, 3184400 bytes"
    },
    {
      "test": "Output format WAV",
      "status": "PASSED",
      "details": "Format: WAV, Size: 4994400 bytes"
    },
    {
      "test": "Output format AIFF",
      "status": "PASSED",
      "details": "Format: AIFF, Size: 5025100 bytes"
    },
    {
      "test": "Sample rate 8000 Hz",
      "status": "PASSED",
      "details": "Verified 8000 Hz, 0.1s"
    },
    {
      "test": "Sample rate 48000 Hz",
      "status": "PASSED",
      "details": "Verified 48000 Hz, 30.8s"
    },
    {
      "test": "Invalid sample rate 4000 Hz (rejection)",
      "status": "PASSED",
      "details": "Correctly rejected"
    },
    {
      "test": "Aspect ratio mode '4:3'",
      "status": "PASSED",
      "details": "Output: 114.2s"
    },
    {
      "test": "Aspect ratio mode 'fit'",
      "status": "PASSED",
      "details": "Output: 114.1s"
    },
    {
      "test": "Aspect ratio mode 'stretch'",
      "status": "PASSED",
      "details": "Output: 114.3s"
    },
    {
      "test": "CW Signature: Standard K4ABC",
      "status": "PASSED",
      "details": "Duration: 128.5s"
    },
    {
      "test": "CW Signature: Fast W5ZZZ 25 WPM",
      "status": "PASSED",
      "details": "Duration: 117.2s"
    },
    {
      "test": "Combined: Robot 72 at 44100 Hz with CW",
      "status": "PASSED",
      "details": "Duration: 72.1s @ 44100 Hz"
    }
  ]
}
```

## Sample Bash Test Suite Output

```
========================================================================
PiSSTVpp Quick Test Suite (Bash Version)
========================================================================
Executable: ./pisstvpp2
Output directory: test_outputs
Using test image: ../test_320x240.png

========== BASIC FUNCTIONALITY ==========
Testing: Missing input file (-i required) ... PASS
Testing: Help output (-h) ... PASS

========== PROTOCOL TESTS ==========
Testing protocol: m1 ... PASS (5046750 bytes)
Testing protocol: m2 ... PASS (2546100 bytes)
Testing protocol: s1 ... PASS (4858800 bytes)
Testing protocol: s2 ... PASS (3125600 bytes)
Testing protocol: sdx ... PASS (11894400 bytes)
Testing protocol: r36 ... PASS (1592200 bytes)
Testing protocol: r72 ... PASS (3184400 bytes)

========== SAMPLE RATE TESTS ==========
Testing sample rate: 8000 Hz ... PASS
Testing sample rate: 11025 Hz ... PASS
Testing sample rate: 22050 Hz ... PASS
Testing sample rate: 32000 Hz ... PASS
Testing sample rate: 44100 Hz ... PASS
Testing sample rate: 48000 Hz ... PASS

========== AUDIO FORMAT TESTS ==========
Testing format: wav ... PASS
Testing format: aiff ... PASS

========== ASPECT RATIO TESTS ==========
Testing aspect mode: 4:3 ... PASS
Testing aspect mode: fit ... PASS
Testing aspect mode: stretch ... PASS

========== CW SIGNATURE TESTS ==========
Testing CW signature: K4ABC @ 15 WPM ... PASS
Testing CW signature: N0CALL/MOBILE @ 20 WPM ... PASS
Testing CW signature: K5ABC/MM @ 10 WPM ... PASS

========== ERROR HANDLING TESTS ==========
Testing error: Invalid protocol rejection ... PASS
Testing error: Out-of-range sample rate rejection ... PASS
Testing error: Invalid aspect mode rejection ... PASS

========== COMBINED OPTION TESTS ==========
Testing combined: Robot 72 + 44kHz + AIFF ... PASS
Testing combined: Martin 2 + stretch aspect ... PASS

========================================================================
Test Summary
========================================================================
Passed: 35
Failed: 0
Total:  35
========================================================================

Generated test files:
-rw-r--r--  1 user  staff   2.5M Jan 24 14:35 test_protocol_m1.wav
-rw-r--r--  1 user  staff   1.2M Jan 24 14:35 test_protocol_m2.wav
-rw-r--r--  1 user  staff   2.3M Jan 24 14:35 test_protocol_s1.wav
-rw-r--r--  1 user  staff   1.5M Jan 24 14:35 test_protocol_s2.wav
-rw-r--r--  1 user  staff   5.7M Jan 24 14:35 test_protocol_sdx.wav
-rw-r--r--  1 user  staff   764K Jan 24 14:35 test_protocol_r36.wav
-rw-r--r--  1 user  staff   1.5M Jan 24 14:35 test_protocol_r72.wav
[... more files ...]

Total: 26 test files, ~45 MB
```

## Generated Test File Listing

```bash
$ ls -lh test_outputs/ | head -30

total 45M
-rw-r--r--  test_aspect_4_3.wav              2.5M  Jan 24 14:37
-rw-r--r--  test_aspect_fit.wav              2.5M  Jan 24 14:37
-rw-r--r--  test_aspect_stretch.wav          2.5M  Jan 24 14:37
-rw-r--r--  test_combined_1.wav              4.8M  Jan 24 14:38
-rw-r--r--  test_combined_2.aiff             4.2M  Jan 24 14:38
-rw-r--r--  test_combined_3.wav              5.7M  Jan 24 14:39
-rw-r--r--  test_cw_K4ABC.wav                2.8M  Jan 24 14:39
-rw-r--r--  test_cw_N0CALL_MOBILE.wav        2.6M  Jan 24 14:39
-rw-r--r--  test_cw_W5ZZZ.wav                2.6M  Jan 24 14:39
-rw-r--r--  test_cw_K5ABC_MM.wav             2.8M  Jan 24 14:39
-rw-r--r--  test_format_aiff.aiff            4.2M  Jan 24 14:40
-rw-r--r--  test_format_wav.wav              4.0M  Jan 24 14:40
-rw-r--r--  test_img_0_test_320x240.wav      2.5M  Jan 24 14:40
-rw-r--r--  test_img_1_test_320x256.wav      2.5M  Jan 24 14:40
-rw-r--r--  test_img_2_test.jpg               2.5M  Jan 24 14:40
-rw-r--r--  test_protocol_m1.wav             5.0M  Jan 24 14:35
-rw-r--r--  test_protocol_m2.wav             2.5M  Jan 24 14:35
-rw-r--r--  test_protocol_s1.wav             4.9M  Jan 24 14:36
-rw-r--r--  test_protocol_s2.wav             3.1M  Jan 24 14:36
-rw-r--r--  test_protocol_sdx.wav           11.9M  Jan 24 14:36
-rw-r--r--  test_protocol_r36.wav            1.6M  Jan 24 14:36
-rw-r--r--  test_protocol_r72.wav            3.2M  Jan 24 14:36
-rw-r--r--  test_rate_8000.wav               352K  Jan 24 14:41
-rw-r--r--  test_rate_11025.wav              485K  Jan 24 14:41
-rw-r--r--  test_rate_22050.wav              970K  Jan 24 14:41
-rw-r--r--  test_rate_32000.wav              1.4M  Jan 24 14:41
-rw-r--r--  test_rate_44100.wav              1.9M  Jan 24 14:41
-rw-r--r--  test_rate_48000.wav              2.1M  Jan 24 14:41
-rw-r--r--  test_results_20240124_143522.json 18K  Jan 24 14:42
```

## Timing Example

```bash
$ time make test
gcc -O3 -ffast-math ... pisstvpp2.c -o pisstvpp2 ...
python3 test_suite.py

[Test output as shown above...]

real    7m24s
user    6m45s
sys     0m32s
```

## Success Indicators

A successful test run shows:

✓ All protocol tests PASSED (7 tests)  
✓ All format tests PASSED (2 tests)  
✓ All sample rate tests PASSED (6 valid tests)  
✓ All invalid rate rejections PASSED (6 tests)  
✓ All aspect mode tests PASSED (3 tests)  
✓ All CW signature tests PASSED (4+ tests)  
✓ All error handling tests PASSED (15+ tests)  
✓ Combined option tests PASSED (3 tests)  
✓ JSON report generated with 0 failures

```
✓ PASSED: 58
✗ FAILED: 0
```

## Example: Failure Output

If a test failed, you would see:

```
Testing: Protocol m1 (Martin 1) ... FAILED
  Reason: Command failed with code 1
  Details: Error: Unable to load image
```

And in the JSON:

```json
{
  "test": "Protocol m1 (Martin 1)",
  "status": "FAILED",
  "reason": "Command failed with code 1\n[ERROR] Failed to load image 'test_320x240.png'"
}
```

This would indicate the test image is missing or corrupted.

