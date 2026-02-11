# Error Handling and Edge Case Audit for PiSSTVpp2

## Overview
This document comprehensively audits all error conditions and edge cases in the PiSSTVpp2 application, particularly focusing on the newly refactored config module and error handling system.

## Error Code System Reference

### Supported Error Categories
- **SUCCESS (0):** Operation completed successfully
- **ARGUMENT ERRORS (100-199):** Command-line argument validation failures
- **IMAGE ERRORS (200-299):** Image loading, processing, and format issues
- **SSTV ERRORS (300-399):** SSTV protocol, mode, and encoding issues  
- **AUDIO ERRORS (400-499):** Audio encoding and format issues
- **FILE ERRORS (500-599):** File I/O, path, and format issues
- **SYSTEM ERRORS (600-699):** Memory, system resource issues

---

## SECTION 1: Configuration Module Error Conditions (`pisstvpp2_config.c`)

### 1.1 Input File Validation (`-i` option)

#### Current Coverage
- ✓ Missing input file (no `-i` specified) → `PISSTVPP2_ERR_NO_INPUT_FILE` (111)
- ✓ Input filename too long → `PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG` (120)

#### Potential Edge Cases to Test
| Test Case | Expected Error | Priority | Status |
|-----------|---|---|---|
| Empty input filename string `-i ""` | `PISSTVPP2_ERR_NO_INPUT_FILE` (111) | HIGH | NEED TEST |
| Non-existent input file | Should be caught later by image_load_from_file() returning ERR_FILE_NOT_FOUND (504) | MEDIUM | HAS TEST |
| Input filename exactly 255 chars | Should succeed | LOW | NEED BOUNDARY TEST |
| Input filename 256 chars (max+1) | `PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG` (120) | HIGH | NEED TEST |
| Input file with spaces in name | Should succeed | MEDIUM | NEED TEST |
| Input file with special characters | Should succeed (or fail gracefully) | LOW | NEED TEST |
| Relative path input (./image.jpg) | Should succeed | LOW | NEED TEST |
| Absolute path input (/tmp/image.jpg) | Should succeed | LOW | NEED TEST |
| Input path with ~/ expansion | Depends on shell, may not occur in config module | LOW | N/A |

### 1.2 Output File Validation (`-o` option)

#### Current Coverage
- ✓ Output filename too long → `PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG` (120)
- ✓ Auto-generation when not specified
- ✓ Extension appending when no extension provided

#### Potential Edge Cases to Test
| Test Case | Expected Error | Priority | Status |
|-----------|---|---|---|
| Empty output filename `-o ""` | Should be treated as "not specified" for auto-generation | MEDIUM | NEED TEST |
| Output filename exactly 255 chars | Should succeed | LOW | NEED BOUNDARY TEST |
| Output filename 256 chars (max+1) | `PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG` (120) | HIGH | NEED TEST |
| Output with already-correct extension `.wav` | Should not double-append | MEDIUM | NEED TEST |
| Output with wrong extension `.txt` | Should append format and get `.txt.wav` or similar | MEDIUM | NEED TEST |
| Output with no extension | Should append `.{format}` | MEDIUM | LIKELY WORKS |
| Output filename exceeds limit after extension append | `PISSTVPP2_ERR_ARG_FILENAME_TOO_LONG` (120) | HIGH | NEED TEST |
| Output in non-existent directory | Should fail at file write time with `PISSTVPP2_ERR_FILE_OPEN` (501) | MEDIUM | NEED TEST |
| Output with insufficient write permissions | Should fail at file write time with `PISSTVPP2_ERR_FILE_PERMISSION` (506) | MEDIUM | NEED TEST |

### 1.3 SSTV Protocol Validation (`-p` option)

#### Current Coverage
- ✓ Invalid protocol → `PISSTVPP2_ERR_ARG_INVALID_PROTOCOL` (112)
- ✓ Valid protocols: m1, m2, s1, s2, sdx, r36, r72

#### Potential Edge Cases to Test
| Test Case | Expected Error | Priority | Status |
|-----------|---|---|---|
| Empty protocol string `-p ""` | `PISSTVPP2_ERR_ARG_INVALID_PROTOCOL` (112) | HIGH | NEED TEST |
| Uppercase protocol `-p M1` | Should fail (case-sensitive) → `PISSTVPP2_ERR_ARG_INVALID_PROTOCOL` (112) | MEDIUM | NEED TEST |
| Mixed case `-p Sdx` | Should fail → `PISSTVPP2_ERR_ARG_INVALID_PROTOCOL` (112) | MEDIUM | NEED TEST |
| Protocol with spaces `-p "m1 "` | Should fail → `PISSTVPP2_ERR_ARG_INVALID_PROTOCOL` (112) | MEDIUM | NEED TEST |
| Typo protocol `-p m12` | `PISSTVPP2_ERR_ARG_INVALID_PROTOCOL` (112) | LOW | NEED TEST |
| Numeric instead of protocol `-p 44` | Should fail (VIS code, not protocol string) → `PISSTVPP2_ERR_ARG_INVALID_PROTOCOL` (112) | LOW | NEED TEST |
| All valid protocols (m1,m2,s1,s2,sdx,r36,r72) | All should succeed | LOW | LIKELY OK |

### 1.4 Audio Format Validation (`-f` option)

#### Current Coverage
- ✓ Invalid format → `PISSTVPP2_ERR_ARG_INVALID_FORMAT` (113)
- ✓ Valid formats: wav, aiff, ogg (if built with support)
- ✓ Format validation via `audio_encoder_is_format_supported()`

#### Potential Edge Cases to Test
| Test Case | Expected Error | Priority | Status |
|-----------|---|---|---|
| Empty format string `-f ""` | `PISSTVPP2_ERR_ARG_INVALID_FORMAT` (113) | HIGH | NEED TEST |
| Uppercase format `-f WAV` | Should fail (case-sensitive) → `PISSTVPP2_ERR_ARG_INVALID_FORMAT` (113) | LOW | NEED TEST |
| Format with spaces `-f "wav "` | Should fail → `PISSTVPP2_ERR_ARG_INVALID_FORMAT` (113) | LOW | NEED TEST |
| Format: vorbis (alt name for ogg) | May be invalid if strictly "ogg" required | LOW | NEED TEST |
| Format: mp3 (unsupported) | `PISSTVPP2_ERR_ARG_INVALID_FORMAT` (113) | LOW | NEED TEST |
| Format: flac (unsupported) | `PISSTVPP2_ERR_ARG_INVALID_FORMAT` (113) | LOW | NEED TEST |
| OGG format without libvorbis compiled | If HAVE_OGG_SUPPORT not set, should fail | MEDIUM | NEED TEST |

### 1.5 Sample Rate Validation (`-r` option)

#### Current Coverage
- ✓ Invalid sample rate (non-numeric) → `PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE` (114)
- ✓ Out-of-range sample rate → `PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE` (114)
- ✓ Valid range: 8000-48000 Hz
- ✓ Robust parsing with `strtol()`, `errno` checking, `endptr` validation

#### Potential Edge Cases to Test
| Test Case | Expected Error | Priority | Status |
|-----------|---|---|---|
| Non-numeric rate `-r abc` | `PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE` (114) | HIGH | LIKELY WORKS |
| Rate with trailing chars `-r 22050Hz` | `PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE` (114) | MEDIUM | NEED TEST |
| Empty rate `-r ""` | `PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE` (114) | HIGH | NEED TEST |
| Negative rate `-r -22050` | `PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE` (114) | MEDIUM | NEED TEST |
| Rate too low: 7999 Hz | `PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE` (114) | HIGH (boundary) | NEED TEST |
| Rate boundary: 8000 Hz | Should succeed | HIGH (boundary) | NEED TEST |
| Rate boundary: 48000 Hz | Should succeed | HIGH (boundary) | NEED TEST |
| Rate too high: 48001 Hz | `PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE` (114) | HIGH (boundary) | NEED TEST |
| Very large number > INT_MAX | Should fail (strtol overflow) | LOW | NEED TEST |
| Float rate `-r 22050.5` | `PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE` (114) | MEDIUM | NEED TEST |
| Rate with leading spaces `-r " 22050"` | Depends on strtol() behavior (likely succeeds as strtol skips whitespace) | LOW | NEED TEST |
| Rate: 0 Hz | `PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE` (114) | MEDIUM | NEED TEST |
| Rate: negative but in valid range (impossible) | N/A | N/A | N/A |

### 1.6 Aspect Mode Validation (`-a` option)

#### Current Coverage
- ✓ Invalid aspect mode → `PISSTVPP2_ERR_ARG_INVALID_ASPECT` (115)
- ✓ Valid modes: center, pad, stretch

#### Potential Edge Cases to Test
| Test Case | Expected Error | Priority | Status |
|-----------|---|---|---|
| Empty aspect mode `-a ""` | `PISSTVPP2_ERR_ARG_INVALID_ASPECT` (115) | HIGH | NEED TEST |
| Uppercase mode `-a CENTER` | Should fail → `PISSTVPP2_ERR_ARG_INVALID_ASPECT` (115) | LOW | NEED TEST |
| Mixed case `-a Center` | Should fail → `PISSTVPP2_ERR_ARG_INVALID_ASPECT` (115) | LOW | NEED TEST |
| Mode with spaces `-a " center"` | Should fail → `PISSTVPP2_ERR_ARG_INVALID_ASPECT` (115) | LOW | NEED TEST |
| Typo `-a centre` (British spelling) | `PISSTVPP2_ERR_ARG_INVALID_ASPECT` (115) | LOW | NEED TEST |
| Duplicate modes repeated 10x each | Only most recent used | LOW | NEED TEST |

### 1.7 CW Callsign Validation (`-C` option)

#### Current Coverage
- ✓ Callsign too long (>31 chars) → `PISSTVPP2_ERR_ARG_CALLSIGN_INVALID` (116)
- ✓ Valid callsigns: up to 31 characters

#### Potential Edge Cases to Test
| Test Case | Expected Error | Priority | Status |
|-----------|---|---|---|
| Empty callsign `-C ""` | Should succeed (CW disabled) or be treated as enabled with empty callsign | MEDIUM | NEED TEST |
| Callsign exactly 31 chars | Should succeed | HIGH (boundary) | NEED TEST |
| Callsign 32 chars (max+1) | `PISSTVPP2_ERR_ARG_CALLSIGN_INVALID` (116) | HIGH (boundary) | NEED TEST |
| Callsign with spaces `-C "N0 CALL"` | Depends on implementation (likely succeeds), 7 chars | LOW | NEED TEST |
| Callsign with special chars `-C "N0-CALL"` | Likely succeeds, but may cause CW encoding issues | LOW | NEED TEST |
| Callsign with numbers `-C "N0CALL123"` | Should succeed | LOW | NEED TEST |
| Callsign with lowercase `-C "n0call"` | Should succeed (Morse is case-insensitive) | LOW | NEED TEST |

### 1.8 CW WPM Validation (`-W` option)

#### Current Coverage  
- ✓ Invalid WPM (non-numeric) → `PISSTVPP2_ERR_ARG_CW_INVALID_WPM` (117)
- ✓ Out-of-range WPM → `PISSTVPP2_ERR_ARG_CW_INVALID_WPM` (117)
- ✓ Valid range: 1-50 WPM
- ✓ Uses `atoi()` (less robust than `strtol()` but simple)

#### Potential Edge Cases to Test
| Test Case | Expected Error | Priority | Status |
|-----------|---|---|---|
| Non-numeric WPM `-W abc` | `PISSTVPP2_ERR_ARG_CW_INVALID_WPM` (117) | HIGH | NEED TEST |
| WPM: 0 | `PISSTVPP2_ERR_ARG_CW_INVALID_WPM` (117) | MEDIUM | NEED TEST |
| WPM boundary: 1 | Should succeed | HIGH (boundary) | NEED TEST |
| WPM boundary: 50 | Should succeed | HIGH (boundary) | NEED TEST |
| WPM too high: 51 | `PISSTVPP2_ERR_ARG_CW_INVALID_WPM` (117) | HIGH (boundary) | NEED TEST |
| Negative WPM `-W -15` | `PISSTVPP2_ERR_ARG_CW_INVALID_WPM` (117) | MEDIUM | NEED TEST |
| WPM with decimal `-W 15.5` | `atoi()` will return 15 (truncates), then check passes | MEDIUM | NEED TEST |
| WPM with spaces `-W " 15"` | `atoi()` skips leading spaces, should work | LOW | NEED TEST |
| Very large WPM > INT_MAX | `atoi()` behavior on overflow | LOW | NEED TEST |
| WPM without callsign | Requires callsign! Should fail validation → `PISSTVPP2_ERR_ARG_CW_MISSING_CALLSIGN` (119) | HIGH | LIKELY WORKS |

### 1.9 CW Tone Frequency Validation (`-T` option)

#### Current Coverage
- ✓ Invalid tone (non-numeric) → `PISSTVPP2_ERR_ARG_CW_INVALID_TONE` (118)
- ✓ Out-of-range tone → `PISSTVPP2_ERR_ARG_CW_INVALID_TONE` (118)  
- ✓ Valid range: 400-2000 Hz
- ✓ Uses `atoi()` (less robust) with explicit range check

#### Potential Edge Cases to Test
| Test Case | Expected Error | Priority | Status |
|-----------|---|---|---|
| Non-numeric tone `-T abc` | `PISSTVPP2_ERR_ARG_CW_INVALID_TONE` (118) | HIGH | NEED TEST |
| Tone: 0 Hz | `PISSTVPP2_ERR_ARG_CW_INVALID_TONE` (118) | MEDIUM | NEED TEST |
| Tone boundary: 399 Hz | `PISSTVPP2_ERR_ARG_CW_INVALID_TONE` (118) | HIGH (boundary) | NEED TEST |
| Tone boundary: 400 Hz | Should succeed | HIGH (boundary) | NEED TEST |
| Tone boundary: 2000 Hz | Should succeed | HIGH (boundary) | NEED TEST |
| Tone too high: 2001 Hz | `PISSTVPP2_ERR_ARG_CW_INVALID_TONE` (118) | HIGH (boundary) | NEED TEST |
| Negative tone `-T -800` | Fails against > 0 check → `PISSTVPP2_ERR_ARG_CW_INVALID_TONE` (118) | MEDIUM | NEED TEST |
| Tone with decimal `-T 800.5` | `atoi()` returns 800 (truncates), passes | MEDIUM | NEED TEST |
| Tone with units `-T 800Hz` | `atoi()` returns 800, passes | MEDIUM | NEED TEST |
| Very large tone > INT_MAX | `atoi()` overflow behavior | LOW | NEED TEST |
| Tone without callsign | Requires callsign! Should fail → `PISSTVPP2_ERR_ARG_CW_MISSING_CALLSIGN` (119) | HIGH | LIKELY WORKS |

### 1.10 Interdependency Validation

#### Current Coverage
- ✓ CW params (-W, -T) without callsign (-C) → `PISSTVPP2_ERR_ARG_CW_MISSING_CALLSIGN` (119)
- ✓ Auto-enable intermediate files in verbose mode

#### Potential Edge Cases to Test
| Test Case | Expected Error | Priority | Status |
|-----------|---|---|---|
| -W without -C | `PISSTVPP2_ERR_ARG_CW_MISSING_CALLSIGN` (119) | HIGH | LIKELY WORKS |
| -T without -C | `PISSTVPP2_ERR_ARG_CW_MISSING_CALLSIGN` (119) | HIGH | LIKELY WORKS |
| -W -T both without -C | `PISSTVPP2_ERR_ARG_CW_MISSING_CALLSIGN` (119) | HIGH | LIKELY WORKS |
| -C alone (no -W or -T) | Should succeed with defaults (WPM=15, Tone=800) | LOW | LIKELY WORKS |
| -v and -Z together | Should succeed, Z implies v | LOW | LIKELY WORKS |
| Duplicate options on command line | Last one wins typically | LOW | NEED TEST |

### 1.11 Feature Interaction Tests

#### Current Coverage
- ✓ Verbose mode auto-enables intermediate file keeping
- ✓ Timestamp logging auto-enables verbose mode

#### Potential Edge Cases to Test
| Test Case | Expected Behavior | Priority | Status |
|-----------|---|---|---|
| `-v` (verbose) | Intermediate files kept automatically | MEDIUM | LIKELY WORKS |
| `-Z` (timestamp) | Implies `-v`, intermediate files kept | MEDIUM | LIKELY WORKS |
| `-K` (keep intermediate) | Works independently | LOW | LIKELY WORKS |
| `-K -v -Z` all together | All should work together | LOW | NEED TEST |

---

## SECTION 2: Image Loading Error Conditions

### 2.1 Image File Errors
- Non-existent file → `PISSTVPP2_ERR_FILE_NOT_FOUND` (504)
- Unreadable file (permissions) → `PISSTVPP2_ERR_FILE_PERMISSION` (506)
- Unsupported format → `PISSTVPP2_ERR_IMAGE_FORMAT_UNSUPPORTED` (202)
- Corrupted image file → `PISSTVPP2_ERR_IMAGE_LOAD` (201)

### 2.2 Potential Edge Cases
- Very large image (memory exhaustion) → `PISSTVPP2_ERR_MEMORY_ALLOC` (601) or `PISSTVPP2_ERR_IMAGE_MEMORY` (206)
- Very small image (< 320x240) → May cause aspect correction issues
- Images with unusual aspect ratios (extreme tall/wide) → Should still work with aspect correction modes
- Corrupted but partially readable image → Depends on libvips robustness

---

## SECTION 3: SSTV Encoding Error Conditions

### 3.1 SSTV Module Errors
- Invalid VIS code (from protocol mapping) → `PISSTVPP2_ERR_SSTV_MODE_NOT_FOUND` (302)
- SSTV memory issues → `PISSTVPP2_ERR_SSTV_INIT` (303)
- CW encoding issues → `PISSTVPP2_ERR_SSTV_CW` (307)

### 3.2 Potential Edge Cases
- Very small sample rate (8000 Hz) with long CW signature → May run out of buffer space
- Sample rate mismatch between SSTV and audio encoder
- CW signature that generates too many samples

---

## SECTION 4: Audio Encoding Error Conditions

### 4.1 Audio Encoder Errors
- Format-specific initialization: `PISSTVPP2_ERR_AUDIO_WAV` (405), `PISSTVPP2_ERR_AUDIO_AIFF` (406), `PISSTVPP2_ERR_AUDIO_OGG` (407)
- File write errors: `PISSTVPP2_ERR_FILE_WRITE` (503), `PISSTVPP2_ERR_FILE_DISK_FULL` (507)
- Sample count exceeds buffer: `PISSTVPP2_ERR_SYSTEM_RESOURCE` (602)

### 4.2 Potential Edge Cases
- Output directory doesn't exist → File open fails
- Disk full during write → Incomplete file written
- Insufficient permissions on output directory
- OGG without libvorbis → Should fail gracefully

---

## SECTION 5: Test Coverage Summary

### Currently Tested ✓
- Help/version info
- Missing input file
- Non-existent input file  
- Basic protocol testing
- Basic format testing
- Basic sample rate testing
- Basic aspect mode testing
- CW signature features
- CW without callsign validation
- Invalid CW parameters
- Long callsign testing
- Invalid format testing
- Invalid protocol testing
- Combined option testing
- Multiple image handling
- Output file naming

### Critical Gaps (HIGH Priority)
- Boundary testing for all numeric parameters (sample rate, CW WPM, CW tone)
- Filename length boundaries (255 char tests)
- Empty string inputs
- Output filename too long after extension append
- Very large numeric inputs (overflow testing)
- Directory/permission errors
- Disk full conditions
- Format auto-generation and extension handling

### Important Gaps (MEDIUM Priority)
- Case sensitivity testing (uppercase/mixed case protocols, formats, modes)
- Parameter combinations with spaces/special characters
- Non-existent output directory
- Write permission errors
- Feature interactions (verbose + timestamp + keep)
- Duplicate parameter specifications

### Nice-to-Have Gaps (LOW Priority)
- Locale/internationalization testing
- Unicode in filenames/callsigns
- Extremely large numbers (overflow)
- OGG support detection

---

## Recommended Test Implementation

### Phase 1: Boundary Value Testing (HIGH Priority)
Create tests for all numeric boundaries:
- Sample rate: 7999, 8000, 48000, 48001
- CW WPM: 0, 1, 50, 51  
- CW Tone: 0, 399, 400, 2000, 2001
- Filenames: 255, 256 chars
- Callsign: 31, 32 chars

### Phase 2: Error Condition Testing (HIGH Priority)
Create tests verifying exact error codes returned:
- Empty input strings for each parameter
- Parameter interdependencies  
- Invalid combos of parameters
- Check that error_log() is called correctly

### Phase 3: Feature Integration Testing (MEDIUM Priority)
- Test verbose/timestamp/keep interactions
- Test format auto-generation with various extensions
- Test output filename generation edge cases

### Phase 4: Robustness Testing (MEDIUM Priority)
- File I/O error conditions
- Permission errors
- Disk full scenarios
- Memory exhaustion scenarios

---

## Implementation Notes

1. **Error Code Consistency**: Verify all functions in config module return correct error codes
2. **Error Logging**: Ensure every error path calls `error_log()` with proper context
3. **Boundary Testing**: Use automated test generation for boundary value analysis
4. **Negative Testing**: Systematically test invalid inputs for each parameter
5. **Integration Testing**: Test parameter combinations, not just individual parameters
6. **Regression Testing**: Ensure new tests don't break existing functionality

