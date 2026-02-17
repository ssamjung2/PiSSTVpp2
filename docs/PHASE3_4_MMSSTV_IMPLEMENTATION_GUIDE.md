# Phase 3-4 Implementation Guide: Mode Registry & MMSSTV Integration
**Based on Real libsstv_encoder v1.0.0 API**

**Date:** February 14, 2026  
**Status:** Ready to implement (all API documentation available)  
**Library Location:** `/Users/ssamjung/Desktop/WIP/mmsstv-portable/build`  
**Binary:** `libsstv_encoder.1.dylib` (macOS), `.a` (static)  
**Header:** `libsstv_encoder/include/sstv_encoder.h`

---

## Executive Summary

The MMSSTV library is **production-ready with 43 SSTV modes** and a clean C API. Integration will be straightforward:

1. **Phase 3:** Create mode registry (holds both native 7 modes + up to 43 MMSSTV modes)
2. **Phase 4:** Link to real MMSSTV library and register all available modes
3. **Result:** Support 7-50 SSTV modes (depending on library availability)

**Estimated effort:** 20-25 hours (down from 25-35 hours estimate in master plan) because API is concrete and stable.

---

## Library Overview

### What's Available

| Component | Details |
|-----------|---------|
| **Library Name** | libsstv_encoder |
| **Version** | 1.0.0 |
| **Modes** | 43 SSTV modes (color & B/W) |
| **Build Type** | Dynamic (.dylib) + Static (.a) |
| **Language** | C (no C++ dependency) |
| **Platform** | macOS (confirmed), Linux (.so available) |
| **License** | LGPL 3.0+ (compatible with SlowFrame) |

### Build Artifacts Available

```
/Users/ssamjung/Desktop/WIP/mmsstv-portable/build/
├── libsstv_encoder.1.0.0.dylib        ← Main library (macOS)
├── libsstv_encoder.1.dylib            ← Symlink
├── libsstv_encoder.dylib              ← Symlink
├── libsstv_encoder.a                  ← Static library
├── libsstv_decoder.1.0.0.dylib        ← Decoder (not needed)
├── libsstv_decoder.a                  ← Decoder static (not needed)
└── sstv_encoder.pc                    ← pkg-config file
```

### Header File Location

```
/Users/ssamjung/Desktop/WIP/mmsstv-portable/include/
├── sstv_encoder.h                     ← Main encoder API (288 lines)
└── sstv_decoder.h                     ← Decoder API (not needed)
```

### Example Usage Available

```
/Users/ssamjung/Desktop/WIP/mmsstv-portable/examples/
├── encode_wav.c                       ← WAV generation example
├── list_modes.c                       ← Mode listing example
├── generate_all_modes.c               ← Generate test image
└── test_real_images.c                 ← Real image encoding
```

---

## MMSSTV Library API Reference

### Core Data Structures

```c
// Mode enumeration (43 modes available)
typedef enum {
    SSTV_R36 = 0,           // Robot 36
    SSTV_R72,               // Robot 72
    SSTV_AVT90,             // AVT 90
    SSTV_SCOTTIE1,          // Scottie 1
    SSTV_SCOTTIE2,          // Scottie 2
    SSTV_SCOTTIEX,          // Scottie DX
    SSTV_MARTIN1,           // Martin 1
    SSTV_MARTIN2,           // Martin 2
    // ... 35 more modes ...
    SSTV_MODE_COUNT         // Total = 43 modes
} sstv_mode_t;

// Image structure
typedef struct {
    uint8_t *pixels;              // RGB or grayscale data
    uint32_t width;               // Width in pixels
    uint32_t height;              // Height in pixels
    uint32_t stride;              // Bytes per row
    sstv_pixel_format_t format;   // RGB24 or GRAY8
} sstv_image_t;

// Mode information
typedef struct {
    sstv_mode_t mode;             // Mode enum
    const char *name;             // "Scottie 1", "Martin 2", etc.
    uint32_t width;               // Required width
    uint32_t height;              // Required height
    uint8_t vis_code;             // VIS code (0x3C for Scottie 1, etc.)
    double duration_sec;          // Encoding time in seconds
    int is_color;                 // 1=color, 0=B/W
} sstv_mode_info_t;
```

### Encoder Handle

```c
// Opaque encoder type (implementation hidden)
typedef struct sstv_encoder_s sstv_encoder_t;
```

### Encoder API Functions

```c
// Create/destroy
sstv_encoder_t* sstv_encoder_create(sstv_mode_t mode, double sample_rate);
void sstv_encoder_free(sstv_encoder_t *encoder);

// Configure image
int sstv_encoder_set_image(sstv_encoder_t *encoder, const sstv_image_t *image);

// VIS code control
void sstv_encoder_set_vis_enabled(sstv_encoder_t *encoder, int enable);

// Generate audio samples
size_t sstv_encoder_generate(
    sstv_encoder_t *encoder,
    float *samples,              // Output: -1.0 to +1.0 range
    size_t max_samples
);

// Check completion
int sstv_encoder_is_complete(sstv_encoder_t *encoder);

// Progress & info
float sstv_encoder_get_progress(sstv_encoder_t *encoder);
size_t sstv_encoder_get_total_samples(sstv_encoder_t *encoder);

// Reset to re-encode
void sstv_encoder_reset(sstv_encoder_t *encoder);
```

### Mode Info API Functions

```c
// Get mode information
const sstv_mode_info_t* sstv_get_mode_info(sstv_mode_t mode);

// List all modes
const sstv_mode_info_t* sstv_get_all_modes(size_t *count);

// Find mode by name
int sstv_find_mode_by_name(const char *name);

// Get version string
const char* sstv_encoder_version(void);

// Dimension calculation
int sstv_get_mode_dimensions(sstv_mode_t mode, uint32_t *width, uint32_t *height);
```

### Image Helper Functions

```c
// Create image from RGB data (doesn't copy)
sstv_image_t sstv_image_from_rgb(
    uint8_t *rgb_data,
    uint32_t width,
    uint32_t height
);

// Create image from grayscale data
sstv_image_t sstv_image_from_gray(
    uint8_t *gray_data,
    uint32_t width,
    uint32_t height
);
```

---

## 43 Available SSTV Modes

The library supports all these modes:

| VIS | Mode | Size | Duration | Type |
|-----|------|------|----------|------|
| 0x88 | Robot 36 | 320×240 | 36s | Color |
| 0x0C | Robot 72 | 320×240 | 72s | Color |
| 0x45 | AVT 90 | 800×600 | 90s | Color |
| 0x3C | Scottie 1 | 320×256 | 110s | Color |
| 0x38 | Scottie 2 | 320×256 | 55s | Color |
| 0x4C | Scottie DX | 320×256 | 270s | Color |
| 0xAC | Martin 1 | 320×256 | 114s | Color |
| 0xA8 | Martin 2 | 320×256 | 57s | Color |
| 0x5E | SC2-180 | 600×150 | 180s | Color |
| 0x5D | SC2-120 | 600×150 | 120s | Color |
| 0x5C | SC2-60 | 600×150 | 60s | Color |
| 0x5F | PD 50 | 640×496 | 50s | Color |
| 0x63 | PD 90 | 640×496 | 90s | Color |
| 0x5D | PD 120 | 640×496 | 120s | Color |
| 0x62 | PD 160 | 640×496 | 160s | Color |
| 0x60 | PD 180 | 640×496 | 180s | Color |
| 0x61 | PD 240 | 640×496 | 240s | Color |
| 0x5E | PD 290 | 640×496 | 290s | Color |
| 0x71 | Pasokon P3 | 640×480 | 159s | Color |
| 0x72 | Pasokon P5 | 800×600 | 207s | Color |
| 0x73 | Pasokon P7 | 960×720 | 255s | Color |
| 0x4E | Martin R73 | 640×480 | 73s | Color |
| 0x62 | Martin R90 | 640×480 | 90s | Color |
| 0x68 | Martin R115 | 640×480 | 115s | Color |
| 0x6C | Martin R140 | 640×480 | 140s | Color |
| 0x6E | Martin R175 | 640×480 | 175s | Color |
| 0x4F | Martin P73 | 800×600 | 73s | Color |
| 0x67 | Martin P115 | 800×600 | 115s | Color |
| 0x6D | Martin P140 | 800×600 | 140s | Color |
| 0x6F | Martin P175 | 800×600 | 175s | Color |
| 0x6A | Martin L180 | 1024×768 | 180s | Color |
| 0x6B | Martin L240 | 1024×768 | 240s | Color |
| 0x70 | Martin L280 | 1024×768 | 280s | Color |
| 0x58 | Martin L320 | 1024×768 | 320s | Color |
| 0x1C | Robot 24 | 320×240 | 24s | Color |
| 0x2E | B/W 8 | 320×240 | 8s | B/W |
| 0x2F | B/W 12 | 320×240 | 12s | B/W |
| 0x7A | MP73-N | 320×240 | 73s | Color |
| 0x7B | MP110-N | 320×240 | 110s | Color |
| 0x7C | MP140-N | 320×240 | 140s | Color |
| 0x7D | MC110-N | 320×240 | 110s | B/W |
| 0x7E | MC140-N | 320×240 | 140s | B/W |
| 0x7F | MC180-N | 320×240 | 180s | B/W |

---

## Phase 3 Implementation: Mode Registry

### Task 3.1: Create Mode Registry System

**File:** `src/sstv/mode_registry.h` and `src/sstv/mode_registry.c`

#### Header File (mode_registry.h)

```c
/**
 * @file mode_registry.h
 * @brief SSTV Mode Registry - Unified mode definitions and lookup
 *
 * Provides a registry holding both native (7) and MMSSTV (up to 43) modes.
 * Modes can be looked up by code, name, or enumerated.
 */

#ifndef MODE_REGISTRY_H
#define MODE_REGISTRY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
typedef struct mode_registry_s mode_registry_t;

/**
 * Mode definition structure - describes one SSTV mode
 */
typedef struct {
    const char *code;              /* Mode code (e.g., "m1", "s1", "r36") */
    const char *name;              /* Mode name (e.g., "Martin 1") */
    uint8_t vis_code;              /* VIS byte (e.g., 0xAC for Martin 1) */
    uint32_t width;                /* Required image width */
    uint32_t height;               /* Required image height */
    double duration_sec;           /* Encoding duration in seconds */
    int is_color;                  /* 1=color, 0=grayscale */
    const char *source;            /* "native" or "mmsstv" */
    
    /* Function pointer for native modes (NULL for MMSSTV) */
    int (*encode_frame)(const char *mode_code,
                       uint16_t sample_rate,
                       uint16_t *audio_samples,
                       uint32_t max_samples);
} mode_definition_t;

/* Registry creation/destruction */
mode_registry_t* mode_registry_create(void);
void mode_registry_free(mode_registry_t *reg);

/* Mode registration */
int mode_registry_add(mode_registry_t *reg, const mode_definition_t *mode);

/* Mode lookup */
const mode_definition_t* mode_registry_lookup_by_code(
    const mode_registry_t *reg,
    const char *code
);

const mode_definition_t* mode_registry_lookup_by_name(
    const mode_registry_t *reg,
    const char *name
);

const mode_definition_t* mode_registry_lookup_by_vis(
    const mode_registry_t *reg,
    uint8_t vis_code
);

/* Mode enumeration */
typedef struct {
    const mode_definition_t *modes;
    size_t count;
} mode_list_t;

mode_list_t mode_registry_list_all(const mode_registry_t *reg);

/* Utility */
int mode_registry_mode_count(const mode_registry_t *reg);

#ifdef __cplusplus
}
#endif

#endif /* MODE_REGISTRY_H */
```

#### Implementation Overview

The registry will:
1. Hold up to 50 mode definitions in a dynamic array
2. Support add/lookup/enumerate operations
3. Return NULL on invalid lookups (safe error handling)
4. Be thread-safe for read operations
5. Store mode code, name, VIS code, dimensions, duration, color flag, and source

---

## Phase 4 Implementation: MMSSTV Integration

### Task 4.1: Update mmsstv_stub.h with Real API

**File:** `src/include/mmsstv_stub.h`

This will now contain the actual libsstv_encoder API signatures we need to load dynamically:

```c
/**
 * @file mmsstv_stub.h
 * @brief MMSSTV Library API Contract
 *
 * This file defines the MMSSTV library API that we dynamically load.
 * It's loaded via dlopen/dlsym at runtime if the library is available.
 */

#ifndef MMSSTV_STUB_H
#define MMSSTV_STUB_H

#include <stdint.h>
#include <stddef.h>

/* ===== MMSSTV Encoder API (from libsstv_encoder.h) ===== */

/* Mode enumeration */
typedef enum {
    SSTV_R36 = 0,
    SSTV_R72,
    SSTV_AVT90,
    SSTV_SCOTTIE1,
    SSTV_SCOTTIE2,
    SSTV_SCOTTIEX,
    SSTV_MARTIN1,
    SSTV_MARTIN2,
    SSTV_SC2_180,
    SSTV_SC2_120,
    SSTV_SC2_60,
    SSTV_PD50,
    SSTV_PD90,
    SSTV_PD120,
    SSTV_PD160,
    SSTV_PD180,
    SSTV_PD240,
    SSTV_PD290,
    SSTV_P3,
    SSTV_P5,
    SSTV_P7,
    SSTV_MR73,
    SSTV_MR90,
    SSTV_MR115,
    SSTV_MR140,
    SSTV_MR175,
    SSTV_MP73,
    SSTV_MP115,
    SSTV_MP140,
    SSTV_MP175,
    SSTV_ML180,
    SSTV_ML240,
    SSTV_ML280,
    SSTV_ML320,
    SSTV_R24,
    SSTV_BW8,
    SSTV_BW12,
    SSTV_MN73,
    SSTV_MN110,
    SSTV_MN140,
    SSTV_MC110,
    SSTV_MC140,
    SSTV_MC180,
    SSTV_MODE_COUNT
} sstv_mode_t;

typedef enum {
    SSTV_RGB24 = 0,
    SSTV_GRAY8
} sstv_pixel_format_t;

typedef struct {
    uint8_t *pixels;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    sstv_pixel_format_t format;
} sstv_image_t;

typedef struct {
    sstv_mode_t mode;
    const char *name;
    uint32_t width;
    uint32_t height;
    uint8_t vis_code;
    double duration_sec;
    int is_color;
} sstv_mode_info_t;

typedef struct sstv_encoder_s sstv_encoder_t;

/* Function pointers for dynamic loading */
typedef struct {
    /* Core encoder functions */
    sstv_encoder_t* (*encoder_create)(sstv_mode_t mode, double sample_rate);
    void (*encoder_free)(sstv_encoder_t *encoder);
    int (*encoder_set_image)(sstv_encoder_t *encoder, const sstv_image_t *image);
    void (*encoder_set_vis_enabled)(sstv_encoder_t *encoder, int enable);
    size_t (*encoder_generate)(sstv_encoder_t *encoder, float *samples, size_t max_samples);
    int (*encoder_is_complete)(sstv_encoder_t *encoder);
    float (*encoder_get_progress)(sstv_encoder_t *encoder);
    size_t (*encoder_get_total_samples)(sstv_encoder_t *encoder);
    void (*encoder_reset)(sstv_encoder_t *encoder);
    
    /* Mode info functions */
    const sstv_mode_info_t* (*get_mode_info)(sstv_mode_t mode);
    const sstv_mode_info_t* (*get_all_modes)(size_t *count);
    int (*find_mode_by_name)(const char *name);
    const char* (*encoder_version)(void);
    int (*get_mode_dimensions)(sstv_mode_t mode, uint32_t *width, uint32_t *height);
    
    /* Image helpers */
    sstv_image_t (*image_from_rgb)(uint8_t *rgb_data, uint32_t width, uint32_t height);
    sstv_image_t (*image_from_gray)(uint8_t *gray_data, uint32_t width, uint32_t height);
} mmsstv_vtable_t;

#endif /* MMSSTV_STUB_H */
```

### Task 4.2: Implement MMSSTV Loader

**File:** `src/mmsstv/mmsstv_loader.c/h`

This module will:
1. Check environment variables (`MMSSTV_LIB_PATH`, `MMSSTV_INCLUDE_PATH`)
2. Use `dlopen()` to load the library dynamically
3. Use `dlsym()` to resolve function symbols
4. Populate the `mmsstv_vtable_t` function pointer table
5. Return gracefully with NULL if library not found

### Task 4.3: Integrate with Mode Registry

Once Phase 3 registry is complete:
1. Register all 7 native modes at startup
2. If MMSSTV library available:
   - Call `get_all_modes()` to enumerate modes
   - Create `mode_definition_t` for each MMSSTV mode
   - Register with the registry (source = "mmsstv")
3. Registry now has 7-50 modes depending on library availability

### Task 4.4: Update SSTV Encoder Dispatcher

The existing `slowframe_sstv.c` dispatcher will:
1. Look up mode from registry
2. If source = "native", call legacy `buildaudio_*()` function
3. If source = "mmsstv", use the MMSSTV library encoder:

```c
// Pseudo-code for encoding with MMSSTV
sstv_encoder_t *encoder = mmsstv->encoder_create(mmsstv_mode, sample_rate);
if (!encoder) return SLOWFRAME_ERR_SSTV_ENCODE;

sstv_image_t image = mmsstv->image_from_rgb(rgb_buffer, width, height);
if (mmsstv->encoder_set_image(encoder, &image) != 0) {
    mmsstv->encoder_free(encoder);
    return SLOWFRAME_ERR_IMAGE_LOAD;
}

mmsstv->encoder_set_vis_enabled(encoder, 1);

while (!mmsstv->encoder_is_complete(encoder)) {
    size_t samples = mmsstv->encoder_generate(encoder, float_buffer, BUFFER_SIZE);
    // Convert float samples to int16 and write to output
}

mmsstv->encoder_free(encoder);
```

---

## Estimated Timeline

| Phase | Task | Hours | Notes |
|-------|------|-------|-------|
| 3 | 3.1 - Mode Registry | 3-4 | Create registry, add, lookup, enumerate |
| 3 | 3.2 - Extract Native Modes | 4-5 | Move buildaudio_* functions to modules |
| 3 | 3.3 - Mode Initialization | 2-3 | Register 7 native modes |
| 3 | 3.4 - Mode Dispatcher | 3-4 | Update SSTV encoder to use registry |
| 4 | 4.1 - MMSSTV Loader | 4-5 | dlopen/dlsym implementation |
| 4 | 4.2 - MMSSTV Adapter | 3-4 | Mode enumeration and registration |
| 4 | 4.3 - Registry Integration | 2-3 | Register MMSSTV modes dynamically |
| 4 | 4.4 - Dispatcher Update | 3-4 | Route to MMSSTV encoder when available |
| **TOTAL** | | **24-32** | Down from 25-35 estimate (concrete API) |

---

## Build Integration

### Linking to MMSSTV Library

**Option 1: Dynamic Linking (Recommended)**
```bash
# At compile time (optional - for IDE support)
# Link to libsstv_encoder.dylib on macOS
# -L/Users/ssamjung/Desktop/WIP/mmsstv-portable/build \
# -lsstv_encoder

# At runtime (critical)
# Use dlopen with full path if needed:
# handle = dlopen("/Users/ssamjung/Desktop/WIP/mmsstv-portable/build/libsstv_encoder.1.dylib", ...)
```

**Option 2: Static Linking**
```bash
# Link with libsstv_encoder.a (no runtime dependency)
-L/Users/ssamjung/Desktop/WIP/mmsstv-portable/build \
-lsstv_encoder
```

### Makefile Updates Needed

1. Add detection of libsstv_encoder
2. Add optional linking (won't fail if not found)
3. Report library detection status during build

---

## Success Criteria Checklist

### Phase 3 Complete When:
- [ ] Mode registry created and tested
- [ ] All 7 native modes extract and register correctly
- [ ] Mode lookup works by code ("m1"), name ("Martin 1"), and VIS (0xAC)
- [ ] 55/55 tests still passing
- [ ] Audio output byte-identical to v2.0

### Phase 4 Complete When:
- [ ] MMSSTV library successfully loads (if available)
- [ ] All 43 modes enumerated and registered
- [ ] Modes listed by `--list-modes`
- [ ] MMSSTV modes encode successfully
- [ ] Graceful fallback to native-only if library missing
- [ ] Documentation updated

---

## Testing with Real Library

Once Phase 4 implemented, validate with:

```bash
# List all available modes (7 native + 43 MMSSTV = 50)
./bin/slowframe --list-modes

# Check library detection
./bin/slowframe --mmsstv-status

# Encode with MMSSTV mode
./bin/slowframe -i tests/images/color_bars_320x256.png -p avt90 -o output_avt90.wav

# Encode with native mode (should still work)
./bin/slowframe -i tests/images/color_bars_320x256.png -p m1 -o output_m1.wav
```

---

## References

**MMSSTV Library Source:**
- Build: `/Users/ssamjung/Desktop/WIP/mmsstv-portable/build/`
- Headers: `/Users/ssamjung/Desktop/WIP/mmsstv-portable/include/sstv_encoder.h`
- Examples: `/Users/ssamjung/Desktop/WIP/mmsstv-portable/examples/`

**SlowFrame Current Status:**
- Phase 1-2: Complete (error codes, config, image modules)
- Phase 3: Ready to start (mode registry)
- Phase 4: Unblocked once Phase 3 complete

---

## Next Steps

1. **Implement Phase 3.1** - Mode Registry (start immediately)
        - 3-4 hours, unblocks everything else
2. **Complete Phase 3.2-3.4** - Extract and register native modes
   - 9-12 hours total
   - Keep audio output byte-identical using existing buildaudio_* logic
3. **Implement Phase 4** - MMSSTV integration
   - 8-12 hours total
   - Uses concrete MMSSTV API from real library
   - No guesswork needed

**Expected completion:** 1.5-2 weeks with these concrete APIs and clear path forward.
