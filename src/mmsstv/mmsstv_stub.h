/**
 * @file mmsstv_stub.h
 * @brief MMSSTV Library API Contract
 * 
 * This header defines the expected API from the mmsstv-portable library.
 * It serves as a stub/contract for what functions and structures we expect
 * from the dynamically loaded MMSSTV library.
 * 
 * This is NOT the actual MMSSTV library - it's our expectation of what
 * the library should provide. The real library implementation comes from
 * the mmsstv-portable project.
 * 
 * Usage:
 *   - Used by mmsstv_loader.c to resolve function pointers via dlsym()
 *   - Documents the contract between SlowFrame and MMSSTV library
 *   - Allows compilation without actual MMSSTV library present
 * 
 * @note This file is based on the mmsstv-portable library API specification
 * @note Library detection and loading is runtime-only (no compile-time dependency)
 * 
 * @date February 15, 2026
 * @version 1.0
 */

#ifndef MMSSTV_STUB_H
#define MMSSTV_STUB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ========================================================================
 * MMSSTV Library Version Info
 * ======================================================================== */

/**
 * Get MMSSTV library version string
 * 
 * Expected format: "1.0.0" or "1.0.0-beta"
 * 
 * @return Version string (static, do not free)
 */
typedef const char* (*mmsstv_get_version_fn)(void);

/**
 * Get library API version (for compatibility checking)
 * 
 * Major version breaks compatibility
 * Minor version adds features
 * Patch version fixes bugs
 * 
 * @param major Output: major version (e.g., 1)
 * @param minor Output: minor version (e.g., 0)
 * @param patch Output: patch version (e.g., 0)
 */
typedef void (*mmsstv_get_api_version_fn)(int *major, int *minor, int *patch);


/* ========================================================================
 * SSTV Mode Definitions (Real Library API)
 * ======================================================================== */

/**
 * SSTV Mode Enumeration (from real library)
 * 
 * Note: Real library uses enums, not string codes
 */
typedef enum {
    SSTV_R36 = 0, SSTV_R72, SSTV_AVT90,
    SSTV_SCOTTIE1, SSTV_SCOTTIE2, SSTV_SCOTTIEX,
    SSTV_MARTIN1, SSTV_MARTIN2,
    SSTV_SC2_180, SSTV_SC2_120, SSTV_SC2_60,
    SSTV_PD50, SSTV_PD90, SSTV_PD120, SSTV_PD160,
    SSTV_PD180, SSTV_PD240, SSTV_PD290,
    SSTV_P3, SSTV_P5, SSTV_P7,
    SSTV_MR73, SSTV_MR90, SSTV_MR115, SSTV_MR140, SSTV_MR175,
    SSTV_MP73, SSTV_MP115, SSTV_MP140, SSTV_MP175,
    SSTV_ML180, SSTV_ML240, SSTV_ML280, SSTV_ML320,
    SSTV_R24, SSTV_BW8, SSTV_BW12,
    SSTV_MN73, SSTV_MN110, SSTV_MN140,
    SSTV_MC110, SSTV_MC140, SSTV_MC180,
    SSTV_MODE_COUNT
} sstv_mode_t;

/**
 * SSTV Mode Information Structure (real library)
 * 
 * Describes a single SSTV mode with all parameters.
 */
typedef struct {
    sstv_mode_t mode;           /**< Mode enumeration value */
    const char *name;           /**< Human-readable name (e.g., "PD120") */
    uint32_t width;             /**< Image width in pixels */
    uint32_t height;            /**< Image height in pixels */
    uint8_t vis_code;           /**< VIS identification code */
    double duration_sec;        /**< Total transmission time (seconds) */
    int is_color;               /**< 1 = color, 0 = grayscale */
} sstv_mode_info_t;

/**
 * Pixel format enumeration
 */
typedef enum {
    SSTV_RGB24 = 0,        /**< 24-bit RGB (R, G, B bytes) */
    SSTV_GRAY8             /**< 8-bit grayscale */
} sstv_pixel_format_t;

/**
 * Image structure for encoding
 */
typedef struct {
    uint8_t *pixels;              /**< Image pixel data */
    uint32_t width;               /**< Image width in pixels */
    uint32_t height;              /**< Image height in pixels */
    uint32_t stride;              /**< Bytes per row */
    sstv_pixel_format_t format;   /**< Pixel format */
} sstv_image_t;

/**
 * Encoder handle (opaque structure)
 */
typedef struct sstv_encoder_s sstv_encoder_t;


/* ========================================================================
 * MMSSTV Mode Enumeration
 * ======================================================================== */

/**
 * Get all modes array (real library API)
 * 
 * @param count Output parameter for mode count
 * @return Pointer to mode table (43 modes)
 */
typedef const sstv_mode_info_t* (*sstv_get_all_modes_fn)(size_t *count);

/**
 * Get mode information by enum value
 * 
 * @param mode Mode enumeration value
 * @return Pointer to mode info, or NULL if invalid
 */
typedef const sstv_mode_info_t* (*sstv_get_mode_info_fn)(sstv_mode_t mode);

/**
 * Find mode by name (case-insensitive)
 * 
 * @param name Mode name (e.g., "Scottie 1", "PD120")
 * @return Mode enum value, or -1 if not found
 */
typedef int (*sstv_find_mode_by_name_fn)(const char *name);

/**
 * Get library version string
 * 
 * @return Version string (e.g., "1.0.0")
 */
typedef const char* (*sstv_encoder_version_fn)(void);


/* ========================================================================
 * Function Pointer Table
 * ======================================================================== */

/**
 * SSTV library function table (real library API)
 * 
 * This structure holds all function pointers loaded from the library.
 * The mmsstv_loader will populate this when loading the library.
 */
typedef struct mmsstv_functions {
    /* Version info */
    sstv_encoder_version_fn encoder_version;
    
    /* Mode enumeration */
    sstv_get_all_modes_fn get_all_modes;
    sstv_get_mode_info_fn get_mode_info;
    sstv_find_mode_by_name_fn find_mode_by_name;
    
} mmsstv_functions_t;


/* ========================================================================
 * Expected Symbol Names in Library
 * ======================================================================== */

/**
 * Symbol names to dlsym() from libmmsstv.so
 * 
 * These are the expected exported symbol names from the MMSSTV library.
 * The loader will use these strings with dlsym() to resolve function pointers.
 */
/* Real library symbol names (sstv_* not mmsstv_*) */
#define SSTV_SYM_ENCODER_VERSION    "sstv_encoder_version"
#define SSTV_SYM_GET_ALL_MODES      "sstv_get_all_modes"
#define SSTV_SYM_GET_MODE_INFO      "sstv_get_mode_info"
#define SSTV_SYM_FIND_MODE_BY_NAME  "sstv_find_mode_by_name"


/* ========================================================================
 * API Compatibility
 * ======================================================================== */

/**
 * Minimum required MMSSTV library API version
 * 
 * SlowFrame requires at least this API version to function correctly.
 */
#define MMSSTV_MIN_API_MAJOR  1
#define MMSSTV_MIN_API_MINOR  0
#define MMSSTV_MIN_API_PATCH  0


#ifdef __cplusplus
}
#endif

#endif /* MMSSTV_STUB_H */
