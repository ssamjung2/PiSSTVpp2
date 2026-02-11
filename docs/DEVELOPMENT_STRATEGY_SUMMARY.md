
# v2.1 Development Strategy - Summary

**Created: February 10, 2026**

---

## What I've Created For You

You now have a **complete, documented strategy** for evolving PiSSTVpp2 while maintaining community accessibility. Here's what exists:

### 1. **MODERNIZATION_AND_MMSSTV_ROADMAP.md** (8000+ words)
The comprehensive architecture document explaining:
- **Part A:** Dynamic mode registry and MMSSTV library detection design
- **Part B:** Code modernization (error handling, modular refactoring)
- **Part C:** MMSSTV integration strategy (runtime detection, graceful fallback)
- **Part D:** Build system enhancements (smart MMSSTV detection)
- **Part E:** Testing & validation approach
- **Part F:** Documentation strategy emphasizing code accessibility
- **Part G:** Risk mitigation and success criteria

**Key insight:** The mode registry becomes the foundation for both native and MMSSTV modes to coexist dynamically.

### 2. **DEVELOPMENT_TASK_LIST_v2.1.md** (5000+ words)
The actionable breakdown with 50+ specific tasks organized by:
- **Phase:** 8 phases from foundations to release
- **Priority:** Must-have, should-have, nice-to-have
- **Effort:** Ranging from 2-6 hours per task
- **Testing:** Success criteria for each task
- **Tracking:** Progress matrix and milestone schedule

**Key insight:** Tasks are small, testable, and can be completed in 2-3 day sprints.

### 3. **DEVELOPMENT_QUICK_REFERENCE.md** (3000+ words)
The navigation and workflow guide with:
- 30-second overview of the entire plan
- Document guide (which doc to read for what)
- Quick navigation by role
- Key design decisions explained simply
- Development workflow and commit strategy
- Constraint and principle review
- Progress tracking checklist

**Key insight:** New developers can get oriented in minutes and start contributing immediately.

---

## The Strategy in One Diagram

```
Current State (v2.0)                 Target State (v2.1)
=====================================  =====================================

Hardcoded 7 Native Modes              Dynamic Mode Registry
      ↓                                     ↓
No MMSSTV Support                    ✓ Native 7 modes (always)
      ↓                               ✓ MMSSTV 50+ modes (if library)
Users must rebuild for new modes     ✓ Runtime library detection
                                     ✓ Environment var configuration
                                     ✓ Graceful fallback
                                     ✓ CLI scales with availability
```

---

## The Four Core Improvements

### 1. **Error Handling Unification** (Phase 1)
```c
// Before: Scattered error codes
if (function() == -1) { ... }
if (function() == 1) { ... }
fprintf(stderr, "Unclear error\n");

// After: Consistent system
if (function() != PISSTVPP2_OK) {
    error_log(PISSTVPP2_ERR_IMAGE_LOAD, "loading image.jpg");
    // Help text: "See docs/BUILD.md#troubleshooting"
}
```

### 2. **Code Modularization** (Phases 2-3)
```c
// Before: Everything interconnected
pisstvpp2.c (811 lines)
  ↓
pisstvpp2_image.c (loading, processing, aspect)
  ↓
pisstvpp2_sstv.c (7 modes hardcoded)
  ↓
pisstvpp2_audio_encoder.c

// After: Clear responsibilities
core/
  config.c/h, context.c/h
image/
  loader.c/h, processor.c/h, aspect.c/h
sstv/
  mode_registry.c/h, native.c/h
  modes_martin.c/h, modes_scottie.c/h, modes_robot.c/h
mmsstv/
  loader.c/h, adapter.c/h
audio/
  encoder.c/h, gen.c/h, wav.c/h, aiff.c/h, ogg.c/h
```

### 3. **Dynamic Mode Loading** (Phase 4)
```bash
# Looks for library in this order:
1. $MMSSTV_LIB_PATH (user override)
2. pkg-config mmsstv-portable (if installed)
3. /usr/lib, /usr/local/lib, /opt/lib (standard paths)
4. Falls back to native-only (that's OK!)

# CLI automatically extends:
Without MMSSTV:  pisstvpp2 -p m1|m2|s1|s2|sdx|r36|r72
With MMSSTV:     pisstvpp2 -p m1|m2|...|r72|pd120|avt90|...
```

### 4. **Code Accessibility** (Throughout)
- Every module has a single, clear purpose
- Large functions extracted to focused steps
- Comments explain WHY, not just WHAT
- Examples provided for common tasks
- Contributor guide for adding new modes/formats

---

## The Three Implementation Strategies

### Strategy A: Keep It Simple
**No breaking changes.** All CLI options work identically. All 55 tests pass throughout. Users don't even know code was refactored.

### Strategy B: Make It Extensible
**Mode registry.** New modes (native or MMSSTV) just plug in. No spaghetti. No duplicated logic.

### Strategy C: Make It Accessible
**Community first.** Code readable by ham radio enthusiasts. Documentation for contributors. Clear "how to add modes" examples.

---

## What This Means For Development

### Phase 1-3 (Weeks 1-4): Foundation
- Build error handling system
- Refactor image module
- Create mode registry
- Validate nothing broke (55/55 tests)
- **Result:** Modular, maintainable codebase

### Phase 4-5 (Weeks 5-6): MMSSTV Ready
- Implement MMSSTV detection
- Create adapter layer
- Dynamic CLI support
- **Result:** App ready for MMSSTV whenever library arrives

### Phase 6-8 (Weeks 7-8): Polish
- Comprehensive testing
- Documentation
- Release preparation
- **Result:** v2.1 ready for community

---

## Why This Approach Works

### For Current Users (Backward Compatibility)
✅ No recompilation required  
✅ All existing commands work identically  
✅ Audio output identical  
✅ Help text enhanced but familiar  

### For Future Development (Extensibility)
✅ Easy to add new SSTV modes (40+ with MMSSTV)  
✅ Easy to add new audio formats  
✅ Clear plugin architecture  
✅ Minimal code duplication  

### For Community Contribution (Accessibility)
✅ Code organized by purpose, not size  
✅ Focused files easier to understand  
✅ Guide for adding new modes step-by-step  
✅ Error messages helpful  
✅ Documentation guides contributors  

### For Maintainability (Long-term)
✅ Clear responsibility boundaries  
✅ Reduced coupling between modules  
✅ Unified error handling  
✅ Comprehensive tests prevent regressions  

---

## Key Metrics

| Metric | v2.0 | v2.1 Target |
|--------|------|-------------|
| Supported modes | 7 | 7-57 (with MMSSTV) |
| Audio formats | 3 | 3-4 (if OGG + MMSSTV) |
| Main program lines | 811 | ~300 (refactored) |
| Source files | 11 | ~25 (modular) |
| Error codes | Mixed (-1,0,1,2) | Unified (0-99) |
| Test coverage | 55 tests | 55+ tests |

---

## Decision Points & Contingencies

### If MMSSTV Library Not Available
✓ App still works perfectly with 7 native modes  
✓ No degraded functionality  
✓ Users can install library later, app auto-detects  

### If Timeline Pressure
1. Skip Phase 2 image refactoring (not critical)
2. Complete Phases 1, 3, 4 (core modernization)
3. Come back to Phase 2 later

### If Contributors Find Issues
1. Small backward-incompatible issue? Fix immediately, document
2. New design suggestion? Evaluate, incorporate if improves code
3. Missing documentation? Add it, thank contributor

---

## Resource Requirements

### Development Time
- One developer: 80-100 hours (2-3 weeks full-time)
- Two developers: ~50-60 hours each (parallel phases)
- With interruptions: 3-4 weeks realistic

### Test Time
- Continuous: 5 min per compile (included in dev time)
- Per phase: 5-10 min validation (included in dev time)
- Final: 30 min full regression test (before release)

### Documentation Time
- Included in task estimates
- Plan 1-2 hour review before release
- Community feedback likely improves further

---

## Success Definition

### Must Achieve
- ✅ 55/55 existing tests pass (no regressions)
- ✅ All native modes produce identical output to v2.0
- ✅ MMSSTV detection works (with/without library)
- ✅ Code compiles on Linux, macOS, Raspberry Pi
- ✅ Help text dynamically updates based on availability

### Should Achieve
- ✅ Code accessibility improved (community can contribute)
- ✅ Documentation expanded (how to add modes, etc.)
- ✅ Build system enhanced (clear status messages)
- ✅ Error messages helpful
- ✅ Zero compiler warnings

### Nice to Achieve
- ✅ Static analysis passing (clang)
- ✅ Memory leak detection passing (valgrind)
- ✅ Contributor playbook complete
- ✅ Example MMSSTV mode included

---

## Next Actions

### For You (Project Lead)
1. ✅ Review MODERNIZATION_AND_MMSSTV_ROADMAP.md
2. ✅ Review DEVELOPMENT_TASK_LIST_v2.1.md
3. ✅ Confirm approach alignment
4. → Identify development resources
5. → Schedule Phase 1 kickoff

### For First Developer (Phase 1)
1. → Read DEVELOPMENT_QUICK_REFERENCE.md
2. → Study DEVELOPMENT_TASK_LIST_v2.1.md§Phase 1
3. → Review current src/pisstvpp2.c structure
4. → Create feature branch: `git checkout -b feature/phase-1-error-codes`
5. → Start Task 1.1 (Error Code System)

### For Community  
- Watch for v2.1 development announcements
- Test development builds when available
- Provide feedback on code accessibility
- Prepare for contributing MMSSTV modes

---

## Additional Information

### Available Documentation
All documents are in `/Users/ssamjung/Desktop/WIP/PiSSTVpp2/docs/`

**New Documents Created Today:**
- `MODERNIZATION_AND_MMSSTV_ROADMAP.md` (comprehensive strategy)
- `DEVELOPMENT_TASK_LIST_v2.1.md` (50+ actionable tasks)
- `DEVELOPMENT_QUICK_REFERENCE.md` (workflow guide)
- `DEVELOPMENT_STRATEGY_SUMMARY.md` (this document)

**Existing Documentation:**
- `ARCHITECTURE.md` - Current code structure
- `USER_GUIDE.md` - Usage reference
- `BUILD.md` - Platform-specific build steps
- `TEST_EXECUTION_SUMMARY.md` - Current test status

### Getting Started
1. `cd /Users/ssamjung/Desktop/WIP/PiSSTVpp2`
2. `make clean && make all` (verify current state)
3. `./tests/test_suite.py` (verify 55/55 pass)
4. Read `docs/DEVELOPMENT_QUICK_REFERENCE.md`
5. Pick a task from Phase 1 to start

---

## Summary Statement

**You now have a complete, documented roadmap for evolving PiSSTVpp2 from a solid v2.0 foundation into a modern, extensible v2.1 that gracefully supports up to 57+ SSTV modes, while maintaining full backward compatibility and remaining accessible to the ham radio community.**

The strategy is:
- **Safe** (no breaking changes, constant testing)
- **Practical** (specific tasks, realistic timeline)
- **Community-focused** (code accessibility first)
- **Modern** (clean architecture, unified error handling)
- **Flexible** (works with or without MMSSTV library)

Everything is documented. You're ready to begin development whenever you are.

---

**Created:** February 10, 2026  
**Status:** Ready for Implementation  
**Feedback:** Refer to planning documents for detailed explanations
