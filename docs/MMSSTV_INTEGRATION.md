# MMSSTV Library Integration - v2.1 Approach

**Updated for v2.1: Dynamic Runtime Detection with Graceful Fallback**

**Previous Version:** 1.0 (January 29, 2026) - Superseded by v2.1 approach  
**Current Version:** 2.0 (February 10, 2026) - Runtime detection strategy

---

## Overview

This document describes how PiSSTVpp2 v2.1 integrates the optional MMSSTV library for runtime access to 50+ additional SSTV modes.

**Key Principle:** The MMSSTV library is **optional**. Your application always works with native 7 modes, but gains 50+ additional modes if the library is present.

## Architecture

### Current Approach (v2.0)
- 7 hardcoded native SSTV modes
- No plugin system
- Adding MMSSTV requires recompilation

### New Approach (v2.1)
- 7 native modes always available (embedded)
- Mode registry system detects MMSSTV library at runtime
- If library found: 50+ MMSSTV modes added to registry
- If library not found: Works perfectly fine with native modes
- No recompilation needed
- User can enable MMSSTV via environment variables or system installation

---

## Implementation Details

Detailed implementation is provided in [PISSTVPP2_v2_1_MASTER_PLAN.md](./PISSTVPP2_v2_1_MASTER_PLAN.md), specifically:

- **Phase 4 (Tasks 4.1-4.4)**: Complete MMSSTV integration implementation
- **Section A2**: MMSSTV Library Detection architecture
- **Phase 5**: Build system enhancements

---

## User Configuration

### Environment Variables

Users can configure MMSSTV detection:

```bash
# Specify exact library path (takes precedence)
export MMSSTV_LIB_PATH="/custom/path/libmmsstv.so"

# Specify header path (for development)
export MMSSTV_INCLUDE_PATH="/custom/path/mmsstv/include"

# Run application
./pisstvpp2 -m m1 input.png output.wav
```

### Installation Instructions

See [MMSSTV_SETUP.md](./MMSSTV_SETUP.md) (created Phase 7) for:
- Where to find mmsstv-portable library
- How to build it
- How to install it system-wide
- How to verify detection
- Troubleshooting

---

## Comparison: v2.0 → v2.1

| Aspect | v2.0 | v2.1 |
|--------|------|------|
| **SSTV Modes** | 7 hardcoded | 7 native + 50+ optional MMSSTV |
| **Adding MMSSTV** | Recompile needed | Not needed, runtime detection |
| **Mode Registry** | Hardcoded switch | Dynamic registry system |
| **If Library Missing** | N/A (not supported) | Works fine with native modes |
| **User Control** | None | Environment variables, --mmsstv-status |
| **Build System** | Static features | Auto-detect library |
| **CLI Help** | Lists 7 modes | Lists available modes dynamically |

---

## Available Modes

### Native Modes (Always Available)

| Protocol | VIS Code | Mode Name          | Resolution  | Duration |
|----------|----------|--------------------|-------------|----------|
| m1       | 44       | Martin 1           | 320x256     | 114s     |
| m2       | 40       | Martin 2           | 320x256     | 58s      |
| s1       | 60       | Scottie 1          | 320x256     | 110s     |
| s2       | 56       | Scottie 2          | 320x256     | 71s      |
| sdx      | 76       | Scottie DX         | 320x256     | 269s     |
| r36      | 8        | Robot 36 Color     | 320x240     | 36s      |
| r72      | 12       | Robot 72 Color     | 320x240     | 72s      |

### MMSSTV Modes (If Library Available)

50+ additional modes including:
- **Martin Family:** M3, M4
- **PD Family:** PD50, PD90, PD120, PD160, PD180, PD240, PD290
- **Wraase:** SC2-60, SC2-120
- **AVT:** AVT24, AVT90, AVT94
- **Additional:** Pasokon, FAX, ML families, and more

For complete list, see [MMSSTV_MODE_REFERENCE.md](./MMSSTV_MODE_REFERENCE.md)

---

## References & Additional Information

For **complete implementation details**, see:
- [PISSTVPP2_v2_1_MASTER_PLAN.md](./PISSTVPP2_v2_1_MASTER_PLAN.md)
  - Section A2: MMSSTV Library Detection architecture
  - Phase 4: Tasks 4.1-4.4 (MMSSTV integration implementation)
  - Phase 5: Build system enhancements

For **mode details**, see:
- [MMSSTV_MODE_REFERENCE.md](./MMSSTV_MODE_REFERENCE.md) - All 50+ available modes
- [MMSSTV_API_UPDATES.md](./MMSSTV_API_UPDATES.md) - Library API specifications
- [MMSSTV_SETUP.md](./MMSSTV_SETUP.md) - Installation guide (Phase 7)

---

## FAQ

**Q: What if I don't want MMSSTV support?**
A: You don't need it. The 7 native modes are always available and fully functional. MMSSTV is optional.

**Q: Will my existing scripts break?**
A: No. All v2.0 CLI options work identically. You can use any of the 7 native modes exactly as before.

**Q: How do I enable MMSSTV?**
A: Install the mmsstv-portable library and the application will auto-detect it.

**Q: What if I have MMSSTV installed but want to use native-only?**
A: Either unset the environment variable or don't install the library. The application always works.

**Q: How can I verify MMSSTV is detected?**
A: Run `./pisstvpp2 --mmsstv-status` to see library detection status.

**Q: Do I need to recompile to enable MMSSTV?**
A: No. The detection happens at runtime.

---

## Document History

- **v2.0 (Feb 10, 2026):** Consolidated for v2.1 development (current, supersedes v1.0)
- **v1.0 (Jan 29, 2026):** Initial design for conditional compilation approach (archived, referenced for historical context)

---

**Status:** v2.1 Planning & Implementation  
**Created:** February 10, 2026  
**Implementation:** Phase 4 of v2.1 Roadmap  
**See also:** [PISSTVPP2_v2_1_MASTER_PLAN.md](./PISSTVPP2_v2_1_MASTER_PLAN.md)
