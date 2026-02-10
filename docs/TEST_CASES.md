# PiSSTVpp2 Test Cases Reference

**Detailed specification of all test cases in the PiSSTVpp2 test suite**

---

## Table of Contents

1. [Test Case Format](#test-case-format)
2. [Help and Information Tests](#help-and-information-tests)
3. [Error Handling Tests](#error-handling-tests)
4. [SSTV Protocol Tests](#sstv-protocol-tests)
5. [Audio Format Tests](#audio-format-tests)
6. [Sample Rate Tests](#sample-rate-tests)
7. [Aspect Ratio Tests](#aspect-ratio-tests)
8. [CW Identification Tests](#cw-identification-tests)
9. [Image Handling Tests](#image-handling-tests)
10. [Combination Tests](#combination-tests)

---

## Test Case Format

Each test case follows this structure:

```
### Test Case ID: [Category].[Number]

**Name**: [Descriptive test name]

**Description**: What is being tested and why

**Prerequisites**: 
- Compiled binary in bin/pisstvpp2
- Test image(s) available
- [Other requirements]

**Test Steps**:
1. Command to execute
2. Parameters to use
3. Wait for completion

**Expected Result**:
- File created: [filename]
- Exit code: [expected code]
- [Other assertions]

**Acceptance Criteria**:
- ✓ File exists and is readable
- ✓ File size is > [minimum size]
- ✓ Audio duration is approximately [expected] seconds
- ✓ [Other criteria]

**Notes**: Any special considerations or platform-specific details
```

---

## Help and Information Tests

### Test Case ID: HELP.001

**Name**: Display Help Message

**Description**: Verify that the `-h` flag displays comprehensive help text

**Prerequisites**:
- Binary compiled and executable
- No specific test images required

**Test Steps**:
```bash
./bin/pisstvpp2 -h
```

**Expected Result**:
- Exit code: 0
- Stdout contains: "Usage:", "REQUIRED OPTIONS", "OPTIONAL OPTIONS", "EXAMPLES"
- Help text includes all 7 protocol names (m1, m2, s1, s2, sdx, r36, r72)
- Help text includes all 3 formats (wav, aiff, ogg)
- No error messages on stderr

**Acceptance Criteria**:
- ✓ Help text displays completely
- ✓ All command options documented
- ✓ All SSTV modes listed with VIS codes
- ✓ All audio formats explained
- ✓ Examples provided

**Notes**: Should run in < 1 second

---

### Test Case ID: HELP.002

**Name**: Help Without Arguments

**Description**: Verify proper behavior when no arguments provided (should show usage)

**Prerequisites**:
- Binary compiled

**Test Steps**:
```bash
./bin/pisstvpp2
```

**Expected Result**:
- Exit code: 1 (error)
- Error message mentions missing "-i" argument
- Usage hint displayed

**Acceptance Criteria**:
- ✓ Error message clear and helpful
- ✓ Program exits with non-zero status
- ✓ No core dump or crash

**Notes**: User-friendly error message is important

---

## Error Handling Tests

### Test Case ID: ERR.001

**Name**: Missing Input File Argument

**Description**: Verify error when required `-i` argument is missing

**Prerequisites**:
- Binary compiled
- Test image available

**Test Steps**:
```bash
./bin/pisstvpp2 -o output.wav
```

**Expected Result**:
- Exit code: 1
- Stderr contains error about missing input file
- No output file created

**Acceptance Criteria**:
- ✓ Clear error message
- ✓ Proper exit code
- ✓ No partial output files created

**Notes**: This is the most common user error

---

### Test Case ID: ERR.002

**Name**: Non-existent Input File

**Description**: Verify error handling when input file doesn't exist

**Prerequisites**:
- Binary compiled

**Test Steps**:
```bash
./bin/pisstvpp2 -i /nonexistent/path/image.jpg -o output.wav
```

**Expected Result**:
- Exit code: 2
- Stderr contains error about file not found
- No output file created

**Acceptance Criteria**:
- ✓ File-not-found error caught
- ✓ Proper exit code
- ✓ No output artifacts

**Notes**: Should handle missing files gracefully

---

### Test Case ID: ERR.003

**Name**: Invalid Protocol Code

**Description**: Verify error when unknown SSTV protocol specified

**Prerequisites**:
- Binary compiled
- Valid test image available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -p invalid_mode -o output.wav
```

**Expected Result**:
- Exit code: 1
- Stderr mentions unknown protocol
- No output file created

**Acceptance Criteria**:
- ✓ Protocol validation works
- ✓ Clear error message
- ✓ No file created

**Notes**: Should suggest valid protocols in error message

---

### Test Case ID: ERR.004

**Name**: Invalid Audio Format

**Description**: Verify error when unsupported audio format specified

**Prerequisites**:
- Binary compiled
- Valid test image

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -f mp3 -o output.mp3
```

**Expected Result**:
- Exit code: 1
- Stderr mentions unsupported format
- No output file created

**Acceptance Criteria**:
- ✓ Format validation works
- ✓ Only wav, aiff, ogg accepted
- ✓ Clear error

**Notes**: Format auto-detection from filename might work

---

### Test Case ID: ERR.005

**Name**: Sample Rate Too Low

**Description**: Verify error when sample rate below minimum

**Prerequisites**:
- Binary compiled
- Valid test image

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -r 4000 -o output.wav
```

**Expected Result**:
- Exit code: 1
- Stderr mentions invalid sample rate
- States valid range: 8000-48000 Hz
- No output file

**Acceptance Criteria**:
- ✓ Validation range enforced
- ✓ Helpful error message
- ✓ Minimum is 8000 Hz

**Notes**: Common user error with low-quality targets

---

### Test Case ID: ERR.006

**Name**: Sample Rate Too High

**Description**: Verify error when sample rate exceeds maximum

**Prerequisites**:
- Binary compiled
- Valid test image

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -r 96000 -o output.wav
```

**Expected Result**:
- Exit code: 1
- Stderr mentions invalid sample rate
- States valid range: 8000-48000 Hz
- No output file

**Acceptance Criteria**:
- ✓ Maximum 48000 Hz enforced
- ✓ Range clearly stated in error
- ✓ No file created

**Notes**: 48 kHz is professional audio standard

---

### Test Case ID: ERR.007

**Name**: CW WPM Without Callsign

**Description**: Verify error when CW speed `-W` specified without callsign `-C`

**Prerequisites**:
- Binary compiled
- Valid test image

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -W 20 -o output.wav
```

**Expected Result**:
- Exit code: 1
- Stderr mentions -W requires -C
- No output file

**Acceptance Criteria**:
- ✓ Dependency validation works
- ✓ Clear error message
- ✓ Suggests using -C

**Notes**: CW features have dependencies

---

### Test Case ID: ERR.008

**Name**: CW Tone Frequency Without Callsign

**Description**: Verify error when CW tone `-T` specified without callsign `-C`

**Prerequisites**:
- Binary compiled
- Valid test image

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -T 1000 -o output.wav
```

**Expected Result**:
- Exit code: 1
- Stderr mentions -T requires -C
- No output file

**Acceptance Criteria**:
- ✓ Dependency validation works
- ✓ Clear error
- ✓ No output

**Notes**: Part of CW validation

---

### Test Case ID: ERR.009

**Name**: Invalid CW WPM (Too Low)

**Description**: Verify error when CW speed below minimum

**Prerequisites**:
- Binary compiled
- Valid test image

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -C "N0CALL" -W 0 -o output.wav
```

**Expected Result**:
- Exit code: 1
- Stderr mentions invalid WPM range
- States: 1-50 WPM valid
- No output

**Acceptance Criteria**:
- ✓ WPM validation works
- ✓ Range clearly stated
- ✓ Minimum is 1 WPM

**Notes**: 1 WPM is very slow but valid

---

### Test Case ID: ERR.010

**Name**: Invalid CW WPM (Too High)

**Description**: Verify error when CW speed exceeds maximum

**Prerequisites**:
- Binary compiled
- Valid test image

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -C "N0CALL" -W 51 -o output.wav
```

**Expected Result**:
- Exit code: 1
- Stderr mentions invalid WPM range
- States: 1-50 WPM valid
- No output

**Acceptance Criteria**:
- ✓ Maximum 50 WPM enforced
- ✓ Range stated
- ✓ No file created

**Notes**: 50 WPM is expert level

---

### Test Case ID: ERR.011

**Name**: Invalid CW Tone (Too Low)

**Description**: Verify error when CW tone frequency below minimum

**Prerequisites**:
- Binary compiled
- Valid test image

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -C "N0CALL" -T 300 -o output.wav
```

**Expected Result**:
- Exit code: 1
- Stderr mentions invalid frequency
- States: 400-2000 Hz valid
- No output

**Acceptance Criteria**:
- ✓ Frequency validation works
- ✓ Minimum 400 Hz clear
- ✓ No file

**Notes**: Below 400 Hz causes audio artifacts

---

### Test Case ID: ERR.012

**Name**: Invalid CW Tone (Too High)

**Description**: Verify error when CW tone frequency exceeds maximum

**Prerequisites**:
- Binary compiled
- Valid test image

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -C "N0CALL" -T 2500 -o output.wav
```

**Expected Result**:
- Exit code: 1
- Stderr mentions invalid frequency
- States: 400-2000 Hz valid
- No output

**Acceptance Criteria**:
- ✓ Maximum 2000 Hz enforced
- ✓ Range clear
- ✓ No output

**Notes**: Above 2000 Hz loses fidelity

---

## SSTV Protocol Tests

### Test Case ID: PROTO.001

**Name**: Martin 1 Mode Encoding

**Description**: Test Martin 1 (m1, VIS 44) SSTV encoding

**Prerequisites**:
- Binary compiled
- test_image.jpg available
- Default sample rate (22050 Hz)

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -p m1 -o m1_output.wav
```

**Expected Result**:
- Exit code: 0
- Output file created: m1_output.wav
- File size > 500 KB
- Duration approximately 114 seconds at 22050 Hz

**Acceptance Criteria**:
- ✓ File created successfully
- ✓ Duration: 113-115 seconds
- ✓ WAV format valid
- ✓ VIS code 44 in transmission
- ✓ 256 lines × 320 pixels encoded

**Notes**: Martin 1 is most widely supported mode; should be default

---

### Test Case ID: PROTO.002

**Name**: Martin 2 Mode Encoding

**Description**: Test Martin 2 (m2, VIS 40) SSTV encoding

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -p m2 -o m2_output.wav
```

**Expected Result**:
- Exit code: 0
- Output file created: m2_output.wav
- Duration approximately 58 seconds
- File size approximately half of m1

**Acceptance Criteria**:
- ✓ File created
- ✓ Duration: 57-59 seconds
- ✓ Twice as fast as m1
- ✓ VIS code 40
- ✓ Same resolution as m1

**Notes**: Martin 2 is faster variant of Martin 1

---

### Test Case ID: PROTO.003

**Name**: Scottie 1 Mode Encoding

**Description**: Test Scottie 1 (s1, VIS 60) SSTV encoding

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -p s1 -o s1_output.wav
```

**Expected Result**:
- Exit code: 0
- Output file created: s1_output.wav
- Duration approximately 110 seconds
- Better sync pulse than Martin 1

**Acceptance Criteria**:
- ✓ File created
- ✓ Duration: 109-111 seconds
- ✓ VIS code 60
- ✓ Improved sync vs Martin
- ✓ 320×256 resolution

**Notes**: Scottie modes have better noise immunity

---

### Test Case ID: PROTO.004

**Name**: Scottie 2 Mode Encoding

**Description**: Test Scottie 2 (s2, VIS 56) SSTV encoding - **recommended default**

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -p s2 -o s2_output.wav
```

**Expected Result**:
- Exit code: 0
- Output file created: s2_output.wav
- Duration approximately 71 seconds
- Good balance of speed and quality

**Acceptance Criteria**:
- ✓ File created
- ✓ Duration: 70-72 seconds
- ✓ VIS code 56
- ✓ User-friendly transmission time
- ✓ High quality output

**Notes**: Recommended for general use - best balance

---

### Test Case ID: PROTO.005

**Name**: Scottie DX Mode Encoding

**Description**: Test Scottie DX (sdx, VIS 76) SSTV encoding - highest quality mode

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -p sdx -o sdx_output.wav
```

**Expected Result**:
- Exit code: 0
- Output file created: sdx_output.wav
- Duration approximately 269 seconds (4.5 minutes)
- Finest detail preservation

**Acceptance Criteria**:
- ✓ File created
- ✓ Duration: 268-270 seconds
- ✓ VIS code 76
- ✓ Longest RGB mode
- ✓ Best color detail

**Notes**: For archival and high-quality transmissions; verify patient will receive

---

### Test Case ID: PROTO.006

**Name**: Robot 36 Mode Encoding

**Description**: Test Robot 36 (r36, VIS 8) SSTV encoding - fastest mode

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -p r36 -o r36_output.wav
```

**Expected Result**:
- Exit code: 0
- Output file created: r36_output.wav
- Duration approximately 36 seconds
- 320×240 resolution (4:3 aspect)

**Acceptance Criteria**:
- ✓ File created
- ✓ Duration: 35-37 seconds
- ✓ VIS code 8
- ✓ YUV 4:2:0 encoding
- ✓ Fast transmission

**Notes**: Fastest mode; good for quick tests and learning

---

### Test Case ID: PROTO.007

**Name**: Robot 72 Mode Encoding

**Description**: Test Robot 72 (r72, VIS 12) SSTV encoding - high-quality fast mode

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -p r72 -o r72_output.wav
```

**Expected Result**:
- Exit code: 0
- Output file created: r72_output.wav
- Duration approximately 72 seconds
- Excellent color fidelity

**Acceptance Criteria**:
- ✓ File created
- ✓ Duration: 71-73 seconds
- ✓ VIS code 12
- ✓ YUV 4:2:2 encoding (better than r36)
- ✓ Good balance

**Notes**: Robot 72 is competitive with RGB modes in quality

---

## Audio Format Tests

### Test Case ID: FMT.001

**Name**: WAV Format Output

**Description**: Test default WAV (16-bit PCM) audio format

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -f wav -o test_output.wav
```

**Expected Result**:
- Exit code: 0
- Output file: test_output.wav
- File readable with standard audio tools
- 16-bit PCM, mono, 22050 Hz

**Acceptance Criteria**:
- ✓ File created and complete
- ✓ WAV header valid (RIFF, fmt, data chunks)
- ✓ Can be played with aplay, afplay, etc.
- ✓ File size correct for duration

**Notes**: WAV is always supported

---

### Test Case ID: FMT.002

**Name**: AIFF Format Output

**Description**: Test AIFF (Apple audio format) output

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -f aiff -o test_output.aiff
```

**Expected Result**:
- Exit code: 0
- Output file: test_output.aiff
- 16-bit PCM, mono, 22050 Hz
- Readable by audio software

**Acceptance Criteria**:
- ✓ File created and complete
- ✓ AIFF header valid (FORM, COMM, SSND chunks)
- ✓ Playable on macOS and other systems
- ✓ Audio content correct

**Notes**: AIFF always supported; native format on macOS

---

### Test Case ID: FMT.003

**Name**: OGG Vorbis Format Output

**Description**: Test OGG Vorbis compressed audio format

**Prerequisites**:
- Binary compiled with OGG support
- test_image.jpg available
- libvorbis installed

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -f ogg -o test_output.ogg
```

**Expected Result**:
- Exit code: 0 (or skip if OGG not supported)
- Output file: test_output.ogg
- Compressed format, ~1/10 file size of WAV
- Readable by standard players

**Acceptance Criteria**:
- ✓ File created if OGG supported
- ✓ File size < 10% of equivalent WAV
- ✓ OGG header valid
- ✓ Playable with OGG-capable player
- ✓ Audio quality preserved in listening tests

**Notes**: Test may skip if libvorbis not compiled in

---

## Sample Rate Tests

### Test Case ID: RATE.001

**Name**: Minimum Sample Rate (8000 Hz)

**Description**: Test audio encoding at minimum sample rate

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -r 8000 -o test_8k.wav
```

**Expected Result**:
- Exit code: 0
- Output file created: test_8k.wav
- Duration approximately 114 seconds (same as 22050 Hz) **at protocol 8000/22050 ratio**

**Acceptance Criteria**:
- ✓ File created
- ✓ Valid WAV header with 8000 Hz sample rate
- ✓ File size proportional to 8000 Hz
- ✓ Duration maintains transmission structure

**Notes**: Audio quality lower but mode still identifiable

---

### Test Case ID: RATE.002

**Name**: Standard Sample Rate (11025 Hz)

**Description**: Test at 11025 Hz (44.1kHz/4)

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -r 11025 -o test_11k.wav
```

**Expected Result**:
- Exit code: 0
- Output file created
- WAV shows 11025 Hz sample rate

**Acceptance Criteria**:
- ✓ File created and valid
- ✓ Sample rate in header: 11025 Hz
- ✓ Audio quality acceptable
- ✓ Duration consistent

**Notes**: Telephony-quality rate

---

### Test Case ID: RATE.003

**Name**: Default Sample Rate (22050 Hz)

**Description**: Test default sample rate

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -o test_default.wav
```

**Expected Result**:
- Exit code: 0
- Output file created without -r flag
- WAV shows 22050 Hz sample rate
- File size consistent with previous tests

**Acceptance Criteria**:
- ✓ File created
- ✓ Default rate is 22050 Hz
- ✓ Duration as expected
- ✓ Quality appropriate for SSTV

**Notes**: Default rate; balance of quality and file size

---

### Test Case ID: RATE.004

**Name**: High Sample Rate (32000 Hz)

**Description**: Test at 32000 Hz

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -r 32000 -o test_32k.wav
```

**Expected Result**:
- Exit code: 0
- Output file created
- 32000 Hz in WAV header

**Acceptance Criteria**:
- ✓ File created
- ✓ Sample rate correct
- ✓ Better quality than 22050
- ✓ Larger file size

**Notes**: Good quality, moderate file size

---

### Test Case ID: RATE.005

**Name**: CD Quality Sample Rate (44100 Hz)

**Description**: Test at CD-quality 44100 Hz

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -r 44100 -o test_44k.wav
```

**Expected Result**:
- Exit code: 0
- Output file created
- 44100 Hz in header

**Acceptance Criteria**:
- ✓ File created
- ✓ Sample rate: 44100 Hz
- ✓ High quality output
- ✓ ~2× file size of 22050 Hz

**Notes**: Professional quality; no perceptible Audio improvement for SSTV

---

### Test Case ID: RATE.006

**Name**: Maximum Sample Rate (48000 Hz)

**Description**: Test at maximum 48000 Hz

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -r 48000 -o test_48k.wav
```

**Expected Result**:
- Exit code: 0
- Output file created
- 48000 Hz in header (professional audio standard)

**Acceptance Criteria**:
- ✓ File created
- ✓ Sample rate: 48000 Hz
- ✓ Professional quality
- ✓ Largest file size

**Notes**: Professional broadcast quality; overkill for SSTV but supported

---

## Aspect Ratio Tests

### Test Case ID: ASPECT.001

**Name**: Center-Crop Aspect Mode

**Description**: Test default center-crop aspect ratio handling

**Prerequisites**:
- Portrait image (e.g., 600×800)
- Binary compiled

**Test Steps**:
```bash
./bin/pisstvpp2 -i portrait.jpg -a center -o center_output.wav
```

**Expected Result**:
- Exit code: 0
- Output created
- Image cropped to 320×256 from center
- No scaling distortion

**Acceptance Criteria**:
- ✓ File created
- ✓ Transmission still 114 seconds (Martin 1)
- ✓ Image properly centered
- ✓ No black bars
- ✓ Quality preserved

**Notes**: Default mode; no data loss outside crop area

---

### Test Case ID: ASPECT.002

**Name**: Pad Aspect Mode (Black Bars)

**Description**: Test letterbox/pillarbox padding mode

**Prerequisites**:
- Wide image (e.g., 800×400)
- Binary compiled

**Test Steps**:
```bash
./bin/pisstvpp2 -i wide.jpg -a pad -o padded_output.wav
```

**Expected Result**:
- Exit code: 0
- Output created
- Image preserved at original aspect
- Black bars added to fit target ratio
- Entire image visible, no cropping

**Acceptance Criteria**:
- ✓ File created
- ✓ Original image preserved
- ✓ Black bars visible in output
- ✓ No distortion
- ✓ Duration unchanged

**Notes**: Preserves all image data; shows black bars

---

### Test Case ID: ASPECT.003

**Name**: Stretch Aspect Mode

**Description**: Test force-stretch mode (may distort)

**Prerequisites**:
- Square image (e.g., 500×500)
- Binary compiled

**Test Steps**:
```bash
./bin/pisstvpp2 -i square.jpg -a stretch -o stretched_output.wav
```

**Expected Result**:
- Exit code: 0
- Output created
- Image stretched to 320×256
- All pixels visible, may be distorted
- Full coverage with no bars

**Acceptance Criteria**:
- ✓ File created
- ✓ Complete 320×256 image
- ✓ No empty space
- ✓ May show vertical stretching
- ✓ Duration unchanged

**Notes**: Fills entire output; use when distortion acceptable

---

## CW Identification Tests

### Test Case ID: CW.001

**Name**: Basic CW Identification

**Description**: Test standard Morse code callsign signature

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -C "N0CALL" -o cw_output.wav
```

**Expected Result**:
- Exit code: 0
- Output file created
- SSTV image data transmitted
- "SSTV DE N0CALL" Morse code appended
- Total duration approximately 120 seconds

**Acceptance Criteria**:
- ✓ File created
- ✓ SSTV image present
- ✓ CW signature audible at end
- ✓ Morse code correct
- ✓ Proper spacing between characters

**Notes**: CW adds ~6 seconds to transmission; test by playing in audio editor

---

### Test Case ID: CW.002

**Name**: CW at Standard Speed (15 WPM)

**Description**: Test Morse code at default 15 words-per-minute speed

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -C "W1AW" -W 15 -o cw_15wpm.wav
```

**Expected Result**:
- Exit code: 0
- Output created
- CW at comfortable speed (15 WPM = 75 words/min)
- Duration approximately 120 seconds

**Acceptance Criteria**:
- ✓ File created
- ✓ CW plays at readable speed
- ✓ Morse code correct
- ✓ Default if -W not specified

**Notes**: 15 WPM is standard amateur radio speed

---

### Test Case ID: CW.003

**Name**: CW at Slow Speed (5 WPM)

**Description**: Test very slow Morse code transmission

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -C "N0CALL" -W 5 -o cw_5wpm.wav
```

**Expected Result**:
- Exit code: 0
- Output created
- CW at very slow speed (5 WPM)
- Dots and dashes clearly separated
- Much longer duration than 15 WPM

**Acceptance Criteria**:
- ✓ File created
- ✓ CW very slow, each character distinct
- ✓ Audible gaps between elements
- ✓ Easy to decode by beginners

**Notes**: For clear reception in poor conditions

---

### Test Case ID: CW.004

**Name**: CW at Fast Speed (25 WPM)

**Description**: Test fast Morse code transmission

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -C "K5ABC" -W 25 -o cw_25wpm.wav
```

**Expected Result**:
- Exit code: 0
- Output created
- CW at fast speed (25 WPM)
- Still readable by experienced operators
- Duration shorter than lower speeds

**Acceptance Criteria**:
- ✓ File created
- ✓ CW at fast speed
- ✓ Morse code correct
- ✓ For experienced receivers

**Notes**: 25 WPM is good for operators who copy regularly

---

### Test Case ID: CW.005

**Name**: CW with Custom Tone (1000 Hz)

**Description**: Test Morse code with non-default tone frequency

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -C "N0CALL" -T 1000 -o cw_1k.wav
```

**Expected Result**:
- Exit code: 0
- Output created
- CW at 1000 Hz (higher pitch than default 800 Hz)
- Morse code content unchanged

**Acceptance Criteria**:
- ✓ File created
- ✓ CW tone at 1000 Hz (audibly higher)
- ✓ Morse code correct
- ✓ Compatible with receivers

**Notes**: Different operators prefer different tones

---

### Test Case ID: CW.006

**Name**: CW Combination (Custom Speed and Tone)

**Description**: Test CW with both custom speed and tone

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -C "W5XYZ" -W 20 -T 700 -o cw_custom.wav
```

**Expected Result**:
- Exit code: 0
- Output created
- CW at 20 WPM with 700 Hz tone
- Personalized transmission

**Acceptance Criteria**:
- ✓ File created
- ✓ CW speed: 20 WPM
- ✓ CW tone: 700 Hz
- ✓ Both parameters work together
- ✓ Morse code correct

**Notes**: Demonstrates flexibility of CW system

---

### Test Case ID: CW.007

**Name**: Portable Callsign with CW

**Description**: Test CW signature with portable callsign (contains slash)

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -C "W5ABC/4" -o cw_portable.wav
```

**Expected Result**:
- Exit code: 0
- Output created
- CW: "SSTV DE W5ABC/4"
- Slash encoded as morse ("/", or used as separator)

**Acceptance Criteria**:
- ✓ File created
- ✓ Portable indicator included
- ✓ Morse code handles special character
- ✓ Phonetically correct

**Notes**: `/4` indicates portable in ARRL region 4

---

### Test Case ID: CW.008

**Name**: Maximum Length Callsign

**Description**: Test CW with maximum allowable callsign length (31 chars)

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -C "W5ABCD/PORTABLE/EXTRA" -o cw_long.wav
# Callsign: 21 chars = within 31 char limit
```

**Expected Result**:
- Exit code: 0
- Output created
- Full callsign transmitted in CW
- Duration longer than short calls

**Acceptance Criteria**:
- ✓ File created
- ✓ Complete callsign encoded
- ✓ CW takes longer but completes
- ✓ Within 31-character limit

**Notes**: 31-character limit for callsign (including formatting)

---

## Image Handling Tests

### Test Case ID: IMG.001

**Name**: PNG Image Format

**Description**: Test PNG image input

**Prerequisites**:
- test_image.png available
- Binary compiled

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.png -o output.wav
```

**Expected Result**:
- Exit code: 0
- Output created
- Image properly loaded and encoded
- PNG format correctly handled

**Acceptance Criteria**:
- ✓ File created
- ✓ PNG loaded without error
- ✓ Image content in output
- ✓ Quality preserved

**Notes**: PNG supports transparency

---

### Test Case ID: IMG.002

**Name**: JPEG Image Format

**Description**: Test JPEG image input

**Prerequisites**:
- test_image.jpg available
- Binary compiled

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -o output.wav
```

**Expected Result**:
- Exit code: 0
- Output created
- JPEG decompressed and encoded
- Lossy compression not visible in output

**Acceptance Criteria**:
- ✓ File created
- ✓ JPEG decompression works
- ✓ Image quality in output
- ✓ No artifacts

**Notes**: JPEG is most common format

---

### Test Case ID: IMG.003

**Name**: GIF Image Format

**Description**: Test GIF image input

**Prerequisites**:
- test_image.gif available
- Binary compiled

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.gif -o output.wav
```

**Expected Result**:
- Exit code: 0
- Output created (first frame if animated)
- GIF correctly handled

**Acceptance Criteria**:
- ✓ File created
- ✓ GIF decoded
- ✓ Color palette preserved
- ✓ First frame used if animated

**Notes**: Only first frame of animated GIF used

---

### Test Case ID: IMG.004

**Name**: BMP Image Format

**Description**: Test BMP image input

**Prerequisites**:
- test_image.bmp available
- Binary compiled

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.bmp -o output.wav
```

**Expected Result**:
- Exit code: 0
- Output created
- BMP loaded and encoded

**Acceptance Criteria**:
- ✓ File created
- ✓ BMP format recognized
- ✓ Image correctly processed
- ✓ Output quality good

**Notes**: Uncompressed format; larger file sizes

---

### Test Case ID: IMG.005

**Name**: Various Image Resolutions

**Description**: Test images with different resolutions

**Prerequisites**:
- Images: 320×240, 640×480, 1024×768, etc.
- Binary compiled

**Test Steps**:
```bash
./bin/pisstvpp2 -i large_image_2048x1536.jpg -p m1 -o scaled.wav
./bin/pisstvpp2 -i tiny_image_80x60.jpg -p m1 -o upscaled.wav
```

**Expected Result**:
- Exit code: 0 for all
- All images scaled to 320×256
- Output duration consistent

**Acceptance Criteria**:
- ✓ Large images downscaled
- ✓ Small images upscaled
- ✓ Aspect ratio corrected
- ✓ Output always 320×256 (or mode-specific)
- ✓ Durations identical

**Notes**: libvips handles scaling well across range

---

## Combination Tests

### Test Case ID: COMBO.001

**Name**: All Modes with Default Settings

**Description**: Test all 7 SSTV modes with common parameters

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
for mode in m1 m2 s1 s2 sdx r36 r72; do
  ./bin/pisstvpp2 -i test_image.jpg -p $mode -o test_${mode}.wav
done
```

**Expected Result**:
- Exit code: 0 for each mode
- 7 output files created
- Each has correct VIS code
- Correct durations

**Acceptance Criteria**:
- ✓ All 7 modes succeed
- ✓ Output files valid
- ✓ Durations: m1(114s), m2(58s), s1(110s), s2(71s), sdx(269s), r36(36s), r72(72s)
- ✓ Different file sizes as expected

**Notes**: Comprehensive protocol coverage

---

### Test Case ID: COMBO.002

**Name**: All Modes with All Audio Formats

**Description**: Test each SSTV mode with each audio format

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
for mode in m1 s2 r36; do
  for fmt in wav aiff ogg; do
    ./bin/pisstvpp2 -i test_image.jpg -p $mode -f $fmt -o test_${mode}_${fmt}.${fmt}
  done
done
```

**Expected Result**:
- Exit code: 0 for all (OGG may skip if not compiled)
- 9 files created (or 6 if no OGG)
- Each format produces valid output
- Content identical, only format differs

**Acceptance Criteria**:
- ✓ All combinations work
- ✓ WAV and AIFF always present
- ✓ OGG skips gracefully if unavailable
- ✓ Same audio content in all formats
- ✓ Different file sizes (OGG compressed)

**Notes**: Tests format independence from protocol

---

### Test Case ID: COMBO.003

**Name**: All Sample Rates with Multiple Protocols

**Description**: Test various sample rates with multiple SSTV modes

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
for rate in 8000 22050 44100; do
  for mode in m1 s2 r72; do
    ./bin/pisstvpp2 -i test_image.jpg -p $mode -r $rate -o test_${mode}_${rate}.wav
  done
done
```

**Expected Result**:
- Exit code: 0 for all
- 9 files created
- Each shows correct sample rate in header
- Duration increases with lower sample rates

**Acceptance Criteria**:
- ✓ All combinations work
- ✓ Sample rates in WAV headers correct
- ✓ File sizes appropriate to rates
- ✓ Audio content correct

**Notes**: Sample rate scales timing proportionally

---

### Test Case ID: COMBO.004

**Name**: Aspect Modes with Image Variations

**Description**: Test all aspect modes with different image shapes

**Prerequisites**:
- Images: portrait (4:5), square (1:1), landscape (16:9)
- Binary compiled

**Test Steps**:
```bash
for aspect in center pad stretch; do
  ./bin/pisstvpp2 -i portrait.jpg -a $aspect -o portrait_${aspect}.wav
  ./bin/pisstvpp2 -i landscape.jpg -a $aspect -o landscape_${aspect}.wav
done
```

**Expected Result**:
- Exit code: 0 for all
- 6 files created
- Each aspect mode produces different handling
- All output 320×256

**Acceptance Criteria**:
- ✓ center: crops without bars
- ✓ pad: adds black bars
- ✓ stretch: distorts to fit
- ✓ All produce valid output
- ✓ Intended behavior visible

**Notes**: Demonstrates flexibility of scaling

---

### Test Case ID: COMBO.005

**Name**: CW with All Audio Formats

**Description**: Test CW signatures with different audio formats

**Prerequisites**:
- Binary compiled
- test_image.jpg available

**Test Steps**:
```bash
./bin/pisstvpp2 -i test_image.jpg -C "N0CALL" -f wav -o cw_test.wav
./bin/pisstvpp2 -i test_image.jpg -C "N0CALL" -f aiff -o cw_test.aiff
./bin/pisstvpp2 -i test_image.jpg -C "N0CALL" -f ogg -o cw_test.ogg
```

**Expected Result**:
- Exit code: 0 for all
- 3 files created
- CW present in each
- Format differences only, content same

**Acceptance Criteria**:
- ✓ All formats work with CW
- ✓ CW audible in all files
- ✓ Content identical
- ✓ Different file sizes

**Notes**: CW independent of audio format

---

### Test Case ID: COMBO.006

**Name**: Stress Test (Large Image + Slow Mode + Low Sample Rate)

**Description**: Test with challenging parameters

**Prerequisites**:
- Large image (2048×2048 or larger)
- Binary compiled

**Test Steps**:
```bash
./bin/pisstvpp2 -i large_image.jpg -p sdx -r 8000 -a pad -o stress_test.wav
```

**Expected Result**:
- Exit code: 0
- File created (large, ~1-2 MB)
- Takes ~30-60 seconds to process
- Output valid

**Acceptance Criteria**:
- ✓ Handles large images
- ✓ Slow mode completes
- ✓ Low sample rate works
- ✓ Output valid and playable
- ✓ No memory errors

**Notes**: Tests system limits gracefully

---

## Summary

**Total Test Cases**: 80+

**Coverage**:
- 2 Information/Help tests
- 12 Error Handling tests
- 7 Protocol tests
- 3 Format tests
- 6 Sample Rate tests
- 3 Aspect Ratio tests
- 8 CW Identification tests
- 5 Image Handling tests
- 6 Combination tests

**Execution Time**: ~15-20 minutes for full suite

**Success Criteria**: All tests must pass, or documented skips (OGG) are acceptable

---

## See Also

- [Testing Plan](TESTING_PLAN.md) - Overall testing strategy
- [Test Suite README](TEST_SUITE_README.md) - Test framework documentation
- [Test Quick Start](TEST_QUICK_START.md) - Running tests quickly
