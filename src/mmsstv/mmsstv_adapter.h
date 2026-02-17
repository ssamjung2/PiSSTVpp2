/**
 * @file mmsstv_adapter.h
 * @brief MMSSTV Library Adapter
 * 
 * This module bridges between the MMSSTV library loader and SlowFrame's mode
 * registry. It converts MMSSTV library modes into mode_definition_t structures
 * and provides integration with the registry system.
 * 
 * Architecture:
 *   1. Initialize loader (may fail gracefully if library not found)
 *   2. Enumerate MMSSTV modes
 *   3. Create mode_definition_t for each mode
 *   4. Register modes with mode registry
 *   5. Provide encoder wrapper to bridge MMSSTV encoding API
 * 
 * Usage Example:
 *   ```c
 *   mode_registry_t *registry = mode_registry_create();
 *   
 *   // Register native modes first
 *   modes_martin_register(registry);
 *   modes_scottie_register(registry);
 *   modes_robot_register(registry);
 *   
 *   // Try to register MMSSTV modes (non-fatal if library absent)
 *   mmsstv_adapter_t *adapter = mmsstv_adapter_init();
 *   if (adapter) {
 *       int mmsstv_count = mmsstv_adapter_register_modes(adapter, registry);
 *       printf("Registered %d MMSSTV modes\n", mmsstv_count);
 *   }
 *   ```
 * 
 * @date February 15, 2026
 * @version 1.0
 */

#ifndef MMSSTV_ADAPTER_H
#define MMSSTV_ADAPTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mmsstv_loader.h"
#include "sstv/mode_registry.h"
#include <stdbool.h>

/* ========================================================================
 * MMSSTV Adapter Handle
 * ======================================================================== */

/**
 * MMSSTV adapter handle (opaque)
 * 
 * Contains:
 *   - MMSSTV library handle
 *   - Cached mode definitions
 *   - Availability status
 */
typedef struct mmsstv_adapter mmsstv_adapter_t;


/* ========================================================================
 * Adapter Initialization
 * ======================================================================== */

/**
 * Initialize MMSSTV adapter
 * 
 * Loads MMSSTV library and enumerates available modes. If library is not
 * found, creates an empty adapter (valid state, zero modes).
 * 
 * @return Adapter handle (always non-NULL), call mmsstv_adapter_destroy() when done
 */
mmsstv_adapter_t* mmsstv_adapter_init(void);

/**
 * Check if MMSSTV library is available
 * 
 * @param adapter Adapter handle from mmsstv_adapter_init()
 * @return true if library loaded and modes available, false otherwise
 */
bool mmsstv_adapter_is_available(const mmsstv_adapter_t *adapter);

/**
 * Destroy adapter and cleanup resources
 * 
 * Unloads library and frees memory.
 * 
 * @param adapter Adapter handle (may be NULL)
 */
void mmsstv_adapter_destroy(mmsstv_adapter_t *adapter);


/* ========================================================================
 * Mode Access
 * ======================================================================== */

/**
 * Get number of available MMSSTV modes
 * 
 * @param adapter Adapter handle
 * @return Mode count, or 0 if library not loaded
 */
int mmsstv_adapter_get_mode_count(const mmsstv_adapter_t *adapter);

/**
 * Get mode definition by index
 * 
 * Returns mode_definition_t suitable for registry.
 * 
 * @param adapter Adapter handle
 * @param index Mode index (0 to count-1)
 * @return Mode definition, or NULL if invalid index or library not loaded
 */
const mode_definition_t* mmsstv_adapter_get_mode(
    const mmsstv_adapter_t *adapter,
    int index
);


/* ========================================================================
 * Registry Integration
 * ======================================================================== */

/**
 * Register all MMSSTV modes with mode registry
 * 
 * Adds all MMSSTV modes to the registry. If library is not loaded,
 * this is a no-op (returns 0).
 * 
 * @param adapter Adapter handle
 * @param registry Mode registry to register into
 * @return Number of modes registered, or 0 if library not loaded
 */
int mmsstv_adapter_register_modes(
    mmsstv_adapter_t *adapter,
    mode_registry_t *registry
);


/* ========================================================================
 * Library Information
 * ======================================================================== */

/**
 * Get MMSSTV library version string
 * 
 * @param adapter Adapter handle
 * @return Version string (e.g., "1.0.0"), or NULL if library not loaded
 */
const char* mmsstv_adapter_get_version(const mmsstv_adapter_t *adapter);

/**
 * Get library status message
 * 
 * Returns human-readable status about library detection and loading.
 * Useful for diagnostics and --mmsstv-status command.
 * 
 * @param adapter Adapter handle
 * @param buffer Output buffer for status message
 * @param buffer_size Size of buffer
 * @return Number of characters written (excluding null terminator)
 */
int mmsstv_adapter_get_status(
    const mmsstv_adapter_t *adapter,
    char *buffer,
    int buffer_size
);

/**
 * Get path to loaded library
 * 
 * @param adapter Adapter handle
 * @return Path to library file, or NULL if not loaded
 */
const char* mmsstv_adapter_get_library_path(const mmsstv_adapter_t *adapter);


#ifdef __cplusplus
}
#endif

#endif /* MMSSTV_ADAPTER_H */
