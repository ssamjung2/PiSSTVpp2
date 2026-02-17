# Mode Registry Quick Reference

**SlowFrame v2.1 - Phase 3.1**

---

## TL;DR - Using the Registry

```c
#include "sstv/mode_registry.h"

// Create registry
mode_registry_t *reg = mode_registry_create();

// Define a mode
mode_definition_t my_mode = {
    .code = "m1",
    .name = "Martin 1",
    .vis_code = 0xAC,
    .width = 320,
    .height = 256,
    .duration_sec = 114.286,
    .is_color = 1,
    .source = "native",
    .encode_frame = martin1_encode_frame,
    .mmsstv_mode_enum = -1
};

// Register it
mode_registry_add(reg, &my_mode);

// Look it up
const mode_definition_t *mode = mode_registry_lookup_by_code(reg, "m1");

// Use it
printf("Mode: %s (%dx%d, %.1fs)\n", 
       mode->name, mode->width, mode->height, mode->duration_sec);

// Cleanup
mode_registry_free(reg);
```

---

## API Cheat Sheet

### Create/Destroy

| Function | Purpose | Returns |
|----------|---------|---------|
| `mode_registry_create()` | Allocate new registry | Pointer or NULL |
| `mode_registry_free(reg)` | Free registry | void |

### Register Modes

| Function | Purpose | Returns |
|----------|---------|---------|
| `mode_registry_add(reg, mode)` | Add mode to registry | 0=success, -1=invalid, -2=duplicate, -3=full |

### Find Modes

| Function | Purpose | Returns |
|----------|---------|---------|
| `mode_registry_lookup_by_code(reg, "m1")` | Find by short code | Pointer or NULL |
| `mode_registry_lookup_by_name(reg, "Martin 1")` | Find by full name | Pointer or NULL |
| `mode_registry_lookup_by_vis(reg, 0xAC)` | Find by VIS code | Pointer or NULL |
| `mode_registry_has_mode(reg, "m1")` | Check existence | 1=exists, 0=not found |

### List Modes

| Function | Purpose | Returns |
|----------|---------|---------|
| `mode_registry_list_all(reg)` | Get all modes | `mode_list_t` |
| `mode_registry_list_by_source(reg, "native")` | Filter by source | `mode_list_t` |
| `mode_registry_mode_count(reg)` | Get total count | int |
| `mode_registry_print_all(reg)` | Debug dump | void |

---

## Mode Definition Structure

```c
typedef struct {
    const char *code;              // "m1", "s1", "r36", "pd120"
    const char *name;              // "Martin 1", "Scottie 1", etc.
    uint8_t vis_code;              // VIS code (0x00-0xFF)
    uint32_t width;                // Image width (pixels)
    uint32_t height;               // Image height (pixels)
    double duration_sec;           // Encoding duration (seconds)
    int is_color;                  // 1=color, 0=grayscale
    const char *source;            // "native" or "mmsstv"
    int (*encode_frame)(...);      // Native encoder (NULL for MMSSTV)
    int mmsstv_mode_enum;          // MMSSTV enum (-1 for native)
} mode_definition_t;
```

---

## Common Patterns

### Pattern 1: Register All Native Modes at Startup

```c
void slowframe_init_modes(slowframe_context_t *ctx) {
    ctx->registry = mode_registry_create();
    
    modes_martin_register(ctx->registry);
    modes_scottie_register(ctx->registry);
    modes_robot_register(ctx->registry);
}
```

### Pattern 2: Validate User-Provided Mode

```c
int validate_mode(const char *mode_code, mode_registry_t *reg) {
    if (!mode_registry_has_mode(reg, mode_code)) {
        fprintf(stderr, "Error: Unknown mode '%s'\n", mode_code);
        mode_registry_print_all(reg); // Show available modes
        return SLOWFRAME_ERR_SSTV_MODE_INVALID;
    }
    return SLOWFRAME_OK;
}
```

### Pattern 3: Dispatch Encoding

```c
int encode_image(const char *mode_code, mode_registry_t *reg, ...) {
    const mode_definition_t *mode = mode_registry_lookup_by_code(reg, mode_code);
    if (!mode) {
        return SLOWFRAME_ERR_SSTV_MODE_INVALID;
    }
    
    if (strcmp(mode->source, "native") == 0) {
        return mode->encode_frame(mode->code, sample_rate, ...);
    } else if (strcmp(mode->source, "mmsstv") == 0) {
        return mmsstv_encode(mode->mmsstv_mode_enum, ...);
    }
    
    return SLOWFRAME_ERR_SSTV_MODE_UNSUPPORTED;
}
```

### Pattern 4: List Available Modes (CLI)

```c
void print_available_modes(mode_registry_t *reg) {
    mode_list_t list = mode_registry_list_all(reg);
    
    printf("Available SSTV Modes (%zu total):\n\n", list.count);
    printf("%-10s %-20s %5s %9s %7s\n",
           "Code", "Name", "VIS", "Size", "Time");
    printf("%-10s %-20s %5s %9s %7s\n",
           "----", "----", "---", "----", "----");
    
    for (size_t i = 0; i < list.count; i++) {
        printf("%-10s %-20s 0x%02X %4ux%-4u %6.1fs\n",
               list.modes[i].code,
               list.modes[i].name,
               list.modes[i].vis_code,
               list.modes[i].width,
               list.modes[i].height,
               list.modes[i].duration_sec);
    }
}
```

### Pattern 5: Filter Native vs MMSSTV Modes

```c
void show_native_modes(mode_registry_t *reg) {
    mode_list_t native = mode_registry_list_by_source(reg, "native");
    printf("Built-in modes: %zu\n", native.count);
}

void show_mmsstv_modes(mode_registry_t *reg) {
    mode_list_t mmsstv = mode_registry_list_by_source(reg, "mmsstv");
    printf("MMSSTV library modes: %zu\n", mmsstv.count);
}
```

---

## Error Handling

### Return Codes from `mode_registry_add()`

| Code | Meaning | Action |
|------|---------|--------|
| `0` | Success | Mode added |
| `-1` | Invalid arguments | NULL pointers or missing fields |
| `-2` | Duplicate code | Mode already exists |
| `-3` | Registry full | Capacity exceeded (64 modes) |

### NULL Returns from Lookup Functions

All lookup functions return `NULL` when mode not found. Always check:

```c
const mode_definition_t *mode = mode_registry_lookup_by_code(reg, "m1");
if (!mode) {
    // Handle not found
    return SLOWFRAME_ERR_SSTV_MODE_INVALID;
}
```

---

## Testing

### Run Mode Registry Tests

```bash
# Manual compilation
gcc -std=c11 -Wall -Wextra -O0 -g \
    -I src/include -I src/util \
    -o bin/test_mode_registry \
    tests/util/test_mode_registry.c \
    src/sstv/mode_registry.c \
    src/util/error.c

./bin/test_mode_registry

# Or use test runner
python3 tests/util/test_mode_registry_runner.py
```

### Expected Output

```
Mode Registry Test Suite - SlowFrame v2.1
=========================================

TEST: registry lifecycle (create/free) ... ✅ PASS
TEST: NULL pointer safety ... ✅ PASS
TEST: add single mode ... ✅ PASS
...
✅ ALL TESTS PASSED
```

---

## Performance Notes

- **Lookup Time:** O(n) linear search (< 1μs for 64 modes)
- **Memory:** 520 bytes (64 pointers + size_t)
- **Thread Safety:** None (single-threaded use only)
- **Capacity:** 64 modes maximum

---

## Integration with Existing Code

### Before (Hardcoded)

```c
// slowframe_sstv.c
if (strcmp(mode, "m1") == 0) {
    return buildaudio_m(image, sample_rate, audio_buffer, ...);
} else if (strcmp(mode, "s1") == 0) {
    return buildaudio_s(image, sample_rate, audio_buffer, ...);
} else {
    return SLOWFRAME_ERR_SSTV_MODE_INVALID;
}
```

### After (Registry-Based)

```c
// slowframe_sstv.c
const mode_definition_t *mode_def = 
    mode_registry_lookup_by_code(ctx->registry, config->mode);

if (!mode_def) {
    return SLOWFRAME_ERR_SSTV_MODE_INVALID;
}

return mode_def->encode_frame(mode_def->code, sample_rate, ...);
```

---

## FAQs

### Q: Can I modify a mode definition after registering?

**A:** No. Registry stores pointers, so modifying the original struct would affect the registry. Mode definitions should be `const` and immutable.

### Q: How do I remove a mode from the registry?

**A:** Not currently supported. Registry is append-only. If needed, destroy and recreate.

### Q: What happens if I register 65+ modes?

**A:** `mode_registry_add()` returns `-3` (full). Mode is not added.

### Q: Are lookups case-sensitive?

**A:** No. All string comparisons use `strcasecmp()` for case-insensitivity.

### Q: Can I have multiple registries?

**A:** Yes. Each `mode_registry_create()` call returns a separate instance.

---

## Related Files

- `src/include/sstv/mode_registry.h` - Public API
- `src/sstv/mode_registry.c` - Implementation
- `tests/util/test_mode_registry.c` - Unit tests
- `docs/PHASE_3_1_COMPLETION_SUMMARY.md` - Full documentation

---

**Last Updated:** February 14, 2026 20:40 PST
