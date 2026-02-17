# Phase 4 Implementation Plan - MMSSTV Integration

**Date:** February 15, 2026  
**Status:** 🟢 READY TO START  
**Prerequisites:** ✅ Phase 3 Complete (Mode Registry Implemented)  
**Estimated Effort:** 30-40 hours

---

## Executive Summary

**Goal:** Add optional MMSSTV library support to SlowFrame, expanding from 7 native modes to 57+ modes when the library is available.

**Key Principle:** MMSSTV is **optional**. The application must work perfectly with only the 7 native modes if the library is not installed.

**Success Criteria:**
- ✅ All 55 existing tests still pass
- ✅ Application runs with 7 native modes (library not present)
- ✅ Application detects and loads 50+ MMSSTV modes (library present)
- ✅ CLI dynamically lists available modes
- ✅ No hardcoded MMSSTV dependencies
- ✅ Graceful error messages if library missing

---

## Architecture Overview

### Current State (Post-Phase 3)
```
┌─────────────────────────────┐
│   Mode Registry (Phase 3)   │
│                             │
│  • 7 native modes           │
│  • Lookup by code/VIS       │
│  • Function pointer dispatch│
└─────────────────────────────┘
```

### Target State (Post-Phase 4)
```
┌─────────────────────────────────────────────┐
│         Mode Registry (Enhanced)            │
├─────────────────────────────────────────────┤
│  Native Modes (always):                     │
│    • Martin M1/M2                           │
│    • Scottie S1/S2/SDX                      │
│    • Robot R36/R72                          │
├─────────────────────────────────────────────┤
│  MMSSTV Modes (optional, if lib present):   │
│    • Martin M3/M4                           │
│    • Scottie S3/S4                          │
│    • PD50/90/120/160/180/240/290            │
│    • Robot R12/R24                          │
│    • Wraase SC2-60/120                      │
│    • AVT modes                              │
│    • Pasokon, FAX, ML families              │
│    • 40+ more...                            │
└─────────────────────────────────────────────┘
             ↑
             │
    ┌────────┴────────┐
    │  MMSSTV Loader  │
    │   (Phase 4.1)   │
    └─────────────────┘
```

---

## Task Breakdown

### Task 4.1: MMSSTV Library Loader (4-5 hours)
**Priority:** High (Foundation)  
**Blocking:** Tasks 4.2, 4.3, 4.4

#### Objectives
1. Create dynamic library loader using dlopen/dlsym (macOS/Linux)
2. Detect library in multiple locations (env vars, pkg-config, standard paths)
3. Provide detailed error reporting
4. **Graceful fallback** - failure is not fatal

#### Files to Create
```
src/mmsstv/
├── mmsstv_loader.h      (Public API)
├── mmsstv_loader.c      (Implementation)
└── mmsstv_stub.h        (MMSSTV library API contract)
```

#### Implementation Checklist

**mmsstv_stub.h** - Define expected MMSSTV library interface:
- [ ] Mode enumeration structures
- [ ] Encoder function signatures
- [ ] Library version checking
- [ ] Mode capability queries

**mmsstv_loader.h** - Public API:
```c
typedef struct mmsstv_library mmsstv_library_t;

// Load library (returns NULL if not found - that's OK!)
mmsstv_library_t* mmsstv_loader_init(void);

// Check if library is loaded
bool mmsstv_loader_is_available(const mmsstv_library_t *lib);

// Get library version
const char* mmsstv_loader_get_version(const mmsstv_library_t *lib);

// Get mode count
int mmsstv_loader_get_mode_count(const mmsstv_library_t *lib);

// Cleanup
void mmsstv_loader_destroy(mmsstv_library_t *lib);
```

**mmsstv_loader.c** - Implementation:
- [ ] Environment variable detection:
  - Check `MMSSTV_LIB_PATH` first (user override)
  - Check `MMSSTV_INCLUDE_PATH` for headers (development)
- [ ] pkg-config detection:
  - Run `pkg-config --libs mmsstv-portable` if available
  - Parse output for library path
- [ ] Standard path search:
  - `/usr/local/lib/libmmsstv.so` (or .dylib on macOS)
  - `/usr/lib/libmmsstv.so`
  - `/opt/mmsstv/lib/libmmsstv.so`
  - `./lib/libmmsstv.so` (local development)
- [ ] Dynamic loading with dlopen():
  - Use `RTLD_LAZY | RTLD_LOCAL` flags
  - Store handle in mmsstv_library_t structure
- [ ] Symbol resolution with dlsym():
  - Load function pointers for encoder functions
  - Load mode list function
  - Load version function
- [ ] Error handling:
  - Use dlerror() for helpful messages
  - Log search paths tried
  - Return NULL (not fatal!) if library not found

#### Testing Strategy
1. **Without library:**
   - `mmsstv_loader_init()` returns NULL
   - Application continues normally with 7 native modes
   - No error messages printed to user

2. **With library (mock):**
   - Create stub .so file for testing
   - Verify detection in all search paths
   - Verify function pointer resolution

3. **Error cases:**
   - Invalid library path in env var → helpful error
   - Corrupted library file → graceful fallback
   - Missing symbols → version mismatch warning

#### Success Criteria
- ✅ Compiles cleanly on macOS and Linux
- ✅ Returns NULL when library absent (non-fatal)
- ✅ Loads library correctly when present
- ✅ Error messages are helpful for debugging

---

### Task 4.2: MMSSTV Adapter (5-6 hours)
**Priority:** High (Core Integration)  
**Depends On:** Task 4.1  
**Blocking:** Task 4.3

#### Objectives
1. Bridge between MMSSTV library API and our mode registry
2. Create mode_definition_t structures for each MMSSTV mode
3. Implement encoder wrapper functions
4. Handle library-specific quirks

#### Files to Create
```
src/mmsstv/
├── mmsstv_adapter.h     (Adapter API)
└── mmsstv_adapter.c     (Implementation)
```

#### Implementation Checklist

**mmsstv_adapter.h** - Public API:
```c
typedef struct mmsstv_adapter mmsstv_adapter_t;

// Initialize adapter (loads library, enumerates modes)
mmsstv_adapter_t* mmsstv_adapter_init(void);

// Get number of available MMSSTV modes
int mmsstv_adapter_get_mode_count(const mmsstv_adapter_t *adapter);

// Get mode definition by index
const mode_definition_t* mmsstv_adapter_get_mode(
    const mmsstv_adapter_t *adapter, 
    int index
);

// Register all MMSSTV modes with registry
int mmsstv_adapter_register_modes(
    mmsstv_adapter_t *adapter,
    mode_registry_t *registry
);

// Cleanup
void mmsstv_adapter_destroy(mmsstv_adapter_t *adapter);
```

**mmsstv_adapter.c** - Implementation:

**1. Initialization:**
- [ ] Call `mmsstv_loader_init()` to load library
- [ ] If loader returns NULL, create empty adapter (valid state!)
- [ ] If library loaded, enumerate available modes
- [ ] Cache mode list in adapter structure

**2. Mode Enumeration:**
- [ ] Query library for mode count
- [ ] For each mode, create `mode_definition_t`:
  ```c
  mode_definition_t mode = {
      .code = "pd120",           // MMSSTV library provides
      .name = "PD 120",
      .vis_code = 95,
      .width = 640,
      .height = 496,
      .duration_ms = 120000,
      .color_space = "YCrCb",
      .source = MODE_SOURCE_MMSSTV,  // NEW FLAG
      .encode_frame = mmsstv_encode_wrapper,  // Adapter function
      .metadata = mmsstv_mode_ptr   // Library-specific data
  };
  ```
- [ ] Store mode definitions in adapter structure

**3. Encoder Wrapper:**
- [ ] Create generic `mmsstv_encode_wrapper()` function:
  ```c
  int mmsstv_encode_wrapper(
      int vis_code,
      bool verbose,
      bool timestamp,
      int sample_rate,
      float *audio_buffer,
      int max_samples,
      int *samples_written
  ) {
      // Extract MMSSTV mode from metadata
      // Call library encoder function
      // Translate errors to our error codes
      // Return result
  }
  ```
- [ ] Handle library-specific image format requirements
- [ ] Translate between our audio format and library's format

**4. Mode Registration:**
- [ ] Implement `mmsstv_adapter_register_modes()`:
  - Iterate through cached mode definitions
  - Call `mode_registry_add()` for each
  - Check for conflicts with native modes (fail if duplicate VIS codes)
  - Log registration count

#### Data Structures
```c
struct mmsstv_adapter {
    mmsstv_library_t *library;      // From loader (may be NULL)
    mode_definition_t *modes;       // Array of mode definitions
    int mode_count;                 // Number of modes
    bool is_available;              // Library successfully loaded
    char version[64];               // Library version string
};
```

#### Testing Strategy
1. **Without library:**
   - Adapter initializes successfully
   - `is_available = false`
   - `mode_count = 0`
   - Registration is no-op

2. **With library:**
   - Adapter loads all modes
   - Mode definitions are valid
   - Encoder wrapper translates calls correctly

3. **Mode conflicts:**
   - Native mode VIS 44 vs. MMSSTV mode VIS 44 → error detected

#### Success Criteria
- ✅ Adapter works with/without library
- ✅ All MMSSTV modes properly registered
- ✅ Encoder wrapper correctly translates calls
- ✅ No memory leaks

---

### Task 4.3: Registry Integration (2-3 hours)
**Priority:** High (Wiring)  
**Depends On:** Task 4.2  
**Blocking:** Task 4.4

#### Objectives
1. Update mode registry to support dynamic mode sources
2. Ensure native modes always load first
3. Add MMSSTV modes after native modes (if available)
4. Handle initialization order correctly

#### Files to Modify
```
src/sstv/mode_registry.h     (Add source tracking)
src/sstv/mode_registry.c     (Update initialization)
```

#### Implementation Checklist

**1. Update mode_definition_t structure:**
```c
// In mode_registry.h
typedef enum {
    MODE_SOURCE_NATIVE,      // Built-in C implementation
    MODE_SOURCE_MMSSTV,      // MMSSTV library
    MODE_SOURCE_PLUGIN       // Future: user plugins
} mode_source_t;

typedef struct mode_definition {
    // ... existing fields ...
    mode_source_t source;    // NEW: Track mode origin
    void *metadata;          // NEW: Source-specific data
} mode_definition_t;
```

**2. Update registry initialization:**
Currently (Phase 3):
```c
mode_registry_t* sstv_get_registry(void) {
    static mode_registry_t *registry = NULL;
    if (!registry) {
        registry = mode_registry_create();
        modes_martin_register(registry);
        modes_scottie_register(registry);
        modes_robot_register(registry);
    }
    return registry;
}
```

New (Phase 4):
```c
mode_registry_t* sstv_get_registry(void) {
    static mode_registry_t *registry = NULL;
    if (!registry) {
        registry = mode_registry_create();
        
        // STEP 1: Always register native modes first
        modes_martin_register(registry);
        modes_scottie_register(registry);
        modes_robot_register(registry);
        
        // STEP 2: Try to register MMSSTV modes (non-fatal if fails)
        mmsstv_adapter_t *adapter = mmsstv_adapter_init();
        if (adapter && mmsstv_adapter_is_available(adapter)) {
            int mmsstv_count = mmsstv_adapter_register_modes(adapter, registry);
            if (mmsstv_count > 0) {
                // Success - log in verbose mode only
                if (g_encode_verbose) {
                    fprintf(stderr, "MMSSTV: Registered %d additional modes\n", 
                            mmsstv_count);
                }
            }
        }
        // Note: Keep adapter alive for the lifetime of registry
        // Store it in registry->mmsstv_adapter field
        registry->mmsstv_adapter = adapter;
    }
    return registry;
}
```

**3. Add mode filtering functions:**
```c
// Get only native modes
int mode_registry_list_native(
    const mode_registry_t *registry,
    const mode_definition_t ***modes_out
);

// Get only MMSSTV modes
int mode_registry_list_mmsstv(
    const mode_registry_t *registry,
    const mode_definition_t ***modes_out
);

// Check if mode is from specific source
bool mode_is_native(const mode_definition_t *mode);
bool mode_is_mmsstv(const mode_definition_t *mode);
```

**4. Update duplicate detection:**
- [ ] When adding mode, check if VIS code already registered
- [ ] If duplicate found:
  - Native + Native → Error (bug in our code)
  - Native + MMSSTV → Keep native, warn about MMSSTV conflict
  - MMSSTV + MMSSTV → Keep first, warn about second
- [ ] Log conflicts in verbose mode

#### Testing Strategy
1. **Without MMSSTV:**
   - Registry contains exactly 7 modes
   - All are `MODE_SOURCE_NATIVE`
   - No error messages

2. **With MMSSTV:**
   - Registry contains 7 + 50+ modes
   - Source tracking correct for each mode
   - Filtering functions work

3. **Conflict handling:**
   - Intentionally create duplicate VIS code
   - Verify native mode wins
   - Verify warning message

#### Success Criteria
- ✅ Registry initializes in correct order
- ✅ Mode source tracking accurate
- ✅ Duplicate handling works
- ✅ All 55 tests still pass

---

### Task 4.4: Dynamic CLI Updates (3-4 hours)
**Priority:** Medium (User-Facing)  
**Depends On:** Task 4.3

#### Objectives
1. Update help text to show available modes dynamically
2. Add `--list-modes` flag for detailed mode listing
3. Add `--mmsstv-status` flag for library diagnostic
4. Update mode validation error messages

#### Files to Modify
```
src/slowframe.c          (CLI handling)
src/slowframe_config.c   (Option parsing)
```

#### Implementation Checklist

**1. Add new CLI flags:**
```c
// In slowframe_config.c, add to getopt:
case 'L': // Already exists for list modes
    config->list_modes = true;
    break;

case 'M': // NEW: MMSSTV status
    config->show_mmsstv_status = true;
    break;
```

**2. Update `list_available_modes()` function:**
Current (Phase 3) - shows all modes in single table:
```
Code    Name          VIS  Resolution  Duration   Color  Source
------  ------------  ---  ----------  ---------  -----  ------
m1      Martin M1     44   320x256     114.286s   RGB    Native
...
```

New (Phase 4) - group by source:
```
Native SSTV Modes (7 modes, always available):
Code    Name          VIS  Resolution  Duration   Color
------  ------------  ---  ----------  ---------  -----
m1      Martin M1     44   320x256     114.286s   RGB
m2      Martin M2     40   320x256     58.286s    RGB
s1      Scottie S1    60   320x256     110.080s   RGB
s2      Scottie S2    56   320x256     71.680s    RGB
sdx     Scottie DX    76   320x256     268.800s   RGB
r36     Robot 36      8    320x240     36.0s      YUV
r72     Robot 72      12   320x240     72.0s      YUV

MMSSTV Modes (50 modes, library detected):
Code    Name          VIS  Resolution  Duration   Color
------  ------------  ---  ----------  ---------  -----
m3      Martin 3      104  256x256     57.0s      RGB
m4      Martin 4      108  256x256     29.0s      RGB
pd50    PD 50         93   320x256     50.0s      YCrCb
pd90    PD 90         99   320x256     90.0s      YCrCb
pd120   PD 120        95   640x496     120.0s     YCrCb
...

Total: 57 modes available
```

If library not found:
```
Native SSTV Modes (7 modes, always available):
[... table ...]

MMSSTV Modes: Not available (library not detected)
  To enable 50+ additional modes, install mmsstv-portable library.
  See documentation for installation instructions.

Total: 7 modes available
```

**3. Implement `--mmsstv-status` flag:**
```c
void show_mmsstv_status(void) {
    printf("MMSSTV Library Status:\n");
    printf("====================\n\n");
    
    mmsstv_adapter_t *adapter = mmsstv_adapter_init();
    
    if (!adapter || !mmsstv_adapter_is_available(adapter)) {
        printf("Status: NOT DETECTED\n");
        printf("\nSearch paths tried:\n");
        printf("  1. Environment: $MMSSTV_LIB_PATH    (not set)\n");
        printf("  2. pkg-config:  mmsstv-portable     (not found)\n");
        printf("  3. /usr/local/lib/libmmsstv.so      (not found)\n");
        printf("  4. /usr/lib/libmmsstv.so            (not found)\n");
        printf("\nTo enable MMSSTV support:\n");
        printf("  - Install mmsstv-portable library\n");
        printf("  - OR set MMSSTV_LIB_PATH environment variable\n");
        printf("  - See documentation for details\n");
    } else {
        printf("Status: LOADED\n");
        printf("Version: %s\n", mmsstv_adapter_get_version(adapter));
        printf("Modes available: %d\n", mmsstv_adapter_get_mode_count(adapter));
        printf("Library path: %s\n", mmsstv_adapter_get_path(adapter));
    }
    
    if (adapter) {
        mmsstv_adapter_destroy(adapter);
    }
}
```

**4. Update help text:**
```c
void show_help(void) {
    // ... existing help ...
    
    printf("\nAvailable SSTV Protocols:\n");
    printf("  Use -L or --list-modes to see all available modes\n");
    
    // Show quick examples from native modes
    printf("\n  Examples (native modes, always available):\n");
    printf("    -p m1      Martin M1 (320x256 RGB, 114s)\n");
    printf("    -p s1      Scottie S1 (320x256 RGB, 110s)\n");
    printf("    -p r36     Robot 36 (320x240 YUV, 36s)\n");
    
    // Show MMSSTV examples IF available
    mode_registry_t *registry = sstv_get_registry();
    if (mode_registry_has_mmsstv(registry)) {
        printf("\n  Examples (MMSSTV modes, library detected):\n");
        printf("    -p pd120   PD 120 (640x496 YCrCb, 120s)\n");
        printf("    -p m3      Martin 3 (256x256 RGB, 57s)\n");
    }
    
    printf("\nDiagnostics:\n");
    printf("  -M, --mmsstv-status    Show MMSSTV library detection status\n");
}
```

**5. Update error messages:**
Change from:
```
Error: Unknown protocol 'xyz'. Use -L to list available modes.
```

To:
```
Error: Unknown protocol 'xyz'
  Use -L to see available modes (7 native modes available)
```

Or if MMSSTV loaded:
```
Error: Unknown protocol 'xyz'
  Use -L to see available modes (57 modes available)
```

#### Testing Strategy
1. **Help text:**
   - Run `./slowframe --help` with/without MMSSTV
   - Verify dynamic content

2. **Mode listing:**
   - Run `./slowframe -L` with/without MMSSTV
   - Verify grouping and counts

3. **Status command:**
   - Run `./slowframe -M` with/without MMSSTV
   - Verify diagnostic output

#### Success Criteria
- ✅ Help text adapts to available modes
- ✅ Mode listing clearly groups native vs. MMSSTV
- ✅ Status command helpful for debugging
- ✅ Error messages guide users to correct mode names

---

## Testing Strategy

### Unit Tests
Location: `tests/unit/test_mmsstv_integration.c`

**Test Cases:**
1. **Loader without library:**
   - `mmsstv_loader_init()` returns NULL
   - No crashes, no errors logged

2. **Loader with mock library:**
   - Create minimal .so with expected symbols
   - Verify successful loading
   - Verify function pointer resolution

3. **Adapter without library:**
   - `mmsstv_adapter_init()` succeeds
   - `is_available() == false`
   - `get_mode_count() == 0`

4. **Adapter with mock library:**
   - Modes enumerated correctly
   - Encoder wrapper callable

5. **Registry integration:**
   - Native modes always present
   - MMSSTV modes added when available
   - Duplicate handling works

### Integration Tests
Location: `tests/util/test_mmsstv_modes.py`

**Test Cases:**
1. **CLI with 7 modes:**
   ```bash
   ./slowframe -L  # Should show 7 native modes
   ```

2. **CLI with 57 modes (requires MMSSTV):**
   ```bash
   export MMSSTV_LIB_PATH=/path/to/libmmsstv.so
   ./slowframe -L  # Should show 7 + 50 modes
   ```

3. **Status command:**
   ```bash
   ./slowframe -M  # Should show detection status
   ```

4. **Encoding with MMSSTV mode:**
   ```bash
   ./slowframe -i test.png -p pd120 -o test.wav
   # Should succeed if library present, error if not
   ```

### Regression Tests
- ✅ All 55 existing tests must still pass
- ✅ Tests run with/without MMSSTV library
- ✅ No performance degradation

---

## Build System Updates

### Makefile Changes

**1. Add MMSSTV source files:**
```makefile
# MMSSTV integration (optional at runtime)
MMSSTV_SOURCES = src/mmsstv/mmsstv_loader.c \
                 src/mmsstv/mmsstv_adapter.c

SOURCES += $(MMSSTV_SOURCES)
```

**2. Add dynamic linking flags:**
```makefile
# Dynamic library loading support
LDFLAGS += -ldl  # For dlopen/dlsym on Linux/macOS
```

**3. Optional: Add detection info:**
```makefile
.PHONY: show-mmsstv-status
show-mmsstv-status: $(BINARY)
	@echo "Checking MMSSTV library status..."
	@./$(BINARY) --mmsstv-status || true
```

**4. Build targets:**
```makefile
.PHONY: test-with-mmsstv
test-with-mmsstv: $(BINARY)
	@if [ -z "$$MMSSTV_LIB_PATH" ]; then \
		echo "Warning: MMSSTV_LIB_PATH not set"; \
		echo "Set it to test MMSSTV integration"; \
	fi
	python3 tests/util/test_mmsstv_modes.py
```

### No Compile-Time Dependencies
**Important:** We do NOT link against libmmsstv.so at compile time. All loading is done at runtime via dlopen(). This means:
- ✅ Build succeeds without MMSSTV library installed
- ✅ Binary runs on systems without MMSSTV library
- ✅ No pkg-config required for building
- ✅ Users can add MMSSTV later without recompiling

---

## Documentation Updates

### Files to Create

**1. `docs/MMSSTV_SETUP.md`**
- How to obtain mmsstv-portable library
- Build instructions
- Installation instructions (system-wide vs. local)
- Troubleshooting

**2. `docs/PHASE_4_COMPLETION_SUMMARY.md`**
- Summary of changes
- API additions
- Testing results
- Migration guide

### Files to Update

**1. `README.md`**
- Add MMSSTV feature mention
- Link to MMSSTV_SETUP.md

**2. `docs/BUILD.md`**
- No changes needed (MMSSTV is optional)
- Maybe add note about runtime detection

**3. `docs/MODE_REFERENCE.md`**
- Mark which modes require MMSSTV
- Reference MMSSTV_MODE_REFERENCE.md

---

## Risk Assessment & Mitigation

### Risk 1: MMSSTV Library Not Available for Testing
**Likelihood:** High  
**Impact:** Medium  
**Mitigation:**
- Create mock/stub library for testing
- Test all code paths with library absent
- Use function pointers to allow stubbing

### Risk 2: MMSSTV API Changes
**Likelihood:** Medium  
**Impact:** Medium  
**Mitigation:**
- Version checking in loader
- Graceful degradation on API mismatch
- Document required library version

### Risk 3: Performance Impact
**Likelihood:** Low  
**Impact:** Low  
**Mitigation:**
- Registry initialization is one-time cost
- Lazy loading (only when needed)
- Benchmark encoding performance

### Risk 4: Memory Leaks
**Likelihood:** Medium  
**Impact:** Medium  
**Mitigation:**
- Valgrind testing
- Clear ownership rules (adapter owns modes)
- Cleanup functions for all structures

---

## Timeline & Milestones

### Week 1 (Days 1-2): Foundation
- ✅ Day 1: Task 4.1 - MMSSTV Loader (4-5 hours)
  - Create stub header
  - Implement loader with dlopen
  - Test without library (must work!)
  
- ✅ Day 2: Task 4.1 continued
  - Create mock library for testing
  - Test with mock library
  - Verify error handling

### Week 1 (Days 3-4): Integration
- ✅ Day 3: Task 4.2 - MMSSTV Adapter (5-6 hours)
  - Create adapter structure
  - Implement mode enumeration
  - Create encoder wrapper
  
- ✅ Day 4: Task 4.2 continued + Task 4.3 start
  - Test adapter with/without library
  - Begin registry integration

### Week 2 (Days 5-6): Wiring & Testing
- ✅ Day 5: Task 4.3 - Registry Integration (2-3 hours)
  - Update registry initialization
  - Add source tracking
  - Test duplicate handling
  
- ✅ Day 6: Task 4.4 - CLI Updates (3-4 hours)
  - Update help text
  - Implement --list-modes
  - Implement --mmsstv-status

### Week 2 (Days 7-8): Polish & Documentation
- ✅ Day 7: Testing & Bug Fixes
  - Run all 55 tests
  - Fix any regressions
  - Integration testing
  
- ✅ Day 8: Documentation
  - Write MMSSTV_SETUP.md
  - Update README, BUILD docs
  - Create completion summary

---

## Success Metrics

### Functional Requirements
- ✅ Application runs with 0 MMSSTV modes (library absent)
- ✅ Application runs with 50+ MMSSTV modes (library present)
- ✅ All 55 regression tests pass
- ✅ CLI dynamically adapts to available modes
- ✅ Error messages are helpful

### Non-Functional Requirements
- ✅ No performance degradation vs. Phase 3
- ✅ Memory usage acceptable (< 10MB additional for 50 modes)
- ✅ Build time unchanged (no compile-time MMSSTV dependency)
- ✅ Code coverage > 80% for new code

### Code Quality
- ✅ No compiler warnings
- ✅ All functions documented
- ✅ Error paths tested
- ✅ No memory leaks (verified with Valgrind)

---

## Next Steps After Phase 4

Once Phase 4 is complete, consider:

**Phase 5: Build System Enhancement**
- Automated MMSSTV library detection in Makefile
- pkg-config integration
- Build info reporting

**Phase 6: Advanced MMSSTV Features**
- Mode-specific options (color correction, etc.)
- MMSSTV encoder configuration
- Performance optimization

**Phase 7: Documentation & Release**
- User manual updates
- Tutorial videos
- Release notes
- Community outreach

---

## Questions for Review

Before starting implementation, confirm:

1. **Do we have access to mmsstv-portable library for testing?**
   - If no: Create comprehensive mock library
   - If yes: Test against real library early

2. **What's the target MMSSTV library version?**
   - Document minimum required version
   - Plan for version compatibility checks

3. **Are there licensing concerns with MMSSTV?**
   - Verify compatible with GPL
   - Document attribution requirements

4. **Should MMSSTV modes have priority over native modes?**
   - Current plan: Native always wins conflicts
   - Alternative: Make configurable

---

**Ready to proceed with Task 4.1!**

Let's start with the MMSSTV loader implementation. This is the foundation for everything else.
