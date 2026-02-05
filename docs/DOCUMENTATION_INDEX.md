# PiSSTVpp2 Documentation Index

**Complete guide to all documentation**

Version 2.0 | Updated: 2024

---

## 📖 Documentation Structure

PiSSTVpp2 documentation is organized into several categories:

### For Users

**Getting Started**
- [README](../README.md) - Project overview and quick start
- [Quick Start Guide](QUICK_START.md) - 5-minute introduction to PiSSTVpp2
- [User Guide](USER_GUIDE.md) - Complete feature reference

**Installation & Building**
- [Build Guide](BUILD.md) - Platform-specific build instructions
- [Installation Instructions](../README.md#installation) - Quick install guide

**Usage Reference**
- [Command-Line Reference](USER_GUIDE.md#command-line-options) - All options explained
- [Mode Reference](MODE_REFERENCE.md) - SSTV mode specifications
- [Sample Output](SAMPLE_OUTPUT.md) - Example transmissions

**Troubleshooting**
- [User Guide - Troubleshooting](USER_GUIDE.md#troubleshooting) - Common issues
- [Build Guide - Troubleshooting](BUILD.md#troubleshooting) - Build problems

### For Developers

**Architecture & Design**
- [Architecture Documentation](ARCHITECTURE.md) - System design and modules
- [Code Structure](ARCHITECTURE.md#system-architecture) - Module overview
- [Data Flow](ARCHITECTURE.md#data-flow) - Processing pipeline

**Development**
- [Build Guide](BUILD.md) - Compilation and dependencies
- [Test Suite Documentation](TEST_SUITE_README.md) - Running and writing tests
- [Extension Points](ARCHITECTURE.md#extension-points) - Adding features

**Testing & Quality**
- [Test Suite Guide](TEST_SUITE_README.md) - Test framework
- [Test Quick Start](TEST_QUICK_START.md) - Running tests
- [Practical Testing Guide](TEST_PRACTICAL_GUIDE.md) - Writing tests
- [Sample Test Output](SAMPLE_TEST_OUTPUT.md) - Expected results

### Advanced Topics

**Future Development**
- [MMSSTV Integration](MMSSTV_INTEGRATION.md) - v2.1 extended modes
- [MMSSTV Mode Reference](MMSSTV_MODE_REFERENCE.md) - 43 SSTV modes
- [MMSSTV API Updates](MMSSTV_API_UPDATES.md) - Integration design
- [Master Plan](PISSTVPP2_v2_0_MASTER_PLAN.md) - Project roadmap
- [Roadmap](../README.md#roadmap) - Future plans

**Reference Materials**
- [SSTV Specifications (PDF)](SSTV Description and mode specifications.pdf) - Official specs
- [SSTV Mode Details (PDF)](SSTV Mode Details.pdf) - Technical details
- [SSTV Modes (PDF)](SSTV Modes.pdf) - Mode reference

### Project Documentation

**Project Management**
- [Version History](../README.md#version-history) - Release notes
- [License](LICENSE.md) - GPL-3.0 terms
- [Contributing](../CONTRIBUTING.md) - How to contribute
- [Documentation Summary](DOCUMENTATION_COMPLETE.md) - v2.0 documentation effort

---

## 🚀 Quick Navigation

### I want to...

**Use PiSSTVpp2**
1. Start here: [Quick Start Guide](QUICK_START.md)
2. Then read: [User Guide](USER_GUIDE.md)
3. For problems: [Troubleshooting](USER_GUIDE.md#troubleshooting)

**Build from source**
1. Read: [Build Guide](BUILD.md)
2. For issues: [Build Troubleshooting](BUILD.md#troubleshooting)

**Understand how it works**
1. Start with: [Architecture Documentation](ARCHITECTURE.md)
2. Then explore: [Code Structure](ARCHITECTURE.md#module-descriptions)

**Add features or fix bugs**
1. Read: [Architecture Documentation](ARCHITECTURE.md)
2. Review: [Extension Points](ARCHITECTURE.md#extension-points)
3. Test with: [Test Suite Guide](TEST_SUITE_README.md)

**Run tests**
1. Quick: [Test Quick Start](TEST_QUICK_START.md)
2. Detailed: [Test Suite Guide](TEST_SUITE_README.md)

---

## 📚 Documentation by Topic

### SSTV Modes

**Current Modes (v2.0):**
- [Mode Reference](MODE_REFERENCE.md) - 7 modes detailed
- [User Guide - SSTV Modes](USER_GUIDE.md#sstv-modes) - Mode selection guide

**Future Modes (v2.1):**
- [MMSSTV Mode Reference](MMSSTV_MODE_REFERENCE.md) - 43 modes planned
- [MMSSTV Integration](MMSSTV_INTEGRATION.md) - Integration plan

### Audio Encoding

**Formats:**
- [User Guide - Audio Formats](USER_GUIDE.md#audio-formats) - WAV, AIFF, OGG
- [Encoding Libraries](ENCODING_LIBRARIES.md) - Technical details
- [Architecture - Audio Encoder](ARCHITECTURE.md#audio-encoder-module) - Code structure

**Sample Rates:**
- [User Guide - Performance Tuning](USER_GUIDE.md#performance-tuning) - Sample rate selection

### Image Processing

**Aspect Ratio:**
- [User Guide - Image Handling](USER_GUIDE.md#image-handling) - Center, pad, stretch modes
- [Architecture - Image Module](ARCHITECTURE.md#pisstvpp2_image-module) - Implementation

**Supported Formats:**
- [User Guide - Image Handling](USER_GUIDE.md#supported-input-formats) - Format list
- [Build Guide - libvips](BUILD.md#libvips-required) - Dependency details

### CW Identification

**Usage:**
- [User Guide - CW Identification](USER_GUIDE.md#cw-identification) - User guide
- [Quick Start - CW](QUICK_START.md#add-your-callsign-cw-identification) - Quick reference

### CW Identification

**Usage:**
- [User Guide - CW Identification](USER_GUIDE.md#cw-identification) - User guide
- [Quick Start - CW](QUICK_START.md#add-your-callsign-cw-identification) - Quick reference
- [Architecture - CW](ARCHITECTURE.md#key-algorithms) - Implementation details

### VIS Header

**Usage:**
- [User Guide - VIS Header](USER_GUIDE.md#vis-header) - What is VIS
- [Architecture - VIS Generation](ARCHITECTURE.md#key-algorithms) - Algorithm details

---

## 📋 Complete Document List

### User Documentation
| Document | Description | Audience |
|----------|-------------|----------|
| [README](../README.md) | Project overview | Everyone |
| [Quick Start](QUICK_START.md) | 5-minute intro | New users |
| [User Guide](USER_GUIDE.md) | Complete reference | All users |
| [Build Guide](BUILD.md) | Build instructions | Users building from source |

### Developer Documentation
| Document | Description | Audience |
|----------|-------------|----------|
| [Architecture](ARCHITECTURE.md) | System design | Developers |
| [Test Suite](TEST_SUITE_README.md) | Testing framework | Developers/Contributors |
| [Test Quick Start](TEST_QUICK_START.md) | Quick testing | Developers |
| [Test Practical Guide](TEST_PRACTICAL_GUIDE.md) | Writing tests | Contributors |

### Future Development
| Document | Description | Status |
|----------|-------------|--------|
| [MMSSTV Integration](MMSSTV_INTEGRATION.md) | v2.1 plan | Planned |
| [MMSSTV Modes](MMSSTV_MODE_REFERENCE.md) | 43 modes | Planned |
| [MMSSTV API](MMSSTV_API_UPDATES.md) | Interface | Planned |
| [Master Plan](PISSTVPP2_v2_0_MASTER_PLAN.md) | Project roadmap | Active |

### Reference Materials
| Document | Format | Description |
|----------|--------|-------------|
| SSTV Specifications | PDF | Official SSTV specifications |
| SSTV Mode Details | PDF | Technical mode details |
| SSTV Modes | PDF | Mode reference charts |

---

## 🔍 Finding Documentation

### By Task

**I need to encode an image**
→ [Quick Start Guide](QUICK_START.md#first-transmission-2-minutes)

**I'm getting build errors**
→ [Build Guide - Troubleshooting](BUILD.md#troubleshooting)

**I want to add a new SSTV mode**
→ [Architecture - Extension Points](ARCHITECTURE.md#adding-new-sstv-modes)

**I need to understand the code structure**
→ [Architecture Documentation](ARCHITECTURE.md)

**I'm adding a feature and need to test it**
→ [Test Practical Guide](TEST_PRACTICAL_GUIDE.md)

**I want to know what changed in v2.0**
→ [Refactoring Summary](REFACTORING_COMPLETE_SUMMARY.md)

### By Experience Level

**Beginner (Just Want to Use It)**
1. [Quick Start Guide](QUICK_START.md)
2. [User Guide](USER_GUIDE.md)

**Intermediate (Building from Source)**
1. [Build Guide](BUILD.md)
2. [User Guide](USER_GUIDE.md)
3. [Test Quick Start](TEST_QUICK_START.md)

**Advanced (Contributing/Extending)**
1. [Architecture Documentation](ARCHITECTURE.md)
2. [Test Suite Guide](TEST_SUITE_README.md)
3. [Extension Points](ARCHITECTURE.md#extension-points)

**Expert (Core Development)**
1. All of the above, plus:
2. [CW Envelope Analysis](CW_ENVELOPE_ANALYSIS.md)
3. [MMSSTV Integration](MMSSTV_INTEGRATION.md)
4. [Code Changes](CODE_CHANGES.md)

---

## 📝 Documentation Standards

### Organization

**File Naming:**
- UPPERCASE.md for project docs (README, LICENSE)
- Title_Case.md for feature docs
- lowercase.md for technical docs (if needed)

**Location:**
- `docs/` - All documentation
- Root - Only README.md, LICENSE, CONTRIBUTING.md

**Structure:**
- Table of Contents for long documents
- Clear headings and sections
- Code examples where applicable
- Links to related documents

### Style

**Writing:**
- Clear, concise language
- Active voice
- Present tense
- Second person ("you") for user docs
- Examples for complex topics

**Code Examples:**
```bash
# Always include comments
command -option argument  # Explain what this does
```

**Links:**
- Use relative links within docs
- Anchor links for same-document navigation
- External links for dependencies

---

## 🛠️ Maintaining Documentation

### When to Update Docs

**Code Changes:**
- New features → Update User Guide, Architecture
- Bug fixes → Update Troubleshooting sections
- API changes → Update Architecture, relevant guides

**Build Changes:**
- New dependencies → Update Build Guide
- Platform support → Update Build Guide, README

**Test Changes:**
- New tests → Update Test Suite README
- Test improvements → Update Test guides

### Documentation Checklist

When adding/changing features:

- [ ] Update README.md if user-facing
- [ ] Update User Guide with usage
- [ ] Update Architecture for code changes
- [ ] Add/update tests and test docs
- [ ] Update Quick Start if relevant
- [ ] Check all related documents
- [ ] Update this index if new doc added

---

## 📦 Documentation Versions

**v2.0 (Current)**
- Complete documentation suite
- User, developer, and technical guides
- Comprehensive test documentation
- MMSSTV integration prep

**v1.x (Legacy)**
- Basic README only
- Minimal documentation
- See [old docs/README.md](README.md) for v1.x info

**v2.1 (Planned)**
- MMSSTV integration complete
- Extended mode documentation
- Updated examples with new modes

---

## 🔗 External Resources

### Dependencies

**libvips:**
- [Official Website](https://libvips.github.io/libvips/)
- [GitHub Repository](https://github.com/libvips/libvips)
- [Documentation](https://libvips.github.io/libvips/API/current/)

**OGG Vorbis:**
- [Xiph.org](https://xiph.org/vorbis/)
- [Vorbis Documentation](https://xiph.org/vorbis/doc/)

### SSTV Resources

**General SSTV:**
- [SSTV Handbook](http://www.barberdsp.com/files/Dayton%20Paper.pdf)
- [Wikipedia - SSTV](https://en.wikipedia.org/wiki/Slow-scan_television)

**Software:**
- [QSSTV](http://users.telenet.be/on4qz/) - Linux SSTV software
- [MMSSTV](https://hamsoft.ca/pages/mmsstv.php) - Windows SSTV software

---

## ✨ Documentation Quality

### Current Status

✅ **Complete Documentation:**
- Quick Start Guide
- Comprehensive User Guide
- Architecture Documentation
- Build Guide for all platforms
- Testing documentation

✅ **Well Organized:**
- Clear structure
- Easy navigation
- Cross-referenced
- This index document

✅ **Up to Date:**
- Reflects v2.0 reality
- All examples tested
- Platform instructions verified

### Future Improvements

**Planned:**
- Contributing guide (CONTRIBUTING.md)
- API reference documentation
- Video tutorials (potential)
- Interactive examples (potential)

---

## 📞 Getting Help

**For Users:**
1. Check [Quick Start Guide](QUICK_START.md)
2. Read [User Guide](USER_GUIDE.md)
3. Check [Troubleshooting](USER_GUIDE.md#troubleshooting)

**For Developers:**
1. Read [Architecture Documentation](ARCHITECTURE.md)
2. Check [Test Suite Guide](TEST_SUITE_README.md)
3. Review [Extension Points](ARCHITECTURE.md#extension-points)

**Still Stuck?**
- File a GitHub Issue (if repository public)
- Check the documentation index (this file)
- Review related documents

---

## 🎯 Documentation Goals

**Achieved ✅:**
- ✅ Complete user documentation
- ✅ Comprehensive developer guides
- ✅ Clear build instructions
- ✅ Organized test documentation
- ✅ Future planning (MMSSTV)

**In Progress 🚧:**
- 🚧 Contributing guidelines
- 🚧 API reference extraction

**Future 📅:**
- 📅 Video tutorials
- 📅 Interactive examples
- 📅 Cookbook/recipes

---

**This index is your map to all PiSSTVpp2 documentation. Start with the Quick Start Guide and explore from there!**

**73!** 📡

*Last updated: 2024*
