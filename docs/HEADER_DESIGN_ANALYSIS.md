# SlowFrame Header Tile Design & Integrity Analysis

**Date**: March 4, 2026  
**Focus**: Review current design, identify limitations, evaluate recovery scenarios

## Executive Summary

The current tile header system faces several architectural issues that contribute to:
1. Invalid tile set detection (65535×65535 spurious groups)
2. Stitching failures with valid tile sets
3. Insufficient corruption detection and recovery
4. Ambiguous session ID handling

This document provides a comprehensive design review and proposes improvements.

---

## Current Header Architecture

### Header Layout (8 rows × 320 pixels)

```
Row 1-3: Primary block (data + CRC)
Row 4:   Unused/reserved
Row 5-7: Backup block (identical copy)
Row 8:   Unused/reserved
```

### Data Structure (per row)

**Format**: 16 blocks × 20px each OR 8 blocks × 40px (legacy)

**Row 1 (Grid & Position Info)**
- Bytes 0-14: Data
  - [0] version
  - [1] grid_cols
  - [2] grid_rows
  - [3] tile_col
  - [4] tile_row
  - [5] total_tiles
  - [6] seq_num
  - [7-14] reserved/overlap_px
- Byte 15: CRC-8 CCITT

**Row 2 (Dimensions & Session)**
- Bytes 0-14: Data
  - [0-1] orig_width (16-bit big-endian)
  - [2-3] orig_height (16-bit big-endian)
  - [4-7] session_id (32-bit)
  - [8-14] reserved
- Byte 15: CRC-8 CCITT

**Row 3 (Metadata/Overlap)**
- Bytes 0-14: Data
  - [0] overlap_px (for 8-block format)
  - [1-14] reserved
- Byte 15: CRC-8 CCITT

### Error Correction Strategy

**SECDED (Single Error Correction, Double Error Detection)**
- XOR parity computed over data bytes (0-14)
- CRC-8 stored at byte 15
- If CRC fails: attempt single-bit correction via parity
- Can detect but NOT correct 2-bit errors

---

## Current Issues & Limitations

### Issue 1: Spurious Group Creation (65535×65535)

**Symptom**: Tile list output shows phantom tile sets with impossible dimensions

**Root Cause**: 
- Corrupted Row 1 (grid dimensions) → tile_row, tile_col = large garbage values
- Row 2 (session ID) still valid
- Grouping algorithm uses (session_id, orig_width/height, grid_cols/rows) as key
- Even one corrupted tile creates a new group if dimensions differ

**Example from output**:
```
Tile Set #2:
  Session ID: 0x69A39092  (← SAME as #1, suggests same image)
  Original:   65535×65535 px  (← IMPOSSIBLE, uint16 max)
  Grid:       3x3
  Available:  1/9 tiles
```

**Why This Happens**:
```c
// Current grouping: uses all 5 fields
if (groups[g].session_id == tile->session_id &&
    groups[g].orig_width == tile->orig_width &&
    groups[g].orig_height == tile->orig_height &&
    groups[g].grid_cols == tile->grid_cols &&
    groups[g].grid_rows == tile->grid_rows) {
    // Same group
}
```

If even ONE dimension differs → new group created
If Row 1 corrupted but Row 2 valid → dimensions change

---

### Issue 2: Stitching Failure Despite Valid Tiles

**Symptom**: `stitch_tiles()` returns -1 even when all 9 tiles are present

**Current Checks** (in `stitch_tiles()`):
```c
for (int row = 0; row < grid_rows; row++) {
    for (int col = 0; col < grid_cols; col++) {
        int idx = row * grid_cols + col;
        TileHeader *h = by_pos[idx];
        if (!h) {
            fprintf(stderr, "Missing tile\n");
            return -1;  // ← FAILURE
        }
        // ... then crop and stitch
    }
}
```

**Issues**:
1. **No graceful degradation**: Single missing tile aborts entire stitch
2. **Array lookup assumes perfect ordering**: `by_pos[(row * grid_cols) + col]` must be filled
3. **No validation of R1 grid consistency**: What if one tile says 3×3 grid, another says 2×2?
4. **Crop parameters silently invalid**: If tile_width/height = 0 → invalid crop bounds

---

### Issue 3: Insufficient Recovery Metadata

**Current Recovery Flags**:
```c
int row1_error_corrected;     // Single-bit error in Row 1
int row2_error_corrected;     // Single-bit error in Row 2
int row3_error_corrected;     // Single-bit error in Row 3
int row1_multi_error;         // Multi-bit error detected
int row2_multi_error;
int row3_multi_error;
int recovered_from_backup;    // Successfully used backup block
int cross_tile_inferred;      // Position inferred from other tiles
int session_id_inferred;      // Session ID inferred
```

**Limitations**:
- No tracking of **which bits** were corrected
- No confidence score for inferred data
- No distinction between "inferred with high confidence" vs "guessed"
- Backup block only useful when BOTH primary AND backup fail, but we don't try backup for success cases

---

### Issue 4: Session ID Inference Not Fool-Proof

**Current Logic**:
```c
// Find reference with same grid dimensions
for (int j = 0; j < count; j++) {
    if (ref->grid_cols == tile->grid_cols && 
        ref->grid_rows == tile->grid_rows &&
        ref->row2_crc_ok) {
        reference_session = ref->session_id;
        found_reference = 1;
        break;
    }
}
```

**Problems**:
1. **Assumes grid dimensions are correct** even if R1 CRC failed
2. **No majority voting**: If multiple "reference" tiles exist with different sessions, uses first match
3. **Doesn't validate that inferred session makes sense**: 
   - What if we infer session A from one tile, but tile has already-valid session B?
   - Creates inconsistency

---

### Issue 5: Limited Backup Block Utilization

**Current Approach**:
- Backup block (rows 5-7) used ONLY if primary block fails CRC
- Never compared with primary for validation  
- Never used to detect data drift
- Not used to improve confidence in inferred data

---

### Issue 6: No Architectural Validation

**Missing Checks**:
1. **Grid consistency**: All tiles claim same (grid_cols, grid_rows)?
2. **Position consistency**: No overlapping tiles at same (row, col)?
3. **Dimension consistency**: All tiles claim same original image dimensions?
4. **Session consistency**: All tiles claim same session ID?
5. **CRC confidence**: How many rows have valid CRCs?

---

## Recovery Scenarios & Limitations

### Scenario 1: Single Row Corruption (Row 1)

**Current behavior**:
- CRC fails → attempt SECDED correction
- If success: marked `row1_error_corrected = 1`
- If fail: tile still accepted with `row1_crc_ok = 0`

**Limitation**: 
- Grid position (tile_row, tile_col) may still be wrong
- Tile gets grouped incorrectly if Row 1 gives wrong position

**Better approach**:
- Validate position against actual file position in grid
- Deduce position from surrounding tiles

---

### Scenario 2: Row 2 Corruption (Dimensions/Session)

**Current behavior**:
- CRC fails → SECDED correction
- If success: use corrected values
- If fail: infer session from other tiles via `infer_corrupted_sessions()`

**Limitation**:
- No validation that inferred session is correct
- If all session IDs differ → no inference possible
- No fallback to detect "primary image" by counting most-common session

---

### Scenario 3: Multiple Row Failures

**Current behavior**:
- Try primary block → if fails, try backup block
- If backup also fails → use "best guess" from partially corrupted primary

**Limitation**:
- No recovery strategy when backup is also corrupted
- No attempt to reconstruct from cross-tile inference
- No confidence scoring

---

### Scenario 4: Tile Position Mismatch

**Example**:
```
File:     horus_42_full-robot36-tile-09-r2c2.png
Header:   position = [0, 0]  (corrupted)
Filename: r2c2 suggests position [2, 2]
```

**Current behavior**:
- Runs `apply_filename_position_hints()` → overwrites header position with filename
- Works BUT:
  - Assumes filename is correct (not always!)
  - May fail for position inference if filename pattern not recognized

---

## Proposed Improvements

### Improvement 1: Validation Framework

```c
typedef struct {
    char validation_status[256];  // "PASS", "WARN", "FAIL:multibit", etc.
    int validation_score;         // 0-100: confidence in recovered data
    int critical_fields_ok;       // R1+R2 most critical rows ok?
    int has_valid_backup;         // Verified backup matches primary
    char confidence_reasons[512]; // Why score is what it is
} ValidationMetadata;
```

### Improvement 2: Enhanced Grouping with Validation

Instead of strict dimension matching:

```c
// Group by SESSION ID primarily
// Use dimension hints but don't require perfect match
// Flag tiles with mismatched dimensions for later investigation

struct TileSetGroup {
    uint32_t session_id;           // Primary key
    uint16_t primary_orig_width;   // Most common dimensions
    uint16_t primary_orig_height;
    uint8_t primary_grid_cols;
    uint8_t primary_grid_rows;
    
    int dimension_mismatch_count;  // Tiles with different dims
    int position_mismatch_count;   // Tiles with invalid positions
    int header_corruption_count;   // Tiles needing recovery
    
    TileHeader *tiles[MAX_TILES];
    int tile_count;
    int missing_count;
};
```

### Improvement 3: Majority Voting for Critical Fields

```c
// For each critical field, compute majority value:
int compute_field_majority(TileHeader *tiles, int count, 
                           int (*get_field)(const TileHeader*)) {
    int histogram[256] = {0};
    for (int i = 0; i < count; i++) {
        int val = get_field(&tiles[i]);
        if (val < 256) histogram[val]++;
    }
    int max_count = 0, best_val = 0;
    for (int i = 0; i < 256; i++) {
        if (histogram[i] > max_count) {
            max_count = histogram[i];
            best_val = i;
        }
    }
    return (max_count > count / 2) ? best_val : -1;  // Majority only if > 50%
}
```

### Improvement 4: Grid Integrity Checking

```c
typedef struct {
    uint8_t grid_cols;
    uint8_t grid_rows;
    int confidence;  // 100=all agree, 50=majority, 0=conflict
} GridConsensus;

GridConsensus detect_grid_consensus(TileHeader *tiles, int count) {
    // Find most common grid dimensions
    // Flag conflicts
    // Return with confidence score
}
```

### Improvement 5: Cross-Tile Inference with Confidence

```c
// Given corrupted tile + list of valid tiles
// Try to infer correct values with score

typedef struct {
    uint32_t inferred_value;
    int confidence;  // 0-100
    char reason[256];  // e.g., "matched 8 other tiles", "only 1 match"
} InferenceResult;

InferenceResult infer_session_with_confidence(
    TileHeader *corrupted,
    TileHeader *reference_tiles, int ref_count) {
    // Count tiles with same grid dimensions AND valid R2
    int matches = 0;
    uint32_t matched_session = 0;
    
    for (int i = 0; i < ref_count; i++) {
        if (reference_tiles[i].grid_cols == corrupted->grid_cols &&
            reference_tiles[i].grid_rows == corrupted->grid_rows &&
            reference_tiles[i].row2_crc_ok) {
            if (matched_session == 0) {
                matched_session = reference_tiles[i].session_id;
            } else if (matched_session != reference_tiles[i].session_id) {
                // Conflict!
                return (InferenceResult){0, 0, "Multiple sessions in grid"};
            }
            matches++;
        }
    }
    
    if (matches > 6) {
        return (InferenceResult){matched_session, 100, "Unanimous match"};
    } else if (matches > 4) {
        return (InferenceResult){matched_session, 80, "Strong majority"};
    } else if (matches > 0) {
        return (InferenceResult){matched_session, 40, "Few matches only"};
    }
    return (InferenceResult){0, 0, "No valid reference found"};
}
```

### Improvement 6: Phantom Group Detection

```c
static int is_phantom_group(TileSetGroup *group) {
    // Check for impossible/unlikely characteristics:
    
    // Impossible dimensions (uint16 max values)
    if (group->primary_orig_width > 8192 || group->primary_orig_height > 8192) {
        return 1;  // Almost certainly phantom
    }
    
    // Grid too large for single image
    if (group->primary_grid_cols * group->grid_rows > 100) {
        return 1;  // Unlikely (would be 100×100 tiles!)
    }
    
    // Only 1 tile in group but not a single-tile image
    if (group->tile_count == 1 && 
        (group->primary_grid_cols > 1 || group->primary_grid_rows > 1)) {
        return 1;  // Single tile from a multi-tile image
    }
    
    // All tiles have corrupted headers
    if (group->header_corruption_count == group->tile_count) {
        return 1;  // No valid reference to verify dimensions
    }
    
    return 0;
}
```

---

## Corruption Analysis: horus_42_full-robot36-tile-09-r2c2.png

### Symptom
- File: tile-09 (position [2,2] from filename)
- Group detection: Two tile sets detected with same session ID
- Stitching: Fails despite all tiles present

### Likely Cause
- **Row 1 and/or Row 2 corruption** in tile-09
- Corrupted dimensions → falls into different group
- During stitching, mismatched grid dimensions cause array lookup failure

### Verification Needed
1. Extract header bytes from all 9 tiles
2. Compare:
   - grid_cols/rows values (should all be 3×3)
   - orig_width/height (should all be 1920×1440)
   - session_id (should all be 0x69A39092)
3. Identify which bytes differ in tile-09

---

## Recommended Implementation Order

1. **Phase 1**: Add validation framework & logging (low risk)
   - Add `ValidationMetadata` struct
   - Log suspected phantom groups
   - No behavioral changes yet

2. **Phase 2**: Implement phantom group detection  
   - Filter out impossible tile sets
   - Prevents spurious grouping

3. **Phase 3**: Enhance grouping with dimension tolerance
   - Group primarily by session ID
   - Flag dimension mismatches but don't break groups
   - Use majority voting for group properties

4. **Phase 4**: Improve stitching robustness
   - Better error messages
   - Validate grid before stitching
   - Graceful handling of position mismatches

5. **Phase 5**: Advanced inference
   - Confidence scoring
   - Cross-tile validation
   - Smart session ID inference

---

## Testing Strategy

### Test Case 1: Single Row Corruption
- Corrupt only Row 1 in one tile
- Should: Infer position from filename
- Verify: Group remains correct

### Test Case 2: Two Row Corruption  
- Corrupt Row 1 + Row 2 in one tile
- Should: Infer both position AND session
- Verify: Stitching still succeeds

### Test Case 3: Identical Session, Different Dimensions
- Tiles from same session but different grids
- Should: Detect conflict, report clearly
- NOT: Create phantom groups

### Test Case 4: All Rows Corrupted
- Corrupt all 3 rows in one tile
- Should: Use backup block OR infer from neighbors
- Should NOT: Crash or create phantom groups

