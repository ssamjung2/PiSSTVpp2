# Structure Audit Action Plan

**Quick Fix Guide** - Apply in this order (estimated: 2 hours total)

---

## Phase 1: Critical Fix (Include Paths) - 15 minutes

### Files to Update: 4 C files in src/image/

**Change pattern:**
```c
// OLD:
#include "../include/image/image_loader.h"
#include "../util/error.h"
#include "../include/logging.h"

// NEW:
#include "image/image_loader.h"
#include "error.h"
#include "logging.h"
```

### Files:
1. **[src/image/image_loader.c](src/image/image_loader.c)** - Lines 40-42
2. **[src/image/image_processor.c](src/image/image_processor.c)** - Lines 23-26
3. **[src/image/image_text_overlay.c](src/image/image_text_overlay.c)** - Lines 20-22
4. **[src/image/image_aspect.c](src/image/image_aspect.c)** - Lines 30-31

### Why: Consistency with all other source files

---

## Phase 2: Portability Fix (Test Paths) - 30 minutes

### Files to Update: 6 Python files in tests/

**Change pattern:**
```python
# OLD:
executable_path="/Users/ssamjung/Desktop/WIP/PiSSTVpp2/bin/pisstvpp2",
images_dir="/Users/ssamjung/Desktop/WIP/PiSSTVpp2/tests/images"

# NEW:
from pathlib import Path
script_dir = Path(__file__).parent
executable_path=str(script_dir.parent / "bin" / "pisstvpp2"),
images_dir=str(script_dir / "images")
```

### Files:
1. **[tests/test_integration.py](tests/test_integration.py)** - Lines 43, 455
2. **[tests/test_file_io_errors.py](tests/test_file_io_errors.py)** - Lines 30, 626
3. **[tests/run_all_tests.py](tests/run_all_tests.py)** - Lines 28, 268

### Why: Portable to any directory, enables CI/CD

---

## Phase 3: Organization Fix (Audio Modules) - 45 minutes

### Steps:
1. Create: `mkdir src/audio`
2. Move: `mv src/audio_encoder_*.c src/audio/`
3. Update [Makefile](makefile) - Add `AUDIO_DIR = $(SRC_DIR)/audio`
4. Update [Makefile](makefile) compile rules for audio files
5. No include changes needed (files already use simple includes)

### Makefile Changes:
```makefile
# Add these lines to directory section:
AUDIO_DIR = $(SRC_DIR)/audio

# Update SRC_FILES to reference new location:
$(SRC_DIR)/audio/audio_encoder_wav.c \
$(SRC_DIR)/audio/audio_encoder_aiff.c \
$(SRC_DIR)/audio/audio_encoder_ogg.c \

# Add new compile rule:
$(BIN_DIR)/%.o: $(AUDIO_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
```

### Why: Matches image module pattern, prepared for future

---

## Phase 4: Test Organization Fix - 30 minutes

### Steps:

1. **Create directory structure:**
```bash
mkdir -p tests/fixtures/audio
mkdir -p tests/test_outputs/text_overlay
mkdir -p tests/test_outputs/text_overlay_comprehensive
```

2. **Move audio fixtures:**
```bash
mv tests/images/*.wav tests/fixtures/audio/
mv tests/images/*.aiff tests/fixtures/audio/
mv tests/images/*.ogg tests/fixtures/audio/
```

3. **Move test outputs:**
```bash
mv tests/custom_output.wav tests/test_outputs/
```

4. **Update any file references** (if code references old paths):
   - [tests/test_file_io_errors.py](tests/test_file_io_errors.py)
   - [tests/test_performance_stress.py](tests/test_performance_stress.py)
   - Any other files that reference moved audio files

### New Structure:
```
tests/
├── images/           (image fixtures only)
├── fixtures/         (other reference data)
│   └── audio/        (audio fixtures)
├── test_outputs/     (generated outputs)
│   ├── text_overlay/
│   └── text_overlay_comprehensive/
└── (test Python files)
```

### Why: Clear distinction between inputs and outputs

---

## Phase 5: Convenience Improvement (Optional) - 5 minutes

### Option A: Create symlink to keep convenience
```bash
ln -s tests/show_overlay_tests.py show_overlay_tests.py
mv tests/show_overlay_tests.py show_overlay_tests.py
```

### Option B: Just move and document
```bash
mv show_overlay_tests.py tests/
```

Then add to README:
```markdown
## Quick Test:
python tests/show_overlay_tests.py
```

### Why: Consistency with other test scripts

---

## Validation Checklist

After each phase, run:

```bash
# After Phase 1:
make clean && make all
grep -r "#include.*\.\." src/  # Should show NOTHING

# After Phase 2 & 3:
cd tests && python3 test_suite.py

# After Phase 4:
ls tests/images/          # Should be .png, .jpg, .gif, .bmp only
ls tests/fixtures/audio/  # Should be .wav, .aiff, .ogg
ls tests/test_outputs/    # Should exist and be writable

# After Phase 5:
which show_overlay_tests.py    # Or confirm tests/show_overlay_tests.py works
```

---

## Estimated Effort

| Phase | Task | Time | Impact |
|-------|------|------|--------|
| 1 | Fix include paths | 15 min | HIGH - Consistency |
| 2 | Fix test paths | 30 min | HIGH - Portability |
| 3 | Audio module structure | 45 min | MEDIUM - Organization |
| 4 | Test fixtures organization | 30 min | MEDIUM - Clarity |
| 5 | Convenience script | 5 min | LOW - Polish |
| **Total** | | **2 hours** | **All improvements** |

---

## Risk Assessment

- **Risk Level:** Very Low
- **Backward Compatibility:** 100% (no functional changes)
- **Build System:** Tested with Makefile already handles paths
- **Tests:** All should pass after changes
- **Rollback:** Simple (git revert if needed)

---

## Notes

- All changes are **organizational only** - no logic changes
- Changes **improve consistency** without breaking anything
- Phase 1 is **blocking** (other phases independent)
- Phases 2-5 can be done in any order
- Each phase can be done in a separate commit for clarity

---

## Git Commands for Implementation

```bash
# Phase 1: Include fixes (manual edits)
git add src/image/*.c
git commit -m "Fix: Use consistent include paths in image modules"

# Phase 2: Test path fixes (manual edits)
git add tests/*.py
git commit -m "Fix: Use relative paths in test files for portability"

# Phase 3: Audio module reorganization
mkdir src/audio
git mv src/audio_encoder_*.c src/audio/
git add Makefile
git commit -m "Refactor: Move audio encoders to src/audio/ for consistency"

# Phase 4: Test organization
mkdir -p tests/fixtures/audio tests/test_outputs/{text_overlay,text_overlay_comprehensive}
git mv tests/images/*.{wav,aiff,ogg} tests/fixtures/audio/ 2>/dev/null || true
git mv tests/custom_output.wav tests/test_outputs/ 2>/dev/null || true
git commit -m "Organize: Separate test fixtures from test outputs"

# Phase 5: Symlink/move convenience script
ln -s tests/show_overlay_tests.py show_overlay_tests.py
# Or: git mv show_overlay_tests.py tests/
git commit -m "Organize: Move test convenience script to tests/"
```

---

## References

- [Full Audit Report](STRUCTURAL_AUDIT_REPORT.md)
- [Architecture Documentation](ARCHITECTURE.md)
- [Build Documentation](BUILD.md)
