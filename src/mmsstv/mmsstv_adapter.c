/**
 * @file mmsstv_adapter.c
 * @brief MMSSTV Library Adapter Implementation
 * 
 * @date February 15, 2026
 * @version 1.0
 */

#include "mmsstv_adapter.h"
#include "slowframe_image.h"
#include "slowframe_sstv.h"  /* For sstv_add_samples_to_buffer */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dlfcn.h>  /* dlsym */
#include <math.h>


/* ========================================================================
 * Forward Declaration of mmsstv_library internals
 * ======================================================================== */

/* We need access to the internal structure to get dl_handle for dlsym */
struct mmsstv_library {
    void *dl_handle;
    /* ...other fields not needed here... */
};


/* ========================================================================
 * Internal Structures
 * ======================================================================== */

/**
 * Adapter state structure
 */
struct mmsstv_adapter {
    mmsstv_library_t *library;       /**< MMSSTV library handle (may be NULL) */
    mode_definition_t *modes;        /**< Array of mode definitions */
    int mode_count;                  /**< Number of modes */
    bool is_available;               /**< Library successfully loaded */
    char status_message[256];        /**< Status/diagnostic message */
};


/* ========================================================================
 * Global State (for encoder wrapper callback)
 * ======================================================================== */

/**
 * Static reference to the current adapter instance.
 * This is used by mmsstv_encode_wrapper() to access the library.
 * Set during mmsstv_adapter_init(), cleared during mmsstv_adapter_destroy().
 */
static mmsstv_adapter_t *g_active_adapter = NULL;


/* ========================================================================
 * Forward Declarations
 * ======================================================================== */

static bool enumerate_modes(mmsstv_adapter_t *adapter);
static mode_definition_t create_mode_definition(
    const sstv_mode_info_t *sstv_mode
);
static void generate_mode_code(const char *name, char *code_buf, size_t buf_size);
static int mmsstv_encode_wrapper(
    const char *mode_code,
    uint16_t sample_rate,
    uint16_t *audio_buffer,
    uint32_t max_samples
);


/* ========================================================================
 * Adapter Initialization
 * ======================================================================== */

mmsstv_adapter_t* mmsstv_adapter_init(void) {
    /* Allocate adapter */
    mmsstv_adapter_t *adapter = calloc(1, sizeof(mmsstv_adapter_t));
    if (!adapter) {
        return NULL;
    }
    
    /* Initialize to empty state */
    adapter->library = NULL;
    adapter->modes = NULL;
    adapter->mode_count = 0;
    adapter->is_available = false;
    snprintf(adapter->status_message, sizeof(adapter->status_message),
             "Not initialized");
    
    /* Try to load MMSSTV library */
    adapter->library = mmsstv_loader_init();
    
    if (adapter->library && mmsstv_loader_is_available(adapter->library)) {
        /* Library loaded successfully - enumerate modes */
        if (enumerate_modes(adapter)) {
            adapter->is_available = true;
            snprintf(adapter->status_message, sizeof(adapter->status_message),
                     "MMSSTV library loaded: %d modes available",
                     adapter->mode_count);
        } else {
            /* Enumeration failed */
            snprintf(adapter->status_message, sizeof(adapter->status_message),
                     "MMSSTV library loaded but mode enumeration failed");
        }
    } else {
        /* Library not found - this is OK! */
        snprintf(adapter->status_message, sizeof(adapter->status_message),
                 "MMSSTV library not detected (native modes only)");
    }
    
    /* Store global reference for encoder wrapper */
    g_active_adapter = adapter;
    
    return adapter;
}

bool mmsstv_adapter_is_available(const mmsstv_adapter_t *adapter) {
    return adapter && adapter->is_available;
}

void mmsstv_adapter_destroy(mmsstv_adapter_t *adapter) {
    if (!adapter) {
        return;
    }
    
    /* Clear global reference if this is the active adapter */
    if (g_active_adapter == adapter) {
        g_active_adapter = NULL;
    }
    
    /* Free mode code strings (allocated in create_mode_definition) */
    if (adapter->modes) {
        for (int i = 0; i < adapter->mode_count; i++) {
            if (adapter->modes[i].code) {
                free((void*)adapter->modes[i].code);
            }
        }
        free(adapter->modes);
    }
    
    /* Destroy library handle */
    if (adapter->library) {
        mmsstv_loader_destroy(adapter->library);
    }
    
    /* Free adapter */
    free(adapter);
}


/* ========================================================================
 * Mode Access
 * ======================================================================== */

int mmsstv_adapter_get_mode_count(const mmsstv_adapter_t *adapter) {
    if (!adapter || !adapter->is_available) {
        return 0;
    }
    return adapter->mode_count;
}

const mode_definition_t* mmsstv_adapter_get_mode(
    const mmsstv_adapter_t *adapter,
    int index
) {
    if (!adapter || !adapter->is_available) {
        return NULL;
    }
    
    if (index < 0 || index >= adapter->mode_count) {
        return NULL;
    }
    
    return &adapter->modes[index];
}


/* ========================================================================
 * Registry Integration
 * ======================================================================== */

int mmsstv_adapter_register_modes(
    mmsstv_adapter_t *adapter,
    mode_registry_t *registry
) {
    if (!adapter || !adapter->is_available || !registry) {
        return 0;
    }
    
    int registered_count = 0;
    
    for (int i = 0; i < adapter->mode_count; i++) {
        int result = mode_registry_add(registry, &adapter->modes[i]);
        if (result == 0) {
            registered_count++;
        } else {
            /* Registration failed (likely duplicate VIS code) */
            fprintf(stderr, "Warning: Failed to register MMSSTV mode '%s' (VIS %d)\n",
                    adapter->modes[i].name, adapter->modes[i].vis_code);
        }
    }
    
    return registered_count;
}


/* ========================================================================
 * Library Information
 * ======================================================================== */

const char* mmsstv_adapter_get_version(const mmsstv_adapter_t *adapter) {
    if (!adapter || !adapter->library) {
        return NULL;
    }
    return mmsstv_loader_get_version(adapter->library);
}

int mmsstv_adapter_get_status(
    const mmsstv_adapter_t *adapter,
    char *buffer,
    int buffer_size
) {
    if (!adapter || !buffer || buffer_size <= 0) {
        return 0;
    }
    
    if (adapter->library) {
        return mmsstv_loader_get_status(adapter->library, buffer, buffer_size);
    } else {
        return snprintf(buffer, buffer_size, "%s", adapter->status_message);
    }
}

const char* mmsstv_adapter_get_library_path(const mmsstv_adapter_t *adapter) {
    if (!adapter || !adapter->library) {
        return NULL;
    }
    return mmsstv_loader_get_path(adapter->library);
}


/* ========================================================================
 * Internal: Mode Enumeration
 * ======================================================================== */

/**
 * Enumerate modes from SSTV library
 * 
 * Creates mode_definition_t structures for all SSTV modes (43 modes).
 * 
 * @param adapter Adapter with loaded library
 * @return true on success, false on failure
 */
static bool enumerate_modes(mmsstv_adapter_t *adapter) {
    if (!adapter || !adapter->library) {
        return false;
    }
    
    /* Get function pointers */
    const mmsstv_functions_t *funcs = mmsstv_loader_get_functions(adapter->library);
    if (!funcs || !funcs->get_all_modes) {
        return false;
    }
    
    /* Get all modes from library */
    size_t count = 0;
    const sstv_mode_info_t *sstv_modes = funcs->get_all_modes(&count);
    if (!sstv_modes || count == 0) {
        return false;
    }
    
    /* Allocate mode definition array */
    adapter->modes = calloc(count, sizeof(mode_definition_t));
    if (!adapter->modes) {
        return false;
    }
    
    /* Convert each SSTV mode to mode_definition_t */
    for (size_t i = 0; i < count; i++) {
        adapter->modes[i] = create_mode_definition(&sstv_modes[i]);
    }
    
    adapter->mode_count = count;
    return true;
}

/**
 * Generate mode code from mode name
 * 
 * Converts mode name to lowercase code (e.g., "Scottie 1" → "scottie1")
 * 
 * @param name Mode name from library
 * @param code_buf Output buffer for code
 * @param buf_size Size of code buffer
 */
static void generate_mode_code(const char *name, char *code_buf, size_t buf_size) {
    if (!name || !code_buf || buf_size == 0) {
        return;
    }
    
    size_t j = 0;
    for (size_t i = 0; name[i] && j < buf_size - 1; i++) {
        char c = name[i];
        /* Skip spaces and special characters */
        if (c == ' ' || c == '-') {
            continue;
        }
        /* Convert to lowercase */
        if (c >= 'A' && c <= 'Z') {
            c = c - 'A' + 'a';
        }
        code_buf[j++] = c;
    }
    code_buf[j] = '\0';
}

/**
 * Create mode_definition_t from SSTV mode (real library)
 * 
 * Converts SSTV library mode structure to our mode registry format.
 * 
 * @param sstv_mode SSTV mode structure from library
 * @return mode_definition_t structure
 */
static mode_definition_t create_mode_definition(const sstv_mode_info_t *sstv_mode) {
    mode_definition_t mode;
    memset(&mode, 0, sizeof(mode_definition_t));
    
    if (!sstv_mode) {
        return mode;
    }
    
    /* Allocate and set name (point directly to library string) */
    mode.name = sstv_mode->name;
    
    /* Generate code from name (e.g., "PD120" → "pd120") */
    /* Allocate storage for the code */
    char *code = malloc(64);
    if (code) {
        generate_mode_code(sstv_mode->name, code, 64);
        mode.code = code;
    } else {
        mode.code = "unknown";
    }
    
    /* Copy other fields */
    mode.vis_code = sstv_mode->vis_code;
    mode.width = sstv_mode->width;
    mode.height = sstv_mode->height;
    mode.duration_sec = sstv_mode->duration_sec;  /* Already in seconds */
    mode.is_color = sstv_mode->is_color;
    
    /* Set source */
    mode.source = "mmsstv";
    
    /* Set encoder function (placeholder) */
    mode.encode_frame = mmsstv_encode_wrapper;
    
    /* Store mode enum for future use */
    mode.mmsstv_mode_enum = (int)sstv_mode->mode;
    
    return mode;
}


/* ========================================================================
 * Internal: Encoder Wrapper
 * ======================================================================== */

/**
 * Encoder wrapper function
 * 
 * This function bridges our mode registry API to the MMSSTV library API.
 * It's called by the mode registry when encoding with an MMSSTV mode.
 * 
 * @param mode_code Mode identifier string
 * @param sample_rate Sample rate in Hz
 * @param audio_buffer Output buffer for audio samples (uint16_t format)
 * @param max_samples Size of audio buffer
 * @return SLOWFRAME_OK on success, or error code on failure
 */
static int mmsstv_encode_wrapper(
    const char *mode_code,
    uint16_t sample_rate,
    uint16_t *audio_buffer,
    uint32_t max_samples
) {
    (void)audio_buffer;
    /* Validate adapter is available */
    if (!g_active_adapter || !g_active_adapter->is_available) {
        fprintf(stderr, "Error: MMSSTV library not available\n");
        return -1;
    }
    
    /* Find the mode by code to get MMSSTV enum */
    int mmsstv_mode_enum = -1;
    int mode_is_color = 1;  /* Default to color */
    for (int i = 0; i < g_active_adapter->mode_count; i++) {
        if (strcasecmp(g_active_adapter->modes[i].code, mode_code) == 0) {
            mmsstv_mode_enum = g_active_adapter->modes[i].mmsstv_mode_enum;
            mode_is_color = g_active_adapter->modes[i].is_color;
            break;
        }
    }
    
    if (mmsstv_mode_enum < 0) {
        fprintf(stderr, "Error: MMSSTV mode '%s' not found\n", mode_code);
        return -1;
    }
    
    /* Get image buffer from image module */
    const ImageBuffer *img_buf = image_get_rgb_data();
    if (!img_buf || !img_buf->data) {
        fprintf(stderr, "Error: No image loaded for MMSSTV encoding\n");
        return -1;
    }
    
    /* Debug: Check if image dimensions match mode expectations */
    const mode_definition_t *mode_def = NULL;
    for (int i = 0; i < g_active_adapter->mode_count; i++) {
        if (strcasecmp(g_active_adapter->modes[i].code, mode_code) == 0) {
            mode_def = &g_active_adapter->modes[i];
            break;
        }
    }
    /* Validate dimensions match mode expectations */
    if (mode_def) {
        if ((uint32_t)img_buf->width != mode_def->width || (uint32_t)img_buf->height != mode_def->height) {
            fprintf(stderr, "Warning: Image dimensions (%ux%u) don't match mode '%s' (%ux%u)\n",
                    img_buf->width, img_buf->height, mode_code, mode_def->width, mode_def->height);
        }
    }
    
    /* Load library functions via dlsym */ 
    void *lib_handle = g_active_adapter->library ? 
        /* Access internal dl_handle - we need a better API for this */
        ((struct mmsstv_library *)g_active_adapter->library)->dl_handle : NULL;
    
    if (!lib_handle) {
        fprintf(stderr, "Error: MMSSTV library handle not available\n");
        return -1;
    }
    
    /* Load encoding functions directly */
    typedef sstv_encoder_t* (*create_fn)(sstv_mode_t, double);
    typedef void (*free_fn)(sstv_encoder_t*);
    typedef int (*set_image_fn)(sstv_encoder_t*, const sstv_image_t*);
    typedef size_t (*generate_fn)(sstv_encoder_t*, float*, size_t);
    typedef void (*set_vis_fn)(sstv_encoder_t*, int);
    typedef int (*is_complete_fn)(sstv_encoder_t*);
    typedef sstv_image_t (*image_from_rgb_fn)(uint8_t*, uint32_t, uint32_t);
    typedef sstv_image_t (*image_from_gray_fn)(uint8_t*, uint32_t, uint32_t);
    
    create_fn encoder_create;
    free_fn encoder_free;
    set_image_fn set_image;
    generate_fn encoder_generate;
    set_vis_fn encoder_set_vis;
    is_complete_fn encoder_is_complete;
    image_from_rgb_fn image_from_rgb;
    image_from_gray_fn image_from_gray;
    /* Use POSIX-idiomatic cast to avoid -Wpedantic object->function-pointer warning */
    *(void **)(&encoder_create)     = dlsym(lib_handle, "sstv_encoder_create");
    *(void **)(&encoder_free)        = dlsym(lib_handle, "sstv_encoder_free");
    *(void **)(&set_image)           = dlsym(lib_handle, "sstv_encoder_set_image");
    *(void **)(&encoder_generate)    = dlsym(lib_handle, "sstv_encoder_generate");
    *(void **)(&encoder_set_vis)     = dlsym(lib_handle, "sstv_encoder_set_vis_enabled");
    *(void **)(&encoder_is_complete) = dlsym(lib_handle, "sstv_encoder_is_complete");
    *(void **)(&image_from_rgb)      = dlsym(lib_handle, "sstv_image_from_rgb");
    *(void **)(&image_from_gray)     = dlsym(lib_handle, "sstv_image_from_gray");
    
    if (!encoder_create || !encoder_free || !set_image || !encoder_generate ||
        !encoder_is_complete || !image_from_rgb) {
        fprintf(stderr, "Error: Failed to load MMSSTV encoding functions\n");
        return -1;
    }
    
    /* Create MMSSTV encoder */
    sstv_encoder_t *encoder = encoder_create((sstv_mode_t)mmsstv_mode_enum, (double)sample_rate);
    if (!encoder) {
        fprintf(stderr, "Error: Failed to create MMSSTV encoder\n");
        return -1;
    }
    
    /* Use MMSSTV VIS/preamble for MMSSTV modes */
    if (encoder_set_vis) {
        encoder_set_vis(encoder, 1);
    }

    /* Pack RGB data if rowstride includes padding */
    uint32_t img_width = (uint32_t)img_buf->width;
    uint32_t img_height = (uint32_t)img_buf->height;
    uint32_t packed_stride = img_width * 3u;
    uint8_t *packed_rgb = NULL;
    uint8_t *rgb_data = img_buf->data;
    
    if (img_buf->rowstride != (int)packed_stride) {
        size_t packed_size = (size_t)packed_stride * (size_t)img_height;
        packed_rgb = (uint8_t *)malloc(packed_size);
        if (!packed_rgb) {
            fprintf(stderr, "Error: Failed to allocate packed RGB buffer\n");
            encoder_free(encoder);
            return -1;
        }

        uint32_t bytes_per_pixel = 0;
        if (img_width > 0 && img_buf->rowstride > 0) {
            bytes_per_pixel = (uint32_t)img_buf->rowstride / img_width;
        }

        for (uint32_t y = 0; y < img_height; y++) {
            const uint8_t *src = img_buf->data + (size_t)y * (size_t)img_buf->rowstride;
            uint8_t *dst = packed_rgb + (size_t)y * (size_t)packed_stride;
            if (bytes_per_pixel >= 4) {
                for (uint32_t x = 0; x < img_width; x++) {
                    const uint8_t *p = src + (size_t)x * (size_t)bytes_per_pixel;
                    dst[x * 3u + 0u] = p[0];
                    dst[x * 3u + 1u] = p[1];
                    dst[x * 3u + 2u] = p[2];
                }
            } else {
                memcpy(dst, src, packed_stride);
            }
        }
        rgb_data = packed_rgb;
    }

    /* Create SSTV image structure */
    sstv_image_t sstv_img;
    uint8_t *gray_data = NULL;
    uint8_t *rgb24_from_gray = NULL;
    
    if (!mode_is_color && image_from_gray) {
        /* B/W mode - convert RGB to grayscale,  then expand to RGB24 as workaround for MMSSTV bug */
        size_t gray_size = (size_t)img_width * (size_t)img_height;
        gray_data = (uint8_t *)malloc(gray_size);
        if (!gray_data) {
            fprintf(stderr, "Error: Failed to allocate grayscale buffer\n");
            free(packed_rgb);
            encoder_free(encoder);
            return -1;
        }
        
        /* Convert RGB to grayscale using standard formula: Y = 0.299*R + 0.587*G + 0.114*B
         * Must account for rowstride when accessing pixels */
        if (packed_rgb) {
            /* If we packed the RGB data, it's sequential (no rowstride) */
            for (uint32_t i = 0; i < img_width * img_height; i++) {
                uint32_t r = rgb_data[i * 3u + 0u];
                uint32_t g = rgb_data[i * 3u + 1u];
                uint32_t b = rgb_data[i * 3u + 2u];
                gray_data[i] = (uint8_t)((299u * r + 587u * g + 114u * b) / 1000u);
            }
        } else {
            /* Original buffer with rowstride - access by row and column */
            uint32_t bytes_per_pixel = (img_width > 0 && img_buf->rowstride > 0) ? 
                                        ((uint32_t)img_buf->rowstride / img_width) : 3;
            for (uint32_t y = 0; y < img_height; y++) {
                for (uint32_t x = 0; x < img_width; x++) {
                    const uint8_t *p = rgb_data + (size_t)y * (size_t)img_buf->rowstride + 
                                                   (size_t)x * (size_t)bytes_per_pixel;
                    uint32_t r = p[0];
                    uint32_t g = p[1];
                    uint32_t b = p[2];
                    uint32_t idx = y * img_width + x;
                    gray_data[idx] = (uint8_t)((299u * r + 587u * g + 114u * b) / 1000u);
                }
            }
        }
        
        /* Workaround: Expand grayscale to RGB24 and use image_from_rgb */
        /* MMSSTV library appears to have a bug in image_from_gray - bottom half darker */
        size_t rgb24_size = (size_t)img_width * (size_t)img_height * 3;
        rgb24_from_gray = (uint8_t *)malloc(rgb24_size);
        if (!rgb24_from_gray) {
            fprintf(stderr, "Error: Failed to allocate RGB24 buffer\n");
            free(gray_data);
            free(packed_rgb);
            encoder_free(encoder);
            return -1;
        }
        
        /* Expand grayscale to RGB (R=G=B=gray_value) */
        for (uint32_t i = 0; i < img_width * img_height; i++) {
            uint8_t gray = gray_data[i];
            rgb24_from_gray[i * 3 + 0] = gray;
            rgb24_from_gray[i * 3 + 1] = gray;
            rgb24_from_gray[i * 3 + 2] = gray;
        }
        
        /* Expanded grayscale to RGB24 for B/W mode (MMSSTV workaround) */
        sstv_img = image_from_rgb(rgb24_from_gray, img_width, img_height);
    } else {
        /* Color mode - use RGB data */
        sstv_img = image_from_rgb(rgb_data, img_width, img_height);
    }
    
    /* Set image on encoder */
    if (set_image(encoder, &sstv_img) != 0) {
        fprintf(stderr, "Error: Failed to set image on MMSSTV encoder\n");
        free(rgb24_from_gray);
        free(gray_data);
        free(packed_rgb);
        encoder_free(encoder);
        return -1;
    }
    
    /* Allocate temporary buffers for float and converted samples */
    const size_t chunk_size = (max_samples < 4096u) ? max_samples : 4096u;
    float *float_samples = (float *)malloc(chunk_size * sizeof(float));
    uint16_t *converted_samples = (uint16_t *)malloc(chunk_size * sizeof(uint16_t));
    if (!float_samples) {
        fprintf(stderr, "Error: Failed to allocate float sample buffer\n");
        encoder_free(encoder);
        return -1;
    }
    if (!converted_samples) {
        fprintf(stderr, "Error: Failed to allocate converted sample buffer\n");
        free(float_samples);
        encoder_free(encoder);
        return -1;
    }
    
    /* Generate samples in chunks until complete */
    size_t total_generated = 0;
    while (!encoder_is_complete(encoder)) {
        if (total_generated >= max_samples) {
            fprintf(stderr, "Error: MMSSTV sample buffer overflow\n");
            free(converted_samples);
            free(float_samples);
            free(packed_rgb);
            encoder_free(encoder);
            return -1;
        }
        
        size_t remaining = max_samples - total_generated;
        size_t request = (remaining < chunk_size) ? remaining : chunk_size;
        size_t samples_generated = encoder_generate(encoder, float_samples, request);
        if (samples_generated == 0) {
            if (encoder_is_complete(encoder)) {
                break;
            }
            fprintf(stderr, "Error: MMSSTV encoder returned no samples\n");
            free(converted_samples);
            free(float_samples);
            free(packed_rgb);
            encoder_free(encoder);
            return -1;
        }
        
        /* Convert float samples (-1.0 to +1.0) to uint16_t (0 to 65535)
         * Center at 32768 (0x8000) for unsigned 16-bit */
        for (size_t i = 0; i < samples_generated; i++) {
            float sample = float_samples[i];
            /* Clamp to [-1.0, +1.0] */
            if (sample < -1.0f) sample = -1.0f;
            if (sample > 1.0f) sample = 1.0f;
            /* Convert to signed 16-bit PCM (matches native encoder format) */
            float scaled = sample * (0.65f * 32767.0f);
            int32_t voltage = (int32_t)lrintf(scaled);
            if (voltage < -32768) voltage = -32768;
            if (voltage > 32767) voltage = 32767;
            converted_samples[i] = (uint16_t)((int16_t)voltage);
        }
        
        /* Add samples to SSTV buffer */
        int add_result = sstv_add_samples_to_buffer(converted_samples, (uint32_t)samples_generated);
        if (add_result != 0) {  /* SLOWFRAME_OK is 0 */
            fprintf(stderr, "Error: Failed to add MMSSTV samples to buffer\n");
            free(converted_samples);
            free(float_samples);
            free(rgb24_from_gray);
            free(gray_data);
            free(packed_rgb);
            encoder_free(encoder);
            return add_result;
        }
        
        total_generated += samples_generated;
    }
    
    /* Cleanup */
    free(converted_samples);
    free(float_samples);
    free(rgb24_from_gray);
    free(gray_data);
    free(packed_rgb);
    encoder_free(encoder);

    return 0;  /* SLOWFRAME_OK */
}
