# Sample PiSSTVpp Output Demonstrations

## Successful Encoding Example (Martin 1, PNG)

```
$ ./pisstvpp2 -i test.png -o test_m1.wav -p m1 -r 22050

PiSSTVpp - SSTV Audio Encoder
Input  file: test.png
Output file: test_m1.wav (wav format, 22050 Hz)
Protocol: m1 (VIS code 44)
Image dimensions: 320x256 pixels (assumed by SSTV spec)

[1/4] Loading image...
   » Detected: 320x256, 3-band image
   » Buffering pixel data...
   ✓ Image buffer ready: 320x256 RGB (245760 bytes)
   ✓ Image loaded successfully

[2/4] Opening output file...
   ✓ Output file ready

[3/4] Encoding image as SSTV audio...
Adding VIS header to audio data.
   ✓ VIS header added
   » Processing pixels...
   » Encoding row 64/256 (25%)
   » Encoding row 128/256 (50%)
   » Encoding row 192/256 (75%)
 done
   ✓ Image encoded
   ✓ VIS trailer added

[4/4] Writing WAV file...
   » Format: WAV
   » Sample rate: 22050 Hz
   ✓ File written

✅ SUCCESS!
   Output: test_m1.wav
   Time elapsed: 0 seconds
   Ready to transmit via SSB radio!
```

---

## Successful Encoding Example (Robot 36, GIF)

```
$ ./pisstvpp2 -i test.gif -o test_r36.wav -p r36 -r 22050

PiSSTVpp - SSTV Audio Encoder
Input  file: test.gif
Output file: test_r36.wav (wav format, 22050 Hz)
Protocol: r36 (VIS code 8)
Image dimensions: 320x256 pixels (assumed by SSTV spec)

[1/4] Loading image...
   » Detected: 320x256, 3-band image
   » Buffering pixel data...
   ✓ Image buffer ready: 320x256 RGB (245760 bytes)
   ✓ Image loaded successfully

[2/4] Opening output file...
   ✓ Output file ready

[3/4] Encoding image as SSTV audio...
Adding VIS header to audio data.
   ✓ VIS header added
   » Processing pixels...
   » Encoding row pair 32/240 (13%)
   » Encoding row pair 64/240 (26%)
   » Encoding row pair 96/240 (40%)
   » Encoding row pair 128/240 (53%)
   » Encoding row pair 160/240 (66%)
   » Encoding row pair 192/240 (80%)
   » Encoding row pair 224/240 (93%)
 done
   ✓ Image encoded
   ✓ VIS trailer added

[4/4] Writing WAV file...
   » Format: WAV
   » Sample rate: 22050 Hz
   ✓ File written

✅ SUCCESS!
   Output: test_r36.wav
   Time elapsed: 0 seconds
   Ready to transmit via SSB radio!
```

Note: Robot 36 shows "row pair" progress since it processes 2 image rows together (YUV format requires averaging adjacent rows).

---

## Error Handling Example 1 - File Not Found

```
$ ./pisstvpp2 -i missing.png -o test.wav -p m1

PiSSTVpp - SSTV Audio Encoder
Input  file: missing.png
Output file: test.wav (wav format, 22050 Hz)
Protocol: m1 (VIS code 44)
Image dimensions: 320x256 pixels (assumed by SSTV spec)

[1/4] Loading image...
   ✗ Failed to load 'missing.png'
   Details: VipsForeignLoad: file "missing.png" does not exist

❌ ERROR: Failed to load image 'missing.png'
   Supported formats: PNG, JPEG, GIF, BMP, TIFF, WebP
   Check file exists and is a valid image file
```

---

## Error Handling Example 2 - Invalid Image File

```
$ ./pisstvpp2 -i textfile.txt -o test.wav -p m1

PiSSTVpp - SSTV Audio Encoder
Input  file: textfile.txt
Output file: test.wav (wav format, 22050 Hz)
Protocol: m1 (VIS code 44)
Image dimensions: 320x256 pixels (assumed by SSTV spec)

[1/4] Loading image...
   ✗ Failed to load 'textfile.txt'
   Details: VipsForeignLoad: not a known image format

❌ ERROR: Failed to load image 'textfile.txt'
   Supported formats: PNG, JPEG, GIF, BMP, TIFF, WebP
   Check file exists and is a valid image file
```

---

## Error Handling Example 3 - Permission Denied

```
$ ./pisstvpp2 -i /root/protected.png -o test.wav -p m1

PiSSTVpp - SSTV Audio Encoder
Input  file: /root/protected.png
Output file: test.wav (wav format, 22050 Hz)
Protocol: m1 (VIS code 44)
Image dimensions: 320x256 pixels (assumed by SSTV spec)

[1/4] Loading image...
   ✗ Failed to load '/root/protected.png'
   Details: VipsForeignLoad: file "protected.png" does not exist

❌ ERROR: Failed to load image '/root/protected.png'
   Supported formats: PNG, JPEG, GIF, BMP, TIFF, WebP
   Check file exists and is a valid image file
```

---

## Multi-Mode Test (Quick Reference)

All modes tested with single command:

```
$ for mode in m1 m2 s1 s2 sdx r36; do
>   ./pisstvpp2 -i test.png -o test_$mode.wav -p $mode -r 22050 2>&1 | tail -5
> done

✅ SUCCESS!
   Output: test_m1.wav
   Time elapsed: 0 seconds
   Ready to transmit via SSB radio!

✅ SUCCESS!
   Output: test_m2.wav
   Time elapsed: 0 seconds
   Ready to transmit via SSB radio!

... (similar for s1, s2, sdx, r36) ...
```

---

## Key Features of New Messaging

1. **Progressive Disclosure**
   - User sees configuration first
   - Each step clearly labeled [1/4] through [4/4]
   - Progress shown during long-running encoding

2. **Clear Status Indicators**
   - ✓ = Success on that step
   - ✗ = Error with details
   - ✅ = Overall success
   - ❌ = Overall failure

3. **Helpful Details**
   - Image format and dimensions
   - Buffer sizes in bytes
   - Audio format and sample rate
   - Percentage completion during encoding
   - File location confirmation

4. **Embedded Progress**
   - Every ~25% of encoding shows progress
   - Especially useful on slower systems (Raspberry Pi)
   - Protocol-aware (row pairs for Robot 36, single rows for others)

5. **Error Context**
   - Shows what failed (file, color conversion, buffering, etc.)
   - Includes underlying error message from libvips
   - Provides helpful hints (supported formats, file existence, etc.)
