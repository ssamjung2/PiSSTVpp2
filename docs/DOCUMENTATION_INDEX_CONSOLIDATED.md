# PiSSTVpp2 Documentation Index - Consolidated

**This is the master navigation guide for all PiSSTVpp2 documentation.**

Last Updated: February 10, 2026

---

## 🎯 START HERE

### For Everyone
**[QUICK_START.md](./QUICK_START.md)** - Get pisstvpp2 running in 5 minutes
- Download/build
- Basic usage
- Example commands

---

## 👨‍💼 I'm a Project Lead / Manager

### Overview
1. **[PISSTVPP2_v2_1_MASTER_PLAN.md](./PISSTVPP2_v2_1_MASTER_PLAN.md)** ← **YOUR MAIN REFERENCE**
   - Executive Summary (status & timeline)
   - 8 phases with effort estimates
   - 50+ specific tasks
   - Success criteria & gates
   - Recommended: Read "Executive Summary" + "Phase Breakdown" sections (60 min)

### Project Status
- **Current Version:** 2.0 (stable, 55/55 tests passing, 7 SSTV modes)
- **Next Version:** 2.1 (modernization + optional MMSSTV support)
- **Effort:** 80-100 hours, 2-3 weeks for 1 developer
- **Status:** Ready for Phase 1 (error handling & configuration)

### Key Documents
- [DELIVERY_SUMMARY.md](./DELIVERY_SUMMARY.md) - v2.1 planning summary
- [DEVELOPMENT_STRATEGY_SUMMARY.md](./DEVELOPMENT_STRATEGY_SUMMARY.md) - High-level strategy

---

## 👨‍💻 I'm a Developer Starting Implementation

### Start Here
1. **[PISSTVPP2_v2_1_MASTER_PLAN.md](./PISSTVPP2_v2_1_MASTER_PLAN.md)** - Full plan
   - Read: "Phase Breakdown & Task List" for your phase
   - Follow: "Development Workflow" section
   - Reference: "Success Criteria" for testing

2. **[DEVELOPMENT_QUICK_REFERENCE.md](./DEVELOPMENT_QUICK_REFERENCE.md)** - Quick workflow guide
   - Git workflow
   - Build commands
   - Test commands
   - Commit standard

### Your Phase
**Identify your phase, read the relevant section:**

- **Phase 1:** Error codes, config, context (10-18 hrs)
  - Tasks 1.1-1.4 in master plan
  - Completion: All modules use unified error system

- **Phase 2:** Image module refactoring (16-20 hrs)
  - Tasks 2.1-2.3 in master plan
  - Completion: Image loading/processing/aspect separated

- **Phase 3:** SSTV mode system (13-17 hrs)
  - Tasks 3.1-3.4 in master plan
  - Completion: Native modes in registry, dispatcher working

- **Phase 4:** MMSSTV integration (14-18 hrs)
  - Tasks 4.1-4.4 in master plan
  - Completion: Dynamic mode loading operational

- **Phase 5+:** Build, testing, docs (see master plan)

### Code References
- [ARCHITECTURE.md](./ARCHITECTURE.md) - Current (v2.0) code structure
- [CODE_WALKTHROUGH.md](./CODE_WALKTHROUGH.md) *(to be created in Phase 7)* - How code flow works
- [DEVELOPER_ACCESSIBILITY.md](./DEVELOPER_ACCESSIBILITY.md) *(to be created in Phase 7)* - Contributing guide

---

## 🔨 I'm a Contributor / Community Member

### Getting Oriented
1. **[README.md](../README.md)** - Project overview
2. **[QUICK_START.md](./QUICK_START.md)** - Build and use it
3. **[ARCHITECTURE.md](./ARCHITECTURE.md)** - How it works

### Contributing
- [DEVELOPER_ACCESSIBILITY.md](./DEVELOPER_ACCESSIBILITY.md) *(Phase 7)* - How to contribute
- [CODE_WALKTHROUGH.md](./CODE_WALKTHROUGH.md) *(Phase 7)* - Code structure guide
- [CONTRIB_ADDING_SSTV_MODES.md](./CONTRIB_ADDING_SSTV_MODES.md) *(Phase 7)* - Add new SSTV mode
- [CONTRIB_ADDING_AUDIO_FORMATS.md](./CONTRIB_ADDING_AUDIO_FORMATS.md) *(Phase 7)* - Add new audio format
- [CONTRIB_FIXING_BUGS.md](./CONTRIB_FIXING_BUGS.md) *(Phase 7)* - Bug fix workflow
- [CONTRIB_IMPROVING_CODE.md](./CONTRIB_IMPROVING_CODE.md) *(Phase 7)* - Improve existing code

### Using pisstvpp2
- [USER_GUIDE.md](./USER_GUIDE.md) - Complete usage reference
- [MODE_REFERENCE.md](./MODE_REFERENCE.md) - All SSTV modes explained
- [CLI_COMPARISON.md](./CLI_COMPARISON.md) - Comparison with other tools
- [BUILD.md](./BUILD.md) - Build from source
- [MMSSTV_SETUP.md](./MMSSTV_SETUP.md) *(Phase 7)* - Optional: Enable 50+ additional modes

---

## 🧪 I'm Testing / QA

### Test Suite Documentation
- [TEST_SUITE_README.md](./TEST_SUITE_README.md) - How the test suite works
- [TEST_QUICK_START.md](./TEST_QUICK_START.md) - Run tests quickly
- [TEST_CASES.md](./TEST_CASES.md) - All test cases explained
- [TEST_PRACTICAL_GUIDE.md](./TEST_PRACTICAL_GUIDE.md) - Hands-on testing
- [TEST_RESULTS.md](./TEST_RESULTS.md) - Latest test run results
- [TEST_EXECUTION_SUMMARY.md](./TEST_EXECUTION_SUMMARY.md) - Summary of execution

### Test Validation (v2.1)
- Phase 6 in [PISSTVPP2_v2_1_MASTER_PLAN.md](./PISSTVPP2_v2_1_MASTER_PLAN.md)
  - Task 6.1: Backward compatibility testing (3-4 hrs)
  - Task 6.2: Modernization validation (3-4 hrs)
  - Task 6.3: MMSSTV integration testing (3-4 hrs)

---

## 📚 I'm Searching for Specific Information

### SSTV & Modulation
- [MODE_REFERENCE.md](./MODE_REFERENCE.md) - Current modes (v2.0)
- [MMSSTV_MODE_REFERENCE.md](./MMSSTV_MODE_REFERENCE.md) - MMSSTV library modes
- [SSTV Specifications.pdf](./SSTV%20Description%20and%20mode%20specifications.pdf) - SSTV standard
- [SSTV Mode Details.pdf](./SSTV%20Mode%20Details.pdf) - Mode technical details
- [SSTV Handbook.pdf](./sstv-handbook.pdf) - Comprehensive reference

### Architecture & Design
- [ARCHITECTURE.md](./ARCHITECTURE.md) - Current code structure (v2.0)
- [PISSTVPP2_v2_1_MASTER_PLAN.md](./PISSTVPP2_v2_1_MASTER_PLAN.md) - v2.1 design (Section B: "Code Modernization Areas")

### Build & Compilation
- [BUILD.md](./BUILD.md) - Complete build instructions
- [PISSTVPP2_v2_1_MASTER_PLAN.md](./PISSTVPP2_v2_1_MASTER_PLAN.md) - Phase 5 discusses build improvements

### Audio Format Support
- [USER_GUIDE.md](./USER_GUIDE.md) - Audio format options
- [PISSTVPP2_v2_1_MASTER_PLAN.md](./PISSTVPP2_v2_1_MASTER_PLAN.md) - Phase 2 discusses audio encoder organization

### MMSSTV Library
- [MMSSTV_SETUP.md](./MMSSTV_SETUP.md) *(Phase 7)* - How to install and configure
- [MMSSTV_MODE_REFERENCE.md](./MMSSTV_MODE_REFERENCE.md) - Available modes
- [MMSSTV_API_UPDATES.md](./MMSSTV_API_UPDATES.md) - Library API details
- [PISSTVPP2_v2_1_MASTER_PLAN.md](./PISSTVPP2_v2_1_MASTER_PLAN.md) - Section A2 "MMSSTV Library Detection"

---

## 🗂️ Complete Document List

### 📖 User Guides & References (For Everyone)
| Document | Purpose | Status |
|----------|---------|--------|
| [README.md](../README.md) | Project overview | ✅ Current |
| [QUICK_START.md](./QUICK_START.md) | 5-minute setup | ✅ Current |
| [USER_GUIDE.md](./USER_GUIDE.md) | Complete usage | ✅ Current |
| [BUILD.md](./BUILD.md) | Build instructions | ✅ Current |
| [MODE_REFERENCE.md](./MODE_REFERENCE.md) | SSTV modes (v2.0) | ✅ Current |
| [CLI_COMPARISON.md](./CLI_COMPARISON.md) | vs other tools | ✅ Reference |

### 🎯 v2.1 Development (Main Focus)
| Document | Purpose | Status |
|----------|---------|--------|
| [PISSTVPP2_v2_1_MASTER_PLAN.md](./PISSTVPP2_v2_1_MASTER_PLAN.md) | **Complete v2.1 roadmap** | ✅ **Active** |
| [DELIVERY_SUMMARY.md](./DELIVERY_SUMMARY.md) | v2.1 overview summary | ✅ Reference |
| [DEVELOPMENT_STRATEGY_SUMMARY.md](./DEVELOPMENT_STRATEGY_SUMMARY.md) | Strategy summary | ✅ Reference |
| [DEVELOPMENT_QUICK_REFERENCE.md](./DEVELOPMENT_QUICK_REFERENCE.md) | Developer workflow | ✅ Reference |
| [PLANNING_DOCUMENTATION_INDEX.md](./PLANNING_DOCUMENTATION_INDEX.md) | v2.1 planning docs | ✅ Reference |

### 🧪 Testing (Quality Assurance)
| Document | Purpose | Status |
|----------|---------|--------|
| [TEST_SUITE_README.md](./TEST_SUITE_README.md) | Test suite overview | ✅ Current |
| [TEST_EXECUTION_SUMMARY.md](./TEST_EXECUTION_SUMMARY.md) | Latest results | ✅ Current |
| [TEST_CASES.md](./TEST_CASES.md) | All 55 test cases | ✅ Current |
| [TEST_PRACTICAL_GUIDE.md](./TEST_PRACTICAL_GUIDE.md) | Hands-on testing | ✅ Current |
| [TEST_QUICK_START.md](./TEST_QUICK_START.md) | Run tests quickly | ✅ Current |
| [TEST_RESULTS.md](./TEST_RESULTS.md) | Detailed results | ✅ Reference |
| [TESTING_PLAN.md](./TESTING_PLAN.md) | Test strategy | ✅ Reference |

### 🏗️ Architecture & Design (v2.0 Reference)
| Document | Purpose | Status |
|----------|---------|--------|
| [ARCHITECTURE.md](./ARCHITECTURE.md) | Current code structure | ✅ Reference (v2.0) |
| [CODE_WALKTHROUGH.md](./CODE_WALKTHROUGH.md) | Code flow guide | ⏳ Planned (Phase 7) |
| [DEVELOPER_ACCESSIBILITY.md](./DEVELOPER_ACCESSIBILITY.md) | Contributing guide | ⏳ Planned (Phase 7) |

### 📡 MMSSTV Integration
| Document | Purpose | Status |
|----------|---------|--------|
| [MMSSTV_MODE_REFERENCE.md](./MMSSTV_MODE_REFERENCE.md) | 50+ MMSSTV modes | ✅ Reference |
| [MMSSTV_API_UPDATES.md](./MMSSTV_API_UPDATES.md) | Library API details | ✅ Reference |
| [MMSSTV_SETUP.md](./MMSSTV_SETUP.md) | Installation & setup | ⏳ Planned (Phase 7) |

### 📝 Legacy & Historical (Reference Only)
| Document | Purpose | Status |
|----------|---------|--------|
| [LEGACY_CODE_ANALYSIS.md](./LEGACY_CODE_ANALYSIS.md) | v1.0 code analysis | 📋 Archived |
| [LEGACY_COMPONENTS.md](./LEGACY_COMPONENTS.md) | v1.0 components | 📋 Archived |
| [LEGACY_BUILD.md](./LEGACY_BUILD.md) | v1.0 build notes | 📋 Archived |
| [DOCUMENTATION_COMPLETE.md](./DOCUMENTATION_COMPLETE.md) | v2.0 doc summary | 📋 Historical |
| [DOCUMENTATION_UPDATE_SUMMARY.md](./DOCUMENTATION_UPDATE_SUMMARY.md) | v2.0 updates | 📋 Historical |
| [PISSTVPP2_v2_0_MASTER_PLAN.md](./PISSTVPP2_v2_0_MASTER_PLAN.md) | v2.0 completion plan | 📋 Archived (superseded) |

### 🔗 References & External
| Document | Purpose | Status |
|----------|---------|--------|
| [SSTV Specifications.pdf](./SSTV%20Description%20and%20mode%20specifications.pdf) | SSTV standard | ✅ Technical |
| [SSTV Mode Details.pdf](./SSTV%20Mode%20Details.pdf) | Mode specs | ✅ Technical |
| [SSTV Handbook.pdf](./sstv-handbook.pdf) | Comprehensive guide | ✅ Technical |
| [LICENSE.md](./LICENSE.md) | GPLv3 license | ✅ Legal |

---

## 🚀 Development Status by Phase

### ✅ Completed
- **v2.0 Release:** All features complete & tested (55/55 tests passing)
- **Phase 0 (Cleanup):** Legacy code separated & documented
- **Documentation:** Initial guide, architecture, test docs

### 🟡 In Planning/Preparation
- **Phase 1-8 (v2.1):** Detailed roadmap complete, ready to begin
  - Phase 1: Error codes, config, context (10-18 hrs)
  - Phase 2: Image refactoring (16-20 hrs)
  - Phase 3: SSTV modes (13-17 hrs)
  - Phase 4: MMSSTV integration (14-18 hrs)
  - Phase 5: Build system (8-10 hrs)
  - Phase 6: Testing (9-12 hrs)
  - Phase 7: Documentation (16-19 hrs)
  - Phase 8: Release (10-13 hrs)

### ⏳ Pending
- Phase-specific contributor guides (created Phase 7)
- MMSSTV setup documentation (created Phase 7)
- Code walkthrough guides (created Phase 7)

---

## 💡 Quick Access by Role

```
IF YOU ARE...                      START WITH...

Project Lead                       PISSTVPP2_v2_1_MASTER_PLAN.md (Exec Summary)
Developer (Phase N)                PISSTVPP2_v2_1_MASTER_PLAN.md (Phase N section)
New Code Contributor               DEVELOPER_ACCESSIBILITY.md (Phase 7)
SSTV Mode Developer                CONTRIB_ADDING_SSTV_MODES.md (Phase 7)
Bug Fix Contributor                CONTRIB_FIXING_BUGS.md (Phase 7)
QA/Tester                          TEST_SUITE_README.md
End User                           QUICK_START.md or USER_GUIDE.md
System Administrator               BUILD.md or MMSSTV_SETUP.md
Researcher/Reference               ARCHITECTURE.md or SSTV PDFs
```

---

## 🎓 Learning Paths

### Path 1: Get Running (15 min)
1. [QUICK_START.md](./QUICK_START.md) - Build & run
2. [MODE_REFERENCE.md](./MODE_REFERENCE.md) - Understand modes
3. [USER_GUIDE.md](./USER_GUIDE.md) - Full options

### Path 2: Understand Architecture (90 min)
1. [README.md](../README.md) - Project goals
2. [ARCHITECTURE.md](./ARCHITECTURE.md) - Current design
3. [PISSTVPP2_v2_1_MASTER_PLAN.md](./PISSTVPP2_v2_1_MASTER_PLAN.md) - Future design
4. [MODE_REFERENCE.md](./MODE_REFERENCE.md) - How modes work

### Path 3: Contribute Code (2-3 hours)
1. [DEVELOPER_ACCESSIBILITY.md](./DEVELOPER_ACCESSIBILITY.md) - Philosophy
2. [CODE_WALKTHROUGH.md](./CODE_WALKTHROUGH.md) - How it works
3. [DEVELOPMENT_QUICK_REFERENCE.md](./DEVELOPMENT_QUICK_REFERENCE.md) - Workflow
4. [CONTRIB_*.md](.) - Your specific contribution

### Path 4: Develop v2.1 Phase (varies)
1. [PISSTVPP2_v2_1_MASTER_PLAN.md](./PISSTVPP2_v2_1_MASTER_PLAN.md) - Your phase
2. [DEVELOPMENT_QUICK_REFERENCE.md](./DEVELOPMENT_QUICK_REFERENCE.md) - Workflow
3. [TEST_SUITE_README.md](./TEST_SUITE_README.md) - How tests work
4. Code & commit!

---

## 📋 Document Management

### Document Status Indicators
- ✅ **Current** - In use, maintained for current version
- 🟡 **Active** - In development, may change
- ✅ **Reference** - Historical, may contain useful info
- 📋 **Archived** - Historical interest only
- ⏳ **Planned** - Scheduled for v2.1 Phase 7

### How Documents Are Organized
1. **User-facing**: README, QUICK_START, USER_GUIDE, BUILD
2. **Development**: Architecture, code guides, contributing guides
3. **Testing**: Test suite and results
4. **Planning**: v2.1 roadmap and task lists
5. **Reference**: SSTV specifications, legacy docs

### Updates & Deprecation
- This index is updated as documents are created/modified
- See [PISSTVPP2_v2_1_MASTER_PLAN.md](./PISSTVPP2_v2_1_MASTER_PLAN.md) "Document Management" section
- Old documents archived with notation, accessible for reference

---

## 🆘 Didn't Find It?

1. **Search the docs directory:** grep -r "your search term" *.md
2. **Check PISSTVPP2_v2_1_MASTER_PLAN.md:** Most comprehensive
3. **Ask in issues:** Community help available

---

**Last Updated:** February 10, 2026  
**Maintained By:** PiSSTVpp2 Project  
**Feedback:** Issues and PRs welcome!
