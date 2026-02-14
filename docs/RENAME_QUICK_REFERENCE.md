# SlowFrame Rename: Quick Reference Guide

**Project Rename Completed:** February 13, 2026  
**From:** PiSSTVpp2 (pisstvpp2) → **To:** SlowFrame (slowframe)

---

## For Developers

### Running the New Executable

```bash
# Build the project
make clean && make all

# Run the new executable
./bin/slowframe --help
./bin/slowframe -i image.jpg -f wav -p m1 -o output.wav
```

### Key Code Changes Summary

| Old Name | New Name | Quantity |
|----------|----------|----------|
| `pisstvpp2.c` | `slowframe.c` | 7 files |
| `pisstvpp2_*.h` | `slowframe_*.h` | 6 files |
| `PISSTVPP2_OK` | `SLOWFRAME_OK` | 63 constants |
| `PISSTVPP2_ERR_*` | `SLOWFRAME_ERR_*` | 40+ error codes |
| `pisstvpp2_*()` | `slowframe_*()` | 100+ functions |
| `PisstvppContext` | `SlowframeContext` | 68+ struct refs |

### Common Dev Tasks

#### Check for old references
```bash
grep -r "pisstvpp2\|PISSTVPP2" src --include="*.c" --include="*.h"
# Should return: 0 (in src/ only)
```

#### Rebuild cleanly
```bash
make clean
make all
```

#### Run tests
```bash
cd tests
python3 util/run_all_tests.py
```

#### Update after git pull
```bash
make clean
make all
# No special steps needed - all refs already updated
```

---

## For Documentation Writers

### Update Paths in Examples

**Old Style:**
```bash
./bin/pisstvpp2 -i photo.jpg
```

**New Style:**
```bash
./bin/slowframe -i photo.jpg
```

### Update Code References

**Old Style:**
```c
int result = pisstvpp2_image_load(filename);
if (result != PISSTVPP2_OK) {
    error_log(result, "Failed to load image");
}
```

**New Style:**
```c
int result = slowframe_image_load(filename);
if (result != SLOWFRAME_OK) {
    error_log(result, "Failed to load image");
}
```

---

## For CI/CD & Build Systems

### Update Scripts

If you have CI/CD scripts that reference the old names:

```bash
# Update executable references
sed -i 's/bin\/pisstvpp2/bin\/slowframe/g' build_script.sh

# Update artifact names
sed -i 's/pisstvpp2/slowframe/g' deploy_script.sh
```

### Key Artifact Names
- **Executable:** `bin/slowframe` (was `bin/pisstvpp2`)
- **Test runner:** `tests/slowframe` (was `tests/pisstvpp2`)
- **Class/Type:** `SlowframeContext` (was `PisstvppContext`)

---

## For Release Management

### Backward Compatibility Notes

**Breaking Changes:** NONE
- Internal rename only
- No API changes
- No protocol changes
- Binary behavior identical to PiSSTVpp2 v2.0

**What Users Need to Know:**
- Executable name changed from `pisstvpp2` to `slowframe`
- All command-line options remain identical
- All input/output formats unchanged
- Project rebrand: "PiSSTVpp2" → "SlowFrame"

### Migration Path for Users

If users have scripts using old name:

```bash
# Old (no longer works)
./pisstvpp2 -i photo.jpg

# New (use this)
./slowframe -i photo.jpg
```

---

## What to Check Before Release

- [ ] Executable builds successfully
- [ ] Test suite passes
- [ ] README.md updated with new name
- [ ] CONTRIBUTING.md updated
- [ ] example commands use `slowframe`
- [ ] no `pisstvpp2` references in active code
- [ ] Release notes mention name change
- [ ] Update any website/documentation links

---

## Git Commands Reference

### View the rename commits
```bash
git log --oneline rename/pisstvpp2-to-slowframe | head -10
```

### Show what changed
```bash
git diff main..rename/pisstvpp2-to-slowframe --stat
```

### Merge to main (when ready)
```bash
git checkout main
git pull origin main
git merge rename/pisstvpp2-to-slowframe
```

---

## Files Changed at a Glance

```
src/
├── slowframe.c                 (was pisstvpp2.c)
├── slowframe_image.c          (was pisstvpp2_image.c)
├── slowframe_sstv.c           (was pisstvpp2_sstv.c)
├── slowframe_audio_encoder.c  (was pisstvpp2_audio_encoder.c)
├── slowframe_config.c         (was pisstvpp2_config.c)
├── slowframe_context.c        (was pisstvpp2_context.c)
├── slowframe_mmsstv_adapter.c (was pisstvpp2_mmsstv_adapter.c)
├── include/
│   ├── slowframe_image.h          (was pisstvpp2_image.h)
│   ├── slowframe_sstv.h           (was pisstvpp2_sstv.h)
│   ├── slowframe_audio_encoder.h  (was pisstvpp2_audio_encoder.h)
│   ├── slowframe_config.h         (was pisstvpp2_config.h)
│   ├── slowframe_context.h        (was pisstvpp2_context.h)
│   └── slowframe_mmsstv_adapter.h (was pisstvpp2_mmsstv_adapter.h)
├── audio_encoder_*.c          (includes updated)
├── util/
│   ├── error.h                (constants renamed)
│   └── error.c                (constants renamed)
└── ... (all other refs updated)

tests/
├── slowframe                  (was pisstvpp2)
├── util/
│   ├── run_all_tests.py       (paths updated)
│   ├── test_*.py              (paths & refs updated)
│   └── *.sh                   (paths updated)
└── ...

makefile
├── TARGET = slowframe         (was pisstvpp2)
└── All file references        (updated)

Documentation
├── README.md                  (32+ references updated)
├── CONTRIBUTING.md            (21+ references updated)
├── docs/*.md                  (81+ files updated)
└── ...
```

---

## Troubleshooting

### Build fails with "not found" errors
```bash
# Clean and rebuild
make clean
make all

# If still failing, check #include paths
grep -n "#include \"pisstvpp2" src/*.c src/*.h
# Should return nothing - all should be "slowframe"
```

### Tests can't find executable
```bash
# Verify test binary exists
ls -la tests/slowframe

# If missing, it was probably renamed. Create symlink:
ln -sf ../bin/slowframe tests/slowframe
```

### Old references in docs
Documentation references to `pisstvpp2` in comments/examples are OK for backward compatibility.
Only remove if updating those specific sections.

---

## Summary

✅ **All the old references have been systematically replaced**  
✅ **Project structure and functionality unchanged**  
✅ **Build system fully updated**  
✅ **Tests and documentation updated**  
✅ **Ready for development and release**

For more details, see [RENAME_COMPLETION_SUMMARY.md](RENAME_COMPLETION_SUMMARY.md)
