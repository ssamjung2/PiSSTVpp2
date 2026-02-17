# SlowFrame Developer Walkthrough

**A guided tour through the SlowFrame v2.1 codebase for new developers**

---

## Welcome!

This document provides a structured walkthrough of the SlowFrame codebase, helping you understand:
- Where code lives and why
- How modules interconnect
- Common patterns and conventions
- Where to start when making changes

**Audience**: Developers new to SlowFrame who want to contribute or understand the internals.

---

## Table of Contents

1. [Repository Overview](#repository-overview)
2. [Starting Points](#starting-points)
3. [Core Modules Deep Dive](#core-modules-deep-dive)
4. [Data Flow Walkthrough](#data-flow-walkthrough)
5. [Common Development Tasks](#common-development-tasks)
6. [Testing Your Changes](#testing-your-changes)
7. [Debugging Tips](#debugging-tips)
8. [Code Conventions](#code-conventions)

---

## Repository Overview

### Directory Structure

```
SlowFrame/
├── bin/                    # Compiled executables
│   └── slowframe          # Main program
├── src/                    # Source code (⭐ YOU ARE HERE)
│   ├── slowframe.c        # Main entry point
│   ├── slowframe_config.c/h    # Configuration management
│   ├── slowframe_context.c/h   # Application context
│   ├── slowframe_sstv.c/h      # SSTV encoding core
│   ├── audio/             # Audio output subsystem
│   ├── image/             # Image processing subsystem
│   ├── mmsstv/            # MMSSTV integration
│   ├── overlay/           # Text overlay subsystem
│   ├── sstv/              # SSTV mode definitions
│   └── util/              # Utilities (error handling, etc.)
├── tests/                 # Test suites
│   └── util/              # Test scripts
├── docs/                  # Documentation
├── util/                  # Build/dev utilities
└── makefile              # Build system
```

### Key Files at a Glance

| File | Purpose | Lines | Complexity |
|------|---------|-------|------------|
| `src/slowframe.c` | Main program | ~800 | ⭐⭐ Medium |
| `src/slowframe_sstv.c` | SSTV encoding | ~1060 | ⭐⭐⭐ High |
| `src/sstv/mode_registry.c` | Mode management | ~400 | ⭐⭐ Medium |
| `src/image/image_loader.c` | Image loading | ~300 | ⭐ Low |
| `src/mmsstv/mmsstv_loader.c` | Dynamic loading | ~250 | ⭐⭐ Medium |
| `src/overlay/overlay_spec.c` | Text overlay | ~500 | ⭐⭐ Medium |

---

## Starting Points

### "I want to understand how SlowFrame works"

**Start here**: `src/slowframe.c` → `main()` function

1. Open `src/slowframe.c`
2. Find `main()` (around line 600)
3. Follow the execution flow:

```c
int main(int argc, char *argv[]) {
    // 1. Parse arguments → slowframe_config.c
    slowframe_config_t config;
    parse_arguments(argc, argv, &config);
    
    // 2. Initialize context → slowframe_context.c
    slowframe_context_t* ctx = slowframe_context_create();
    
    // 3. Load image → image/image_loader.c
    VipsImage* img = load_image(config.input_file);
    
    // 4. Apply overlays → overlay/overlay_spec.c
    apply_overlays(img, config.overlays);
    
    // 5. Encode SSTV → slowframe_sstv.c
    encode_sstv(img, config.protocol, output_file);
    
    // 6. Cleanup
    slowframe_context_destroy(ctx);
}
```

**Key takeaway**: SlowFrame is a **pipeline**: Args → Image → Overlay → SSTV → Audio

### "I want to add a new SSTV mode"

**Start here**: `src/sstv/modes_robot.c` (simplest example)

1. Read `src/sstv/mode_registry.h` - Understand `mode_definition_t` structure
2. Examine `src/sstv/modes_robot.c` - See how Robot modes are defined
3. Study `src/slowframe_sstv.c` - Find `buildaudio_r36()` encoding function
4. Follow along with [CONTRIB_ADDING_SSTV_MODES.md](CONTRIB_ADDING_SSTV_MODES.md)

### "I want to understand MMSSTV integration"

**Start here**: `src/mmsstv/mmsstv_loader.c`

1. Read `src/mmsstv/mmsstv_loader.h` - API definitions
2. Trace library loading in `mmsstv_load_library()`
3. See mode registration in `src/mmsstv/mmsstv_adapter.c`
4. Check fallback logic in `src/sstv/mode_registry.c`

### "I want to modify text overlay features"

**Start here**: `src/overlay/overlay_spec.c`

1. Understand `overlay_spec_t` structure (line ~30)
2. Find parsing logic in `overlay_parse_from_string()`
3. See rendering in `overlay_apply_to_image()`
4. libcairo integration around line 200+

---

## Core Modules Deep Dive

### Module 1: Error Handling (`src/util/error.c/h`)

**Purpose**: Centralized error management

**Key Structures**:
```c
typedef enum {
    ERROR_SUCCESS = 0,
    ERROR_GENERAL = 100,
    ERROR_FILE_NOT_FOUND = 200,
    ERROR_IMAGE_LOAD_FAILED = 300,
    // ... 40+ error codes
} error_code_t;
```

**Key Functions**:
- `error_string()` - Get human-readable error message
- `error_log()` - Log error with context
- `error_get_category()` - Get error category

**Usage Pattern**:
```c
int result = some_operation();
if (result != ERROR_SUCCESS) {
    error_log(result, "Operation failed: %s", filename);
    return result;
}
```

**When to modify**: Adding new error categories or codes

**File locations**:
- Definition: `src/util/error.h` (lines 1-100)
- Implementation: `src/util/error.c` (lines 1-250)

---

### Module 2: Configuration (`src/slowframe_config.c/h`)

**Purpose**: Manage all program configuration

**Key Structure**:
```c
typedef struct {
    const char* input_file;
    const char* output_file;
    const char* protocol;       // SSTV mode name
    const char* format;         // Audio format
    int sample_rate;
    // ... 12+ fields
} slowframe_config_t;
```

**Key Functions**:
- `slowframe_config_init()` - Initialize with defaults
- `slowframe_config_validate()` - Validate parameters
- `slowframe_config_cleanup()` - Free resources

**Validation Logic** (lines 200-400):
- File path checks
- Mode name validation
- Parameter range checks
- Inter-parameter dependencies

**When to modify**: Adding new command-line options

**Example validation**:
```c
// Check sample rate is valid
if (config->sample_rate != 22050 && config->sample_rate != 44100) {
    return ERROR_CONFIG_INVALID_SAMPLE_RATE;
}
```

---

### Module 3: Mode Registry (`src/sstv/mode_registry.c/h`)

**Purpose**: Central database of all SSTV modes

**Key Structure**:
```c
typedef struct {
    const char* name;           // "m1", "pd90", etc.
    int vis_code;               // VIS code (0-127)
    int width;                  // Image width
    int height;                 // Image height
    double duration;            // Transmission duration
    const char* family;         // "Martin", "Robot", etc.
    const char* color_mode;     // "color", "mono", "grayscale"
    mode_encode_func encode;    // Encoding function pointer
    void* user_data;            // Mode-specific data
} mode_definition_t;
```

**Key Functions**:
- `mode_registry_init()` - Initialize registry
- `mode_registry_add()` - Register a mode
- `mode_registry_find_by_name()` - Lookup by name
- `mode_registry_find_by_vis()` - Lookup by VIS code
- `mode_registry_list_modes()` - Enumerate all modes

**Internal Storage** (line ~50):
```c
static mode_definition_t* modes[MAX_MODES];
static int mode_count = 0;
```

**Registration Flow**:
```
1. Native modes register (Martin, Scottie, Robot)
2. MMSSTV modes register (if library available)
3. Registry locked (no more additions)
```

**When to modify**: 
- Changing mode definition structure
- Adding registry features (filtering, sorting)
- Modifying mode enumeration

**Example usage**:
```c
// Find Martin 1 mode
mode_definition_t* mode = mode_registry_find_by_name("m1");
if (mode) {
    printf("VIS: %d, Resolution: %dx%d\n", 
           mode->vis_code, mode->width, mode->height);
}
```

---

### Module 4: SSTV Encoding (`src/slowframe_sstv.c`)

**Purpose**: Core SSTV audio generation

**File Structure**:
```
Lines 1-200:    Headers, includes, tone generation functions
Lines 200-800:  Mode-specific encoding functions (buildaudio_*)
Lines 800-1060: Public API, VIS generation, helpers
```

**Tone Generation** (lines 50-150):
```c
// Generate frequency tone for duration
void generate_tone(int16_t* buffer, double freq, double duration, int sample_rate) {
    int samples = (int)(duration * sample_rate);
    for (int i = 0; i < samples; i++) {
        double t = (double)i / sample_rate;
        buffer[i] = (int16_t)(sin(2.0 * M_PI * freq * t) * 16000.0);
    }
}
```

**VIS Header** (lines 850-950):
```c
// Standard SSTV VIS header
// Leader → Break → Start → 8 data bits → Stop
void generate_vis_header(int vis_code, int16_t* buffer, int sample_rate);
```

**Mode Encoding Functions** (lines 200-800):
Each mode has a `buildaudio_X()` function:

```c
// Example: Martin 1 encoder
static int buildaudio_m1(
    VipsImage* image,
    int sample_rate,
    audio_encoder_t* encoder
) {
    for (int row = 0; row < 256; row++) {
        // Sync pulse (4.862ms @ 1200 Hz)
        generate_tone(buffer, 1200, 0.004862, sample_rate);
        
        // Porch (0.572ms @ 1500 Hz)
        generate_tone(buffer, 1500, 0.000572, sample_rate);
        
        // Green scan (146.432ms)
        scan_line(image, row, GREEN_CHANNEL, 0.146432, sample_rate);
        
        // Blue scan
        scan_line(image, row, BLUE_CHANNEL, 0.146432, sample_rate);
        
        // Red scan
        scan_line(image, row, RED_CHANNEL, 0.146432, sample_rate);
    }
}
```

**Pixel → Frequency Mapping** (lines 100-200):
```c
// SSTV standard: 1500 Hz (black) → 2300 Hz (white)
double pixel_to_freq(uint8_t pixel_value) {
    return 1500.0 + (pixel_value / 255.0) * 800.0;
}
```

**When to modify**:
- Adding new native modes
- Changing tone generation
- Modifying VIS header
- Adjusting timing constants

---

### Module 5: Image Processing (`src/image/`)

**Submodules**:

**image_loader.c** (image loading):
```c
VipsImage* image_loader_load(image_loader_t* loader, const char* path) {
    // 1. Detect format
    // 2. Load with libvips
    // 3. Convert to RGB
    // 4. Return image
}
```

**image_processor.c** (image manipulation):
```c
VipsImage* image_processor_scale(
    image_processor_t* proc,
    VipsImage* input,
    int target_width,
    int target_height
) {
    // Resize image using libvips
}
```

**image_aspect.c** (aspect ratio correction):
```c
VipsImage* aspect_handler_correct(
    aspect_handler_t* handler,
    VipsImage* input,
    int target_width,
    int target_height,
    const char* mode  // "center", "pad", "stretch"
) {
    if (strcmp(mode, "pad") == 0) {
        // Add black bars (letterbox/pillarbox)
    } else if (strcmp(mode, "center") == 0) {
        // Center-crop to aspect ratio
    } else {
        // Force resize (stretch)
    }
}
```

**When to modify**:
- Adding image processing features
- Supporting new input formats
- Changing aspect ratio algorithms
- Optimizing image pipeline

---

### Module 6: Text Overlay (`src/overlay/overlay_spec.c`)

**Purpose**: Render text overlays on images

**Data Structure** (lines 1-50):
```c
typedef struct {
    char* text;                // Text to display
    char* font_family;         // Font name
    int font_size;             // 8-72 pt
    int color_r, color_g, color_b;  // RGB 0-255
    double opacity;            // 0.0-1.0
    char position[3];          // "LT", "CM", "RB", etc.
    bool background_bar;
    int bar_height;
    int bar_color_r, bar_color_g, bar_color_b;
    double bar_opacity;
} overlay_spec_t;
```

**Parsing** (lines 100-300):
```c
// Parse "-T text:K9ABC|size:24|color:255,255,0|pos:LT"
overlay_spec_t* overlay_parse_from_string(const char* spec) {
    // Split by '|'
    // Parse each key:value pair
    // Validate parameters
    // Return overlay structure
}
```

**Rendering** (lines 300-500, uses libcairo):
```c
void overlay_apply_to_image(
    VipsImage* image,
    overlay_spec_t* overlay
) {
    // 1. Create Cairo surface from VipsImage
    // 2. Calculate text position
    // 3. Draw background bar (if enabled)
    // 4. Set font and color
    // 5. Render text
    // 6. Composite back to image
}
```

**Position Mapping** (lines 250-280):
```c
// "LT" → top-left, "CM" → center-middle, "RB" → right-bottom
void calculate_position(
    const char* position,
    int image_width,
    int image_height,
    int text_width,
    int text_height,
    int* x,
    int* y
) {
    // L/C/R: left, center, right
    // T/M/B: top, middle, bottom
}
```

**When to modify**:
- Adding overlay features
- Supporting new fonts
- Changing positioning
- Adding effects (shadow, outline, etc.)

---

### Module 7: MMSSTV Integration (`src/mmsstv/`)

**mmsstv_loader.c** - Dynamic library loading:
```c
bool mmsstv_load_library(void) {
    // 1. Check environment variable
    const char* lib_path = getenv("SLOWFRAME_MMSSTV_LIB");
    
    // 2. Search standard paths
    const char* search_paths[] = {
        "./libsstv_encoder.1.0.0.dylib",
        "/usr/local/lib/libsstv_encoder.1.0.0.dylib",
        "/opt/homebrew/lib/libsstv_encoder.1.0.0.dylib"
    };
    
    // 3. dlopen() library
    void* handle = dlopen(lib_path, RTLD_LAZY);
    
    // 4. Resolve symbols
    encode_func = dlsym(handle, "sstv_encode");
    
    // 5. Return success/failure
    return (handle != NULL && encode_func != NULL);
}
```

**mmsstv_adapter.c** - Bridge to mode registry:
```c
void modes_mmsstv_register(mode_registry_t* registry) {
    if (!mmsstv_library_available()) {
        return;  // Graceful fallback
    }
    
    // Register all 43 MMSSTV modes
    for (int i = 0; i < mmsstv_mode_count; i++) {
        mode_definition_t mode = {
            .name = mmsstv_modes[i].name,
            .vis_code = mmsstv_modes[i].vis,
            .encode = mmsstv_encode_wrapper,  // Delegate to library
            // ...
        };
        mode_registry_add(registry, &mode);
    }
}
```

**When to modify**:
- Updating MMSSTV API version
- Adding library search paths
- Changing fallback behavior
- Debugging library loading

---

## Data Flow Walkthrough

### Complete Encoding Pipeline

Let's trace a single command through the entire codebase:

```bash
./bin/slowframe -i photo.jpg -p m1 -T "text:K9ABC|pos:LT" -C "K9ABC" -o output.wav
```

**Step 1: Argument Parsing** (`src/slowframe.c`, line ~100)
```c
parse_arguments(argc, argv, &config);
// config.input_file = "photo.jpg"
// config.protocol = "m1"
// config.overlays[0] = overlay_spec("text:K9ABC|pos:LT")
// config.cw_callsign = "K9ABC"
// config.output_file = "output.wav"
```

**Step 2: Context Initialization** (`src/slowframe_context.c`, line ~50)
```c
slowframe_context_t* ctx = slowframe_context_create();
// Initializes all subsystems:
//   - Error handler
//   - Config manager
//   - Image loader
//   - Mode registry
//   - MMSSTV loader (if available)
```

**Step 3: Image Loading** (`src/image/image_loader.c`, line ~100)
```c
VipsImage* img = image_loader_load(ctx->loader, "photo.jpg");
// 1. Detects format (JPEG)
// 2. Loads with libvips
// 3. Converts to RGB
// 4. Returns VipsImage* (in-memory image)
```

**Step 4: Aspect Correction** (`src/image/image_aspect.c`, line ~150)
```c
VipsImage* corrected = aspect_handler_correct(
    ctx->aspect, img, 320, 256, config.aspect_mode
);
// Adjusts image to 320×256 (Martin 1 resolution)
// Default mode: "pad" (adds black bars if needed)
```

**Step 5: Text Overlay** (`src/overlay/overlay_spec.c`, line ~300)
```c
for (int i = 0; i < config.overlay_count; i++) {
    overlay_apply_to_image(corrected, config.overlays[i]);
}
// Renders "K9ABC" in top-left corner using libcairo
```

**Step 6: Mode Lookup** (`src/sstv/mode_registry.c`, line ~200)
```c
mode_definition_t* mode = mode_registry_find_by_name("m1");
// Returns Martin 1 mode definition:
//   vis_code: 44
//   width: 320, height: 256
//   encode: buildaudio_m1
```

**Step 7: Audio Encoder Setup** (`src/audio/slowframe_audio_encoder.c`, line ~50)
```c
audio_encoder_t* encoder = audio_encoder_create(
    "output.wav", "wav", 22050
);
// Creates WAV encoder instance
```

**Step 8: VIS Header** (`src/slowframe_sstv.c`, line ~850)
```c
generate_vis_header(mode->vis_code, encoder, 22050);
// Generates standard SSTV VIS header
// Writes to audio encoder
```

**Step 9: SSTV Encoding** (`src/slowframe_sstv.c`, line ~400)
```c
mode->encode(corrected, 22050, encoder);
// Calls buildaudio_m1()
// For each of 256 rows:
//   - Generates sync pulse (1200 Hz, 4.862ms)
//   - Generates porch (1500 Hz, 0.572ms)
//   - Scans Green channel (146.432ms)
//   - Scans Blue channel (146.432ms)
//   - Scans Red channel (146.432ms)
// Total: ~114 seconds of audio
```

**Step 10: CW Identification** (`src/slowframe.c`, line ~500)
```c
if (config.cw_callsign) {
    generate_cw_id(encoder, config.cw_callsign, config.cw_wpm, config.cw_tone);
}
// Appends Morse code "K9ABC" to audio
// Default: 20 WPM, 800 Hz
```

**Step 11: Finalize & Cleanup** (`src/slowframe.c`, line ~700)
```c
audio_encoder_destroy(encoder);  // Closes WAV file
slowframe_context_destroy(ctx);  // Cleans up all subsystems
// output.wav now contains complete SSTV + CW transmission
```

**Result**: `output.wav` contains:
- VIS header (7 seconds)
- Martin 1 image (114 seconds)
- CW ID "K9ABC" (~3 seconds)
- Total: ~124 seconds, ~5.4 MB

---

## Common Development Tasks

### Task 1: Add a New Command-Line Option

**Example**: Add `--quality` option for JPEG quality control

**Files to modify**:
1. `src/slowframe_config.h` - Add field to structure
2. `src/slowframe_config.c` - Add default value, validation
3. `src/slowframe.c` - Add argument parsing
4. Relevant module (e.g., `image_loader.c`) - Use new parameter

**Step-by-step**:

```c
// 1. src/slowframe_config.h (line ~40)
typedef struct {
    // ... existing fields ...
    int jpeg_quality;  // NEW: 1-100
} slowframe_config_t;

// 2. src/slowframe_config.c (line ~20)
void slowframe_config_init(slowframe_config_t* config) {
    // ... existing defaults ...
    config->jpeg_quality = 85;  // Default quality
}

// 3. src/slowframe_config.c (line ~100)
int slowframe_config_validate(slowframe_config_t* config) {
    // ... existing validation ...
    if (config->jpeg_quality < 1 || config->jpeg_quality > 100) {
        error_log(ERROR_CONFIG_INVALID_PARAMETER, 
                  "JPEG quality must be 1-100");
        return ERROR_CONFIG_INVALID_PARAMETER;
    }
    return ERROR_SUCCESS;
}

// 4. src/slowframe.c (line ~150, in argument parsing)
} else if (strcmp(arg, "--quality") == 0) {
    if (i + 1 >= argc) {
        error_log(ERROR_CONFIG_MISSING_VALUE, "--quality requires value");
        return ERROR_CONFIG_MISSING_VALUE;
    }
    config->jpeg_quality = atoi(argv[++i]);
}

// 5. src/image/image_loader.c (line ~200, use parameter)
vips_jpegsave(image, output_path, "Q", config->jpeg_quality, NULL);
```

**Test**:
```bash
make clean && make
./bin/slowframe -i test.jpg -p m1 --quality 95 -o test.wav
```

---

### Task 2: Add a New Error Code

**Example**: Add error for "unsupported audio format"

**Files to modify**:
1. `src/util/error.h` - Add error code
2. `src/util/error.c` - Add error message

**Step-by-step**:

```c
// 1. src/util/error.h (line ~50, in encoding section)
#define ERROR_ENCODING_UNSUPPORTED_FORMAT    550

// 2. src/util/error.c (line ~150, in error_string())
case ERROR_ENCODING_UNSUPPORTED_FORMAT:
    return "Unsupported audio format";

// 3. Use in code (e.g., audio_encoder.c)
if (strcmp(format, "wav") != 0 && strcmp(format, "ogg") != 0) {
    error_log(ERROR_ENCODING_UNSUPPORTED_FORMAT, 
              "Format '%s' not supported", format);
    return ERROR_ENCODING_UNSUPPORTED_FORMAT;
}
```

---

### Task 3: Debug a Mode Encoding Issue

**Scenario**: Martin 1 mode produces garbled output

**Debugging approach**:

**Step 1**: Enable verbose logging
```bash
./bin/slowframe -v -i test.jpg -p m1 -o test.wav
```

**Step 2**: Check mode registry
```c
// Add debug prints in src/slowframe_sstv.c
mode_definition_t* mode = mode_registry_find_by_name("m1");
printf("DEBUG: Mode found: %s, VIS: %d, %dx%d\n",
       mode->name, mode->vis_code, mode->width, mode->height);
```

**Step 3**: Verify encoding function
```c
// In buildaudio_m1() (src/slowframe_sstv.c, line ~400)
printf("DEBUG: Encoding row %d/%d\n", row, total_rows);

// Check timing constants
printf("DEBUG: Sync duration: %.6f, Scan duration: %.6f\n",
       sync_duration, scan_duration);
```

**Step 4**: Inspect audio output
```bash
# Check file properties
file test.wav
# Output: RIFF (little-endian) data, WAVE audio, Microsoft PCM, 16 bit, mono 22050 Hz

# Verify duration (~114s for Martin 1)
ffprobe test.wav 2>&1 | grep Duration
# Output: Duration: 00:01:54.23

# Analyze waveform
ffplay test.wav  # Listen for correct tones
```

**Step 5**: Compare with reference
```bash
# Generate reference (known working version)
git checkout v2.0.0
make clean && make
./bin/slowframe -i test.jpg -p m1 -o reference.wav

# Compare
python3 tests/util/compare_audio.py reference.wav test.wav
```

---

## Testing Your Changes

### Unit Tests

**Location**: `tests/util/`

**Run all tests**:
```bash
cd tests/util
python3 test_suite.py          # Backward compatibility (69 tests)
python3 test_modernization.py  # v2.1 features (54 tests)
python3 test_mmsstv_integration.py  # MMSSTV (22 tests)
python3 test_bw24_mode.py      # Robot B&W 24 (22 tests)
```

**Run specific test**:
```bash
cd tests/util
python3 test_suite.py TestModernization.test_mode_list
```

### Integration Testing

**Manual test**:
```bash
# Build
make clean && make

# Quick smoke test
./bin/slowframe -i tests/images/test.jpg -p m1 -o /tmp/test.wav

# Verify output
file /tmp/test.wav
ls -lh /tmp/test.wav
ffprobe /tmp/test.wav
```

### Regression Testing

**After making changes**:
```bash
# Run full test suite
cd tests/util
python3 test_suite.py

# Check for failures
echo "Exit code: $?"  # Should be 0

# Compare outputs
python3 util/validate_fix.py before.wav after.wav
```

---

## Debugging Tips

### Compile with Debug Symbols

```bash
# Modify makefile (line ~10)
CFLAGS = -g -O0  # Debug mode
# CFLAGS = -O2    # Production mode

make clean && make
```

### Use GDB

```bash
# Start debugger
gdb ./bin/slowframe

# Set breakpoint
(gdb) break slowframe_sstv.c:buildaudio_m1

# Run
(gdb) run -i test.jpg -p m1 -o test.wav

# Inspect variables
(gdb) print mode->vis_code
(gdb) print sample_rate

# Continue execution
(gdb) continue
```

### Valgrind (Memory Leaks)

```bash
# Install valgrind (macOS)
brew install valgrind

# Run with leak detection
valgrind --leak-check=full ./bin/slowframe -i test.jpg -p m1 -o test.wav

# Check output for leaks
# Look for: "definitely lost", "indirectly lost"
```

### Print Debugging

```c
// Use fprintf to stderr (won't interfere with output)
fprintf(stderr, "DEBUG: %s:%d: variable = %d\n", __FILE__, __LINE__, value);

// Conditional debug output
#ifdef DEBUG
    printf("Debug info: %s\n", debug_str);
#endif
```

### Logging

```c
// Use error_log even for debug (if verbose mode)
if (config->verbose) {
    error_log(ERROR_SUCCESS, "Processing row %d", row);
}
```

---

## Code Conventions

### Naming

**Functions**:
- `module_verb_noun()` - e.g., `mode_registry_find_by_name()`
- Private functions: `static` keyword

**Variables**:
- `snake_case` - e.g., `sample_rate`, `vis_code`
- Globals: avoid (use context structures)

**Constants**:
- `UPPER_SNAKE_CASE` - e.g., `MAX_MODES`, `ERROR_FILE_NOT_FOUND`

**Types**:
- `snake_case_t` - e.g., `mode_definition_t`, `image_loader_t`

### File Organization

**Header (.h)**:
```c
#ifndef MODULE_NAME_H
#define MODULE_NAME_H

// Includes
// Type definitions
// Function declarations
// Constants

#endif
```

**Implementation (.c)**:
```c
// Includes
// Static (private) functions
// Public functions
```

### Comments

**Function documentation**:
```c
/**
 * Load an image from file
 * 
 * @param loader Image loader instance
 * @param path Path to image file
 * @return VipsImage* on success, NULL on failure
 */
VipsImage* image_loader_load(image_loader_t* loader, const char* path);
```

**Inline comments**:
```c
// Sync pulse: 1200 Hz for 4.862ms (Martin 1 spec)
generate_tone(buffer, 1200, 0.004862, sample_rate);
```

### Error Handling

**Always check return values**:
```c
// Bad
mode_definition_t* mode = mode_registry_find_by_name("m1");
mode->encode(...);  // Crash if mode is NULL!

// Good
mode_definition_t* mode = mode_registry_find_by_name("m1");
if (!mode) {
    error_log(ERROR_MODE_NOT_FOUND, "Mode 'm1' not found");
    return ERROR_MODE_NOT_FOUND;
}
mode->encode(...);
```

**Use error codes**:
```c
// Bad
int load_image(...) {
    if (error) return -1;  // What error?
}

// Good
int load_image(...) {
    if (error) return ERROR_IMAGE_LOAD_FAILED;  // Specific error
}
```

---

## Quick Reference

### Build & Run

```bash
# Compile
make clean && make

# Run
./bin/slowframe -i input.jpg -p m1 -o output.wav

# List modes
./bin/slowframe -L

# Help
./bin/slowframe -h
```

### Test

```bash
cd tests/util
python3 test_suite.py
python3 test_modernization.py
python3 test_mmsstv_integration.py
```

### Debug

```bash
# Verbose mode
./bin/slowframe -v -i test.jpg -p m1 -o test.wav

# GDB
gdb ./bin/slowframe

# Valgrind
valgrind --leak-check=full ./bin/slowframe ...
```

### File Locations Cheat Sheet

| Task | File |
|------|------|
| Main entry point | `src/slowframe.c` |
| Add SSTV mode | `src/sstv/modes_*.c` |
| Modify encoding | `src/slowframe_sstv.c` |
| Change config | `src/slowframe_config.c/h` |
| Add error code | `src/util/error.c/h` |
| Image processing | `src/image/image_*.c` |
| Text overlay | `src/overlay/overlay_spec.c` |
| MMSSTV integration | `src/mmsstv/*.c` |
| Audio formats | `src/audio/audio_encoder_*.c` |

---

## Next Steps

After completing this walkthrough:

1. **Read ARCHITECTURE.md** - Deeper technical details
2. **Try a simple change** - Add debug output, modify constant
3. **Review contributor guides**:
   - [CONTRIB_ADDING_SSTV_MODES.md](CONTRIB_ADDING_SSTV_MODES.md)
   - [CONTRIB_FIXING_BUGS.md](CONTRIB_FIXING_BUGS.md)
4. **Pick a task** - Check GitHub issues for "good first issue" labels
5. **Ask questions** - File GitHub discussions for help

---

## Resources

- [ARCHITECTURE.md](ARCHITECTURE.md) - Complete architecture documentation
- [CONTRIBUTING.md](../CONTRIBUTING.md) - Contribution guidelines
- [BUILD.md](BUILD.md) - Build system details
- [MMSSTV_SETUP_GUIDE.md](MMSSTV_SETUP_GUIDE.md) - User guide for MMSSTV

---

**Happy Coding!** 🚀

*Last Updated: February 16, 2026*  
*SlowFrame v2.1.0*
