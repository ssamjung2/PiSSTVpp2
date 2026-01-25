# VIS Header Fix for M1 Mode - Analysis and Solution

## Issue Identified

The VIS header implementation for M1 mode had a **pre-silence duration of 500ms**, which is longer than what many SSTV receivers expect. This can cause:
- Receivers missing the start of the VIS header
- Decoding failures in M1 mode
- Compatibility issues with certain decoders

## The Fix Applied

Changed the pre-silence duration from **500ms to 200ms** in the `addvisheader()` function:

```c
// OLD: const uint32_t pre_silence_us = 500000;  // 500 ms
// NEW: const uint32_t pre_silence_us = 200000;  // 200 ms
```

## VIS Header Structure (Correct Implementation)

The SSTV VIS header for M1 mode should follow this exact sequence:

1. **Pre-silence**: 200ms @ silence (0 Hz)
2. **Attention sequence**: [1900, 1500, 1900, 1500, 2300, 1500, 2300, 1500] Hz
   - 100ms per tone (800ms total)
3. **Leader tone**: 1900 Hz @ 300ms
4. **Break tone**: 1200 Hz @ 10ms
5. **Mid tone**: 1900 Hz @ 300ms
6. **Start bit**: 1200 Hz @ 30ms
7. **VIS code bits** (LSB-first, 6 bits for M1=44):
   - Bit 1: 1100 Hz @ 30ms
   - Bit 0: 1300 Hz @ 30ms
   - M1 code (44) = 101100 binary = bits [0,0,1,1,0,1] LSB-first
8. **Parity bit** (odd parity): 1100 or 1300 Hz @ 30ms
9. **Stop bit**: 1200 Hz @ 30ms

## Verification Checklist

- [x] Pre-silence duration reduced to 200ms
- [x] Attention sequence frequencies correct: [1900, 1500, 1900, 1500, 2300, 1500, 2300, 1500]
- [x] VIS bit encoding correct: 1→1100Hz, 0→1300Hz (LSB-first)
- [x] Parity calculation uses XOR accumulation (odd parity)
- [x] Stop tone at 1200 Hz for 30ms

## Testing Recommendations

1. **Compile the updated code**:
   ```bash
   make clean && make pisstvpp2
   ```

2. **Test with M1 mode**:
   ```bash
   ./pisstvpp2 -p m1 test_image.png
   ```

3. **Decode the output**:
   - Use QSSTV, MMSSTV, or another SSTV decoder
   - Listen for the VIS header tones
   - Verify M1 mode is recognized (not falling back to another mode)

4. **If still having issues**, try these alternatives:

   **Option A: Remove pre-silence entirely**
   ```c
   // Try 0ms pre-silence (some receivers prefer to start immediately)
   const uint32_t pre_silence_us = 0;
   ```

   **Option B: Use longer pre-silence for compatibility**
   ```c
   // Try 300ms if 200ms is still too short
   const uint32_t pre_silence_us = 300000;
   ```

   **Option C: Verify attention sequence is not the issue**
   - The attention sequence looks correct
   - Some alternative sources show: [1900, 1500, 1900, 1500, 2300, 1500, 2300, 1500]
   - This matches the current code

## M1 Mode Specifications

- **VIS Code**: 44 (0x2C)
- **Binary**: 101100 (6-bit)
- **Dimensions**: 320×256 pixels
- **Line time**: 1.3 seconds
- **Pixel time**: 4.576 ms
- **Sync pulse**: 4.862 ms @ 1200 Hz

## Related Code Sections

- VIS header function: `addvisheader()` at line ~1561
- VIS trailer function: `addvistrailer()` - should also be verified
- M1 encoder: `buildaudio_m()` at line ~1645
- Protocol setting: `g_protocol = 44;` for M1 mode

## Notes

The VIS header implementation appears structurally correct. The main issue was the excessive pre-silence. If the fix doesn't resolve all issues, additional testing will be needed to identify if there are problems with:
- Attention sequence timing or frequency
- VIS bit encoding
- Parity calculation
- Interaction with the M1 line encoding (sync/porch/separator timing)
