/**
 * @file mmsstv_loader.h
 * @brief Dynamic MMSSTV Library Loader
 * 
 * This module handles runtime detection and loading of the optional MMSSTV
 * library (libmmsstv.so / libmmsstv.dylib). It uses dlopen/dlsym for dynamic
 * loading, which means:
 *   - No compile-time dependency on MMSSTV library
 *   - Application works perfectly without MMSSTV installed
 *   - Library can be added/removed without recompiling
 * 
 * Detection Strategy:
 *   1. Check MMSSTV_LIB_PATH environment variable (highest priority)
 *   2. Try pkg-config for mmsstv-portable
 *   3. Search standard paths (/usr/local/lib, /usr/lib, etc.)
 *   4. Graceful fallback if not found (return NULL, not an error!)
 * 
 * Usage Example:
 *   ```c
 *   mmsstv_library_t *lib = mmsstv_loader_init();
 *   if (lib && mmsstv_loader_is_available(lib)) {
 *       // MMSSTV available, use it
 *       int mode_count = lib->functions.get_mode_count();
 *       printf("MMSSTV modes: %d\n", mode_count);
 *   } else {
 *       // MMSSTV not available, use native modes only
 *       printf("Using native modes only\n");
 *   }
 *   mmsstv_loader_destroy(lib);
 *   ```
 * 
 * @date February 15, 2026
 * @version 1.0
 */

#ifndef MMSSTV_LOADER_H
#define MMSSTV_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mmsstv_stub.h"
#include <stdbool.h>

/* ========================================================================
 * MMSSTV Library Handle
 * ======================================================================== */

/**
 * MMSSTV library handle (opaque)
 * 
 * Contains:
 *   - dlopen handle
 *   - Function pointer table
 *   - Library path
 *   - Version info
 *   - Status flags
 */
typedef struct mmsstv_library mmsstv_library_t;


/* ========================================================================
 * Library Detection & Loading
 * ======================================================================== */

/**
 * Initialize MMSSTV library loader
 * 
 * Attempts to detect and load MMSSTV library in this order:
 *   1. MMSSTV_LIB_PATH environment variable
 *   2. pkg-config --libs mmsstv-portable
 *   3. Standard system paths
 * 
 * If library is not found, this function SUCCEEDS and returns a valid
 * handle, but mmsstv_loader_is_available() will return false.
 * 
 * This design ensures the application always works, with or without MMSSTV.
 * 
 * @return Library handle (always non-NULL), call mmsstv_loader_destroy() when done
 */
mmsstv_library_t* mmsstv_loader_init(void);

/**
 * Check if MMSSTV library is successfully loaded
 * 
 * @param lib Library handle from mmsstv_loader_init()
 * @return true if library loaded and usable, false otherwise
 */
bool mmsstv_loader_is_available(const mmsstv_library_t *lib);

/**
 * Destroy library handle and cleanup resources
 * 
 * Unloads the library (if loaded) and frees memory.
 * Safe to call even if library was never loaded.
 * 
 * @param lib Library handle (may be NULL)
 */
void mmsstv_loader_destroy(mmsstv_library_t *lib);


/* ========================================================================
 * Library Information
 * ======================================================================== */

/**
 * Get library version string
 * 
 * Example: "1.0.0" or "1.0.0-beta"
 * 
 * @param lib Library handle
 * @return Version string, or NULL if library not loaded
 */
const char* mmsstv_loader_get_version(const mmsstv_library_t *lib);

/**
 * Get library API version
 * 
 * @param lib Library handle
 * @param major Output: major version
 * @param minor Output: minor version
 * @param patch Output: patch version
 * @return true if version retrieved, false if library not loaded
 */
bool mmsstv_loader_get_api_version(
    const mmsstv_library_t *lib,
    int *major,
    int *minor,
    int *patch
);

/**
 * Get path to loaded library file
 * 
 * @param lib Library handle
 * @return Path to .so/.dylib file, or NULL if not loaded
 */
const char* mmsstv_loader_get_path(const mmsstv_library_t *lib);

/**
 * Get detection status message
 * 
 * Returns human-readable message about library detection:
 *   - If loaded: "Loaded from /usr/local/lib/libmmsstv.so (v1.0.0)"
 *   - If not found: "Not detected (searched: env var, pkg-config, /usr/local/lib, /usr/lib)"
 * 
 * Useful for diagnostics and --mmsstv-status command.
 * 
 * @param lib Library handle
 * @param buffer Output buffer for status message
 * @param buffer_size Size of buffer
 * @return Number of characters written (excluding null terminator)
 */
int mmsstv_loader_get_status(
    const mmsstv_library_t *lib,
    char *buffer,
    int buffer_size
);


/* ========================================================================
 * Function Access
 * ======================================================================== */

/**
 * Get function table
 * 
 * Provides direct access to MMSSTV library functions.
 * 
 * @param lib Library handle
 * @return Pointer to function table, or NULL if library not loaded
 * 
 * @warning Caller must check mmsstv_loader_is_available() before using!
 * 
 * Example:
 *   ```c
 *   const mmsstv_functions_t *funcs = mmsstv_loader_get_functions(lib);
 *   if (funcs) {
 *       int count = funcs->get_mode_count();
 *   }
 *   ```
 */
const mmsstv_functions_t* mmsstv_loader_get_functions(const mmsstv_library_t *lib);


/* ========================================================================
 * Convenience Wrappers
 * ======================================================================== */

#ifdef __cplusplus
}
#endif

#endif /* MMSSTV_LOADER_H */
