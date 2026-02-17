/**
 * @file mode_registry.h
 * @brief SSTV Mode Registry - Unified mode definitions and lookup system
 *
 * ## Overview
 * The mode registry provides a centralized system for managing SSTV mode definitions.
 * It holds both native modes (built-in to SlowFrame) and dynamically loaded modes
 * from the MMSSTV library (if available).
 *
 * ## Design Goals
 * - **Unified Interface:** Single lookup mechanism for all modes regardless of source
 * - **Dynamic Loading:** Support runtime addition of MMSSTV modes
 * - **Type Safety:** Strong typing with mode_definition_t structure
 * - **Performance:** O(1) hash-based lookups where possible
 * - **Extensibility:** Easy to add new modes or mode sources
 *
 * ## Usage Example
 * @code
 * // Create registry
 * mode_registry_t *reg = mode_registry_create();
 * 
 * // Register native modes
 * mode_definition_t martin1 = {
 *     .code = "m1",
 *     .name = "Martin 1",
 *     .vis_code = 0xAC,
 *     .width = 320,
 *     .height = 256,
 *     .duration_sec = 114.286,
 *     .is_color = 1,
 *     .source = "native",
 *     .encode_frame = martin1_encode_frame
 * };
 * mode_registry_add(reg, &martin1);
 * 
 * // Lookup mode
 * const mode_definition_t *mode = mode_registry_lookup_by_code(reg, "m1");
 * if (mode) {
 *     printf("Mode: %s (%s)\n", mode->name, mode->code);
 * }
 * 
 * // List all modes
 * mode_list_t list = mode_registry_list_all(reg);
 * for (size_t i = 0; i < list.count; i++) {
 *     printf("%s\n", list.modes[i].name);
 * }
 * 
 * // Cleanup
 * mode_registry_free(reg);
 * @endcode
 *
 * @author SlowFrame Contributors
 * @version 2.1.0
 * @date February 2026
 */

#ifndef MODE_REGISTRY_H
#define MODE_REGISTRY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration of opaque registry type */
typedef struct mode_registry_s mode_registry_t;

/**
 * @brief SSTV Mode Definition Structure
 *
 * Describes a single SSTV mode with all metadata needed for encoding.
 * This structure is used for both native modes and MMSSTV library modes.
 */
typedef struct {
    /** Short mode code (e.g., "m1", "s1", "r36", "pd120") */
    const char *code;
    
    /** Full mode name (e.g., "Martin 1", "Scottie 1", "Robot 36") */
    const char *name;
    
    /** VIS code byte for auto-detection (e.g., 0xAC for Martin 1) */
    uint8_t vis_code;
    
    /** Required image width in pixels */
    uint32_t width;
    
    /** Required image height in pixels */
    uint32_t height;
    
    /** Encoding duration in seconds (e.g., 114.286 for Martin 1) */
    double duration_sec;
    
    /** Color mode flag: 1 = color, 0 = grayscale */
    int is_color;
    
    /** Mode source: "native" for built-in modes, "mmsstv" for library modes */
    const char *source;
    
    /** 
     * Function pointer for native mode encoding (NULL for MMSSTV modes)
     * 
     * @param mode_code    Mode identifier
     * @param sample_rate  Audio sample rate (Hz)
     * @param audio_buffer Output audio samples buffer
     * @param max_samples  Maximum buffer capacity
     * @return Number of samples generated, or error code
     */
    int (*encode_frame)(const char *mode_code,
                       uint16_t sample_rate,
                       uint16_t *audio_buffer,
                       uint32_t max_samples);
    
    /** Internal use: MMSSTV library mode enum value (if applicable) */
    int mmsstv_mode_enum;
} mode_definition_t;

/**
 * @brief Mode list structure for enumeration results
 */
typedef struct {
    const mode_definition_t *modes;  /**< Array of mode pointers */
    size_t count;                     /**< Number of modes in array */
} mode_list_t;

/*==============================================================================
 * REGISTRY LIFECYCLE
 *============================================================================*/

/**
 * @brief Create a new mode registry
 *
 * Allocates and initializes an empty mode registry. The registry must be
 * freed with mode_registry_free() when no longer needed.
 *
 * @return Pointer to new registry, or NULL on allocation failure
 */
mode_registry_t* mode_registry_create(void);

/**
 * @brief Free mode registry and all associated resources
 *
 * Frees the registry structure and all internal memory. Does not free
 * mode definitions themselves (they are typically static or stack-allocated).
 * NULL-safe - can be called with NULL pointer.
 *
 * @param reg Registry to free (can be NULL)
 */
void mode_registry_free(mode_registry_t *reg);

/*==============================================================================
 * MODE REGISTRATION
 *============================================================================*/

/**
 * @brief Add a mode to the registry
 *
 * Registers a new mode definition. The mode definition structure must remain
 * valid for the lifetime of the registry (typically static storage).
 * Duplicate mode codes are rejected.
 *
 * @param reg  Registry to add mode to
 * @param mode Mode definition to register (must be non-NULL)
 * @return 0 on success, negative error code on failure:
 *         -1 = invalid arguments (NULL pointers)
 *         -2 = duplicate mode code
 *         -3 = registry full (max 64 modes)
 */
int mode_registry_add(mode_registry_t *reg, const mode_definition_t *mode);

/*==============================================================================
 * MODE LOOKUP
 *============================================================================*/

/**
 * @brief Look up mode by short code
 *
 * Finds a mode by its short code identifier (e.g., "m1", "s1", "r36").
 * Lookup is case-insensitive.
 *
 * @param reg  Registry to search
 * @param code Mode code to find (e.g., "m1")
 * @return Pointer to mode definition, or NULL if not found
 */
const mode_definition_t* mode_registry_lookup_by_code(
    const mode_registry_t *reg,
    const char *code
);

/**
 * @brief Look up mode by full name
 *
 * Finds a mode by its full name (e.g., "Martin 1", "Scottie 1").
 * Lookup is case-insensitive and ignores extra whitespace.
 *
 * @param reg  Registry to search
 * @param name Mode name to find
 * @return Pointer to mode definition, or NULL if not found
 */
const mode_definition_t* mode_registry_lookup_by_name(
    const mode_registry_t *reg,
    const char *name
);

/**
 * @brief Look up mode by VIS code
 *
 * Finds a mode by its VIS code byte (e.g., 0xAC for Martin 1).
 * Note: Some modes may share VIS codes, so this returns the first match.
 *
 * @param reg      Registry to search
 * @param vis_code VIS code byte to find
 * @return Pointer to mode definition, or NULL if not found
 */
const mode_definition_t* mode_registry_lookup_by_vis(
    const mode_registry_t *reg,
    uint8_t vis_code
);

/*==============================================================================
 * MODE ENUMERATION
 *============================================================================*/

/**
 * @brief Get list of all registered modes
 *
 * Returns an array of all mode definitions in the registry. The returned
 * array is valid until the next call to mode_registry_add() or
 * mode_registry_free().
 *
 * @param reg Registry to enumerate
 * @return Mode list structure with array and count
 */
mode_list_t mode_registry_list_all(const mode_registry_t *reg);

/**
 * @brief Get total number of registered modes
 *
 * @param reg Registry to query
 * @return Number of modes currently registered
 */
int mode_registry_mode_count(const mode_registry_t *reg);

/**
 * @brief Get list of modes from a specific source
 *
 * Returns only modes from the specified source ("native" or "mmsstv").
 *
 * @param reg    Registry to search
 * @param source Source filter ("native" or "mmsstv")
 * @return Mode list structure with matching modes
 */
mode_list_t mode_registry_list_by_source(
    const mode_registry_t *reg,
    const char *source
);

/*==============================================================================
 * UTILITY FUNCTIONS
 *============================================================================*/

/**
 * @brief Check if a mode code exists in the registry
 *
 * @param reg  Registry to search
 * @param code Mode code to check
 * @return 1 if mode exists, 0 if not found
 */
int mode_registry_has_mode(
    const mode_registry_t *reg,
    const char *code
);

/**
 * @brief Print all registered modes (debug utility)
 *
 * Prints a formatted list of all modes with their metadata.
 * Useful for debugging and --list-modes CLI implementation.
 *
 * @param reg Registry to print
 */
void mode_registry_print_all(const mode_registry_t *reg);

#ifdef __cplusplus
}
#endif

#endif /* MODE_REGISTRY_H */
