# PiSSTVpp2 Architecture Documentation

**Internal design and structure of PiSSTVpp2 v2.0**

---

## Table of Contents

1. [Overview](#overview)
2. [System Architecture](#system-architecture)
3. [Module Descriptions](#module-descriptions)
4. [Data Flow](#data-flow)
5. [Key Algorithms](#key-algorithms)
6. [Dependencies](#dependencies)
7. [Build System](#build-system)
8. [Extension Points](#extension-points)
9. [Testing Framework](#testing-framework)

---

## Overview

### Design Philosophy

PiSSTVpp2 v2.0 follows these principles:

**Modularity**: Clear separation of concerns (image, SSTV, audio)

**Simplicity**: Straightforward data flow with minimal abstraction

**Portability**: Standard C11, minimal dependencies

**Testability**: Modular design enables comprehensive testing

**Maintainability**: Clean code structure, consistent patterns

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    pisstvpp2 (main)                     │
│  - Argument parsing                                     │
│  - Module coordination                                  │
│  - CW identification                                    │
└───────┬─────────────────────────────┬───────────────────┘
        │                             │
        ▼                             ▼
┌──────────────────┐         ┌────────────────────┐
│  Image Module    │         │   SSTV Module      │
│  (pisstvpp2_     │────────▶│   (pisstvpp2_      │
│   image.c/.h)    │         │    sstv.c/.h)      │
│                  │         │                    │
│ - Load image     │         │ - VIS generation   │
│ - Aspect corr.   │         │ - Scan encoding    │
│ - Scaling        │         │ - Sample output    │
└──────────────────┘         └──────────┬─────────┘
                                        │
                                        ▼
                             ┌────────────────────┐
                             │  Audio Encoder     │
                             │  (pisstvpp2_audio_ │
                             │   encoder.c/.h)    │
                             │                    │
                             │ - Format handling  │
                             │ - File writing     │
                             └──────────┬─────────┘
                                        │
                    ┌──────────┬────────┼────────┬──────────┐
                    ▼          ▼        ▼        ▼          ▼
              ┌─────────┐ ┌──────┐ ┌─────┐  [Other formats]
              │ WAV     │ │ AIFF │ │ OGG │
              │ Encoder │ │ Enc. │ │ Enc.│
              └─────────┘ └──────┘ └─────┘
```

---

## System Architecture

### Core Components

#### 1. Main Program (`pisstvpp2.c`)

**Responsibilities:**
- Parse command-line arguments
- Coordinate module execution
- Generate CW identification (optional)
- Error handling and reporting

**Key Functions:**
```c
int main(int argc, char *argv[])
void parse_arguments(int argc, char *argv[], config_t *config)
void generate_cw_id(audio_encoder_t *encoder, config_t *config)
```

**Data Structures:**
```c
typedef struct {
    char *input_file;
    char *output_file;
    char *protocol;
    char *format;
    int sample_rate;
    char *aspect_mode;
    char *cw_callsign;
    int cw_wpm;
    int cw_tone;
    bool no_vis;
    bool verbose;
} config_t;
```

#### 2. Image Processing Module (`pisstvpp2_image.c/.h`)

**Responsibilities:**
- Load images via libvips
- Color space conversion (to RGB)
- Aspect ratio correction
- Resize to target resolution

**Key Functions:**
```c
int load_and_prepare_image(
    const char *filename,
    int target_width,
    int target_height,
    const char *aspect_mode,
    VipsImage **out
);

void apply_aspect_correction(
    VipsImage *input,
    int target_width,
    int target_height,
    const char *mode,
    VipsImage **out
);
```

**Supported Aspect Modes:**
- `center`: Center-crop to target aspect
- `pad`: Add black bars (letterbox/pillarbox)
- `stretch`: Force resize (may distort)

#### 3. SSTV Encoding Module (`pisstvpp2_sstv.c/.h`)

**Responsibilities:**
- SSTV mode configuration
- VIS header generation
- Line-by-line image encoding
- Audio sample generation

**Key Functions:**
```c
typedef struct {
    char *name;
    int vis_code;
    int width;
    int height;
    double sync_pulse_duration;
    double porch_duration;
    double scan_pixel_duration;
    // ... mode-specific timings
} sstv_mode_t;

void generate_vis_header(
    sstv_encoder_t *encoder,
    int vis_code,
    int sample_rate
);

void encode_sstv_image(
    VipsImage *image,
    sstv_mode_t *mode,
    int sample_rate,
    audio_encoder_t *audio_encoder
);
```

**Mode Definitions:**
```c
static const sstv_mode_t sstv_modes[] = {
    // Martin modes
    { "m1", 44, 320, 256, /* ... timings ... */ },
    { "m2", 40, 320, 256, /* ... timings ... */ },
    
    // Scottie modes
    { "s1", 60, 320, 256, /* ... timings ... */ },
    { "s2", 56, 320, 256, /* ... timings ... */ },
    { "sdx", 76, 320, 256, /* ... timings ... */ },
    
    // Robot modes
    { "r36", 8, 320, 240, /* ... timings ... */ },
    { "r72", 12, 320, 240, /* ... timings ... */ },
};
```

#### 4. Audio Encoder Module (`pisstvpp2_audio_encoder.c/.h`)

**Responsibilities:**
- Abstract audio format handling
- Coordinate format-specific encoders
- Sample buffer management
- File I/O

**Key Functions:**
```c
typedef struct audio_encoder {
    char *filename;
    char *format;
    int sample_rate;
    void *encoder_state;
    
    int (*write_samples)(struct audio_encoder *, int16_t *, size_t);
    int (*close)(struct audio_encoder *);
} audio_encoder_t;

audio_encoder_t* audio_encoder_create(
    const char *filename,
    const char *format,
    int sample_rate
);

int audio_encoder_write_samples(
    audio_encoder_t *encoder,
    int16_t *samples,
    size_t count
);

void audio_encoder_destroy(audio_encoder_t *encoder);
```

#### 5. Format-Specific Encoders

**WAV Encoder** (`audio_encoder_wav.c`):
```c
typedef struct {
    FILE *fp;
    size_t samples_written;
    int sample_rate;
} wav_encoder_t;

int wav_encoder_init(audio_encoder_t *encoder);
int wav_encoder_write(audio_encoder_t *encoder, int16_t *samples, size_t count);
int wav_encoder_close(audio_encoder_t *encoder);
```

**AIFF Encoder** (`audio_encoder_aiff.c`):
```c
typedef struct {
    FILE *fp;
    size_t samples_written;
    int sample_rate;
} aiff_encoder_t;

int aiff_encoder_init(audio_encoder_t *encoder);
int aiff_encoder_write(audio_encoder_t *encoder, int16_t *samples, size_t count);
int aiff_encoder_close(audio_encoder_t *encoder);
```

**OGG Encoder** (`audio_encoder_ogg.c`):
```c
typedef struct {
    FILE *fp;
    ogg_stream_state os;
    vorbis_info vi;
    vorbis_comment vc;
    vorbis_dsp_state vd;
    vorbis_block vb;
    size_t samples_written;
} ogg_encoder_t;

int ogg_encoder_init(audio_encoder_t *encoder);
int ogg_encoder_write(audio_encoder_t *encoder, int16_t *samples, size_t count);
int ogg_encoder_close(audio_encoder_t *encoder);
```

---

## Module Descriptions

### pisstvpp2_image Module

**Purpose:** Handle all image processing operations

**Dependencies:**
- libvips (image I/O and processing)
- Standard C library

**Operations Flow:**

1. **Load Image**
   ```c
   VipsImage *image;
   vips_image_new_from_file(filename, &image)
   ```

2. **Convert Color Space**
   ```c
   if (image->bands == 1) {
       // Grayscale to RGB
       vips_bandjoin_const(image, &rgb, 3, ...)
   }
   ```

3. **Apply Aspect Correction**
   - **Center Mode**: `vips_crop()` from center
   - **Pad Mode**: `vips_embed()` with black background
   - **Stretch Mode**: Direct resize (no intermediate step)

4. **Resize to Target**
   ```c
   vips_resize(image, &output, 
               (double)target_width / image->Xsize)
   ```

5. **Return VipsImage**
   - Caller extracts pixel data
   - Line-by-line access for SSTV encoding

**Error Handling:**
- Validates file existence
- Checks image dimensions
- Verifies color space support
- Reports libvips errors

### pisstvpp2_sstv Module

**Purpose:** Encode SSTV transmissions

**Key Concepts:**

1. **Mode Tables**
   - Static configuration for each SSTV mode
   - Timing parameters (sync, porch, scan)
   - Resolution (width × height)
   - VIS code

2. **VIS Header Generation**
   ```
   Leader Tone: 1900 Hz, 300ms
   Break: 1200 Hz, 10ms
   Start Bit: 1900 Hz, 30ms
   Data Bits (8): 1100/1300 Hz (0/1)
   Parity Bit: Even parity
   Stop Bit: 1200 Hz, 30ms
   ```

3. **Line Encoding**
   - For each scan line:
     - Generate sync pulse
     - Generate porch (if mode requires)
     - For each pixel:
       - Map pixel value to frequency (1500-2300 Hz)
       - Generate samples for pixel duration

4. **Sample Generation**
   ```c
   // Direct Digital Synthesis
   double phase = 0.0;
   for (int i = 0; i < num_samples; i++) {
       sample[i] = (int16_t)(32767.0 * sin(phase));
       phase += 2.0 * M_PI * frequency / sample_rate;
   }
   ```

**Encoding Patterns:**

**Martin/Scottie (RGB):**
```
For each line:
    Sync pulse
    Porch (if applicable)
    Green scan
    Blue scan  
    Red scan
```

**Robot (YCrCb):**
```
For each line:
    Sync pulse
    Porch
    Y (luminance) scan
    Cr (red chroma) scan
    Cb (blue chroma) scan
```

### pisstvpp2_audio_encoder Module

**Purpose:** Abstract audio format differences

**Design Pattern:** Strategy pattern with function pointers

**Initialization:**
```c
audio_encoder_t *encoder = audio_encoder_create(
    "output.wav", "wav", 22050
);
// Internally dispatches to wav_encoder_init()
```

**Writing Samples:**
```c
int16_t samples[1024];
// ... fill samples ...
audio_encoder_write_samples(encoder, samples, 1024);
// Calls encoder->write_samples() function pointer
```

**Cleanup:**
```c
audio_encoder_destroy(encoder);
// Calls encoder->close(), updates headers, frees resources
```

**Format Detection:**
```c
// Auto-detect from filename extension
if (ends_with(filename, ".wav")) format = "wav";
else if (ends_with(filename, ".aiff")) format = "aiff";
else if (ends_with(filename, ".ogg")) format = "ogg";

// Or explicit via -f flag
```

---

## Data Flow

### Complete Encoding Pipeline

```
┌──────────────┐
│ Command Line │
│  Arguments   │
└──────┬───────┘
       │
       ▼
┌─────────────────────────────────────────┐
│ Parse & Validate Arguments              │
│  - Input file path                      │
│  - Output file path                     │
│  - SSTV mode                            │
│  - Audio format                         │
│  - Sample rate                          │
│  - Options (aspect, CW, VIS, verbose)   │
└──────┬──────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────────┐
│ Load & Prepare Image                    │
│  1. Load via libvips                    │
│  2. Convert to RGB                      │
│  3. Apply aspect correction             │
│  4. Resize to mode resolution           │
│  Output: VipsImage (320×256 or 320×240) │
└──────┬──────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────────┐
│ Initialize Audio Encoder                │
│  - Create encoder for format            │
│  - Open output file                     │
│  - Write format headers                 │
│  - Prepare for sample writing           │
└──────┬──────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────────┐
│ Generate VIS Header (if enabled)        │
│  1. Leader tone (1900 Hz, 300ms)        │
│  2. VIS code bits                       │
│  3. Write samples to audio encoder      │
└──────┬──────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────────┐
│ Encode SSTV Image                       │
│  For each scan line (0 to height-1):    │
│    1. Generate sync pulse               │
│    2. Generate porch (if mode requires) │
│    3. For each pixel (0 to width-1):    │
│       a. Get pixel RGB value            │
│       b. Map to frequency               │
│       c. Generate tone samples          │
│       d. Write to audio encoder         │
│  Output: Complete SSTV signal           │
└──────┬──────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────────┐
│ Generate CW ID (if enabled)             │
│  1. Parse callsign to Morse code        │
│  2. Generate dit/dah patterns           │
│  3. Apply envelope shaping              │
│  4. Write samples to audio encoder      │
└──────┬──────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────────┐
│ Finalize Audio File                     │
│  1. Close audio encoder                 │
│  2. Update file headers (size, etc.)    │
│  3. Close file handle                   │
│  Output: Complete audio file            │
└──────┬──────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────────┐
│ Cleanup & Report                        │
│  - Free VipsImage                       │
│  - Free encoder resources               │
│  - Print summary (if verbose)           │
│  - Exit                                 │
└─────────────────────────────────────────┘
```

### Data Structures Flow

```
config_t (arguments)
    │
    ▼
VipsImage (loaded image)
    │
    ▼
sstv_mode_t (mode configuration)
    │
    ▼
audio_encoder_t (audio output)
    │
    ▼
int16_t samples[] (audio data)
    │
    ▼
Output File
```

---

## Key Algorithms

### 1. Aspect Ratio Correction

**Center Mode:**
```c
double src_aspect = (double)width / height;
double tgt_aspect = (double)target_width / target_height;

if (src_aspect > tgt_aspect) {
    // Image wider than target, crop horizontally
    int crop_width = (int)(height * tgt_aspect);
    int offset_x = (width - crop_width) / 2;
    vips_crop(input, &cropped, offset_x, 0, crop_width, height);
} else {
    // Image taller than target, crop vertically
    int crop_height = (int)(width / tgt_aspect);
    int offset_y = (height - crop_height) / 2;
    vips_crop(input, &cropped, 0, offset_y, width, crop_height);
}
```

**Pad Mode:**
```c
double src_aspect = (double)width / height;
double tgt_aspect = (double)target_width / target_height;

if (src_aspect < tgt_aspect) {
    // Need horizontal padding
    int pad_width = (int)(height * tgt_aspect);
    int pad_left = (pad_width - width) / 2;
    vips_embed(input, &padded, pad_left, 0, pad_width, height,
               "background", {0, 0, 0});
} else {
    // Need vertical padding
    int pad_height = (int)(width / tgt_aspect);
    int pad_top = (pad_height - height) / 2;
    vips_embed(input, &padded, 0, pad_top, width, pad_height,
               "background", {0, 0, 0});
}
```

### 2. VIS Code Generation

```c
void generate_vis_code(int vis_code, int sample_rate, audio_encoder_t *encoder)
{
    // 1. Leader tone: 1900 Hz, 300ms
    generate_tone(1900, 0.3, sample_rate, encoder);
    
    // 2. Break: 1200 Hz, 10ms
    generate_tone(1200, 0.01, sample_rate, encoder);
    
    // 3. Start bit: 1900 Hz, 30ms
    generate_tone(1900, 0.03, sample_rate, encoder);
    
    // 4. Data bits (LSB first)
    int parity = 0;
    for (int i = 0; i < 8; i++) {
        int bit = (vis_code >> i) & 1;
        int freq = bit ? 1300 : 1100;
        generate_tone(freq, 0.03, sample_rate, encoder);
        parity ^= bit;
    }
    
    // 5. Parity bit (even parity)
    int parity_freq = parity ? 1100 : 1300;
    generate_tone(parity_freq, 0.03, sample_rate, encoder);
    
    // 6. Stop bit: 1200 Hz, 30ms
    generate_tone(1200, 0.03, sample_rate, encoder);
}
```

### 3. Frequency-to-Sample Conversion

```c
void generate_tone(double freq, double duration, int sample_rate,
                   audio_encoder_t *encoder)
{
    int num_samples = (int)(duration * sample_rate);
    int16_t *samples = malloc(num_samples * sizeof(int16_t));
    
    double phase = 0.0;
    double phase_increment = 2.0 * M_PI * freq / sample_rate;
    
    for (int i = 0; i < num_samples; i++) {
        samples[i] = (int16_t)(32767.0 * sin(phase));
        phase += phase_increment;
        
        // Keep phase wrapped to avoid precision loss
        if (phase >= 2.0 * M_PI) {
            phase -= 2.0 * M_PI;
        }
    }
    
    audio_encoder_write_samples(encoder, samples, num_samples);
    free(samples);
}
```

### 4. Pixel-to-Frequency Mapping

```c
// Map pixel value (0-255) to frequency (1500-2300 Hz)
double pixel_to_frequency(uint8_t pixel_value)
{
    const double BLACK_FREQ = 1500.0;
    const double WHITE_FREQ = 2300.0;
    const double FREQ_RANGE = WHITE_FREQ - BLACK_FREQ;
    
    return BLACK_FREQ + (pixel_value / 255.0) * FREQ_RANGE;
}
```

### 5. CW Envelope Shaping

```c
// Apply raised-cosine envelope to prevent clicks
void apply_envelope(int16_t *samples, int count, 
                    int attack_samples, int release_samples)
{
    // Attack (rise)
    for (int i = 0; i < attack_samples && i < count; i++) {
        double factor = 0.5 * (1.0 - cos(M_PI * i / attack_samples));
        samples[i] = (int16_t)(samples[i] * factor);
    }
    
    // Release (fall)
    int release_start = count - release_samples;
    for (int i = 0; i < release_samples && release_start >= 0; i++) {
        double factor = 0.5 * (1.0 - cos(M_PI * (release_samples - i) / release_samples));
        samples[release_start + i] = (int16_t)(samples[release_start + i] * factor);
    }
}
```

---

## Dependencies

### External Libraries

#### libvips

**Version:** 8.0+

**Purpose:** Image loading, processing, and color space conversion

**API Usage:**
```c
#include <vips/vips.h>

// Initialize
vips_init(argv[0]);

// Load image
VipsImage *image;
vips_image_new_from_file("input.jpg", &image, NULL);

// Process
vips_resize(image, &resized, scale, NULL);
vips_crop(image, &cropped, x, y, width, height, NULL);

// Access pixels
VipsRegion *region = vips_region_new(image);
vips_region_prepare(region, &rect);
uint8_t *pixels = VIPS_REGION_ADDR(region, x, y);

// Cleanup
g_object_unref(image);
vips_shutdown();
```

**Why libvips?**
- Fast (parallelized operations)
- Memory efficient (streaming processing)
- Wide format support (JPEG, PNG, GIF, TIFF, WebP, etc.)
- Simple API

**Migration from libgd:**
- v1.x used libgd (simpler but less capable)
- v2.0 uses libvips (faster, more formats, better quality)

#### libogg + libvorbis (Optional)

**Version:** Any recent version

**Purpose:** OGG Vorbis audio encoding

**API Usage:**
```c
#include <vorbis/vorbisenc.h>

// Initialize
vorbis_info_init(&vi);
vorbis_encode_init_vbr(&vi, 1, sample_rate, 0.4);

// Encode
float **buffer = vorbis_analysis_buffer(&vd, samples);
// Fill buffer with float samples
vorbis_analysis_wrote(&vd, samples);

// Get packets
while (vorbis_analysis_blockout(&vd, &vb) == 1) {
    vorbis_analysis(&vb, NULL);
    vorbis_bitrate_addblock(&vb);
    
    ogg_packet op;
    while (vorbis_bitrate_flushpacket(&vd, &op)) {
        ogg_stream_packetin(&os, &op);
        // ... write to file ...
    }
}

// Cleanup
vorbis_block_clear(&vb);
vorbis_dsp_clear(&vd);
vorbis_info_clear(&vi);
```

**Compilation:**
- If libraries present: OGG support enabled
- If libraries missing: OGG support disabled, WAV/AIFF only

### Standard C Library

**Used APIs:**
- `stdio.h`: File I/O
- `stdlib.h`: Memory allocation, string conversion
- `string.h`: String manipulation
- `math.h`: sin(), cos(), M_PI
- `stdint.h`: Fixed-width integers (int16_t, uint8_t)
- `stdbool.h`: Boolean type
- `getopt.h`: Argument parsing

---

## Build System

### Makefile Structure

```makefile
# Compiler and flags
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
LDFLAGS = 

# Dependencies
VIPS_CFLAGS = $(shell pkg-config --cflags vips)
VIPS_LIBS = $(shell pkg-config --libs vips)

# Check for OGG support
OGG_AVAILABLE = $(shell pkg-config --exists vorbisenc && echo yes)
ifeq ($(OGG_AVAILABLE),yes)
    OGG_CFLAGS = $(shell pkg-config --cflags vorbisenc)
    OGG_LIBS = $(shell pkg-config --libs vorbisenc vorbis ogg)
    CFLAGS += -DHAVE_OGG $(OGG_CFLAGS)
    LDFLAGS += $(OGG_LIBS)
endif

# Source files
SRCS = src/pisstvpp2.c \
       src/pisstvpp2_image.c \
       src/pisstvpp2_sstv.c \
       src/pisstvpp2_audio_encoder.c \
       src/audio_encoder_wav.c \
       src/audio_encoder_aiff.c

# Add OGG encoder if available
ifeq ($(OGG_AVAILABLE),yes)
    SRCS += src/audio_encoder_ogg.c
endif

# Object files
OBJS = $(SRCS:.c=.o)

# Target binary
TARGET = bin/pisstvpp2

# Build rules
all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p bin
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS) $(VIPS_LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(VIPS_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
```

### Build Targets

**all** (default):
- Compiles all source files
- Links binary to `bin/pisstvpp2`

**clean**:
- Removes object files
- Removes binary

**install** (optional, user-added):
```makefile
install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/
```

### Conditional Compilation

**OGG Support:**
```c
#ifdef HAVE_OGG
    // OGG encoder code
    #include "audio_encoder_ogg.h"
#else
    // Stub or error message
    fprintf(stderr, "OGG support not compiled\n");
#endif
```

**Detection at Build Time:**
```bash
# Check for vorbisenc pkg-config
pkg-config --exists vorbisenc
if [ $? -eq 0 ]; then
    # Define HAVE_OGG
    CFLAGS="$CFLAGS -DHAVE_OGG"
fi
```

---

## Extension Points

### Adding New SSTV Modes

**Step 1: Define Mode in `pisstvpp2_sstv.c`**

```c
static const sstv_mode_t sstv_modes[] = {
    // ... existing modes ...
    
    // New mode
    {
        .name = "new_mode",
        .vis_code = 99,  // Assign VIS code
        .width = 320,
        .height = 256,
        .sync_pulse_duration = 0.009,    // 9ms
        .porch_duration = 0.0015,         // 1.5ms
        .scan_pixel_duration = 0.000457,  // Per pixel timing
        // ... other timing parameters ...
    }
};
```

**Step 2: Update Mode Lookup**

```c
sstv_mode_t* get_mode_by_name(const char *name)
{
    for (size_t i = 0; i < sizeof(sstv_modes) / sizeof(sstv_modes[0]); i++) {
        if (strcmp(sstv_modes[i].name, name) == 0) {
            return &sstv_modes[i];
        }
    }
    return NULL;
}
```

**Step 3: Update Help Text**

```c
void print_help()
{
    printf("Available modes:\n");
    printf("  m1, m2    - Martin 1, Martin 2\n");
    printf("  s1, s2, sdx - Scottie modes\n");
    printf("  r36, r72  - Robot modes\n");
    printf("  new_mode  - New mode description\n");  // Add this
}
```

**Step 4: Add Tests**

```bash
# tests/test_suite.sh
test_new_mode() {
    ./bin/pisstvpp2 -i tests/images/test.jpg -o /tmp/new_mode.wav -p new_mode
    check_success "New mode encoding"
}
```

### Adding New Audio Formats

**Step 1: Create Format Encoder**

```c
// src/audio_encoder_newformat.c

typedef struct {
    FILE *fp;
    // Format-specific state
} newformat_encoder_t;

int newformat_encoder_init(audio_encoder_t *encoder)
{
    newformat_encoder_t *nf = calloc(1, sizeof(newformat_encoder_t));
    encoder->encoder_state = nf;
    
    nf->fp = fopen(encoder->filename, "wb");
    // Write format headers...
    
    return 0;
}

int newformat_encoder_write(audio_encoder_t *encoder,
                             int16_t *samples, size_t count)
{
    newformat_encoder_t *nf = encoder->encoder_state;
    // Convert and write samples...
    fwrite(samples, sizeof(int16_t), count, nf->fp);
    return 0;
}

int newformat_encoder_close(audio_encoder_t *encoder)
{
    newformat_encoder_t *nf = encoder->encoder_state;
    // Finalize format (update headers, etc.)
    fclose(nf->fp);
    free(nf);
    return 0;
}
```

**Step 2: Register Format in `pisstvpp2_audio_encoder.c`**

```c
audio_encoder_t* audio_encoder_create(const char *filename,
                                      const char *format,
                                      int sample_rate)
{
    audio_encoder_t *encoder = calloc(1, sizeof(audio_encoder_t));
    encoder->filename = strdup(filename);
    encoder->format = strdup(format);
    encoder->sample_rate = sample_rate;
    
    if (strcmp(format, "wav") == 0) {
        wav_encoder_init(encoder);
    } else if (strcmp(format, "aiff") == 0) {
        aiff_encoder_init(encoder);
    } else if (strcmp(format, "ogg") == 0) {
        ogg_encoder_init(encoder);
    } else if (strcmp(format, "newformat") == 0) {  // Add this
        newformat_encoder_init(encoder);
    } else {
        fprintf(stderr, "Unknown format: %s\n", format);
        free(encoder);
        return NULL;
    }
    
    return encoder;
}
```

**Step 3: Update Makefile**

```makefile
SRCS += src/audio_encoder_newformat.c
```

**Step 4: Update Help Text**

```c
printf("  -f, --format FORMAT   Audio format (wav, aiff, ogg, newformat)\n");
```

### Adding New Aspect Correction Modes

**In `pisstvpp2_image.c`:**

```c
void apply_aspect_correction(VipsImage *input, 
                             int target_width, int target_height,
                             const char *mode,
                             VipsImage **out)
{
    if (strcmp(mode, "center") == 0) {
        // ... existing center code ...
    } else if (strcmp(mode, "pad") == 0) {
        // ... existing pad code ...
    } else if (strcmp(mode, "stretch") == 0) {
        // ... existing stretch code ...
    } else if (strcmp(mode, "newmode") == 0) {
        // New aspect correction algorithm
        // ...
        *out = processed_image;
    } else {
        fprintf(stderr, "Unknown aspect mode: %s\n", mode);
        *out = vips_image_copy(input);
    }
}
```

### Plugin Architecture (Future)

For v3.0+, consider plugin system:

```c
// Plugin interface
typedef struct {
    char *name;
    int (*init)(void);
    int (*encode)(VipsImage *, audio_encoder_t *);
    void (*cleanup)(void);
} mode_plugin_t;

// Load plugins from shared libraries
void *handle = dlopen("libsstv_plugin.so", RTLD_LAZY);
mode_plugin_t *plugin = dlsym(handle, "mode_plugin");
plugin->init();
```

---

## Testing Framework

### Test Structure

```
tests/
├── test_suite.sh         # Main test runner (bash)
├── test_suite.py         # Alternative (Python)
├── images/               # Test images
│   ├── test_320x256.png
│   ├── test_portrait.jpg
│   ├── test_landscape.jpg
│   └── ...
└── test_outputs/         # Generated outputs
    └── test_results/     # Test reports
```

### Test Categories

**1. Mode Tests**
```bash
test_martin1() {
    ./bin/pisstvpp2 -i tests/images/test.jpg -o /tmp/m1.wav -p m1
    check_exit_code $?
    check_file_exists /tmp/m1.wav
    check_file_size /tmp/m1.wav "4800000:5200000"  # Expected range
}
```

**2. Format Tests**
```bash
test_wav_format() {
    ./bin/pisstvpp2 -i tests/images/test.jpg -o /tmp/test.wav -f wav
    check_file_magic /tmp/test.wav "RIFF.*WAVE"
}

test_ogg_format() {
    ./bin/pisstvpp2 -i tests/images/test.jpg -o /tmp/test.ogg -f ogg
    check_file_magic /tmp/test.ogg "OggS"
}
```

**3. Aspect Tests**
```bash
test_aspect_center() {
    ./bin/pisstvpp2 -i tests/images/portrait.jpg -o /tmp/out.wav -a center
    # Verify no errors
}

test_aspect_pad() {
    ./bin/pisstvpp2 -i tests/images/portrait.jpg -o /tmp/out.wav -a pad
    # Check output has expected duration (black bars don't change time)
}
```

**4. CW Tests**
```bash
test_cw_id() {
    ./bin/pisstvpp2 -i tests/images/test.jpg -o /tmp/cw.wav -C "N0CALL"
    # Check file is longer than SSTV-only version
    # Verify CW added
}
```

**5. Edge Cases**
```bash
test_invalid_mode() {
    ./bin/pisstvpp2 -i tests/images/test.jpg -o /tmp/out.wav -p invalid
    check_exit_code_nonzero $?
}

test_missing_file() {
    ./bin/pisstvpp2 -i /nonexistent.jpg -o /tmp/out.wav
    check_exit_code_nonzero $?
}
```

### Test Utilities

**Python Utilities** (`util/`):
- `analyze_cw.py`: Verify CW encoding
- `compare_audio.py`: Compare audio outputs
- `validate_fix.py`: Regression testing

**Usage:**
```bash
# Verify CW in transmission
python3 util/analyze_cw.py /tmp/cw.wav

# Compare two versions
python3 util/compare_audio.py old.wav new.wav

# Run validation suite
python3 util/validate_fix.py
```

### Running Tests

```bash
# Run all tests
cd tests
./test_suite.sh

# Run specific category
./test_suite.sh modes

# Run with verbose output
./test_suite.sh -v

# Generate report
./test_suite.sh --report > test_report.txt
```

---

## Future Enhancements

### Planned for v2.1 (MMSSTV Integration)

**New Modes:**
- 43 SSTV modes from MMSSTV
- PD, AVT, Wrap, FAX modes
- Historical modes

**Architecture Changes:**
- Integration with mmsstv-portable library
- Streaming API for sample generation
- Float sample support

**Files to Add:**
- `pisstvpp2_mmsstv_adapter.c/.h` (already stubbed)
- Link to `libsstv_encoder.so`

### Possible for v2.2+

**New Features:**
- Digital modes (MFSK, Olivia, etc.)
- Watermarking
- Image overlays (text, logos)
- Batch processing built-in
- GUI (optional)

**Performance:**
- Multi-threading for encoding
- GPU acceleration (CUDA/OpenCL)
- Streaming output (pipe to transmitter)

**Formats:**
- FLAC output
- Direct PiFM transmission
- Network streaming (RTP, UDP)

---

## Conclusion

PiSSTVpp2 v2.0 architecture is:

✅ **Modular**: Clear separation of image, SSTV, audio concerns

✅ **Extensible**: Easy to add modes, formats, features

✅ **Testable**: Comprehensive test suite (55 tests)

✅ **Portable**: Standard C11, minimal dependencies

✅ **Maintainable**: Clean code, consistent patterns

**For Developers:**
- Read this document first
- Review module interfaces
- Follow existing patterns when adding features
- Write tests for new functionality
- Update documentation

**For Contributors:**
- See [CONTRIBUTING.md](../CONTRIBUTING.md) (to be created)
- Follow coding standards
- Submit pull requests with tests
- Update relevant documentation

---

**73!** 📡

*End of Architecture Documentation*
