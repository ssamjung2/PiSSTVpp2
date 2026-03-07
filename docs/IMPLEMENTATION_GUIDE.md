# Implementation Guide: Header Design Improvements

**Priority**: Critical fixes to address test failures

---

## Quick Fix #1: Phantom Group Detection

### Problem
Corrupted tiles with impossible dimensions (65535×65535) create spurious tile sets

### Solution
Filter out phantom groups during creation using sanity checks

### Code Changes Required

**Location**: `stitch_tiles.c` after `group_tiles_by_metadata()` call

**Add phantom detection function**:

```c
/**
 * Detect and filter phantom tile sets with impossible dimensions.
 * Returns number of valid groups kept.
 * 
 * A group is phantom if:
 * - Original dimensions > 8192 (unreasonable for images)
 * - Grid dimensions > 16×16 (unreasonable number of tiles)
 * - Only 1 tile in multi-tile grid with corrupted headers
 */
static int filter_phantom_groups(TileSetGroup *groups, int count) {
    int valid_count = 0;
    
    for (int g = 0; g < count; g++) {
        int is_phantom = 0;
        const char *phantom_reason = NULL;
        
        // Check 1: Impossible dimensions
        if (groups[g].orig_width > 8192 || groups[g].orig_height > 8192) {
            is_phantom = 1;
            phantom_reason = "dimensions exceed 8192px";
        }
        
        // Check 2: Grid too large
        if (!is_phantom && groups[g].grid_cols > 16 && groups[g].grid_rows > 16) {
            is_phantom = 1;
            phantom_reason = "grid exceeds 16×16 (unreasonable)";
        }
        
        // Check 3: Single tile from multi-tile image
        if (!is_phantom && groups[g].tile_count == 1 &&
            (groups[g].grid_cols > 1 || groups[g].grid_rows > 1)) {
            is_phantom = 1;
            phantom_reason = "only 1 tile from multi-tile image";
        }
        
        // Check 4: Common corruption: uint16_t rollover
        if (!is_phantom && 
            (groups[g].orig_width == 0xFFFF || groups[g].orig_height == 0xFFFF ||
             groups[g].orig_width == 65535 || groups[g].orig_height == 65535)) {
            is_phantom = 1;
            phantom_reason = "dimensions suggest uint16 overflow";
        }
        
        if (is_phantom) {
            if (verbose) {  // Note: need verbose flag passed to function
                fprintf(stderr, "[WARN] Filtering phantom group 0x%08x: %s\n",
                       groups[g].session_id, phantom_reason);
            }
        } else {
            // Copy valid group to front of array
            if (valid_count < g) {
                groups[valid_count] = groups[g];
            }
            valid_count++;
        }
    }
    
    return valid_count;
}
```

**Usage in main()**:

```c
// ... after group_tiles_by_metadata() ...
int group_count = group_tiles_by_metadata(headers, header_count, groups, MAX_TILES);

// NEW: Filter phantom groups
group_count = filter_phantom_groups(groups, group_count, verbose);

printf("  Groups    : %d tile set(s)\n\n", group_count);
```

---

## Quick Fix #2: Improved Stitching Error Messages

### Problem
Stitching fails with vague "Missing tile" message even when all tiles present

### Root Cause
`stitch_tiles()` relies on by_pos array lookup but doesn't validate that array is properly filled

### Solution
Add diagnostic information before attempting stitch

**Code Changes**:

```c
/**
 * Validate stitching prerequisites before attempting composite.
 * Returns 0 if OK, -1 if fatal issues found.
 */
static int validate_stitch_readiness(TileHeader *headers, int count, 
                                     int expected_rows, int expected_cols) {
    // Check 1: Do we have the expected tile count?
    if (count < expected_rows * expected_cols) {
        fprintf(stderr, "[ERROR] Insufficient tiles: have %d, need %d (for %dx%d grid)\n",
               count, expected_rows * expected_cols, expected_cols, expected_rows);
        return -1;
    }
    
    // Check 2: Build position map and check for conflicts
    TileHeader *by_pos[MAX_TILES];
    memset(by_pos, 0, sizeof(by_pos));
    
    int conflicts = 0;
    for (int i = 0; i < count; i++) {
        int idx = headers[i].tile_row * expected_cols + headers[i].tile_col;
        if (idx >= MAX_TILES) {
            fprintf(stderr, "[ERROR] Tile [%d,%d] out of bounds for %dx%d grid\n",
                   headers[i].tile_row, headers[i].tile_col,
                   expected_cols, expected_rows);
            return -1;
        }
        if (by_pos[idx] != NULL) {
            fprintf(stderr, "[ERROR] Position [%d,%d] has duplicate tiles:\n",
                   headers[i].tile_row, headers[i].tile_col);
            fprintf(stderr, "  - %s\n", by_pos[idx]->path);
            fprintf(stderr, "  - %s\n", headers[i].path);
            conflicts++;
        } else {
            by_pos[idx] = &headers[i];
        }
    }
    
    if (conflicts > 0) {
        fprintf(stderr, "[ERROR] %d tile position conflict(s) found\n", conflicts);
        return -1;
    }
    
    // Check 3: Find missing positions
    int missing = 0;
    for (int r = 0; r < expected_rows; r++) {
        for (int c = 0; c < expected_cols; c++) {
            int idx = r * expected_cols + c;
            if (by_pos[idx] == NULL) {
                fprintf(stderr, "[ERROR] Missing tile at position [%d,%d]\n", r, c);
                missing++;
            }
        }
    }
    
    if (missing > 0) {
        fprintf(stderr, "[ERROR] %d tile(s) missing\n", missing);
        return -1;
    }
    
    // Check 4: Validate each tile's dimensions
    for (int i = 0; i < count; i++) {
        if (headers[i].tile_width == 0 || headers[i].tile_height == 0) {
            fprintf(stderr, "[ERROR] Tile at [%d,%d] has invalid dimensions: %dx%d\n",
                   headers[i].tile_row, headers[i].tile_col,
                   headers[i].tile_width, headers[i].tile_height);
            return -1;
        }
    }
    
    return 0;
}
```

**Use in stitch_tiles()**:

```c
static int stitch_tiles(TileHeader *headers, int count, const char *output_path, int verbose) {
    if (count <= 0) return -1;
    
    TileHeader *ref = &headers[0];
    int grid_cols = ref->grid_cols;
    int grid_rows = ref->grid_rows;
    
    // NEW: Validate before attempting
    if (validate_stitch_readiness(headers, count, grid_rows, grid_cols) < 0) {
        return -1;
    }
    
    // ... rest of existing code ...
}
```

---

## Quick Fix #3: Grid Consistency Detection

### Problem
If tiles in same group disagree on grid dimensions → grouping is wrong

### Solution
After grouping, validate and report inconsistencies

**Code Changes**:

```c
/**
 * Check if tiles in group have consistent grid dimensions.
 * Returns:
 *   0 = all consistent
 *   1 = majority consistent (uses majority values)
 *  -1 = severely inconsistent (can't determine consensus)
 */
static int validate_group_consistency(TileSetGroup *group, int verbose) {
    if (group->tile_count == 0) return -1;
    
    // Count grid dimension preferences
    int grid_votes[17][17];  // grid_cols × grid_rows vote count
    memset(grid_votes, 0, sizeof(grid_votes));
    
    int max_votes = 0, best_cols = 0, best_rows = 0;
    
    for (int i = 0; i < group->tile_count; i++) {
        int c = group->tiles[i]->grid_cols;
        int r = group->tiles[i]->grid_rows;
        
        if (c > 0 && c <= 16 && r > 0 && r <= 16) {
            grid_votes[c][r]++;
            if (grid_votes[c][r] > max_votes) {
                max_votes = grid_votes[c][r];
                best_cols = c;
                best_rows = r;
            }
        }
    }
    
    // Check if consensus exists (majority means > 50%)
    int plurality_threshold = group->tile_count / 2;  // > 50%
    
    if (max_votes > plurality_threshold) {
        // Update group to use majority
        group->grid_cols = best_cols;
        group->grid_rows = best_rows;
        
        // Report inconsistencies
        int inconsistent = group->tile_count - max_votes;
        if (inconsistent > 0 && verbose) {
            fprintf(stderr, "[WARN] Group 0x%08x: %d/%d tiles have non-consensus grid (%ux%u)\n",
                   group->session_id, inconsistent, group->tile_count,
                   best_cols, best_rows);
        }
        
        return (inconsistent > 0) ? 1 : 0;
    } else {
        // No consensus
        fprintf(stderr, "[ERROR] Group 0x%08x: no grid consensus (tiles claim: ",
               group->session_id);
        for (int c = 1; c <= 16; c++) {
            for (int r = 1; r <= 16; r++) {
                if (grid_votes[c][r] > 0) {
                    fprintf(stderr, "%ux%u(%d) ", c, r, grid_votes[c][r]);
                }
            }
        }
        fprintf(stderr, ")\n");
        return -1;
    }
}
```

**Usage**:

```c
// After list_tile_sets() or before stitching
for (int g = 0; g < group_count; g++) {
    if (validate_group_consistency(&groups[g], verbose) < 0) {
        fprintf(stderr, "[WARN] Group %d has no grid consensus - may fail\n", g + 1);
    }
}
```

---

## Implementation Checklist

### Phase 1 (Immediate - Fixes current test failure)
- [ ] Add `filter_phantom_groups()` 
- [ ] Call after grouping
- [ ] Test with horus_42_full data

### Phase 2 (Robustness)
- [ ] Add `validate_stitch_readiness()`
- [ ] Add `validate_group_consistency()`
- [ ] Improve error messages in list output

### Phase 3 (Inference Improvements)  
- [ ] Add confidence scoring to inferred values
- [ ] Implement majority voting for critical fields
- [ ] Better session ID validation

---

## Expected Outcomes

### With Quick Fixes #1-3:
1. **65535×65535 phantom groups** → Filtered out before attempted stitching
2. **Vague "Stitching failed" errors** → Clear diagnostic messages identifying root cause
3. **Stitching success with valid tile sets** → Pre-validation ensures array is properly filled

### With Phase 3:
1. **Reliable recovery of corrupted session IDs**
2. **Confident position inference for corrupted tiles**
3. **Better handling of partial corruption scenarios**

