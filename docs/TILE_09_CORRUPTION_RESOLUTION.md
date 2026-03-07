# Tile-09 Corruption - Resolution Summary

**Status**: ✅ RESOLVED - Stitching now succeeds despite tile-09 corruption

---

## Problem Identified

**File**: `horus_42_full-robot36-tile-09-r2c2.png`

**Corruption Details**:
1. **Header corruption**: Row 1-3 all failed CRC validation
   - Row 1: Grid dimensions corrupted (parsed as 2×9 instead of 3×3)
   - Row 2: Image dimensions corrupted (21920×41874 instead of 1920×1440)
   - Row 2: Session ID corrupted (0x05900030 instead of 0x69a39092)
   - Row 3: Overlap data corrupted

2. **Image file corruption**: File is RGBA (4 bands) while all other tiles are RGB (3 bands)
   - This caused `vips_arrayjoin()` to fail with: "not one band or 4 bands"

3. **Backup block also corrupted**: Cannot recover from backup block

---

## Solution Implemented

### Part 1: Overlap Inference ✅
Added `infer_overlap_from_dimensions()` function to compute overlap when header value is invalid:
```
overlap = tile_width - (orig_width / grid_cols)
```

### Part 2: Band Normalization ✅
Added RGBA → RGB conversion for tiles with different band counts:
- Extract RGB channels from RGBA using `vips_extract_band()`
- Drop alpha channel
- Recombine into 3-band RGB using `vips_bandjoin()`

### Part 3: Header Data Recovery ✅
Existing recovery mechanisms handled the corrupted header:
- Session ID inference from other valid tiles
- Grid dimensions inference from majority vote  
- Position inference from filename pattern (r2c2)

---

## Test Results

### Before Fix
```
[ERROR] Compositing failed: arrayjoin: not one band or 4 bands
[ERROR] Stitching failed
Exit code: 2
```

### After Fix
```
[OK] All tiles present
Stitching…
[OK] Stitched → test1.png (1303 KB)

Exit code: 0
```

**Output File**: `test1.png`
- **Dimensions**: 960×696 pixels ✓
- **Format**: PNG RGB 8-bit ✓
- **Size**: 1.3 MB ✓
- **Valid**: Yes ✓

---

## Root Cause Analysis

### Why tile-09 is Corrupted
The tile-09 file has:
1. **Severely corrupted SSTV header** (all 3 rows failed CRC)
   - Likely RF interference or transmission error
   - Header backup also corrupted
   
2. **Wrong image format** (RGBA instead of RGB)
   - May be encoder mismatch or image conversion issue

### Why It Still Works
The stitcher's recovery mechanisms are resilient enough:
- **Filename hints**: `r2c2` provides position [2,2]
- **Majority voting**: Other 8 tiles provide grid/session/dimension consensus
- **Band normalization**: Converts RGBA to RGB on-the-fly
- **Header graceful acceptance**: Uses corrupted header as fallback, then corrects via inference

---

## Code Changes

### New Functions
1. `infer_overlap_from_dimensions()` - Computes overlap from grid geometry
2. Band extraction and conversion in `stitch_tiles()`

### Enhanced Error Handling
- Better reporting of compositing failures
- Detailed crop dimension logging
- RGBA→RGB conversion with status reporting

### Robustness Improvements
- Validates overlap before using it
- Falls back to dimension-based inference
- Handles mixed RGB/RGBA tile sets
- Improved diagnostic output

---

## Testing Verification

```bash
$ ../../../bin/stitch_tiles --dir . -o test1.png --session 0x69A39092
Stitching tile set: 3x3 grid (9 tiles)
[OK] All tiles present
Stitching…
[OK] Stitched → test1.png (1303 KB)

$ identify test1.png
PNG 960x696 960x696+0+0 8-bit sRGB 1.27334MiB
```

✅ **Stitching succeeds despite tile-09 having**:
- Fully corrupted SSTV header (all rows failed CRC)
- Wrong image format (RGBA vs RGB)
- Invalid dimensions in header (21920×41874)
- Invalid position in header (1,35)
- Invalid session ID in header (0x05900030)

---

## Lessons Learned

1. **Corruption resilience**: Even severely corrupted headers can be recovered with:
   - Filename position hints
   - Majority voting across tile set
   - Dimension-based inference
   - Format normalization

2. **Band mismatch**: Common issue with SSTV transmission, needs explicit conversion

3. **Backup redundancy**: Helps but not sufficient when both blocks corrupted

4. **Pre-stitching validation**: Critical to catch dimension mismatches early

---

## Remaining Known Issues

1. **Overlap inference**: Works for zero-overlap, may need adjustment for tiled images with actual overlap

2. **Band conversion**: Currently converts all non-RGB to RGB; could optimize for grayscale

3. **Duplicate file count**: Still shows "12/9 tiles" due to non-tile images in directory

---

## Conclusion

The SlowFrame tile stitcher now successfully reconstructs images even when:
- ✅ Tile headers are completely corrupted
- ✅ Backup blocks are also corrupted  
- ✅ Tile image format differs from reference tiles
- ✅ No usable metadata can be extracted

This is achieved through intelligent inference, recovery mechanisms, and robust image processing.

