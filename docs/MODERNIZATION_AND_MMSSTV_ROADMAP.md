
# PiSSTVpp2 Modernization & MMSSTV Integration Roadmap

**Document Version:** 1.0  
**Date:** February 10, 2026  
**Status:** Planning Phase  
**Target Release:** v2.1 (Q2 2026)

---

## Executive Summary

This roadmap describes the strategy for adding optional MMSSTV library support to PiSSTVpp2 while simultaneously modernizing the codebase. The key principle is **graceful degradation**: the application remains fully functional with native SSTV modes, but detects and enables MMSSTV functionality when the library is available.

**Key Features of This Approach:**
- ✅ Backward compatible (all existing functionality preserved)
- ✅ Dynamic detection (no required recompilation for MMSSTV support)
- ✅ User-friendly (environment variables for library path)
- ✅ Code accessibility (clean, modular, well-documented for ham radio community)
- ✅ Gradual migration (can build new features without rewriting everything at once)

---

## Part A: Architecture & Design

### A1. Dynamic Mode Registry System

**Current State:**
- Modes are hardcoded in `pisstvpp2_sstv.c` (7 native modes)
- No plugin or dynamic mechanism

**Proposed State:**
```c
// include/pisstvpp2_mode_registry.h

typedef struct {
    const char *code;              // "m1", "pd120", etc.
    const char *name;              // "Martin 1", "PD-120"
    uint8_t vis_code;
    uint16_t width;
    uint16_t height;
    uint32_t transmission_time_ms;
    
    // Function pointers for mode-specific encoding
    int (*init)(void);
    int (*encode_frame)(const image_buffer_t *img, audio_samples_t *audio);
    int (*cleanup)(void);
    
    // Metadata
    const char *source;            // "native" or "mmsstv"
    int priority;                  // For display ordering
} mode_definition_t;

// Mode registry API
typedef struct {
    mode_definition_t *modes;
    uint16_t count;
    uint16_t capacity;
} mode_registry_t;

// Interface
mode_registry_t* registry_create(void);
int registry_add_mode(mode_registry_t *reg, const mode_definition_t *mode);
const mode_definition_t* registry_lookup_by_code(mode_registry_t *reg, const char *code);
void registry_list_modes(mode_registry_t *reg);  // For -h output
void registry_free(mode_registry_t *reg);
```

**Benefits:**
- Native modes always available
- MMSSTV modes added at runtime if library found
- Help text dynamically generated
- No recompilation needed for MMSSTV availability

### A2. MMSSTV Library Detection Strategy

**Environment Variables:**
```bash
# User can specify library location
export MMSSTV_LIB_PATH="/usr/local/lib/libmmsstv.so"
export MMSSTV_INCLUDE_PATH="/usr/local/include/mmsstv"

# Or system-wide discovery (pkg-config, standard paths)
# Priority order:
# 1. MMSSTV_LIB_PATH env var
# 2. pkg-config (mmsstv-portable)
# 3. Standard system paths (/usr/lib, /usr/local/lib, etc.)
# 4. Fallback: native modes only
```

**Detection Mechanism:**
```c
// include/pisstvpp2_mmsstv_loader.h

typedef enum {
    MMSSTV_STATUS_NOT_AVAILABLE,
    MMSSTV_STATUS_FOUND,
    MMSSTV_STATUS_LOADED,
    MMSSTV_STATUS_ERROR
} mmsstv_status_t;

typedef struct {
    mmsstv_status_t status;
    const char *library_path;
    const char *error_message;
    void *handle;  // dlopen handle
    // ... function pointers
} mmsstv_context_t;

// Detection API
mmsstv_context_t* mmsstv_init(void);
int mmsstv_is_available(void);
const char* mmsstv_get_status_message(mmsstv_context_t *ctx);
void mmsstv_cleanup(mmsstv_context_t *ctx);
```

**Search Strategy:**
1. Check `MMSSTV_LIB_PATH` environment variable
2. Try pkg-config: `pkg-config --cflags --libs mmsstv-portable`
3. Check standard paths: `/usr/lib`, `/usr/local/lib`, `/opt/mmsstv/lib`
4. Check relative paths: `./lib/mmsstv`
5. Gracefully degrade to native-only mode

### A3. Unified Encoding Pipeline

**Current:**
```
main → image_load → image_aspect_correct → sstv_encode_frame 
       → audio_encoder_encode → write_file
```

**Proposed:**
```
main → image_load → image_aspect_correct → mode_registry_lookup 
       → (native_encode | mmsstv_encode) → audio_encoder_encode → write_file
```

**Implementation:**
```c
// include/pisstvpp2_encoder.h

typedef struct {
    const mode_definition_t *mode;
    const image_buffer_t *image;
    const config_t *config;
    audio_encoder_t *audio_out;
} encoding_context_t;

int encode_sstv_image(encoding_context_t *ctx);
// Dispatches to native or MMSSTV based on mode->source
```

---

## Part B: Code Modernization

### B1. Unified Error Handling System

**Goal:** Replace inconsistent error codes (-1, 0, 1, 2) with named constants.

```c
// include/pisstvpp2_error.h

#define PISSTVPP2_OK              0
#define PISSTVPP2_ERR_ARGS        1    // Invalid arguments
#define PISSTVPP2_ERR_IMAGE_LOAD  2    // Image load failed
#define PISSTVPP2_ERR_IMAGE_PROC  3    // Image processing failed
#define PISSTVPP2_ERR_SSTV_INIT   4    // SSTV initialization failed
#define PISSTVPP2_ERR_AUDIO_INIT  5    // Audio encoder initialization failed
#define PISSTVPP2_ERR_AUDIO_ENC   6    // Audio encoding failed
#define PISSTVPP2_ERR_FILE_IO     7    // File I/O error
#define PISSTVPP2_ERR_MEMORY      8    // Memory allocation failed
#define PISSTVPP2_ERR_MMSSTV      9    // MMSSTV library error
#define PISSTVPP2_ERR_UNKNOWN     99   // Unknown error

const char* error_string(int error_code);
void error_log(int error_code, const char *context);
```

**Benefit:** Consistent error handling across all modules, easier testing, better error messages.

### B2. Modular Refactoring Plan

**Current File Structure:**
```
src/
├── pisstvpp2.c               (811 lines - main entry)
├── pisstvpp2_image.c/h       (image loading/processing)
├── pisstvpp2_sstv.c/h        (native SSTV encoding)
├── pisstvpp2_audio_encoder.c/h (audio format abstraction)
├── audio_encoder_*.c         (format-specific encoders)
└── include/logging.h         (utility)
```

**Target Structure:**
```
src/
├── core/
│   ├── pisstvpp2_main.c      (Entry point, argument parsing)
│   ├── pisstvpp2_config.c/h  (Configuration management)
│   └── pisstvpp2_context.c/h (Global state management)
│
├── image/
│   ├── image_loader.c/h      (Image loading)
│   ├── image_processor.c/h   (Scaling, color conversion)
│   └── image_aspect.c/h      (Aspect ratio correction)
│
├── sstv/
│   ├── sstv_core.c/h         (Base SSTV encoding)
│   ├── sstv_native.c/h       (Native 7 modes)
│   ├── mode_registry.c/h     (Mode management)
│   └── sstv_modes/           (Mode definitions)
│       ├── modes_martin.c
│       ├── modes_scottie.c
│       └── modes_robot.c
│
├── mmsstv/
│   ├── mmsstv_loader.c/h     (Runtime library detection)
│   ├── mmsstv_adapter.c/h    (Library interface)
│   └── mmsstv_modes.c/h      (Mode bridge)
│
├── audio/
│   ├── audio_encoder.c/h     (Factory & abstraction)
│   ├── audio_gen.c/h         (Sample generation)
│   ├── audio_wav.c/h         (WAV encoder)
│   ├── audio_aiff.c/h        (AIFF encoder)
│   └── audio_ogg.c/h         (OGG encoder)
│
├── util/
│   ├── logging.c/h           (Logging)
│   ├── error.c/h             (Error codes & strings)
│   ├── memory.c/h            (Memory management helpers)
│   └── string.c/h            (String utilities)
│
└── include/
    ├── config.h              (Build-time config)
    └── public/
        └── pisstvpp2.h       (Public API for library use)
```

**Refactoring Phases:**

**Phase 1: Establish Foundations (No Breaking Changes)**
- ✅ Create error.h and error.c (unified error codes)
- ✅ Create pisstvpp2_config.h/c (config management)
- ✅ Create pisstvpp2_context.h/c (state management)
- ✅ Update existing modules to use error codes
- Keep all functionality working
- Estimated effort: 8-10 hours

**Phase 2: Modularize Image Processing**
- Extract image_loader.c from pisstvpp2_image.c
- Extract image_processor.c for scaling/color ops
- Extract image_aspect.c for aspect ratio logic
- Add proper header guards and documentation
- Validate with tests: all 55 tests should still pass
- Estimated effort: 6-8 hours

**Phase 3: Modularize SSTV Encoding**
- Extract native modes into sstv_native.c
- Create mode registry system
- Create mode_martin.c, mode_scottie.c, mode_robot.c
- Implement mode lookup and dispatch
- Validate: all 7 modes should work identically
- Estimated effort: 10-12 hours

**Phase 4: Audio Separation**
- Extract tone synthesis (audio_gen.c)
- Separate format encoders into individual modules
- Clean up audio generator context
- Validate: all 3 formats should work identically
- Estimated effort: 6-8 hours

**Phase 5: Utility Extraction**
- Create util/memory.c for allocation helpers
- Create util/string.c for string operations
- Consolidate logging (util/logging.c)
- Estimated effort: 3-4 hours

### B3. Code Quality Improvements

**Static Analysis:**
```bash
# Add to CI/CD
make analyze              # Run clang-analyzer
make check-format         # Check code formatting
make check-style          # Check style compliance
make check-const          # Check const-correctness
```

**Memory Safety:**
```bash
# Runtime checking
make sanitize            # AddressSanitizer + UBSan
make valgrind            # Valgrind memory leak detection
```

**Code Coverage:**
```bash
# Coverage reporting
make coverage            # Generate coverage reports
make coverage-report     # HTML coverage output
```

---

## Part C: MMSSTV Integration

### C1. Library Detection & Loading

**File: `src/mmsstv/mmsstv_loader.c/h`**

```c
// Responsibility: Detect and load MMSSTV library at runtime
// No build-time dependencies on MMSSTV
// Uses dlopen/dlsym for dynamic loading
// Graceful failure if library not found

typedef struct {
    void *handle;
    
    // Function pointers
    mmsstv_encoder_t* (*create)(void);
    void (*destroy)(mmsstv_encoder_t *enc);
    
    // Mode discovery
    int (*list_modes)(mmsstv_mode_list_t *modes);
    const mmsstv_mode_t* (*get_mode)(const char *code);
    
    // Encoding
    int (*init)(mmsstv_encoder_t *enc, mmsstv_mode_t *mode, int sample_rate);
    int (*encode_pixel)(mmsstv_encoder_t *enc, uint8_t r, uint8_t g, uint8_t b);
    int (*finish)(mmsstv_encoder_t *enc);
    
    char error_message[256];
} mmsstv_loader_t;

mmsstv_loader_t* mmsstv_loader_create(void);
int mmsstv_loader_load(mmsstv_loader_t *loader);
int mmsstv_is_available(void);
const char* mmsstv_get_error(mmsstv_loader_t *loader);
void mmsstv_loader_free(mmsstv_loader_t *loader);
```

### C2. Mode Registry Integration

```c
// File: src/sstv/mode_registry.c/h

mode_registry_t* registry_create(void) {
    mode_registry_t *reg = malloc(sizeof(mode_registry_t));
    
    // Add native 7 modes (always available)
    registry_add_mode(reg, &mode_martin_1);
    registry_add_mode(reg, &mode_martin_2);
    // ... other native modes
    
    // Try to load MMSSTV modes (optional)
    mmsstv_loader_t *mmsstv = mmsstv_loader_create();
    if (mmsstv_loader_load(mmsstv) == 0) {
        // Successfully loaded MMSSTV
        mmsstv_mode_list_t modes;
        if (mmsstv->list_modes(&modes) == 0) {
            for (int i = 0; i < modes.count; i++) {
                mode_definition_t mode = {
                    .code = modes.modes[i].code,
                    .name = modes.modes[i].name,
                    .source = "mmsstv",
                    .encode_frame = mmsstv_encode_frame,
                    // ...
                };
                registry_add_mode(reg, &mode);
            }
        }
    }
    
    return reg;
}
```

### C3. CLI Enhancement

**Before:**
```bash
pisstvpp2 -i input.jpg -p m1 -o output.wav
# Only 7 modes: m1, m2, s1, s2, sdx, r36, r72
```

**After (with MMSSTV):**
```bash
pisstvpp2 -i input.jpg -p pd120 -o output.wav
# 7 native + 50+ MMSSTV modes if library available
# Fallback to native modes if library missing

# List available modes
pisstvpp2 --list-modes
# Output includes source (native/mmsstv) and availability
```

**Dynamic Help:**
```bash
pisstvpp2 -h
# Help text includes:
# - All available modes (native ✓ + mmsstv ✓/✗)
# - Library status
# - How to enable MMSSTV if missing
```

---

## Part D: Build System Enhancement

### D1. Makefile Improvements

**Current Structure:**
```makefile
CFLAGS += $(VIPS_CFLAGS)
LDFLAGS += $(VIPS_LIBS)
```

**Enhanced Structure:**
```makefile
# ============================================================================
# MMSSTV Library Detection
# ============================================================================

MMSSTV_AVAILABLE := $(shell pkg-config --exists mmsstv-portable && echo 1 || echo 0)
ifneq ($(MMSSTV_AVAILABLE),1)
  # Try alternate detection methods
  ifneq ($(MMSSTV_LIB_PATH),)
    MMSSTV_AVAILABLE := $(shell test -f $(MMSSTV_LIB_PATH) && echo 1 || echo 0)
  endif
endif

ifeq ($(MMSSTV_AVAILABLE),1)
  MMSSTV_CFLAGS := -DHAVE_MMSSTV=1
  MMSSTV_LIBS := -L$(MMSSTV_LIB_PATH) -lmmsstv
  $(info [INFO] MMSSTV library detected)
else
  $(info [INFO] MMSSTV library not found - native modes only)
endif

CFLAGS += $(MMSSTV_CFLAGS)
LDFLAGS += $(MMSSTV_LIBS)

# ============================================================================
# Build Targets
# ============================================================================

# Standard build
all: $(TARGET)

# Build with debugging symbols
debug: CFLAGS += -g -O0
debug: $(TARGET)

# Build with all sanitizers
sanitize: CFLAGS += -fsanitize=address -fsanitize=undefined -g
sanitize: $(TARGET)

# Code analysis
analyze:
	clang --analyze $(CFLAGS) $(SRC_FILES)

# Coverage
coverage: CFLAGS += --coverage
coverage: $(TARGET)
	$(MAKE) test
	gcov $(SRC_FILES)

# Install
install: $(TARGET)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/

help:
	@echo "PiSSTVpp2 Build System"
	@echo "======================="
	@echo ""
	@echo "MMSSTV Library Status: $(MMSSTV_AVAILABLE)"
	@echo ""
	@echo "Targets:"
	@echo "  make all          - Build main executable"
	@echo "  make debug        - Build with debug symbols"
	@echo "  make sanitize     - Build with sanitizers"
	@echo "  make test         - Run test suite"
	@echo "  make analyze      - Static analysis"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make install      - Install to system"
	@echo ""
	@echo "Environment Variables:"
	@echo "  MMSSTV_LIB_PATH   - Path to libmmsstv.so"
	@echo "  PREFIX            - Installation prefix (default: /usr/local)"
	@echo "  DESTDIR           - Installation destination"
```

### D2. Build Output

**Informative Build Messages:**
```
$ make all

[BUILD] Detecting dependencies...
[INFO]  - libvips: ✓ /usr/lib/libvips.so
[INFO]  - libogg:  ✓ /usr/lib/libogg.so
[INFO]  - libvorbis: ✓ /usr/lib/libvorbis.so
[INFO]  - MMSSTV: ✗ not found
[BUILD] Compiling core/pisstvpp2_main.c...
[BUILD] Compiling image/image_loader.c...
[BUILD] Compiling sstv/sstv_core.c...
[BUILD] Compiling sstv/sstv_native.c (7 modes)...
[BUILD] Compiling mmsstv/mmsstv_loader.c (detection only)...
[BUILD] Linking pisstvpp2...
[BUILD] Build complete: bin/pisstvpp2 (137 KB)

Available SSTV modes: 7 native modes
(MMSSTV modes not available - see docs/BUILD.md#mmsstv for setup)

$ export MMSSTV_LIB_PATH=/opt/mmsstv/lib/libmmsstv.so
$ make clean && make all

[BUILD] Detecting dependencies...
[INFO]  - libvips: ✓ /usr/lib/libvips.so
[INFO]  - MMSSTV: ✓ /opt/mmsstv/lib/libmmsstv.so
[BUILD] Compiling mmsstv/mmsstv_adapter.c (dynamic mode loading)...
[BUILD] Linking pisstvpp2...
[BUILD] Build complete: bin/pisstvpp2 (165 KB)

Available SSTV modes: 7 native + 50 MMSSTV = 57 total modes
```

---

## Part E: Testing & Validation

### E1. Test Coverage Expansion

**Unit Tests:**
- Mode registry (add, lookup, list)
- Error codes (all error paths)
- Config parsing (all option combinations)
- MMSSTV detection (with/without library)
- Image processing (all aspect modes)

**Integration Tests:**
- Native mode encode (all 7)
- MMSSTV modes (when library available)
- Format combinations (mode + format + sample rate)
- Error handling (graceful degradation)

**Validation:**
```bash
# Ensure modernization doesn't break functionality
make clean && make all
./tests/test_suite.py
# Target: Still 55/55 tests passing
```

### E2. Backward Compatibility Testing

**Regression Tests:**
```bash
# Old command-line syntax should work identically
./bin/pisstvpp2 -i test.jpg -p m1 -o test1.wav
./bin/pisstvpp2 -i test.jpg -p s2 -f aiff -o test2.aiff
./bin/pisstvpp2 -i test.jpg -p r36 -r 44100 -o test3.wav

# Compare outputs to v2.0 baseline
# Audio files should be identical within tolerance
```

---

## Part F: Documentation

### F1. Core Documentation Updates

**New Documents:**
- `docs/MMSSTV_SETUP.md` - How to obtain and configure MMSSTV library
- `docs/MODERNIZATION_GUIDE.md` - Code structure overview for contributors
- `docs/DEVELOPER_WORKFLOW.md` - Contributing code changes
- `docs/ARCHITECTURE_v2_1.md` - Updated architecture post-refactoring

**Updated Documents:**
- `README.md` - Expand MMSSTV section
- `BUILD.md` - Add MMSSTV build instructions
- `ARCHITECTURE.md` - Document mode registry system

### F2. Developer Accessibility

**Primary Goal:** Make code easy for ham radio enthusiasts to understand and modify.

**Strategy:**
- Keep code simple and readable (no over-engineering)
- Add detailed comments explaining WHY not just WHAT
- Use clear naming conventions
- Provide examples for common tasks (adding a new mode, new audio format)
- Create "contributor playbook" with step-by-step guides

**Example: Adding a New MMSSTV Mode (Once Library Available)**
```c
// File: src/sstv/modes_pd.c

#include "pisstvpp2_mode_registry.h"
#include "pisstvpp2_mmsstv_loader.h"

// Define the mode using MMSSTV library
static int pd120_encode_frame(const image_buffer_t *img, audio_samples_t *audio) {
    // Implementation using MMSSTV library
    // See docs/DEVELOPER_WORKFLOW.md#adding_modes for details
}

// Register with mode registry
const mode_definition_t mode_pd_120 = {
    .code = "pd120",
    .name = "PD-120 (Pasokon Densitsu)",
    .vis_code = 93,
    .width = 512,
    .height = 256,
    .transmission_time_ms = 240000,
    .encode_frame = pd120_encode_frame,
    .source = "mmsstv",
};
```

---

## Implementation Timeline

### Phase 1: Foundations (Weeks 1-2)
- [ ] Design and implement error handling system
- [ ] Create config and context management
- [ ] Update all modules to use new error codes
- [ ] Validate: 55/55 tests still passing

### Phase 2: Image Module Refactoring (Weeks 2-3)
- [ ] Extract image_loader.c
- [ ] Extract image_processor.c
- [ ] Extract image_aspect.c
- [ ] Validate: all image operations identical

### Phase 3: SSTV Native Module Refactoring (Weeks 3-4)
- [ ] Create mode registry system
- [ ] Extract mode definitions to individual files
- [ ] Implement mode lookup and dispatch
- [ ] Validate: all 7 modes work identically

### Phase 4: MMSSTV Integration (Weeks 5-6)
- [ ] Implement MMSSTV library detection
- [ ] Create mmsstv_loader.c
- [ ] Create mmsstv_adapter.c
- [ ] Integrate with mode registry
- [ ] Test with/without library

### Phase 5: Build System Enhancement (Week 6)
- [ ] Update Makefile with MMSSTV detection
- [ ] Add build targets (debug, sanitize, analyze)
- [ ] Create install target
- [ ] Document environment variables

### Phase 6: Audio Module Refactoring (Week 7)
- [ ] Extract audio_gen.c
- [ ] Separate format encoders
- [ ] Validate: all formats work identically

### Phase 7: Testing & Validation (Week 7-8)
- [ ] Expand unit tests
- [ ] Add MMSSTV integration tests
- [ ] Regression testing
- [ ] Backward compatibility validation
- [ ] Target: 55/55 + new tests passing

### Phase 8: Documentation (Week 8)
- [ ] Update docs for new architecture
- [ ] Create MMSSTV setup guide
- [ ] Create developer workflow guide
- [ ] Code accessibility review

---

## Principles & Guidelines

### Code Accessibility
1. **Keep it simple**: No fancy patterns or over-abstraction
2. **Document reasoning**: WHY decisions, not just WHAT code does
3. **Example code**: Provide concrete examples for common tasks
4. **Modularity**: Small focused files, clear responsibilities
5. **Gradual learning**: Can understand one module without whole codebase

### Backward Compatibility
1. CLI options unchanged
2. Default behavior identical
3. Output format identical (or improved transparently)
4. Graceful fallback if MMSSTV unavailable

### Modern C Practices
1. Consistent error handling
2. Memory safety (valgrind verified)
3. Const-correctness
4. Static analysis passing
5. No undefined behavior

### Ham Radio Community
1. Open to experimentation
2. Encourage modifications
3. Clear contribution process
4. Responsive to feedback
5. Educational value

---

## Task List Summary

### Must-Have (Core Functionality)
- [ ] Unified error handling system (Part B1)
- [ ] Config and context management (Part B1)
- [ ] Mode registry system (Part A1)
- [ ] MMSSTV library detection (Part C1)
- [ ] Dynamic CLI support (Part C3)
- [ ] Enhanced Makefile (Part D1)

### Should-Have (Code Quality)
- [ ] Image module refactoring (Part B2)
- [ ] SSTV module refactoring (Part B2)
- [ ] MMSSTV adapter layer (Part C2)
- [ ] Expanded test coverage (Part E1)
- [ ] Developer documentation (Part F2)

### Nice-to-Have (Polish)
- [ ] Audio module refactoring (Part B2)
- [ ] Static analysis targets (Part B3)
- [ ] Coverage reporting (Part B3)
- [ ] Contributor playbook (Part F2)

---

## Success Criteria

### Functional Requirements
✅ All 7 native SSTV modes work identically to v2.0  
✅ All 3 audio formats work identically to v2.0  
✅ 55/55 existing tests pass without modification  
✅ MMSSTV library detected at runtime when available  
✅ CLI dynamically expanded with MMSSTV modes (if available)  
✅ Graceful fallback if MMSSTV library missing  

### Code Quality Requirements
✅ Unified error handling (no mixed error codes)  
✅ Static analysis passes (clang-analyzer)  
✅ Memory leak free (valgrind)  
✅ Const correctness throughout  
✅ No compiler warnings (all platforms)  

### Accessibility Requirements
✅ Code well-commented for readability  
✅ Clear file organization  
✅ Contribution guide for new features  
✅ Example code for common tasks  
✅ Suitable for ham radio community enhancement  

---

## Risk Mitigation

### Risk: Break existing functionality
**Mitigation:** Keep 55/55 test suite passing at each phase, frequent validation

### Risk: MMSSTV library API unknown
**Mitigation:** Create stub interface first, iterate as library becomes available

### Risk: Build system complexity
**Mitigation:** Keep pkg-config detection simple, graceful fallback to native-only

### Risk: Community doesn't adopt 
**Mitigation:** Strong documentation, keep code simple, responsive to feedback

---

## Next Steps

1. **Consensus**: Validate this approach with team
2. **Start Phase 1**: Error handling system foundation
3. **Parallel tracking**: Keep tab on mmsstv-portable library status
4. **Iterate**: Constant testing and validation
5. **Engage community**: Share progress with ham radio community early

