# SlowFrame Code Cleanup Action Plan
**Date:** February 12, 2026  
**Version:** 1.0  
**Based on:** LEGACY_CODE_AUDIT_REPORT.md

---

## IMMEDIATE ACTIONS (Week 1 - No Risk)

### Task 1.1: Update Misleading Comment in image_loader.c
**Priority:** HIGH  
**Effort:** 15 minutes  
**Risk:** None  
**File:** [src/image/image_loader.c](src/image/image_loader.c#L65)

**Current Code:**
```c
/**
 * Retrieve the image buffer currently stored in the loader state.
 *
 * Used by legacy compatibility functions; modular code should use loaders directly.
 * The caller should NOT free this buffer - the loader maintains ownership.
 */
```

**Issues:**
- "legacy compatibility functions" is misleading
- These ARE the modular loader functions
- Comment suggests code is old, but it's actively used

**Corrected Code:**
```c
/**
 * Retrieve the image buffer currently stored in the loader state.
 *
 * Primary entry point for loading images. Handles detection of file type
 * and delegates to appropriate loader (PNG, JPEG, GIF, etc.).
 * The caller should NOT free this buffer - the loader maintains ownership.
 */
```

**Validation Steps:**
1. Read the full function context
2. Verify it's indeed the primary loader
3. Update comment
4. Compile to verify no syntax errors
5. Run test suite to verify no functional changes

---

### Task 1.2: Add Documentation to Legacy Folder
**Priority:** HIGH  
**Effort:** 30 minutes  
**Risk:** None  
**File:** [src/legacy/README.md](src/legacy/README.md)

**Current State:**
File exists but needs enhancement with clear deprecation status.

**Required Content to Add:**

```markdown
# SlowFrame Legacy Code Archive

This folder contains the original v1.x and early v2.0 implementation code.
These implementations are NO LONGER ACTIVE and should NOT be used.

## What's Here

- **pifm_sstv.c** - Original PiFM (FM modulation through GPIO) implementation
- **pisstvpp.c** - Early image-to-audio converter implementation
- **Makefile.legacy** - Build configuration for v1.x
- **build_legacy.sh** - Legacy build script

## Why This Code Is Archived

1. **Obsolete Approach:** v1.x used direct GPIO bitbanging to generate FM signals
   - Modern approach: libvips image processing + audio file generation
   - Modern approach is portable to any system, not just Raspberry Pi

2. **Missing Features:** v1.x had no support for:
   - Text overlays
   - Flexible aspect ratio correction
   - Multiple audio formats (only direct GPIO output)
   - Extended SSTV protocols beyond 7 basic modes

3. **Code Quality:** 
   - v1.x comments explicitly state "hacked together"
   - v2.1 is production-ready, well-documented, and tested

## When to Reference This Code

- **Historical curiosity:** Understanding how the original approach worked
- **Algorithm reference:** If you need the original tone calculation approach
- **Educational purposes:** Learning about SSTV protocol implementation

## DO NOT USE THIS CODE FOR:
- Active development (use src/ instead)
- Production deployments (v2.1 is current)
- Bug fixes (report issues against v2.1 in src/)
- New features (implement in src/, not here)

## Current Version

The active version is SlowFrame v2.1.0 in `/src/`

For development, building, and usage:
- See `/docs/BUILD.md` for setup instructions
- See `/README.md` for feature list
- See `/docs/QUICK_START.md` for usage examples

---

Last Updated: February 2026
Status: Archived - Read-Only Reference
```

**Validation Steps:**
1. Add content to README.md
2. Verify formatting is clear
3. Check all cross-references are correct
4. Add to documentation index if not already present

---

## SHORT-TERM ACTIONS (Weeks 2-4 - Planned)

### Task 2.1: Document Text Overlay Color Limitation
**Priority:** MEDIUM  
**Effort:** 1 hour  
**Risk:** None (documentation only)  
**File:** [src/image/image_text_overlay.c](src/image/image_text_overlay.c)

**Current State:**
Code has TODO at line 81 about color mapping.

**Action Steps:**

1. **Add detailed header documentation:**
```c
/**
 * @file image_text_overlay.c
 * @brief Text overlay rendering module
 *
 * ## Current Capabilities
 * - Colored rectangles with text (placeholder rendering)
 * - Background color support (CSS-named colors + hex RGB)
 * - Text placement (top, bottom, left, right, center)
 * - Font size adjustment
 * - Text alignment (left, center, right)
 *
 * ## Known Limitations
 * - Text itself is currently fixed to white or black (based on background brightness)
 * - TODO: Support arbitrary text colors per overlay specification
 * - No actual font rendering (uses text area as placeholder)
 *
 * ## Implementation Notes
 * Phase 2.4 implements basic colored rectangles.
 * Phase 2.5 will add actual text rendering with FreeType or similar.
 * Text color support deferred to v2.2 due to complexity.
 *
 * @see TextOverlaySpec in overlay_spec.h for specification structure
 * @see image_apply_overlay_list() for integration point
 */
```

2. **Create detailed TODO documentation:**
```c
/* TODO: Apply color mapping for non-black colors [v2.2]
 * Current: Text is always white (light bg) or black (dark bg)
 * Goal: Support arbitrary text colors per specification
 * 
 * Complexity: 
 * - Requires FreeType or similar for real text rendering
 * - Would need font file distribution
 * - Increases dependencies
 * 
 * Workaround for now:
 * - Users can set background color to contrast with text
 * - Solid background provides sufficient visual separation
 * 
 * Implementation approach for v2.2:
 * 1. Integrate FreeType2 library
 * 2. Load system fonts or bundle default font
 * 3. Render text to RGB buffer with specified color
 * 4. Composite onto base image
 * 
 * Estimate: 4-6 hours implementation + 2 hours testing
 * References:
 *   - FreeType2: https://freetype.org/
 *   - Font file paths: /usr/share/fonts/ (Linux), /Library/Fonts/ (macOS)
 */
```

3. **Create enhancement ticket template:**
```markdown
# [v2.2 Enhancement] Text Overlay Color Support

**Description:** Currently, overlay text is always white or black.
Implement support for arbitrary text colors per overlay specification.

**User Impact:** Users want colored text, not just colored backgrounds.

**Current Workaround:** Set background color to provide contrast.

**Proposed Implementation:**
- Integrate FreeType2 for proper text rendering
- Render text glyphs with specified color
- Composite rendered text onto image

**Estimated Effort:** 4-6 hours
**Complexity:** Medium
**Dependencies:** FreeType2, font files

**Related:** #123 (initial overlay implementation)
```

**Validation:**
1. Documentation added and clear
2. No code changes (documentation only)
3. TODO clearly explains scope and timeline

---

### Task 2.2: Document MMSSTV Adapter Limitation
**Priority:** MEDIUM  
**Effort:** 1 hour  
**Risk:** None (documentation only)  
**File:** [src/slowframe_mmsstv_adapter.c](src/slowframe_mmsstv_adapter.c#L263)

**Current State:**
TODO at line 263 about hardcoded mode details.

**Action Steps:**

1. **Enhance existing TODO with full context:**
```c
/* TODO: Get this from sstv module once refactored [v2.2]
 * 
 * Current Implementation (Hardcoded):
 * Mode details are manually coded based on SSTV protocol specs.
 * This works correctly but is inflexible.
 * 
 * Problem:
 * - If MMSSTV library adds new modes, we can't automatically support them
 * - Requires manual code changes to add new mode information
 * - Not maintainable long-term
 * 
 * Ideal Solution:
 * - Query MMSSTV library at runtime for mode metadata
 * - Automatically discover available modes
 * - Handle extended modes without code changes
 * 
 * Implementation approach for v2.2:
 * 1. Analyze MMSSTV library API for metadata functions
 * 2. Create wrapper function: mmsstv_get_mode_info_runtime()
 * 3. Call at initialization instead of using static table
 * 4. Cache results for performance
 * 5. Fallback to hardcoded table if library fails
 * 
 * Estimate: 2-3 hours implementation + 1 hour testing
 * 
 * Current Status: Working correctly with hardcoded data - no blocker
 * Severity: Low (works fine, just not ideal architecture)
 * 
 * MMSSTV Integration Status:
 * - Basic mode enumeration: ✓ WORKING
 * - Sample rate selection: ✓ WORKING
 * - Protocol detection: ✓ WORKING
 * - Mode details: ⚠ HARDCODED (works, not dynamic)
 */
```

2. **Update function header:**
```c
/**
 * Get detailed mode information for a protocol variant
 *
 * Returns technical details about an SSTV mode:
 * - Resolution (width x height)
 * - Transmission time in seconds
 * - VIS code
 * - Color encoding (RGB vs YUV)
 * 
 * @param protocol SSTV protocol code (e.g., SSTV_PROT_M1)
 * @param out_details Pointer to mode details structure
 * @return Error code (SLOWFRAME_OK on success)
 *
 * @note Currently uses hardcoded mode table. Future versions
 *       will query MMSSTV library dynamically if available.
 *       Current approach is valid and thoroughly tested.
 *
 * @see MMSSTVMode - contains resolution, timing, encoding info
 */
```

**Validation:**
1. Documentation clear and helpful
2. No code changes required
3. Explains current design decision

---

### Task 2.3: Create Configuration Flag Migration Guide
**Priority:** MEDIUM  
**Effort:** 2 hours  
**Risk:** None (documentation only)  
**Files:** New file `docs/CONFIG_MIGRATION_v2.0_to_v2.1.md`

**Content:**

```markdown
# SlowFrame Configuration Migration: v2.0 → v2.1

This guide helps users migrate from v2.0 to v2.1 CLI flags.

## What Changed

### Removed: `-G` (Grid Square) Flag
**v2.0:** `-G <grid_square>`  
**v2.1:** Removed - use overlay system instead  
**Why:** Overlapping functionality with new generic overlay system

### Deprecated (Still Works): `-S` (Station ID)
**v2.0:** `-S <callsign>` - Creates CW Morse output  
**v2.1:** `-S <callsign>` - Still works, now creates overlay text (backward compat)  
**Status:** Works but should migrate to new `-O` flag

### New: Unified Overlay System
**Flags:** `-O`, `-P`, `-C`, `-B`, `-F`, `-A`  
**Benefit:** Supports unlimited overlays with full customization

## Migration Examples

### Example 1: Simple Callsign Overlay

#### v2.0 Syntax
```bash
slowframe -i photo.jpg -S "N0CALL" -o output.wav
```

#### v2.1 New Syntax (Recommended)
```bash
slowframe -i photo.jpg -O "N0CALL" -P bottom -o output.wav
```

#### v2.1 Alternative (Old Syntax Still Works)
```bash
slowframe -i photo.jpg -S "N0CALL" -o output.wav
```

Note: In v2.1, if `-S` is used without CW parameters (`-W`, `-T`),
it's automatically treated as an overlay.

### Example 2: Dual Overlay (Callsign + Grid)

#### v2.0 Syntax (No Longer Works)
```bash
slowframe -i photo.jpg -S "N0CALL" -G "EM12ab" -o output.wav
# This fails in v2.1: -G flag removed
```

#### v2.1 New Syntax (Required)
```bash
slowframe -i photo.jpg \
  -O "N0CALL" -P top \
  -O "EM12ab" -P bottom \
  -o output.wav
```

#### v2.1 Unified Syntax (Alternative)
```bash
slowframe -i photo.jpg \
  -T "N0CALL|p=top" \
  -T "EM12ab|p=bottom" \
  -o output.wav
```

### Example 3: Styled Overlay

#### v2.0
Not possible - only default styling.

#### v2.1
```bash
slowframe -i photo.jpg \
  -O "N0CALL" -P bottom -C white -B blue -F 32 \
  -o output.wav
```

Breakdown:
- `-O "N0CALL"` - Overlay text
- `-P bottom` - Place at bottom of image
- `-C white` - Text color (white)
- `-B blue` - Background color (blue)
- `-F 32` - Font size (32 pixels)

### Example 4: CW Morse Code (With Callsign Audio)

This feature remains unchanged in v2.1.

```bash
slowframe -i photo.jpg -C "N0CALL" -W 20 -T 700 -o output.wav
```

Note: The `-C` flag here means "CW callsign", not "color".
It generates Morse code audio at the end of the transmission,
separate from text overlays.

## Important Distinctions in v2.1

### `-S` Flag (Multiple Uses)
- Without `-W` or `-T`: Creates text overlay (backward compat)
- With `-W` or `-T`: Creates CW Morse code audio signature
- Confusing but maintained for backward compatibility

### `-C` Flag (Two Different Uses)
- With `-O` or `-S` (overlay context): Overlay text color
- Standalone: CW callsign for Morse code audio
- Context determines meaning

**Recommendation:** Use `-O` for overlays instead of `-S` to avoid confusion.

## Feature Availability

| Feature | v2.0 | v2.1 | Status |
|---------|------|------|--------|
| Text overlay | Basic | Full | ✅ Improved |
| Multiple overlays | 1 | Unlimited | ✅ New |
| Custom colors | No | Yes | ✅ New |
| Custom placement | Limited | Full | ✅ New |
| Custom font size | Limited | Full | ✅ New |
| Text alignment | No | Yes | ✅ New |
| CW code audio | Yes | Yes | ✅ Unchanged |
| Grid square | Yes | No* | ⚠️ Removed |

*Grid square can still be added as text overlay in v2.1

## Deprecation Timeline

**v2.1** (Current): Old flags work, new flags recommended  
**v2.2** (Planned): Old flags show deprecation warnings  
**v2.3+** (Future): Old flags may be removed

Recommendation: Migrate to new syntax now to avoid surprises in future versions.

## Backward Compatibility Notes

The v2.1 parser includes automatic fallback logic:
1. Detects old flag patterns
2. Converts to new overlay system automatically
3. Shows informational message if verbose mode enabled

This means your old scripts will keep working, but consider updating
them to use the new more-powerful syntax.

## Testing Your Migration

After upgrading to v2.1:

```bash
# Test old syntax still works
slowframe -i photo.jpg -S "N0CALL" -v

# Test new syntax
slowframe -i photo.jpg -O "N0CALL" -P bottom -v

# Test color support (new)
slowframe -i photo.jpg -O "N0CALL" -C "#FF0000" -v

# Verify -G flag no longer works
slowframe -i photo.jpg -G "EM12ab" 2>&1
# Should show: "unknown option -- G" or similar
```

## Getting Help

If your old scripts break:
1. Check what warning/error message is shown
2. Refer to the examples above for new equivalent syntax
3. Review the CLI help: `slowframe -h`
4. Check [BUILD.md](BUILD.md) for additional examples

See [QUICK_START.md](QUICK_START.md) for more examples using v2.1 syntax.
```

**Validation:**
1. All code examples work
2. Clear migration path shown
3. Feature matrix is accurate
4. Testing instructions provided

---

## MEDIUM-TERM ACTIONS (Weeks 4-8)

### Task 3.1: Implement Text Color Support
**Priority:** HIGH  
**Effort:** 4-6 hours  
**Risk:** MEDIUM  
**File:** [src/image/image_text_overlay.c](src/image/image_text_overlay.c)

**When:** After v2.1 release, start of v2.2 development

**Implementation Plan:**

1. **Research & Planning (30 min)**
   - Evaluate FreeType2 integration complexity
   - Check font availability on target systems
   - Decide on font fallback strategy

2. **Dependency Management (1 hour)**
   - Add FreeType2 to Makefile
   - Update CI/CD configurations
   - Document new dependency requirement

3. **Implementation (3-4 hours)**
   - Create font loading utility
   - Implement text glyph rendering
   - Add color support to rendering pipeline
   - Update overlay compositor

4. **Testing (1 hour)**
   - Unit tests for color rendering
   - Integration tests with full pipeline
   - Edge case testing (invalid colors, missing fonts)

5. **Documentation (30 min)**
   - Update function headers
   - Add implementation notes
   - Document font requirements

**Tracking:** Create GitHub issue in project for this work

---

### Task 3.2: Refactor MMSSTV Mode Discovery
**Priority:** MEDIUM  
**Effort:** 2-3 hours  
**Risk:** MEDIUM (requires testing with MMSSTV library)  
**File:** [src/slowframe_mmsstv_adapter.c](src/slowframe_mmsstv_adapter.c)

**When:** v2.2 development, coordinate with MMSSTV library updates

**Implementation Plan:**

1. **API Analysis (30 min)**
   - Study MMSSTV library API
   - Identify runtime metadata functions
   - Design wrapper interface

2. **Implementation (1.5-2 hours)**
   - Create runtime query functions
   - Implement fallback to hardcoded data
   - Add caching for performance
   - Update mode enumeration

3. **Testing (30 min)**
   - Verify metadata correctness
   - Test with multiple MMSSTV versions
   - Verify fallback behavior
   - Performance validation

**Tracking:** Create GitHub issue depending on MMSSTV library

---

## LONG-TERM ACTIONS (Months 3+)

### Task 4.1: Implement Color Bar Visual Rendering
**Priority:** MEDIUM  
**Effort:** 5-6 hours  
**Risk:** LOW (new feature, no backward compat concerns)  
**File:** [src/image/image_color_bars.c](src/image/image_color_bars.c)

**Status:** Structure defined but not yet implemented

**Implementation Approach:**
- Render colored vertical/horizontal bars
- Support multiple color specifications
- Integrate with image composite pipeline
- Add comprehensive test coverage

---

### Task 4.2: Documentation Consolidation
**Priority:** LOW  
**Effort:** 3-4 hours  
**Risk:** None (documentation only)

**Goals:**
- Update ARCHITECTURE.md with v2.1 details
- Create DEVELOPMENT_GUIDE.md for contributors
- Consolidate deprecation notices
- Archive old design documents

---

## SUCCESS CRITERIA

### For Each Task:
- [ ] Code compiles without warnings
- [ ] All existing tests pass
- [ ] New tests added if functionality changed
- [ ] Documentation updated
- [ ] Backward compatibility maintained (when applicable)

### Overall Audit Completion:
- [ ] All immediate actions completed
- [ ] All deprecated code clearly marked
- [ ] All TODO items documented and prioritized
- [ ] Migration guides in place
- [ ] Team trained on new architecture

---

## TRACKING SPREADSHEET TEMPLATE

```
Task ID | Title | Priority | Effort | Status | Owner | Notes
--------|-------|----------|--------|--------|-------|-------
1.1 | Update comment | HIGH | 15m | TODO | - | -
1.2 | Legacy README | HIGH | 30m | TODO | - | -
2.1 | Document colors | MEDIUM | 1h | TODO | - | v2.2 target
2.2 | MMSSTV docs | MEDIUM | 1h | TODO | - | v2.2 target
2.3 | Migration guide | MEDIUM | 2h | TODO | - | pre-v2.1
3.1 | Color impl | HIGH | 5h | BACKLOG | - | v2.2 development
3.2 | MMSSTV refactor | MEDIUM | 3h | BACKLOG | - | v2.2 development
4.1 | Color bars | MEDIUM | 6h | BACKLOG | - | v2.3+
4.2 | Doc consolidation | LOW | 4h | BACKLOG | - | v2.3+
```

---

## COMMUNICATION PLAN

### For Users
1. Update CHANGELOG with deprecation notices
2. Add migration guide to documentation
3. Show informational messages in verbose mode
4. Maintain backward compatibility

### For Developers
1. Code review checklist for backward compatibility
2. Contributing guide with architecture overview
3. TODO prioritization and tracking
4. Regular sync meetings on deprecation timeline

### For Maintainers
1. Monthly progress reports
2. Deprecation timeline tracking
3. Risk assessment for each change
4. Performance regression testing

---

**End of Action Plan**

Next Steps:
1. Assign tasks to team members
2. Create GitHub/project management issues
3. Set target dates for each phase
4. Schedule review meetings
5. Monitor progress weekly
