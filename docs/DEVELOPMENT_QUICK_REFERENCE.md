
# PiSSTVpp2 v2.1 Development Guide - Quick Reference

**Your roadmap for modernizing and extending PiSSTVpp2 with optional MMSSTV support**

---

## What We're Building

A **modernized, community-friendly SSTV encoder** that:

✅ **Keeps native 7 modes always working** (no breaking changes)  
✅ **Dynamically detects MMSSTV library** (if installed by user)  
✅ **Expands CLI modes automatically** (7 native → 50+ with MMSSTV)  
✅ **Stays accessible to ham radio community** (readable, documented, extensible code)  
✅ **Maintains 100% backward compatibility** (all 55 tests passing)  

---

## The Plan in 30 Seconds

### Phase 1: Foundations (10-18 hrs)
Define error codes, config management, and state handling. Create the infrastructure that makes the rest possible.

### Phase 2: Image Refactoring (16-20 hrs) 
Split image loading/processing/aspect into focused modules. Makes code easier to understand and maintain.

### Phase 3: SSTV Mode System (12-16 hrs)
Build dynamic mode registry. Paves way for native + MMSSTV modes to coexist.

### Phase 4: MMSSTV Integration (11-14 hrs)
Library detection, adapter layer, mode loading. The bridge to 50+ new modes.

### Phase 5: Build System (6-8 hrs)
Smart detection of MMSSTV library. Graceful fallback if not available.

### Phase 6-7: Testing & Docs (24-29 hrs)
Validate everything works, document for contributors.

### Phase 8: Release (6-13 hrs)
Final validation, release notes, publication.

**Total: 80-100 hours** (roughly 2-3 weeks of focused work)

---

## Document Guide

### For Overall Understanding
1. **START HERE:** [MODERNIZATION_AND_MMSSTV_ROADMAP.md](./MODERNIZATION_AND_MMSSTV_ROADMAP.md)
   - Big picture architecture
   - Design philosophy
   - Component diagrams
   - Success criteria

### For Task Planning & Tracking
2. **THEN READ:** [DEVELOPMENT_TASK_LIST_v2.1.md](./DEVELOPMENT_TASK_LIST_v2.1.md)
   - 50+ specific, actionable tasks
   - Estimated effort for each
   - Testing strategy per task
   - Success criteria per phase

### For Implementation Details
3. **REFERENCE:** Project structure
   - Current: [ARCHITECTURE.md](./ARCHITECTURE.md)
   - Future: Updated version of ARCHITECTURE.md (to be created during work)

### For Understanding Community Value
4. **UNDERSTAND:** Why code accessibility matters
   - Ham radio culture values experimentation
   - Open source best practices
   - Making code contribution-friendly

---

## Quick Navigation by Role

### If You're a **Code Reviewer**
1. Read MODERNIZATION_AND_MMSSTV_ROADMAP.md sections A-C (Architecture & Design)
2. Use DEVELOPMENT_TASK_LIST_v2.1.md to verify tasks complete as expected
3. Check against success criteria in both documents

### If You're a **Developer Starting Phase 1**
1. Read MODERNIZATION_AND_MMSSTV_ROADMAP.md Part B1 (Unified Error Handling)
2. Open DEVELOPMENT_TASK_LIST_v2.1.md§Phase 1
3. Read the specific task: Task 1.1, Task 1.2, Task 1.3, Task 1.4
4. Implement each task in order, testing after each

### If You're Working on **MMSSTV Integration**
1. Read MODERNIZATION_AND_MMSSTV_ROADMAP.md Part A (Architecture)
2. Focus on DEVELOPMENT_TASK_LIST_v2.1.md§Phase 4
3. Reference mmsstv_stub.h (will be created during Phase 4.1)
4. Understand the detection flow before implementation

### If You're **Writing Documentation**
1. Read MODERNIZATION_AND_MMSSTV_ROADMAP.md Part F (Documentation)
2. Focus on DEVELOPMENT_TASK_LIST_v2.1.md§Phase 7
3. Use existing docs as style reference

### If You're **Creating Tests**
1. Read MODERNIZATION_AND_MMSSTV_ROADMAP.md Part E (Testing)
2. Focus on DEVELOPMENT_TASK_LIST_v2.1.md§Phase 6
3. Reference current test suite structure in tests/

---

## Key Design Decisions (Why We're Doing This)

### Why Mode Registry?
**Current:** Hardcoded 7 modes in pisstvpp2_sstv.c  
**Problem:** Can't easily add MMSSTV modes without massive refactor  
**Solution:** Dynamic registry that native + MMSSTV modes plug into  
**Benefit:** Clean separation, easy to add modes, community contributions simpler  

### Why Runtime Detection?
**Current:** Would need rebuild to enable MMSSTV support  
**Problem:** Builds fail if MMSSTV not installed; users don't know why  
**Solution:** Detect library at runtime, graceful fallback  
**Benefit:** Users install MMSSTV anytime, app automatically uses it  

### Why Environment Variables?
**Current:** No way to tell app where library is  
**Problem:** Standard paths might not work in all environments  
**Solution:** MMSSTV_LIB_PATH env var, multiple fallback paths  
**Benefit:** Works in dev environments, Docker, custom installations  

### Why Keep Native Modes?
**Current:** Only option is to wait for MMSSTV library  
**Problem:** Blocks users who want working app now  
**Solution:** Native modes always available, MMSSTV optional  
**Benefit:** Best of both worlds - works today, extends tomorrow  

### Why Code Accessibility?
**Current:** Complex codebase harder to understand/modify  
**Problem:** Ham radio community values DIY; complex code discourages participation  
**Solution:** Modular design, clear naming, documentation for contributors  
**Benefit:** More people can understand, improve, extend the code  

---

## Development Workflow

### Getting Started

```bash
# 1. Read the master plan
cat docs/MODERNIZATION_AND_MMSSTV_ROADMAP.md | less

# 2. See the task breakdown
cat docs/DEVELOPMENT_TASK_LIST_v2.1.md | less

# 3. Pick a phase to work on
# 4. Check current status
git status

# 5. Create feature branch
git checkout -b feature/phase-1-error-codes

# 6. Build current version
make clean && make all

# 7. Verify tests pass
./tests/test_suite.py

# 8. Make your changes
# 9. Test again
./tests/test_suite.py
```

### Commit Strategy

```bash
# Small, focused commits with clear messages

git commit -m "feat(error): Add unified error code system

- Create src/util/error.h with PISSTVPP2_OK, ERR_* constants
- Implement error_string() function
- Add error_log() for consistent logging
- Tests: All 55 tests still passing"

# Link to task list
# E.g., "Completes Task 1.1 from DEVELOPMENT_TASK_LIST_v2.1.md"
```

### Testing Between Phases

After each phase, verify:
```bash
make clean && make all    # No compile errors
./tests/test_suite.py     # All 55 tests pass
make analyze              # No static analysis issues (future)
```

---

## Infrastructure Created

### New Files Structure (By Phase)

**Phase 1: Error Handling & Config**
```
src/util/
├── error.h              (Error code constants)
└── error.c              (Error string mapping)

src/core/
├── pisstvpp2_config.h   (Config structures)
├── pisstvpp2_config.c   (Config management)
├── pisstvpp2_context.h  (Global state)
└── pisstvpp2_context.c  (State management)
```

**Phase 2-3: Modularization**
```
src/image/
├── image_loader.c/h     (Loading)
├── image_processor.c/h  (Processing)
└── image_aspect.c/h     (Aspect correction)

src/sstv/
├── mode_registry.c/h    (Dynamic mode system)
├── sstv_native.c/h      (Native mode registration)
├── sstv_encoder.c/h     (Unified dispatcher)
├── modes_martin.c/h     (Martin mode defs)
├── modes_scottie.c/h    (Scottie mode defs)
└── modes_robot.c/h      (Robot mode defs)
```

**Phase 4: MMSSTV Integration**
```
src/include/
├── mmsstv_stub.h        (MMSSTV API definition)

src/mmsstv/
├── mmsstv_loader.c/h    (Library detection)
└── mmsstv_adapter.c/h   (Mode bridge)
```

---

## Important Constraints & Principles

### Must Not Break
- ✗ Existing CLI options (all must work exactly as before)
- ✗ Audio output quality (identical to v2.0)
- ✗ Test suite (must maintain 55/55 passing)
- ✗ Build compatibility (Linux, macOS, Raspberry Pi)

### Must Maintain
- ✓ Backward compatibility (no v1 → v2 migration needed)
- ✓ 100% test pass rate
- ✓ Code comprehensibility (accessible to community)
- ✓ Native mode availability (always works)

### Should Improve
- ✓ Code organization (modular, focused files)
- ✓ Error handling (consistent, helpful messages)
- ✓ Documentation (clear, with examples)
- ✓ Extensibility (easy to add new modes/formats)

---

## Resources & References

### Existing Documentation
- Current Architecture: [ARCHITECTURE.md](./ARCHITECTURE.md) (1260 lines)
- User Guide: [USER_GUIDE.md](./USER_GUIDE.md) (1962 lines)
- Quick Start: [QUICK_START.md](./QUICK_START.md) (307 lines)
- Test Results: [TEST_EXECUTION_SUMMARY.md](./TEST_EXECUTION_SUMMARY.md) (295 lines)

### Code to Study
- Main entry: `src/pisstvpp2.c` (811 lines) - understand current flow
- SSTV encoder: `src/pisstvpp2_sstv.c` (784 lines) - becomes mode functions
- Image module: `src/pisstvpp2_image.c` (~400 lines) - refactor into 3 modules
- Audio encoder: `src/pisstvpp2_audio_encoder.c/h` (~240 lines) - mostly changes refs

### External References
- MMSSTV (original): http://www.qsl.net/mmsstv/
- libvips documentation: https://libvips.github.io/
- Ham radio resources (to come)

---

## Progress Tracking

### Current Status (As of Feb 10, 2026)
- ✅ v2.0 core complete (7 modes, 3 formats, 55/55 tests)
- ✅ Documentation comprehensive (28 files)
- ✅ Legacy code cleaned up and separated
- ⏳ Modernization planning complete (this document)
- ⏳ Development ready to begin

### Tracking Checklist
- [ ] Phase 1 complete (error codes, config, context)
- [ ] Phase 1 validation (55/55 tests passing)
- [ ] Phase 2 complete (image module refactoring)
- [ ] Phase 2 validation (tests passing, identical output)
- [ ] Phase 3 complete (mode registry, native modes)
- [ ] Phase 3 validation (tests passing, dynamic modes working)
- [ ] Phase 4 complete (MMSSTV detection, integration)
- [ ] Phase 4 validation (graceful with/without MMSSTV)
- [ ] Phase 5 complete (build system)
- [ ] Phase 6-7 complete (testing, documentation)
- [ ] Phase 8 complete (release prep)
- [ ] ✅ v2.1 Released

---

## Support & Escalation

### Need Clarification?
- Refer to MODERNIZATION_AND_MMSSTV_ROADMAP.md (detailed design)
- Refer to DEVELOPMENT_TASK_LIST_v2.1.md (specific tasks)
- Check ARCHITECTURE.md (current code structure)

### Running Into Issues?
- Check if covered in existing tests (tests/ directory)
- Look for similar patterns in current code
- Consult documentation for that module
- Escalate with details: what you tried, what failed, expected behavior

### Need to Make a Decision?
- Check if covered in roadmap "Design Decisions" section
- Refer to "Principles & Guidelines" in main roadmap
- Consider impact on backward compatibility
- Consult with project lead if unclear

---

## Next Steps

### For Project Lead
1. Review MODERNIZATION_AND_MMSSTV_ROADMAP.md
2. Confirm approach aligns with vision
3. Identify any timeline constraints
4. Schedule start of Phase 1

### For First Developer (Phase 1)
1. Read both planning documents thoroughly
2. Set up development branch: `git checkout -b feature/phase-1-error-codes`
3. Start with Task 1.1 (Error Code System)
4. Implement `src/util/error.h` and `src/util/error.c`
5. Verify compilation: `make clean && make all`
6. Document progress

### For Community
1. Watch for Phase 1 completion announcement
2. Test development builds (when available)
3. Provide feedback
4. Prepare for MMSSTV integration (Phases 4-5)

---

## Questions?

This guide should answer:
- **What are we building?** ← See "What We're Building"
- **Why are we building it?** ← See "Key Design Decisions"
- **How do we build it?** ← See full MODERNIZATION_AND_MMSSTV_ROADMAP.md
- **What are the tasks?** ← See DEVELOPMENT_TASK_LIST_v2.1.md
- **What's the timeline?** ← See "Development Timeline" in roadmap
- **How do I contribute?** ← See "Development Workflow"

For questions not covered: Check the main roadmap documents or escalate.

---

**Remember:** The goal is to create code that ham radio enthusiasts can understand, modify, and extend. Keep it simple, document reasoning, and value community participation.

Good luck! 🎉
