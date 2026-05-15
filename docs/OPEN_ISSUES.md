 # SlowFrame — Open Issues
**Last updated:** Based on comprehensive code audit (Feb 14, 2026)  
**Detailed task descriptions:** See `docs/PROJECT_IMPROVEMENT_TASKS.md`  
**Security issues tracker:** See `docs/SECURITY_TEST_REPORT.md`

---

## 🔴 CRITICAL — Must fix before public release

### SEC-1: Callsign field accepts dangerous characters
**File:** `src/slowframe_config.c` — `_validate_callsign()`  
**Detail:** The `-C` callsign option accepts tab, carriage return, backticks, printf
format strings (`%x`), and ANSI escape sequences.  Amateur radio callsigns are
`[A-Z0-9/]` only; anything else should be rejected at parse time.  Backtick and format
string acceptance are injection-class vulnerabilities when SlowFrame is called from
scripts that incorporate untrusted or user-supplied callsigns.  
**Test coverage:** Tests exist in `SECURITY_TEST_REPORT.md` (7 failing cases in category 1).  
**Fix:** Add character-whitelist validation in `_validate_callsign()`.

### SEC-2: Octal notation accepted for numeric parameters
**File:** `src/slowframe_config.c`  
**Detail:** `-W 017` is accepted as 15 WPM (octal interpretation via `atoi()`-family
call) instead of being rejected.  Same issue may affect `-r` (sample rate) and the
numeric form of `-T` (CW tone).  
**Fix:** Replace `atoi()` with `strtol(..., 10)` using `endptr` validation; reject
any input with a leading `0` that isn't exactly `"0"`.

### SEC-3: Output filename accepts shell metacharacters
**File:** `src/slowframe_config.c`  
**Detail:** Output filenames containing `&` or printf format strings (e.g.,
`output_%x.wav`) are accepted without validation.  
**Fix:** Validate that output filename characters are in a safe set before opening the
output file.

---

## 🟡 HIGH — Should fix before release

### CODE-1: Duplicate functions in `slowframe_image.c`
**File:** `src/slowframe_image.c`  
**Detail:** `get_file_extension()`, `apply_center_transformation()`,
`apply_pad_transformation()`, `apply_stretch_transformation()` are defined in both
`slowframe_image.c` and in the `src/image/` sub-modules (`image_loader.c`,
`image_aspect.c`).  The sub-module versions are canonical; the duplicates in
`slowframe_image.c` should be removed and calls updated to go through `src/image/`.  
**See:** `PROJECT_IMPROVEMENT_TASKS.md` §H-2 for exact function list and steps.

### CODE-2: Audio encoders use inconsistent error reporting
**Files:** `src/audio_encoder_wav.c`, `src/audio_encoder_aiff.c`, `src/audio_encoder_ogg.c`  
**Detail:** Several encoder init/encode/finish/destroy functions use
`fprintf(stderr, "Error: ...")/return -1` instead of the project-standard
`error_log(SLOWFRAME_ERR_*, ...)/return SLOWFRAME_ERR_*` pattern.  This means errors
from the audio pipeline are reported differently from all other modules and can't be
caught uniformly by callers.  
**See:** `PROJECT_IMPROVEMENT_TASKS.md` §H-3 for the full function list.

### DOC-1: `USER_GUIDE.md` describes v2.0 (7 modes), not v2.1 (51 modes)
**File:** `docs/USER_GUIDE.md`  
**Detail:** The user guide still says "Version 2.0" and lists 7 modes.  v2.1 has 51
modes (8 native + 43 MMSSTV), text overlay, QRM recovery, and three audio formats.
The guide needs a major update before any public announcement links to it.

---

## 🟢 MEDIUM — Known limitations to document clearly

### LIM-1: Text overlay renders colored rectangle, not text glyphs (v2.1)
**Files:** `src/image/image_text_overlay.c:81`, `docs/USER_GUIDE.md`  
**Detail:** In v2.1, the `-T` text overlay feature composites a colored background
rectangle at the specified position.  Text color is auto-selected (white or black based
on background brightness) via Pango/Cairo, but custom text colors are not supported.
No actual font/glyph rendering beyond what libvips `vips_text()` provides is implemented.  
**Status:** Intentional deferral to v2.2 (FreeType integration planned).  
**Required action:** Document this limitation clearly in `USER_GUIDE.md` and `QUICK_START.md`
so users are not surprised.  The TODO comment at `image_text_overlay.c:81` should be
expanded to explain the roadmap.  
**See:** `PROJECT_IMPROVEMENT_TASKS.md` §H-4.

### LIM-2: `src/legacy/` contains ~900 lines of dead code
**Directory:** `src/legacy/`  
**Detail:** `pisstvpp.c` (~500 ln) and `pifm_sstv.c` (~400 ln) are never compiled into
the main binary.  They should be removed from the active tree; git history preserves them.
Update `src/legacy/README.md` to document their removal.  
**See:** `PROJECT_IMPROVEMENT_TASKS.md` §M-1.

---

## 🔵 DEFERRED — Planned for v2.2

- **FreeType integration** for custom text colors and fonts (§L-2)
- **Color bar rendering** for visual callsign bars (§L-1)
- **Unit test framework** (CUnit or similar) for C code (§L-3)
- **Refactor large functions** (`slowframe_config_parse()` ~400 ln,
  `apply_single_overlay()` ~376 ln, `main()` ~500 ln) (§M-6)
- **Reorganize audio encoder files** into `src/audio/` subdirectory (§M-3)
- **Move reference audio files** out of `tests/images/` into `tests/fixtures/audio/` (§M-4)

---

## ✅ Recently resolved (do not re-open)

| Issue | Resolution | Date |
|-------|-----------|------|
| Text overlay compositing (`vips_composite2` not called) | Fixed; compositing confirmed working | Feb 14, 2026 |
| Include path inconsistency in `src/image/*.c` | Standardized to non-relative includes | Feb 14, 2026 |
| Grid square references in headers | Removed; feature sunsetted | Feb 14, 2026 |
| Phantom group detection not in `stitch_tiles.c` | `filter_phantom_groups()` is implemented at line 1058 | Verified |
| Local developer paths in docs | `PHASE3_4_MMSSTV_IMPLEMENTATION_GUIDE.md` to be deleted | Pending deletion |
