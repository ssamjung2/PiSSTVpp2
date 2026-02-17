/**
 * @file mmsstv_loader.c
 * @brief MMSSTV Library Loader Implementation
 * 
 * @date February 15, 2026
 * @version 1.0
 */

#include "mmsstv_loader.h"
#include "mmsstv_stub.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>   /* dlopen, dlsym, dlclose, dlerror */
#include <unistd.h>  /* access */

/* ========================================================================
 * Internal Structures
 * ======================================================================== */

/**
 * MMSSTV library handle structure
 */
struct mmsstv_library {
    void *dl_handle;                /**< dlopen handle (NULL if not loaded) */
    mmsstv_functions_t functions;   /**< Function pointer table */
    char library_path[512];         /**< Path to loaded library */
    char version[64];               /**< Library version string */
    char status_message[256];       /**< Status/error message */
    bool is_loaded;                 /**< Successfully loaded flag */
    int api_major;                  /**< API major version */
    int api_minor;                  /**< API minor version */
    int api_patch;                  /**< API patch version */
};

/* ========================================================================
 *Forward Declarations
 * ======================================================================== */

static bool try_load_library(mmsstv_library_t *lib, const char *path);
static bool resolve_symbols(mmsstv_library_t *lib);
static bool check_api_compatibility(mmsstv_library_t *lib);
static const char* find_library_path(mmsstv_library_t *lib);
static bool file_exists(const char *path);

/* ========================================================================
 * Library Initialization
 * ======================================================================== */

mmsstv_library_t* mmsstv_loader_init(void) {
    /* Allocate handle */
    mmsstv_library_t *lib = calloc(1, sizeof(mmsstv_library_t));
    if (!lib) {
        return NULL;
    }
    
    /* Initialize to empty state */
    lib->dl_handle = NULL;
    lib->is_loaded = false;
    lib->api_major = 0;
    lib->api_minor = 0;
    lib->api_patch = 0;
    memset(&lib->functions, 0, sizeof(mmsstv_functions_t));
    snprintf(lib->status_message, sizeof(lib->status_message), 
             "Not initialized");
    
    /* Try to find and load library */
    const char *library_path = find_library_path(lib);
    
    if (library_path) {
        if (try_load_library(lib, library_path)) {
            /* Successfully loaded */
            snprintf(lib->status_message, sizeof(lib->status_message),
                     "Loaded from %s (v%s)",
                     lib->library_path,
                     lib->version[0] ? lib->version : "unknown");
        }
    } else {
        /* Library not found - this is OK! */
        snprintf(lib->status_message, sizeof(lib->status_message),
                 "Not detected (searched: $MMSSTV_LIB_PATH, pkg-config, "
                 "/usr/local/lib, /usr/lib, /opt/mmsstv/lib)");
    }
    
    return lib;
}

bool mmsstv_loader_is_available(const mmsstv_library_t *lib) {
    return lib && lib->is_loaded;
}

void mmsstv_loader_destroy(mmsstv_library_t *lib) {
    if (!lib) {
        return;
    }
    
    /* Close library (real library doesn't need shutdown call) */
    if (lib->dl_handle) {
        dlclose(lib->dl_handle);
    }
    
    /* Free handle */
    free(lib);
}

/* ========================================================================
 * Library Information
 * ======================================================================== */

const char* mmsstv_loader_get_version(const mmsstv_library_t *lib) {
    if (!lib || !lib->is_loaded) {
        return NULL;
    }
    return lib->version[0] ? lib->version : NULL;
}

bool mmsstv_loader_get_api_version(
    const mmsstv_library_t *lib,
    int *major,
    int *minor,
    int *patch
) {
    if (!lib || !lib->is_loaded) {
        return false;
    }
    
    if (major) *major = lib->api_major;
    if (minor) *minor = lib->api_minor;
    if (patch) *patch = lib->api_patch;
    
    return true;
}

const char* mmsstv_loader_get_path(const mmsstv_library_t *lib) {
    if (!lib || !lib->is_loaded) {
        return NULL;
    }
    return lib->library_path[0] ? lib->library_path : NULL;
}

int mmsstv_loader_get_status(
    const mmsstv_library_t *lib,
    char *buffer,
    int buffer_size
) {
    if (!lib || !buffer || buffer_size <= 0) {
        return 0;
    }
    
    return snprintf(buffer, buffer_size, "%s", lib->status_message);
}

/* ========================================================================
 * Function Access
 * ======================================================================== */

const mmsstv_functions_t* mmsstv_loader_get_functions(const mmsstv_library_t *lib) {
    if (!lib || !lib->is_loaded) {
        return NULL;
    }
    return &lib->functions;
}

/* ========================================================================
 * Internal: Library Detection
 * ======================================================================== */

/**
 * Find library path via detection strategy
 * 
 * Priority order:
 *   1. MMSSTV_LIB_PATH environment variable
 *   2. pkg-config --libs mmsstv-portable
 *   3. Standard paths
 * 
 * @param lib Library handle (for logging, currently unused)
 * @return Path to library, or NULL if not found
 */
static const char* find_library_path(mmsstv_library_t *lib) {
    (void)lib;  /* Unused parameter - reserved for future logging */
    static char path_buffer[512];
    
    /* Strategy 1: MMSSTV_LIB_PATH environment variable */
    const char *env_path = getenv("MMSSTV_LIB_PATH");
    if (env_path && file_exists(env_path)) {
        snprintf(path_buffer, sizeof(path_buffer), "%s", env_path);
        return path_buffer;
    }
    
    /* Strategy 2: pkg-config */
    FILE *pkg_config = popen("pkg-config --variable=libdir mmsstv-portable 2>/dev/null", "r");
    if (pkg_config) {
        char pkg_path[512];
        if (fgets(pkg_path, sizeof(pkg_path), pkg_config)) {
            /* Remove trailing newline */
            pkg_path[strcspn(pkg_path, "\n")] = '\0';
            
            /* Try multiple library name variants */
            const char *lib_names[] = {
#ifdef __APPLE__
                "libsstv_encoder.1.0.0.dylib",
                "libsstv_encoder.dylib",
                "libmmsstv.dylib",
#else
                "libsstv_encoder.so.1.0.0",
                "libsstv_encoder.so",
                "libmmsstv.so",
#endif
                NULL
            };
            
            for (int i = 0; lib_names[i]; i++) {
                snprintf(path_buffer, sizeof(path_buffer), "%s/%s", pkg_path, lib_names[i]);
                if (file_exists(path_buffer)) {
                    pclose(pkg_config);
                    return path_buffer;
                }
            }
        }
        pclose(pkg_config);
    }
    
    /* Strategy 3: Standard paths */
    const char *standard_paths[] = {
#ifdef __APPLE__
        /* Actual library name variants */
        "/usr/local/lib/libsstv_encoder.1.0.0.dylib",
        "/usr/local/lib/libsstv_encoder.dylib",
        "/opt/homebrew/lib/libsstv_encoder.1.0.0.dylib",
        "/opt/homebrew/lib/libsstv_encoder.dylib",
        "../mmsstv-portable/build/libsstv_encoder.1.0.0.dylib",
        "../mmsstv-portable/build/libsstv_encoder.dylib",
        /* Legacy/fallback names */
        "/usr/local/lib/libmmsstv.dylib",
        "/opt/homebrew/lib/libmmsstv.dylib",
        "/usr/lib/libmmsstv.dylib",
        "/opt/mmsstv/lib/libmmsstv.dylib",
        "./lib/libmmsstv.dylib",
#else
        /* Actual library name variants */
        "/usr/local/lib/libsstv_encoder.so.1.0.0",
        "/usr/local/lib/libsstv_encoder.so",
        "../mmsstv-portable/build/libsstv_encoder.so.1.0.0",
        "../mmsstv-portable/build/libsstv_encoder.so",
        /* Legacy/fallback names */
        "/usr/local/lib/libmmsstv.so",
        "/usr/lib/libmmsstv.so",
        "/opt/mmsstv/lib/libmmsstv.so",
        "./lib/libmmsstv.so",
#endif
        NULL
    };
    
    for (int i = 0; standard_paths[i]; i++) {
        if (file_exists(standard_paths[i])) {
            snprintf(path_buffer, sizeof(path_buffer), "%s", standard_paths[i]);
            return path_buffer;
        }
    }
    
    /* Not found */
    return NULL;
}

/**
 * Check if file exists and is accessible
 * 
 * @param path Path to check
 * @return true if exists, false otherwise
 */
static bool file_exists(const char *path) {
    return path && access(path, R_OK) == 0;
}

/* ========================================================================
 * Internal: Library Loading
 * ======================================================================== */

/**
 * Try to load library from specific path
 * 
 * @param lib Library handle
 * @param path Path to library file
 * @return true on success, false on failure
 */
static bool try_load_library(mmsstv_library_t *lib, const char *path) {
    /* Try to open library */
    lib->dl_handle = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    
    if (!lib->dl_handle) {
        snprintf(lib->status_message, sizeof(lib->status_message),
                 "Failed to load %s: %s", path, dlerror());
        return false;
    }
    
    /* Store path */
    snprintf(lib->library_path, sizeof(lib->library_path), "%s", path);
    
    /* Resolve function symbols */
    if (!resolve_symbols(lib)) {
        dlclose(lib->dl_handle);
        lib->dl_handle = NULL;
        return false;
    }
    
    /* Get version info */
    if (lib->functions.encoder_version) {
        const char *version = lib->functions.encoder_version();
        if (version) {
            snprintf(lib->version, sizeof(lib->version), "%s", version);
        }
    }
    
    /* API version check not needed (real library is 1.0.0) */
    lib->api_major = 1;
    lib->api_minor = 0;
    lib->api_patch = 0;
    
    /* Check API compatibility */
    if (!check_api_compatibility(lib)) {
        dlclose(lib->dl_handle);
        lib->dl_handle = NULL;
        return false;
    }
    
    /* Success! */
    lib->is_loaded = true;
    return true;
}

/**
 * Resolve all function symbols from library
 * 
 * @param lib Library handle
 * @return true if all required symbols found, false otherwise
 */
static bool resolve_symbols(mmsstv_library_t *lib) {
    if (!lib || !lib->dl_handle) {
        return false;
    }
    
    /* Clear dlerror */
    dlerror();
    
    /* Macro to load symbol and check for error */
    /* Note: dlsym returns void*, which we assign to function pointers.
     * This is standard practice and safe, but triggers -Wpedantic warnings.
     * Suppress these warnings for the dlsym assignments. */
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    
    #define LOAD_SYMBOL(field, symbol_name, required) \
        lib->functions.field = dlsym(lib->dl_handle, symbol_name); \
        if (!lib->functions.field && required) { \
            snprintf(lib->status_message, sizeof(lib->status_message), \
                     "Missing required symbol: %s", symbol_name); \
            return false; \
        }
    
    /* Load required symbols for real library API */
    LOAD_SYMBOL(get_all_modes, SSTV_SYM_GET_ALL_MODES, true);
    LOAD_SYMBOL(get_mode_info, SSTV_SYM_GET_MODE_INFO, true);
    
    /* Load optional symbols */
    LOAD_SYMBOL(encoder_version, SSTV_SYM_ENCODER_VERSION, false);
    LOAD_SYMBOL(find_mode_by_name, SSTV_SYM_FIND_MODE_BY_NAME, false);
    
    #undef LOAD_SYMBOL
    
    #pragma GCC diagnostic pop
    
    return true;
}

/**
 * Check API compatibility
 * 
 * For the real library, we always return true since we know it's version 1.0.0.
 * 
 * @param lib Library handle
 * @return true (always compatible with real library)
 */
static bool check_api_compatibility(mmsstv_library_t *lib) {
    /* Real library is always compatible */
    (void)lib;  /* Suppress unused parameter warning */
    return true;
}
