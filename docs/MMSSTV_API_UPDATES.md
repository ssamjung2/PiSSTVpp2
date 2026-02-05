# MMSSTV Integration - Updated for mmsstv-portable API

**Date:** January 29, 2026  
**Status:** Stub interface updated to match actual mmsstv-portable design  

---

## API Changes Summary

The stub interface has been updated to match the actual mmsstv-portable library design based on the porting plan provided.

### Key Design Differences from Original Stub

#### 1. Streaming API (Not Callback-Based)

**Original stub** used callbacks:
```c
// OLD: Callback-based approach
typedef int (*mmsstv_pixel_callback_t)(...);
typedef int (*mmsstv_audio_callback_t)(...);
int mmsstv_encoder_set_pixel_callback(...);
int mmsstv_encoder_set_audio_callback(...);
```

**Actual mmsstv-portable** uses streaming:
```c
// NEW: Streaming approach
size_t sstv_encoder_generate(
    sstv_encoder_t *encoder,
    float *samples,          // Output buffer
    size_t max_samples
);
```

#### 2. Float Samples (Not uint16_t)

**Original stub**: uint16_t PCM samples  
**Actual mmsstv-portable**: float samples (-1.0 to +1.0 range)

**Implication**: Adapter must convert float → uint16_t for PiSSTVpp2's audio buffer

#### 3. Direct Image Structure (Not Callback)

**Original stub** used pixel callback for lazy image access:
```c
int get_pixel(int x, int y, uint8_t *r, uint8_t *g, uint8_t *b);
```

**Actual mmsstv-portable** uses direct pixel buffer:
```c
typedef struct {
    uint8_t *pixels;              // Direct access to pixel data
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    sstv_pixel_format_t format;
} sstv_image_t;
```

**Implication**: Adapter must convert PiSSTVpp2's `ImageBuffer` to `sstv_image_t`

#### 4. Simplified Mode Enumeration

**Original stub**: Complex VIS code-based enumeration with families  
**Actual mmsstv-portable**: Simple sequential enum (0-42) for 43 modes

```c
typedef enum {
    SSTV_ROBOT36 = 0,
    SSTV_ROBOT72,
    SSTV_SCOTTIE1,
    // ... 43 total modes
    SSTV_MODE_COUNT
} sstv_mode_t;
```

#### 5. Mode Information Structure

Simpler structure focusing on essentials:
```c
typedef struct {
    sstv_mode_t mode;
    const char *name;
    uint32_t width;
    uint32_t height;
    uint8_t vis_code;
    double duration_sec;
    int is_color;
} sstv_mode_info_t;
```

---

## Integration Flow (Updated)

### PiSSTVpp2 → mmsstv-portable Data Flow

```
1. Image Loading (PiSSTVpp2)
   ├─ Load image with libvips
   ├─ Apply aspect correction
   └─ Store in ImageBuffer (RGB, 3 bytes/pixel)

2. Format Conversion (Adapter)
   ├─ Create sstv_image_t from ImageBuffer
   ├─ Set pixels pointer
   ├─ Set dimensions and stride
   └─ Format = SSTV_RGB24

3. Encoder Creation (mmsstv-portable)
   ├─ sstv_encoder_create(mode, sample_rate)
   ├─ sstv_encoder_set_image(encoder, &image)
   └─ sstv_encoder_set_vis_enabled(encoder, 1)

4. Audio Generation (Streaming Loop)
   ├─ Allocate float buffer (e.g., 4096 samples)
   ├─ while (!sstv_encoder_is_complete(encoder))
   │   ├─ count = sstv_encoder_generate(encoder, floatbuf, 4096)
   │   ├─ Convert float (-1.0 to +1.0) → uint16_t (0-65535)
   │   └─ Append to PiSSTVpp2 audio buffer
   └─ encoding complete

5. Audio Output (PiSSTVpp2)
   └─ Write audio buffer to WAV/AIFF/OGG
```

---

## Adapter Implementation Requirements

### 1. Image Conversion

```c
// Convert PiSSTVpp2 ImageBuffer to sstv_image_t
sstv_image_t mmsstv_adapter_create_image(void) {
    sstv_image_t image;
    
    // Get image buffer from PiSSTVpp2 image module
    image.pixels = image_get_buffer();
    image.width = image_get_width();
    image.height = image_get_height();
    image.stride = image.width * 3;  // RGB = 3 bytes/pixel
    image.format = SSTV_RGB24;
    
    return image;
}
```

### 2. Sample Format Conversion

```c
// Convert float samples to uint16_t PCM
void convert_float_to_pcm(
    const float *float_samples,
    uint16_t *pcm_samples,
    size_t count
) {
    for (size_t i = 0; i < count; i++) {
        // Clamp to [-1.0, +1.0] range
        float sample = float_samples[i];
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        
        // Convert to uint16_t: [-1.0, +1.0] → [0, 65535]
        pcm_samples[i] = (uint16_t)((sample + 1.0f) * 32767.5f);
    }
}
```

### 3. Streaming Encoder Loop

```c
int mmsstv_adapter_encode_frame(
    mmsstv_adapter_ctx_t *ctx,
    uint16_t *audio_buffer,
    uint32_t max_samples,
    uint32_t *samples_written
) {
    // Create image structure
    sstv_image_t image = mmsstv_adapter_create_image();
    
    // Set image in encoder
    if (sstv_encoder_set_image(ctx->sstv_encoder, &image) != 0) {
        return -1;
    }
    
    // Streaming generation loop
    float float_buf[4096];
    uint32_t offset = 0;
    
    while (!sstv_encoder_is_complete(ctx->sstv_encoder)) {
        // Generate float samples
        size_t generated = sstv_encoder_generate(
            ctx->sstv_encoder,
            float_buf,
            4096
        );
        
        if (generated == 0) break;
        
        // Check buffer space
        if (offset + generated > max_samples) {
            set_error("Audio buffer overflow");
            return -1;
        }
        
        // Convert float to PCM and append
        convert_float_to_pcm(
            float_buf,
            &audio_buffer[offset],
            generated
        );
        
        offset += generated;
        
        // Progress update
        if (ctx->verbose) {
            float progress = sstv_encoder_get_progress(ctx->sstv_encoder);
            LOG_PROGRESS("Encoding: %.1f%%", progress * 100.0f);
        }
    }
    
    *samples_written = offset;
    return 0;
}
```

---

## Mode Mapping Updates

The adapter's mode table must map PiSSTVpp2 protocol strings to mmsstv-portable's enum:

```c
static const mmsstv_mode_map_t g_mode_table[] = {
    /* Legacy modes */
    { "m1",     44, "Martin 1",    true,  SSTV_MARTIN1 },
    { "m2",     40, "Martin 2",    true,  SSTV_MARTIN2 },
    { "s1",     60, "Scottie 1",   true,  SSTV_SCOTTIE1 },
    { "s2",     56, "Scottie 2",   true,  SSTV_SCOTTIE2 },
    { "sdx",    76, "Scottie DX",  true,  SSTV_SCOTTIEX },
    { "r36",    8,  "Robot 36",    true,  SSTV_ROBOT36 },
    { "r72",    12, "Robot 72",    true,  SSTV_ROBOT72 },
    
    /* Extended MMSSTV modes */
    { "pd120",  95,  "PD 120",     false, SSTV_PD120 },
    { "pd180",  96,  "PD 180",     false, SSTV_PD180 },
    { "pd240",  97,  "PD 240",     false, SSTV_PD240 },
    // ... etc for all 43 modes
};
```

---

## Sample Usage (Updated)

### Encoding with mmsstv-portable

```c
// PiSSTVpp2 integration example
#include "sstv_encoder.h"
#include "pisstvpp2_image.h"

int encode_with_mmsstv(const char *protocol, uint32_t sample_rate) {
    // Map protocol to mode
    sstv_mode_t mode = map_protocol_to_mode(protocol);
    
    // Create encoder
    sstv_encoder_t *encoder = sstv_encoder_create(mode, (double)sample_rate);
    if (!encoder) {
        return -1;
    }
    
    // Get image from PiSSTVpp2 image module
    sstv_image_t image;
    image.pixels = image_get_buffer();
    image.width = image_get_width();
    image.height = image_get_height();
    image.stride = image.width * 3;
    image.format = SSTV_RGB24;
    
    // Set image and enable VIS
    sstv_encoder_set_image(encoder, &image);
    sstv_encoder_set_vis_enabled(encoder, 1);
    
    // Pre-allocate audio buffer
    size_t total_samples = sstv_encoder_get_total_samples(encoder);
    uint16_t *audio_buffer = malloc(total_samples * sizeof(uint16_t));
    
    // Generate audio (streaming)
    float float_samples[4096];
    size_t offset = 0;
    
    while (!sstv_encoder_is_complete(encoder)) {
        size_t count = sstv_encoder_generate(encoder, float_samples, 4096);
        
        // Convert and append
        for (size_t i = 0; i < count; i++) {
            float sample = float_samples[i];
            audio_buffer[offset++] = (uint16_t)((sample + 1.0f) * 32767.5f);
        }
    }
    
    // Clean up
    sstv_encoder_free(encoder);
    
    // Audio buffer now contains samples - pass to audio encoder module
    return 0;
}
```

---

## Testing Strategy (Updated)

### 1. Image Conversion Test

```c
void test_image_conversion(void) {
    // Load test image with PiSSTVpp2
    image_load("test.png");
    
    // Convert to sstv_image_t
    sstv_image_t image = mmsstv_adapter_create_image();
    
    // Verify dimensions match
    assert(image.width == 320);
    assert(image.height == 256);
    assert(image.stride == 960);  // 320 * 3
    assert(image.format == SSTV_RGB24);
    
    // Verify pixel access
    uint8_t r = image.pixels[0];
    uint8_t g = image.pixels[1];
    uint8_t b = image.pixels[2];
    // Should match image_get_pixel_rgb(0, 0, ...)
}
```

### 2. Sample Conversion Test

```c
void test_sample_conversion(void) {
    float floats[] = {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f};
    uint16_t pcm[5];
    
    convert_float_to_pcm(floats, pcm, 5);
    
    assert(pcm[0] == 0);       // -1.0 → 0
    assert(pcm[1] == 16384);   // -0.5 → 16384
    assert(pcm[2] == 32768);   // 0.0 → 32768
    assert(pcm[3] == 49151);   // 0.5 → 49151
    assert(pcm[4] == 65535);   // 1.0 → 65535
}
```

### 3. Integration Test

```c
void test_full_encoding(void) {
    // Load color bars test pattern
    image_load("colorbars_320x256.png");
    
    // Encode with Scottie 1
    uint16_t *audio_buffer = malloc(SSTV_MAX_SAMPLES * sizeof(uint16_t));
    uint32_t samples_written;
    
    int result = mmsstv_adapter_encode_frame(
        adapter_ctx,
        audio_buffer,
        SSTV_MAX_SAMPLES,
        &samples_written
    );
    
    assert(result == 0);
    assert(samples_written > 0);
    
    // Verify duration matches expected (Scottie 1 = ~110s at 48kHz = ~5.28M samples)
    double duration_sec = (double)samples_written / 48000.0;
    assert(fabs(duration_sec - 110.0) < 1.0);  // Within 1 second
    
    free(audio_buffer);
}
```

---

## Build Integration (Updated)

### Makefile Changes

```makefile
# MMSSTV library support
ifdef HAVE_MMSSTV
    # Use actual library headers
    CFLAGS += -DHAVE_MMSSTV_SUPPORT
    
    # Link against mmsstv-portable
    LDFLAGS += -lsstv_encoder
    
    # Use pkg-config if available
    MMSSTV_CFLAGS := $(shell pkg-config --cflags sstv_encoder 2>/dev/null || echo "")
    MMSSTV_LIBS := $(shell pkg-config --libs sstv_encoder 2>/dev/null || echo "-lsstv_encoder")
    
    CFLAGS += $(MMSSTV_CFLAGS)
    LDFLAGS += $(MMSSTV_LIBS)
endif
```

---

## Summary of Changes

### Stub Interface (mmsstv_stub.h)
- ✅ Changed from callback-based to streaming API
- ✅ Updated to use `float` samples instead of `uint16_t`
- ✅ Added `sstv_image_t` structure for direct pixel access
- ✅ Simplified mode enumeration (43 modes, 0-42)
- ✅ Matches actual mmsstv-portable `sstv_encoder.h` design

### Adapter Layer (Required Updates)
- ⏸️ Add image conversion: `ImageBuffer` → `sstv_image_t`
- ⏸️ Add sample conversion: `float` → `uint16_t`
- ⏸️ Implement streaming loop for audio generation
- ⏸️ Update mode mapping table for 43 modes

### Integration Benefits
- **Simpler API**: Direct buffer access, no callbacks
- **Better performance**: Streaming generation, predictable memory
- **Easier debugging**: Clear data flow, no callback complexity
- **Standard format**: Float samples are industry standard

---

## Next Steps

1. **Wait for mmsstv-portable completion** (in progress)
2. **Test compilation** with updated stub headers
3. **Implement adapter updates** when library is ready:
   - Image conversion helper
   - Sample conversion helper
   - Streaming generation loop
4. **Integration testing** with actual library
5. **Update documentation** with real examples

---

*Updated: January 29, 2026*  
*Status: Stub interface aligned with mmsstv-portable design*  
*Ready for: Library completion and integration*
