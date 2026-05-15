# SlowFrame — OSS Release Cleanup Plan
**Status:** Full content-based analysis complete (all 151 docs reviewed)  
**Analysis Date:** Based on full document content review  
**Pre-cleanup git tag:** `v2.1-pre-cleanup`

---

## Overview

This plan was produced by reading every document in `docs/` (151 `.md` files) and
cross-referencing their claims against each other and the source code.  The project is
in good shape to release, but has significant doc debt from a rapid-fire development log
style.  The target state is ~30 clean user/developer-facing documents.

**Current state:** 151 `.md` files in `docs/` + 4 PDFs (not evaluated)  
**Target state:** ≈ 30 files in `docs/`, plus revised `README.md`/`CONTRIBUTING.md` in root

---

## Part 1 — Critical Code Issues Found During Review

These issues were uncovered during the document review.  They must be resolved or
formally deferred before tagging a public release.

### 1.1  Confirmed Issues (Open as of Feb 14, 2026)

| ID | Severity | Location | Issue | Source doc |
|----|----------|----------|-------|------------|
| **H-2** | High | `src/slowframe_image.c` | Duplicate functions: `get_file_extension()`, `apply_center_transformation()` etc. also exist in `src/image/` submodules.  Remove dups from `slowframe_image.c`. | PROJECT_IMPROVEMENT_TASKS.md §H-2 |
| **H-3** | High | `src/audio_encoder_*.c` | Inconsistent error handling — some encoder functions use `fprintf(stderr,...)` + return `-1` instead of `error_log()` + return `SLOWFRAME_ERR_*` code. | PROJECT_IMPROVEMENT_TASKS §H-3 |
| **H-4** | Medium | `src/image/image_text_overlay.c:81` | Color rendering limitation is undocumented.  The `-T` overlay renders a colored rectangle placeholder; actual text color is auto-selected (white/black by bg brightness only).  FreeType custom color support is deferred to v2.2 — this must be documented clearly for users. | PROJECT_IMPROVEMENT_TASKS §H-4 |
| **M-4** | Low | `tests/images/` | Misplaced files — `.wav`/`.aiff`/`.ogg` reference files live in `tests/images/` alongside PNGs/JPEGs.  Should move to `tests/fixtures/audio/`. | PROJECT_IMPROVEMENT_TASKS §M-4 |
| **M-6** | Medium | `src/slowframe_config.c`, `src/slowframe_image.c`, `src/slowframe.c` | Three functions exceed 200 lines (`slowframe_config_parse()` ~400 ln, `apply_single_overlay()` ~376 ln, `main()` ~500 ln).  Not blocking but a maintainability concern for contributors. | PROJECT_IMPROVEMENT_TASKS §M-6 |

### 1.2  Previously Identified Issues Since Resolved (do not re-open)

| Issue | Resolution | Date |
|-------|-----------|------|
| Text overlay compositing bug (`vips_composite2` not called) | Fixed; confirmed working with `--verbose` output | Feb 14, 2026 |
| Include path inconsistency (`src/image/*.c` used `../include/`) | All image files updated to non-relative includes | Feb 14, 2026 |
| Grid square feature references in headers | References removed; feature sunsetted | Feb 14, 2026 |

### 1.3  Security Issues (from `docs/SECURITY_TEST_REPORT.md`)

The security test report documents 11 open input-validation gaps.  All involve the
callsign/text fields accepting potentially dangerous characters (backtick, `%s` format
strings, control chars, null bytes, >4096-byte inputs).  These are the most important
issues to fix before public release since users will be passing callsigns from config
files or shell scripts.

**Do not delete `SECURITY_TEST_REPORT.md`** — it is the authoritative issue tracker
for these 11 items.

### 1.4  `docs/USER_GUIDE.md` Is Outdated

`USER_GUIDE.md` still says "Version 2.0" and lists only 7 SSTV modes.  The current
release is v2.1 with 51 modes, text overlay, and MMSSTV support.  This file needs a
major update before release.

### 1.5  `docs/PHASE3_4_MMSSTV_IMPLEMENTATION_GUIDE.md` Contains Local Paths

This doc hardcodes `/Users/ssamjung/Desktop/WIP/mmsstv-portable/build` — a developer's
local machine path.  The doc is also superseded by `MMSSTV_SETUP_GUIDE.md`.  Delete it.

---

## Part 2 — Document Disposition

### 2A — KEEP AS-IS (25 files)

These documents have clear user or developer value and need no changes.

| File | Value |
|------|-------|
| `ARCHITECTURE.md` | Project architecture; module map |
| `BUILD.md` | Build instructions; dependency table |
| `CLI_COMPARISON.md` | Migration reference: pisstvpp → slowframe CLI mapping |
| `CONTRIB_ADDING_SSTV_MODES.md` | Mode contribution guide for developers |
| `CONTRIB_FIXING_BUGS.md` | Bug fix contribution guide |
| `DEVELOPER_WALKTHROUGH.md` | Code walkthrough for new contributors |
| `DEVELOPMENT_QUICK_REFERENCE.md` | One-page cheat-sheet: build, flags, mode registry |
| `EXIF_QUICK_REFERENCE.md` | EXIF format reference for tiled SSTV metadata |
| `LEGACY_BUILD.md` | pisstvpp/pifm build reference for v1.x users |
| `LICENSE.md` | License |
| `MMSSTV_INTEGRATION.md` | How MMSSTV library integrates with SlowFrame |
| `MMSSTV_MODE_REFERENCE.md` | MMSSTV mode specs/parameters |
| `MMSSTV_SETUP_GUIDE.md` | MMSSTV library installation and configuration |
| `MODE_REFERENCE.md` | Complete mode list with frequency/timing specs |
| `MODE_REGISTRY_QUICK_REFERENCE.md` | Mode registry API quick reference |
| `PROJECT_IMPROVEMENT_TASKS.md` | Comprehensive open task list (authoritative as of Feb 14) |
| `QRM_TESTING_GUIDE.md` | QRM/noise testing procedures |
| `QUICK_START.md` | 5-minute getting-started guide |
| `RECOVERY_INTEGRATION.md` | QRM recovery pipeline integration reference |
| `RECOVERY_QUICK_REFERENCE.md` | QRM recovery quick-command card |
| `RECOVERY_TOOLS_REFERENCE.md` | Recovery tool documentation |
| `SECURITY_TEST_REPORT.md` | Active security issues tracker (11 open items) |
| `SSTV_MODES_RESEARCH.md` | Research notes on mode specifications (historical context) |
| `SSTV_NOISE_GENERATOR_GUIDE.md` | Noise generator usage for QRM testing |
| `TEST_CASES.md` | Test case specifications |
| `TEST_OVERLAY_COMPREHENSIVE_README.md` | Overlay test suite documentation |
| `TEST_SUITE_INTEGRATION_GUIDE.md` | Test framework integration guide (developer) |
| `TESTING_COMPREHENSIVE_README.md` | Main test suite guide |
| `VERIFICATION_GUIDE.md` | Verification workflows and commands |

### 2B — KEEP + UPDATE REQUIRED (5 files)

| File | Required Change |
|------|----------------|
| `USER_GUIDE.md` | **Major rewrite.** Currently says v2.0 with 7 modes. Update to v2.1: 51 modes, `-T` text overlay (with color-box limitation noted), MMSSTV, QRM recovery, all audio formats. |
| `LEGACY_COMPONENTS.md` | **Rename** to `LEGACY_TOOLS.md`. Minor edits to reflect that `src/legacy/` files should be removed (task M-1 in PROJECT_IMPROVEMENT_TASKS.md). |
| `SSTV_MODES_QUICK_REFERENCE.md` | Remove "draft" and "TBD" markers; verify all 51 modes are listed. |
| `QUICK_START.md` | Add a second example showing text overlay (`-T "K9ABC"`) and note the colored-rectangle limitation.  Add MMSSTV quick-start reference. |
| `VERIFICATION_GUIDE.md` | Verify all CLI examples use current v2.1 flag syntax; remove any v2.0 examples. |

### 2C — CONSOLIDATE (4 files → 1 new file)

Create **`docs/OVERLAY_REFERENCE.md`** by merging the four overlay feature documents:

| Source file | Content to extract |
|-------------|-------------------|
| `BACKGROUND_BAR_FEATURE.md` | Background bar parameters, CLI examples |
| `VERTICAL_BAR_FEATURE.md` | Vertical bar parameters, CLI examples |
| `XY_POSITIONING_GUIDE.md` | Position keywords, custom XY coords, anchor points |
| `TIMESTAMP_FEATURE.md` | Timestamp format string syntax, timezone handling |

After creating `OVERLAY_REFERENCE.md`, delete all four source files.

> Note: `TEXT_OVERLAY_REDESIGN_GENERIC_MULTIOVERLAY.md` (design doc, feature is
> implemented) can be deleted without extracting — the design is now the code.

### 2D — DELETE: Redundant Docs Covered by Existing Kept Files

These documents are superseded by or duplicated by files in the Keep list:

| File | Reason to delete |
|------|-----------------|
| `LEGACY_CODE_ANALYSIS.md` | Subset of `LEGACY_COMPONENTS.md` |
| `MMSSTV_API_UPDATES.md` | Superseded by `MMSSTV_INTEGRATION.md` |
| `TEST_SUITE_README.md` | Older version of `TESTING_COMPREHENSIVE_README.md` |
| `TEST_PRACTICAL_GUIDE.md` | Superseded by `TESTING_COMPREHENSIVE_README.md` |
| `TEST_QUICK_START.md` | Content in `TESTING_COMPREHENSIVE_README.md` + `QUICK_START.md` |
| `VERIFICATION_SYSTEM_SUMMARY.md` | Superseded by `VERIFICATION_GUIDE.md` |
| `OVERLAY_TEST_QUICK_REFERENCE.md` | Covered by `TEST_OVERLAY_COMPREHENSIVE_README.md` |
| `TESTING_QUICK_REFERENCE.md` | Covered by `TESTING_COMPREHENSIVE_README.md` |
| `COMPREHENSIVE_TEST_QUICK_REFERENCE.md` | Covered by `TESTING_COMPREHENSIVE_README.md` |

### 2E — DELETE: Issue/Audit Docs (findings captured in kept files)

Audit and code review reports whose findings are now fully captured in
`PROJECT_IMPROVEMENT_TASKS.md` and `SECURITY_TEST_REPORT.md`:

```
CODE_CLEANUP_ACTION_PLAN.md
COMPREHENSIVE_CODE_AUDIT_REPORT.md
COMPREHENSIVE_CODEBASE_ANALYSIS_2026-02-14.md
DEVELOPMENT_TASK_LIST_v2.1.md
ERROR_HANDLING_AUDIT.md
HEADER_DESIGN_ANALYSIS.md
HOLISTIC_CODE_REVIEW_FEB12_2026.md
IMPLEMENTATION_GUIDE.md
INTEGRATION_GAP_ANALYSIS_TEXT_OVERLAY.md
LEGACY_CODE_AUDIT_REPORT.md
PADDING_INCONSISTENCY_ANALYSIS.md
STRUCTURAL_AUDIT_REPORT.md
TEST_HARNESS_IMPROVEMENT_PLAN.md
TEXT_OVERLAY_DEVELOPMENT_TASK_LIST_v2_1.md
TEXT_OVERLAY_IMPLEMENTATION_ANALYSIS_v2_1.md
TEXT_OVERLAY_REDESIGN_GENERIC_MULTIOVERLAY.md
TEXT_OVERLAY_SYNTAX_ALTERNATIVES.md
V2_1_STATUS_UPDATE_CRITICAL_ANALYSIS_FEB11_2026.md
```

> ⚠️  Before deleting `IMPLEMENTATION_GUIDE.md`: verify whether the
> `filter_phantom_groups()` function it describes has been added to
> `src/stitch_tiles.c`.  If not, extract the C code to a comment/issue in
> `PROJECT_IMPROVEMENT_TASKS.md` first.

### 2F — DELETE: Internal Development Logs

Phase summaries, session/week logs, planning snapshots, and test-result snapshots
produced during development that have no user value in a public repo:

```
ANALYSIS_COMPLETION_SUMMARY.md
ANALYSIS_DELIVERY_SUMMARY.md
BACKGROUND_BAR_COMPLETION_SUMMARY.md
BW24_IMPLEMENTATION_COMPLETE.md
CODE_CLEANUP_COMPLETION_SUMMARY.md
COMPLETE_RECOVERY_STRATEGIES_DEPLOYMENT.md
COMPREHENSIVE_TEST_COMPLETION.md
CONSOLIDATION_COMPLETE.md
DELIVERY_SUMMARY.md
DEVELOPMENT_STRATEGY_SUMMARY.md
DOCUMENTATION_COMPLETE.md
DOCUMENTATION_INDEX.md
DOCUMENTATION_INDEX_CONSOLIDATED.md
DOCUMENTATION_UPDATE_SUMMARY.md
EXIF_FULL_ENDTOEND_TEST_REPORT.md
EXIF_PRESERVATION_FIX.md
EXIF_PRESERVATION_IMPLEMENTATION.md
EXIF_PRESERVATION_TEST_RESULTS.md
FULL_TEST_REPORT_HORUS_42_FULL.md
GRID_SQUARE_SUNSET_SUMMARY.md
HEADER_IMPROVEMENTS_SUMMARY.md
HEAVY_QRM_RECOVERY_SUMMARY.md
IMPROVEMENTS_STATUS.md
LEGACY_IMPROVEMENTS_SUMMARY.md
MODERNIZATION_AND_MMSSTV_ROADMAP.md
MODERNIZATION_PROGRESS_STATUS.md
OVERLAY_IMPLEMENTATION_COMPLETE.md
OVERLAY_TEST_SUMMARY.md
PADDING_FIX_VERIFICATION.md
PATH_REORGANIZATION_VERIFICATION.md
PHASE3_4_MMSSTV_IMPLEMENTATION_GUIDE.md
PHASE_1_COMPREHENSIVE_SUMMARY.md
PHASE_1_QUICK_START.md
PHASE_1_STATUS_ANALYSIS.md
PHASE_1_TASK_1_4_ACTION_PLAN.md
PHASE_2_1_COMPLETION_SUMMARY.md
PHASE_2_1_IMAGE_LOADER_ANALYSIS.md
PHASE_2_2_COMPLETION_SUMMARY.md
PHASE_2_3_COMPLETION_SUMMARY.md
PHASE_2_4_COMPLETION_SUMMARY.md
PHASE_2_READINESS_SUMMARY.md
PHASE_2_STARTUP.md
PHASE_3_1_COMPLETION_SUMMARY.md
PHASE_3_COMPLETION_SUMMARY.md
PHASE_4_IMPLEMENTATION_PLAN.md
PHASE_4_TASK_4_2_COMPLETION.md
PHASE_6_1_VALIDATION_FIXES.md
PHASE_6_2_MODERNIZATION_VALIDATION.md
PHASE_6_3_MMSSTV_INTEGRATION.md
PHASE_7_DOCUMENTATION_COMPLETE.md
PISSTVPP2_v2_0_MASTER_PLAN.md
PISSTVPP2_v2_1_MASTER_PLAN.md
PLANNING_DOCUMENTATION_INDEX.md
QRM_RECOVERY_EXECUTIVE_SUMMARY.md
QRM_STITCHING_TEST_RESULTS.md
QRM_TEST_RESULTS.md
REAL_LIBRARY_INTEGRATION_COMPLETE.md
REAL_WORLD_PROOF_OF_CONCEPT.md
REASSEMBLY_SUCCESS_PROOF.md
RECOVERY_IMPLEMENTATION_COMPLETE.md
RECOVERY_STRATEGY_TEST_RESULTS.md
RENAME_COMPLETION_SUMMARY.md
RENAME_PLAN_COMPREHENSIVE.md
RENAME_QUICK_REFERENCE.md
ROBUST_HEADER_PROTECTION_STRATEGY.md
SECURITY_HARDENING_COMPLETE.md
SESSION_COMPLETION_SUMMARY.md
STRUCTURE_FIX_ACTION_PLAN.md
TASK_LIST_UPDATE_COMPLETION_SUMMARY.md
TESTING_COMPLETE_SUMMARY.md
TESTING_FRAMEWORK_SUMMARY.md
TESTING_PLAN.md
TEST_COMPLETION_SUMMARY.md
TEST_EXECUTION_SUMMARY.md
TEST_RESULTS.md
TEST_RESULTS_OVERLAY_VERIFICATION.md
TEST_SUITE_TEXT_OVERLAY_ENHANCEMENT.md
TEST_UTILITIES_PATH_FIXES.md
TEXT_OVERLAY_VERIFICATION_REPORT.md
TILE_09_CORRUPTION_RESOLUTION.md
V2_1_PROGRESS_STATUS_FEB16_2026.md
V2_1_READINESS_STATUS.md
WEEK1_INFRASTRUCTURE_COMPLETE.md
WEEK2_COMPLETION_SUMMARY.md
WEEK2_OVERLAY_ENHANCEMENT_COMPLETE.md
WEEK2_QUICK_START.md
WEEK2_TESTING_INFRASTRUCTURE_OVERLAY.md
WEEK3_P1_QUICK_REFERENCE.md
WEEK3_P1_TESTING_PLAN.md
WEEK3_PHASE1_PROGRESS_REPORT.md
WEEK3_SAMPLE_REPORTS_GENERATED.md
WEEK3_SESSION_SUMMARY.md
WEEK3_VALIDATOR_COMPLETE.md
```

---

## Part 3 — New Files to Create

### `docs/OVERLAY_REFERENCE.md`  (from consolidation of 4 feature docs)

Content outline:
1. Overview of the `-T` flag syntax
2. Text positioning: position keywords, custom XY, anchor rules
3. Background bar options: horizontal bar, vertical bar
4. Timestamp syntax and format strings
5. Multiple overlays (chaining `-T` flags)
6. Known limitation: text color is auto-selected (white/black by bg brightness); custom text
   color via FreeType is planned for v2.2
7. Complete CLI examples

### `docs/DOCUMENTATION_INDEX.md`  (new clean index)

After cleanup, create a short index mapping topic → file so users can find docs.
This replaces the two deleted `DOCUMENTATION_INDEX*.md` files.

---

## Part 4 — Code-Level Cleanup (ordered by priority)

These are drawn from `PROJECT_IMPROVEMENT_TASKS.md` with priority added for OSS release.

### Must fix before public release

1. **Security: input validation** (from `SECURITY_TEST_REPORT.md`)  
   11 open gaps — callsign/text fields accept backtick injection, `%s` format strings,
   null bytes, overlength input.  These are exploitable when SlowFrame is called from
   scripts that incorporate user-controlled input.

2. **Duplicate functions H-2** — Remove `get_file_extension()`,
   `apply_center_transformation()`, `apply_pad_transformation()`,
   `apply_stretch_transformation()` from `slowframe_image.c`; use the versions in
   `src/image/`.

3. **USER_GUIDE.md accuracy** — Update version number, mode count, and add the
   text overlay color limitation before any public announcement uses this guide.

### Should fix before release

4. **Audio encoder error handling H-3** — Replace `fprintf(stderr,...)/return -1`
   with `error_log()/return SLOWFRAME_ERR_*` in all three audio encoder files.

5. **Text overlay color limitation documentation H-4** — Add a clear comment block
   to `image_text_overlay.c:81` and a note in `USER_GUIDE.md` explaining that v2.1
   only supports auto-selected text colors; custom color support is v2.2.

6. **src/legacy/ removal M-1** — Remove `src/legacy/*.c` (dead code, ~900 lines,
   never compiled into `bin/slowframe`).  Update `src/legacy/README.md` to explain
   they were removed but can be found in git history.

### Can defer to v2.2

7. Break down large functions (M-6): `slowframe_config_parse()`, `apply_single_overlay()`, `main()`
8. Reorganize audio encoder files into `src/audio/` subdirectory (M-3)
9. Reorganize test fixtures — move audio files out of `tests/images/` (M-4)
10. Add unit test framework (L-3)
11. FreeType integration for custom text colors (L-2)
12. Color bar rendering (L-1)

---

## Part 5 — Pre-Release Checklist

Before tagging a public release, complete these in order:

```
[ ] Resolve all 11 items in SECURITY_TEST_REPORT.md
[ ] Fix H-2: Remove duplicate functions from slowframe_image.c
[ ] Fix H-3: Audio encoder error handling consistency
[ ] Update USER_GUIDE.md to v2.1 (modes count, overlay section)
[ ] Update QUICK_START.md (add text overlay example with noted limitation)
[ ] Update SSTV_MODES_QUICK_REFERENCE.md (remove draft markers, verify 51 modes)
[ ] Update VERIFICATION_GUIDE.md (verify all CLI examples are current)
[ ] Create docs/OVERLAY_REFERENCE.md (consolidate 4 overlay feature docs)
[ ] Verify filter_phantom_groups() in stitch_tiles.c (see §2E note)
[ ] Remove src/legacy/*.c files (M-1 in PROJECT_IMPROVEMENT_TASKS.md)
[ ] Execute doc deletions (Parts 2D, 2E, 2F - ~120 files)
[ ] Rename LEGACY_COMPONENTS.md → LEGACY_TOOLS.md
[ ] Create docs/DOCUMENTATION_INDEX.md (clean new index)
[ ] Verify build passes: make clean && make all
[ ] Verify tests pass: cd tests && python3 run_all_tests.py
[ ] Confirm no local paths in any remaining doc (grep -r "ssamjung" docs/)
[ ] Confirm no local paths in source (grep -r "ssamjung" src/)
[ ] Update root README.md version to v2.1
[ ] Tag release: git tag v2.1.0
```

---

## Summary Statistics

| Category | Count |
|----------|-------|
| Keep as-is | 25 |
| Keep + update | 5 |
| Consolidate (4 → 1 new OVERLAY_REFERENCE.md) | -3 net |
| Delete: superseded docs | 9 |
| Delete: audit/issue docs | 18 |
| Delete: development logs | ~87 |
| **Files remaining after cleanup** | **~30** |

---

*This plan supersedes the previous surface-level `OSS_CLEANUP_PLAN.md`.*
*All file dispositions are based on full content review of each document.*
