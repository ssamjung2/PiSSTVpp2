# MMSSTV Library Integration Guide

**Document Version:** 1.0  
**Date:** January 29, 2026  
**Status:** Integration Planning Phase  

---

## Overview

This document describes the integration architecture between **PiSSTVpp2** and the **mmsstv-portable** library currently under development. The integration is designed to:

1. **Maintain backward compatibility** - Existing 7 modes continue working via legacy code
2. **Enable gradual migration** - New modes use MMSSTV library when available
3. **Support conditional compilation** - Build with or without MMSSTV support
4. **Provide unified interface** - Application code doesn't need to know which backend is used

---

## Architecture

### Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                      PiSSTVpp2 Main                             │
│                   (pisstvpp2.c)                                 │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         │ Protocol string (e.g., "m1", "pd120")
                         ├
┌────────────────────────▼────────────────────────────────────────┐
│              MMSSTV Adapter Layer                               │
│          (pisstvpp2_mmsstv_adapter.c/h)                         │
│                                                                 │
│  • Mode mapping and validation                                 │
│  • Backend selection (legacy vs MMSSTV)                        │
│  • Callback management                                         │
│  • Unified encoding interface                                  │
└─────────────┬─────────────────────────┬─────────────────────────┘
              │                         │
              │ Legacy modes            │ Extended modes
              │ (m1, m2, s1, etc.)     │ (pd120, avt90, etc.)
              │                         │
┌─────────────▼─────────┐     ┌─────────▼────────────────────────┐
│  Legacy SSTV Module   │     │   MMSSTV Library                 │
│  (pisstvpp2_sstv.c)   │     │   (mmsstv-portable)              │
│                       │     │                                  │
│  • Martin 1/2         │     │  • 100+ modes                    │
│  • Scottie 1/2/DX     │     │  • PD, Wraase, AVT, ML families  │
│  • Robot 36/72        │     │  • Extensible architecture       │
└───────────────────────┘     └──────────────────────────────────┘
```

### Data Flow

```
Image Loading → Aspect Correction → Pixel Buffer
                                          │
                                          │ Pixel callback
                                          │
                   ┌──────────────────────▼────────────────────┐
                   │                                           │
              Legacy Backend                            MMSSTV Backend
                   │                                           │
                   │                                           │
              Tone synthesis                           Library encoder
              RGB → Frequency                          Mode-specific encoding
                   │                                           │
                   └──────────────────────┬────────────────────┘
                                          │
                                          │ Audio callback
                                          ▼
                                    Audio Buffer
                                          │
                                          ▼
                              Audio Encoder (WAV/AIFF/OGG)
```

---

## File Structure

### New Files Created

```
src/include/mmsstv_stub.h
    └─ Stub interface for mmsstv-portable library
       • Defines expected API contract
       • Mode definitions and enums
       • Encoder interface
       • Callback types

src/include/pisstvpp2_mmsstv_adapter.h
    └─ Adapter layer public interface
       • Mode discovery functions
       • Encoder creation/configuration
       • Unified encoding interface

src/pisstvpp2_mmsstv_adapter.c
    └─ Adapter implementation
       • Mode mapping table (7 legacy + 100+ MMSSTV)
       • Backend routing logic
       • Pixel/audio callbacks
       • Error handling
```

### Modified Files (Future)

```
src/pisstvpp2.c
    └─ Main application
       • Replace direct SSTV module calls with adapter
       • Extended protocol options in CLI
       • Help text for new modes

Makefile
    └─ Build system
       • Conditional MMSSTV compilation
       • pkg-config integration
       • Feature flags

docs/README.md
    └─ User documentation
       • Extended mode list
       • MMSSTV library requirements
```

---

## Build Configuration

### Compilation Flags

The integration uses conditional compilation to support both scenarios:

**Without MMSSTV** (legacy only):
```bash
make clean
make all
# Only 7 modes available: m1, m2, s1, s2, sdx, r36, r72
```

**With MMSSTV** (100+ modes):
```bash
make clean
make all HAVE_MMSSTV=1
# All modes available: legacy + PD + Wraase + AVT + ML + etc.
```

### Makefile Integration (Proposed)

```makefile
# MMSSTV library support (optional)
ifdef HAVE_MMSSTV
    CFLAGS += -DHAVE_MMSSTV_SUPPORT
    LDFLAGS += -lmmsstv
    # Add pkg-config detection once library provides it
    # CFLAGS += $(shell pkg-config --cflags mmsstv)
    # LDFLAGS += $(shell pkg-config --libs mmsstv)
endif

# Adapter module (always compiled)
OBJS += src/pisstvpp2_mmsstv_adapter.o
```

---

## Mode Mapping

### Legacy Modes (Always Available)

| Protocol | VIS Code | Mode Name          | Resolution  | Duration |
|----------|----------|--------------------|-------------|----------|
| m1       | 44       | Martin 1           | 320x256     | 114s     |
| m2       | 40       | Martin 2           | 320x256     | 58s      |
| s1       | 60       | Scottie 1          | 320x256     | 110s     |
| s2       | 56       | Scottie 2          | 320x256     | 71s      |
| sdx      | 76       | Scottie DX         | 320x256     | 269s     |
| r36      | 8        | Robot 36 Color     | 320x240     | 36s      |
| r72      | 12       | Robot 72 Color     | 320x240     | 72s      |

### MMSSTV Modes (Require Library)

#### Martin Family
| Protocol | VIS Code | Mode Name | Resolution | Duration |
|----------|----------|-----------|------------|----------|
| m3       | 104      | Martin 3  | 256x256    | 57s      |
| m4       | 108      | Martin 4  | 256x256    | 29s      |

#### PD Family
| Protocol | VIS Code | Mode Name | Resolution | Duration |
|----------|----------|-----------|------------|----------|
| pd50     | 93       | PD 50     | 320x256    | 50s      |
| pd90     | 99       | PD 90     | 320x256    | 90s      |
| pd120    | 95       | PD 120    | 640x496    | 120s     |
| pd160    | 98       | PD 160    | 512x400    | 160s     |
| pd180    | 96       | PD 180    | 640x496    | 180s     |
| pd240    | 97       | PD 240    | 640x496    | 240s     |
| pd290    | 94       | PD 290    | 800x616    | 290s     |

#### Additional Families
- **Wraase**: sc2_60, sc2_120
- **AVT**: avt24, avt90, avt94
- **Pasokon**: p3, p5, p7
- **FAX**: fax480
- **ML**: ml180, ml240, ml320

**Total**: 7 legacy + 25+ MMSSTV = **32+ modes** (with room for expansion to 100+)

---

## Interface Contracts

### MMSSTV Library Expected Interface

Based on typical SSTV library patterns, the mmsstv-portable library should provide:

```c
/* Initialization */
int mmsstv_init(void);
void mmsstv_cleanup(void);
const char* mmsstv_version(void);

/* Mode discovery */
int mmsstv_get_mode_params(mmsstv_mode_t mode, mmsstv_mode_params_t *params);
int mmsstv_list_modes(mmsstv_mode_t *modes, int max_modes);

/* Encoder lifecycle */
mmsstv_encoder_ctx_t* mmsstv_encoder_create(mmsstv_mode_t mode, uint32_t sample_rate);
void mmsstv_encoder_destroy(mmsstv_encoder_ctx_t *ctx);

/* Configuration */
int mmsstv_encoder_set_pixel_callback(mmsstv_encoder_ctx_t *ctx, 
                                      mmsstv_pixel_callback_t callback,
                                      void *user_data);
int mmsstv_encoder_set_audio_callback(mmsstv_encoder_ctx_t *ctx,
                                      mmsstv_audio_callback_t callback,
                                      void *user_data);
int mmsstv_encoder_set_vis_header(mmsstv_encoder_ctx_t *ctx, bool enable);

/* Encoding */
int mmsstv_encoder_encode_frame(mmsstv_encoder_ctx_t *ctx);
uint32_t mmsstv_encoder_get_sample_count(mmsstv_encoder_ctx_t *ctx);

/* Error handling */
const char* mmsstv_get_error(void);
void mmsstv_clear_error(void);
```

### Callbacks Required

**Pixel Access Callback:**
```c
int pixel_callback(int x, int y, uint8_t *r, uint8_t *g, uint8_t *b, void *user_data) {
    // PiSSTVpp2 implementation delegates to image module
    return image_get_pixel_rgb(x, y, r, g, b);
}
```

**Audio Output Callback:**
```c
int audio_callback(const uint16_t *samples, uint32_t count, void *user_data) {
    // PiSSTVpp2 implementation writes to audio buffer
    memcpy(&audio_buffer[offset], samples, count * sizeof(uint16_t));
    offset += count;
    return count;
}
```

---

## Integration Steps

### Phase 1: Stub Interface (COMPLETE ✓)

- [x] Create `mmsstv_stub.h` with expected interface
- [x] Create `pisstvpp2_mmsstv_adapter.h` with adapter interface
- [x] Create `pisstvpp2_mmsstv_adapter.c` with stub implementation
- [x] Document integration architecture

### Phase 2: MMSSTV Library Completion (EXTERNAL - In Progress)

External project: `/Users/ssamjung/Desktop/WIP/mmsstv-portable`

Required deliverables:
- [ ] Library implementation matching stub interface
- [ ] Header files (mmsstv.h)
- [ ] Compiled library (libmmsstv.a or libmmsstv.so)
- [ ] pkg-config support (mmsstv.pc)
- [ ] Documentation and examples

### Phase 3: Adapter Implementation (PENDING)

Once mmsstv-portable is complete:

- [ ] Replace stub headers with actual library headers
- [ ] Implement adapter encoding functions
- [ ] Test callback integration
- [ ] Validate all mode encodings
- [ ] Benchmark performance

### Phase 4: Main Application Integration (PENDING)

Modify PiSSTVpp2 main module:

- [ ] Replace direct SSTV module calls with adapter calls
- [ ] Extend CLI protocol options (add PD, AVT, etc.)
- [ ] Update help text with new modes
- [ ] Add mode listing command (`-l` or `--list-modes`)

### Phase 5: Build System (PENDING)

- [ ] Add MMSSTV detection to Makefile
- [ ] Create pkg-config integration
- [ ] Add feature flags
- [ ] Update documentation

### Phase 6: Testing (PENDING)

- [ ] Extend test suite for new modes
- [ ] Create mode validation tests
- [ ] Add performance benchmarks
- [ ] Test both legacy and MMSSTV backends

---

## Usage Examples

### With Legacy Modes (Current)

```bash
# Martin 1 (legacy backend)
./bin/pisstvpp2 -i test.png -p m1 -o output.wav

# Robot 72 (legacy backend)
./bin/pisstvpp2 -i test.png -p r72 -o output.wav
```

### With MMSSTV Modes (Future)

```bash
# PD 120 (MMSSTV backend)
./bin/pisstvpp2 -i test.png -p pd120 -o output.wav

# AVT 90 (MMSSTV backend)
./bin/pisstvpp2 -i test.png -p avt90 -o output.ogg

# List all available modes
./bin/pisstvpp2 --list-modes

# Show mode details
./bin/pisstvpp2 --mode-info pd120
```

---

## Error Handling

### Build-Time Errors

**Without MMSSTV support:**
```
$ ./bin/pisstvpp2 -i test.png -p pd120 -o output.wav
Error: Mode 'pd120' requires MMSSTV library support
       This binary was compiled without MMSSTV support.
       Available modes: m1, m2, s1, s2, sdx, r36, r72
```

**With MMSSTV support but library missing:**
```
$ ./bin/pisstvpp2 -i test.png -p pd120 -o output.wav
Error: Failed to initialize MMSSTV library
       libmmsstv.so not found or incompatible version
```

### Runtime Errors

The adapter provides detailed error messages:

```c
const char *error = mmsstv_adapter_get_error();
if (error) {
    fprintf(stderr, "MMSSTV Error: %s\n", error);
}
```

---

## Testing Strategy

### Unit Tests

Test adapter in isolation:
- Mode mapping validation
- Backend selection logic
- Error handling

### Integration Tests

Test with both backends:
- Legacy mode encoding (m1, s2, r72)
- MMSSTV mode encoding (pd120, avt90)
- Callback functionality
- Buffer management

### Regression Tests

Ensure backward compatibility:
- All existing tests pass
- Audio output identical for legacy modes
- Performance not degraded

### Extended Test Suite

```python
# tests/test_mmsstv_modes.py

MMSSTV_MODES = [
    'pd50', 'pd90', 'pd120', 'pd160', 'pd180', 'pd240',
    'avt24', 'avt90', 'avt94',
    'p3', 'p5', 'p7',
    'ml180', 'ml240', 'ml320'
]

for mode in MMSSTV_MODES:
    test_mode_encoding(mode)
    test_mode_with_cw(mode)
    test_mode_high_sample_rate(mode)
```

---

## Performance Considerations

### Memory Requirements

**Legacy modes**: 57.6M samples max @ 48kHz (600s buffer)

**MMSSTV modes**: May require larger buffers for high-resolution modes (ML, Pasokon)

**Solution**: Dynamic buffer allocation based on mode parameters

### CPU Usage

**Target**: <5% CPU on Raspberry Pi 3B+ during encoding

**Optimization opportunities**:
- Pre-compute mode parameters
- Optimize callbacks (minimize function call overhead)
- Use inline pixel access where possible

### Encoding Speed

**Benchmark targets**:
- Martin 1 (114s): Encode in <5s on Pi 3B+
- PD 120 (120s): Encode in <10s on Pi 3B+
- ML 320 (320s): Encode in <20s on Pi 3B+

---

## Documentation Requirements

### User Documentation

- [ ] Update README.md with extended mode list
- [ ] Create MMSSTV integration guide
- [ ] Add mode reference table
- [ ] Update build instructions

### Developer Documentation

- [ ] API reference for adapter layer
- [ ] Callback implementation guide
- [ ] Adding new modes guide
- [ ] Performance tuning guide

---

## Next Steps

1. **Wait for mmsstv-portable completion**
   - Monitor progress in `/Users/ssamjung/Desktop/WIP/mmsstv-portable`
   - Validate interface matches stub expectations

2. **Test stub interface**
   - Compile PiSSTVpp2 with stub headers
   - Verify no compilation errors
   - Check symbol visibility

3. **Prepare test infrastructure**
   - Create test images for new modes
   - Set up MMSSTV decoder validation
   - Prepare performance benchmarks

4. **Integration when ready**
   - Replace stubs with actual library
   - Implement adapter functions
   - Run full test suite
   - Document changes

---

## Contact and Coordination

**Integration Status**: Waiting for mmsstv-portable library completion

**Stub Interface**: Ready and documented in:
- `src/include/mmsstv_stub.h`
- `src/include/pisstvpp2_mmsstv_adapter.h`
- `src/pisstvpp2_mmsstv_adapter.c`

**Questions/Issues**: Check stub interface for expected API contract

---

*Document prepared: January 29, 2026*  
*Status: Ready for mmsstv-portable integration*
