# PiSSTV++ v2.1 Release Readiness Status

**Document Date:** February 10, 2026  
**Status:** ✅ READY FOR PHASE 1 START  
**Current Build:** v2.0 (137 KB) - All 55 tests passing

---

## Executive Summary

The **PISSTVPP2_v2_1_MASTER_PLAN.md** provides a unified, realistic strategy for modernizing PiSSTV++ with:
- **Total Scope:** 8 phases, 50+ specific tasks
- **Estimated Effort:** 90-115 developer hours
- **Realistic Timeline:** 2-3 weeks (one developer)
- **Key Features:** Dynamic mode registry, optional MMSSTV integration, FCC-compliant text overlay

This is a **significant improvement** over the previous v2.0 plan (500-620 hours, 13 scattered parts).

---

## Why v2.1 Over v2.0

| Aspect | v2.0 | v2.1 | Advantage |
|--------|------|------|-----------|
| **Total Hours** | 500-620 | 90-115 | **5.5× more realistic** |
| **Structure** | 13 scattered parts | 8 cohesive phases | **Better organization** |
| **MMSSTV** | Library integration blocked | Dynamic, graceful | **Solves real blocker** |
| **Text Overlay** | Not planned | Native FCC support | **New capability** |
| **Timeline** | 3-4 months | 2-3 weeks | **Deliverable** |
| **Backward Compat** | Assumed | Explicitly maintained | **55/55 tests always passing** |
| **Success Gates** | Vague | Clear Phase gates | **Trackable progress** |

---

## Current State (v2.0)

**What Works:**
- ✅ 55 integration tests passing
- ✅ 7 native SSTV modes (Martin, Scottie, Robot)
- ✅ 4 audio formats (WAV, AIFF, OGG, custom)
- ✅ Core image processing (load, scale, aspect ratio)
- ✅ Error code adoption (11/12 codes deployed)
- ✅ CLI functional and responsive
- ✅ Binary small (137 KB)

**Limitations in v2.0:**
- ❌ Monolithic code structure
- ❌ No mode registry system
- ❌ MMSSTV library blocked by dependencies
- ❌ No text overlay capability (FCC compliance missing)
- ❌ Error handling scattered (multiple styles)
- ❌ Configuration not modularized
- ❌ Limited documentation for contributors

**v2.1 Addresses All of These**

---

## v2.1 Phase Breakdown

### Phase 1: Unified Error Handling (10-18 hours) ✅ READY
**Status:** Straightforward refactoring, no blockers  
**Tasks:**
- Task 1.1: Error code system (src/util/error.h/c)
- Task 1.2: Config management module
- Task 1.3: Context/state encapsulation
- Task 1.4: Update all modules to use unified codes

**Deliverable:** All 55 tests still passing with consistent error handling  
**Success Gate:** No regressions, cleaner code

---

### Phase 2: Image Module Refactoring (24-26 hours) ✅ READY  
**Status:** New feature added: FCC-compliant text overlay  
**Tasks:**
- Task 2.1: Image loader extraction
- Task 2.2: Image processor extraction
- Task 2.3: Aspect ratio correction extraction
- **Task 2.4: Text overlay & color bars (8-10 hours)** ← NEW

**Why Text Overlay?**
- FCC Part 97.113(a)(4) requires station ID in SSTV transmissions
- Enables professional ham radio QSO documentation
- Adds call sign, grid square, timestamp support
- Supports color bars for contrast/branding

**Text Overlay Features:**
```bash
# Add call sign to bottom bar (most common)
./pisstvpp2 -i image.png \
  --text-overlay "K0ABC/EN96" \
  --text-placement bottom \
  -m m1 -o output.wav

# FCC-compliant multi-element layout
./pisstvpp2 -i image.png \
  --color-bar top \
  --text-overlay "K0ABC" \
  --color-bar bottom \
  --text-overlay-secondary "EN96ab" \
  -m s1 -o output.wav
```

**Deliverable:** All image processing modularized + FCC compliance feature  
**Success Gate:** 55 tests passing, text overlay tested with various placements

---

### Phase 3: SSTV & Mode Registry (13-17 hours) ✅ READY
**Status:** Clean refactoring, enables dynamic system  
**Tasks:**
- Task 3.1: Mode registry system design
- Task 3.2: Extract native mode definitions (Martin, Scottie, Robot)
- Task 3.3: Mode initialization & registration
- Task 3.4: Mode dispatcher implementation

**Result:** 7 native modes stored in registry, dispatcher selects at runtime

**Deliverable:** Dynamic mode selection system in place  
**Success Gate:** All 55 tests passing, mode lookup works

---

### Phase 4: MMSSTV Integration (14-18 hours) ✅ READY
**Status:** Graceful, non-blocking approach  
**Tasks:**
- Task 4.1: MMSSTV library detection (with fallback)
- Task 4.2: Dynamic mode adapter
- Task 4.3: Registry integration
- Task 4.4: CLI enhancement for dynamic modes

**Key Design:**
```
If MMSSTV library found:
  → Load 50+ additional modes at runtime
  → Registry expands to 57 total modes
  → CLI shows all available modes

If MMSSTV library missing:
  → Fall back to native 7 modes
  → App fully functional
  → No failures, no error messages
  → User can install MMSSTV later
```

**Environment Detection:**
```bash
# Library searches in order:
1. MMSSTV_LIB_PATH environment variable
2. pkg-config mmsstv-portable
3. Standard system paths (/usr/lib, /usr/local/lib, /opt/mmsstv/lib)
4. Graceful fallback if all fail
```

**Deliverable:** Dynamic mode loading, works with or without MMSSTV  
**Success Gate:** 7+ modes always available, MMSSTV modes when library present

---

### Phase 5: Build System Enhancement (8-10 hours) ✅ READY
**Status:** Non-invasive improvements  
**Tasks:**
- Task 5.1: Smart MMSSTV detection in Makefile
- Task 5.2: New build targets (debug, sanitize, analyze, coverage)
- Task 5.3: Improved build messages

**Example Output:**
```
[BUILD] Detecting MMSSTV library...
[INFO]  MMSSTV: ✓ found at /usr/local/lib/libmmsstv.so
[BUILD] Building with dynamic mode support (57 modes)
[BUILD] Compiling src/
[BUILD] Linking...
[✓] Binary: bin/pisstvpp2 (142 KB)
```

**Deliverable:** Smart build system, informative output  
**Success Gate:** Same binary size (or smaller), 55+ tests passing

---

### Phase 6: Testing & Validation (9-12 hours) ✅ READY
**Status:** Builds on existing test framework  
**Tasks:**
- Task 6.1: Backward compatibility testing (55/55 passing)
- Task 6.2: New module unit tests
- Task 6.3: MMSSTV integration testing (graceful skip if library absent)

**Testing Strategy:**
```
After Phase 1: All 55 tests pass ✓
After Phase 2: All 55 + new image tests pass ✓
After Phase 3: All 55 + registry tests pass ✓
After Phase 4: All 55 + MMSSTV tests pass (or skip gracefully) ✓
Final: Suite of 70-80 tests, 100% passing ✓
```

**Deliverable:** Comprehensive test coverage, documented success criteria  
**Success Gate:** All tests passing on Linux, macOS, RPi

---

### Phase 7: Documentation (16-19 hours) ✅ READY
**Status:** Required before release (includes FCC compliance docs)  
**Tasks:**
- Task 7.1: Architecture update (new diagrams for dynamic system)
- Task 7.2: Developer accessibility guide
- Task 7.3: Contributor playbooks (4 guides for common tasks)
- Task 7.4: MMSSTV setup guide

**Contributor Playbooks Include:**
- How to add new SSTV modes
- How to add new audio formats
- How to fix bugs in the system
- How to improve code organization

**FCC Compliance Documentation:**
- Task 2.4 includes FCC Part 97 compliance guide
- Text overlay feature documentation
- Examples of proper station ID transmission formats

**Deliverable:** Community-ready documentation  
**Success Gate:** Any contributor can understand system and add new modes

---

### Phase 8: Final Integration & Release (10-13 hours) ✅ READY
**Status:** Final validation before release  
**Tasks:**
- Task 8.1: Comprehensive testing pass (static analysis, memory, platforms)
- Task 8.2: Documentation review (all docs current)
- Task 8.3: Release notes & version updates

**Platform Testing:**
- [ ] Linux (x86, ARM)
- [ ] macOS (Intel, Apple Silicon)
- [ ] Raspberry Pi (any recent model)

**Quality Gates:**
- ✅ All tests passing
- ✅ No memory leaks (valgrind clean)
- ✅ Static analysis clean (clang)
- ✅ Backward compatible (all 55 original tests pass)
- ✅ Documentation complete
- ✅ Release notes written

**Deliverable:** v2.1 ready for production release  
**Success Gate:** v2.1 released to public

---

## Readiness Assessment

### Prerequisites Met ✅
- [x] v2.0 compiles successfully
- [x] All 55 tests passing
- [x] Clean codebase and git history
- [x] Comprehensive master plan documented
- [x] Clear task breakdown (50+ tasks)
- [x] Success criteria defined for each phase
- [x] Risk mitigation strategies in place
- [x] Development workflow documented
- [x] Contributor accessibility planned

### Resource Requirements
- **Developer Time:** 90-115 hours (realistic for one developer)
- **Test Infrastructure:** ✅ Already in place (249 comprehensive tests)
- **Build System:** ✅ Makefile ready to enhance
- **Documentation:** ✅ Existing docs as foundation
- **External Dependencies:** 
  - libvips (already required) ✅
  - MMSSTV library (optional, graceful fallback) ✅

### Risk Assessment ✅ LOW RISK
| Risk | Mitigation | Status |
|------|-----------|--------|
| Break existing code | Maintain 55/55 tests throughout | ✅ Planned |
| MMSSTV API unknown | Design stub interface & iterate | ✅ Designed |
| Timeline slip | Phase 2 can defer to v2.2 if needed | ✅ Fallback ready |
| Build complexity | Keep detection simple, fallback to native | ✅ Strategy clear |

---

## Immediate Next Steps

### Week 1: Phase 1 (Error Handling)
**Goal:** Unified error system, Context/Config modules

```bash
# 1. Create feature branch
git checkout -b feature/phase-1-error-codes

# 2. Read full task 1.1 from master plan
# File: /Users/ssamjung/Desktop/WIP/PiSSTVpp2/docs/PISSTVPP2_v2_1_MASTER_PLAN.md

# 3. Implement Task 1.1: Error Code System
create src/util/error.h
create src/util/error.c
- Define PISSTVPP2_OK, ERR_* constants
- Implement error_string() and error_log()
- Test all 55 tests still passing

# 4. Implement Task 1.2-1.4 following checklist
# 5. Commit when all tests pass
# 6. Ready for Phase 2
```

### Success Metrics Phase 1
- [ ] `make clean && make all` succeeds
- [ ] `./tests/test_suite.py` shows 55/55 passing
- [ ] No static analysis warnings
- [ ] Code follows existing style
- [ ] Error handling consistent throughout

---

## Timeline Estimate

| Phase | Duration | Status | Start |
|-------|----------|--------|-------|
| Phase 1: Error Handling | 10-18 hrs | ✅ Ready | Week 1 |
| Phase 2: Image Refactoring | 24-26 hrs | ✅ Ready | Week 1-2 |
| Phase 3: Mode Registry | 13-17 hrs | ✅ Ready | Week 2-3 |
| Phase 4: MMSSTV Integration | 14-18 hrs | ✅ Ready | Week 3 |
| Phase 5: Build System | 8-10 hrs | ✅ Ready | Week 3 |
| Phase 6: Testing | 9-12 hrs | ✅ Ready | Week 4 |
| Phase 7: Documentation | 16-19 hrs | ✅ Ready | Week 4-5 |
| Phase 8: Release Prep | 10-13 hrs | ✅ Ready | Week 5 |
| **TOTAL** | **90-115 hrs** | **✅ READY** | **Now** |

---

## Key Features in v2.1

### 1. Dynamic Mode Registry
**What:** Modes stored in runtime registry, not hardcoded  
**Why:** Enables MMSSTV library integration without recompilation  
**Benefit:** Add 50+ modes by installing library

### 2. Optional MMSSTV Integration
**What:** Gracefully load external MMSSTV mode library  
**Why:** License/dependency management  
**Benefit:** Scales from 7 to 57 modes without code changes

### 3. FCC-Compliant Text Overlay
**What:** Add call sign, grid square, timestamps to SSTV images  
**Why:** FCC Part 97.113(a)(4) requires station ID transmission  
**Benefit:** Professional SSTV operations, proper documentation

### 4. Modular Architecture
**What:** Code organized by responsibility (image, sstv, audio, etc.)  
**Why:** Easier to understand and contribute  
**Benefit:** Community can extend without deep knowledge

### 5. Unified Error Handling
**What:** All modules use consistent error codes and logging  
**Why:** Debugging, logging, consistency  
**Benefit:** Better user experience, easier maintenance

---

## Success Criteria for v2.1

### Before Release
- [x] Master plan complete
- [ ] Phase 1 complete (error codes) ← START HERE
- [ ] Phase 2 complete (image + text overlay)
- [ ] Phase 3 complete (mode registry)
- [ ] Phase 4 complete (MMSSTV integration)
- [ ] Phase 5 complete (build system)
- [ ] Phase 6 complete (testing)
- [ ] Phase 7 complete (documentation)
- [ ] Phase 8 complete (release prep)

### Release Checklist
- [ ] All 55 original tests passing
- [ ] New test suite passing (70-80 tests)
- [ ] Static analysis clean
- [ ] Memory tests clean (valgrind)
- [ ] Platform testing complete
- [ ] Documentation complete
- [ ] Release notes written
- [ ] Version bumped to 2.1

---

## Comparison: v2.0 vs v2.1

### v2.0 Master Plan (Superseded)
- 13 parts, scattered approach
- 500-620 hours estimated
- 3-4 month timeline
- Focused on cleanup and refactoring
- MMSSTV blocked on dependency
- No text overlay feature
- No clear success gates

### v2.1 Master Plan (Current)
- 8 cohesive phases ✅
- 90-115 hours estimated ✅
- 2-3 week timeline ✅
- Includes dynamic mode registry ✅
- MMSSTV solved with graceful integration ✅
- Adds FCC-compliant text overlay ✅
- Clear success gates per phase ✅

**v2.1 is 5-6× more realistic and includes more features.**

---

## Documentation References

**Read These Files (for understanding):**
1. [PISSTVPP2_v2_1_MASTER_PLAN.md](./PISSTVPP2_v2_1_MASTER_PLAN.md) - Complete strategy
2. [ARCHITECTURE.md](./ARCHITECTURE.md) - Current code structure
3. [QUICK_START.md](./QUICK_START.md) - Setup guide

**For Phase 1 (Error Codes):**
- Section "Phase 1: Unified Error Handling & Configuration" in master plan
- Tasks 1.1 through 1.4 with full checklists

---

## Recommendation

**Status:** ✅ **READY TO START PHASE 1**

The v2.1 master plan is comprehensive, realistic, and well-structured. All prerequisites are met:
- Codebase is stable (55/55 tests passing)
- Test infrastructure is excellent (249 comprehensive tests)
- Master plan is detailed (50+ specific tasks)
- Success criteria are clear (phase gates)
- Timeline is realistic (90-115 hours)

**Next Action:** Begin Phase 1 with Task 1.1 (Error Code System)

---

**Last Updated:** February 10, 2026  
**Status:** ✅ READY FOR DEVELOPMENT  
**Next Milestone:** Phase 1 Completion (expect 1-2 weeks)
