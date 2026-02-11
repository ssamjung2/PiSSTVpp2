/**
 * @file pisstvpp2_context.h
 * @brief PiSSTVpp2 Application Context and State Management
 *
 * This module provides centralized application state management and lifecycle
 * control for pisstvpp2. It encapsulates:
 *
 * - Application configuration (command-line parameters)
 * - Image processing state and resources
 * - SSTV encoding state and mode registration
 * - Audio encoder state and format selection
 * - Resource lifecycle (initialization and cleanup)
 *
 * ## Architecture
 *
 * The context module serves as the "glue" between configuration and execution
 * modules. It ensures all subsystems are initialized in the correct order and
 * cleaned up safely in reverse order.
 *
 * **Initialization Order:**
 * 1. Config module parses command-line arguments
 * 2. Context module initializes based on config
 * 3. Image module loads and processes image
 * 4. SSTV module encodes to audio samples
 * 5. Audio module writes samples to file
 *
 * **Cleanup Order (reverse):**
 * - Audio encoder cleanup
 * - SSTV module cleanup
 * - Image module cleanup
 * - Context cleanup
 *
 * ## Usage Pattern
 *
 * @code
 * PisstvppContext ctx;
 * int result = pisstvpp_context_init(&ctx, &config);
 * if (result != PISSTVPP2_OK) {
 *     error_log(result, "Failed to initialize context");
 *     return 1;
 * }
 *
 * // Modules now have access to config and state via context
 * result = process_image(&ctx);
 * if (result != PISSTVPP2_OK) {
 *     error_log(result, "Image processing failed");
 *     pisstvpp_context_cleanup(&ctx);
 *     return 1;
 * }
 *
 * pisstvpp_context_cleanup(&ctx);
 * @endcode
 *
 * ## Thread Safety
 *
 * The context is NOT inherently thread-safe. It's designed for single-threaded
 * use in the main application. If threading is added in the future:
 * - Config should be read-only after initialization (safe)
 * - Module states should be guarded by mutex locks
 *
 * ## Error Handling
 *
 * All context operations return error codes from the unified error system.
 * Errors during initialization trigger automatic cleanup of partially-
 * initialized subsystems.
 *
 * @author PiSSTVpp2 Contributors
 * @version 2.1.0
 * @date February 2026
 */

#ifndef PISSTVPP2_CONTEXT_H
#define PISSTVPP2_CONTEXT_H

#include "pisstvpp2_config.h"
#include <vips/vips.h>

// ===========================================================================
// FORWARD DECLARATIONS
// ===========================================================================

// Opaque pointers to module-specific state
typedef struct PisstvppImageState PisstvppImageState;
typedef struct PisstvppSSTVState PisstvppSSTVState;
typedef struct PisstvppAudioState PisstvppAudioState;

// ===========================================================================
// STRUCTURES
// ===========================================================================

/**
 * @brief Unified application context holding all module state
 *
 * This structure encapsulates the complete application state, including:
 * - Configuration from command-line arguments
 * - Module-specific state (opaque pointers)
 * - Lifecycle tracking flags
 *
 * **Initialization State:**
 * Each *_initialized flag tracks whether that subsystem has been set up.
 * This ensures cleanup operations only attempt to clean up initialized
 * subsystems, preventing double-free and null-pointer errors.
 *
 * **Module State Pointers:**
 * These are opaque (forward-declared) to avoid circular dependencies between
 * header files. Each module (image, SSTV, audio) defines its own state
 * structure internally, keeping implementation details private.
 *
 * **Configuration Reference:**
 * The context holds ownership of the config structure. It's initialized by
 * pisstvpp_context_init and cleaned up by pisstvpp_context_cleanup.
 */
typedef struct {
    // Configuration (owned by context)
    PisstvppConfig config;

    // Module state (opaque pointers to hide implementation)
    PisstvppImageState *image_state;        /**< Image processing module state */
    PisstvppSSTVState *sstv_state;          /**< SSTV encoding module state */
    PisstvppAudioState *audio_state;        /**< Audio encoder module state */

    // Lifecycle tracking
    int config_initialized;                 /**< Config parsing completed */
    int image_initialized;                  /**< Image module ready */
    int sstv_initialized;                   /**< SSTV module ready */
    int audio_initialized;                  /**< Audio encoder ready */

    // libvips state (shared across modules)
    int vips_initialized;                   /**< libvips is initialized */

} PisstvppContext;

// ===========================================================================
// PUBLIC FUNCTION DECLARATIONS
// ===========================================================================

/**
 * @brief Initialize application context from configuration
 *
 * Performs complete application initialization based on the provided config:
 * 1. Initializes libvips library
 * 2. Validates configuration
 * 3. Allocates and initializes image processing module
 * 4. Allocates and initializes SSTV encoding module
 * 5. Allocates and initializes audio encoder module
 *
 * **Error Handling:**
 * If any step fails, automatic cleanup of successfully initialized steps
 * is performed (in reverse order). The context is left in a clean state
 * suitable for immediate pisstvpp_context_cleanup().
 *
 * @param ctx Pointer to uninitialized PisstvppContext
 * @param config Pointer to valid PisstvppConfig (created bytes pisstvpp_config_parse)
 * @return Error code (PISSTVPP2_OK on success, error code on failure)
 *
 * @retval PISSTVPP2_OK Successfully initialized all subsystems
 * @retval PISSTVPP2_ERR_ARG_INVALID_PROTOCOL Config has invalid protocol
 * @retval PISSTVPP2_ERR_ARG_INVALID_FORMAT Config has invalid format
 * @retval PISSTVPP2_ERR_MEMORY_ALLOC Failed to allocate module state
 * @retval PISSTVPP2_ERR_SSTV_INIT SSTV module initialization failed
 * @retval PISSTVPP2_ERR_AUDIO_ENCODE Audio encoder initialization failed
 *
 * @note The context takes ownership of config's memory (does not copy)
 * @note libvips must not be already initialized (this function initializes it)
 */
int pisstvpp_context_init(PisstvppContext *ctx, PisstvppConfig *config);

/**
 * @brief Clean up application context and release all resources
 *
 * Performs orderly shutdown of all initialized subsystems:
 * 1. Audio encoder module cleanup
 * 2. SSTV module cleanup
 * 3. Image module cleanup
 * 4. libvips cleanup
 *
 * Each cleanup step only executes if that subsystem was successfully
 * initialized, preventing errors from double-cleanup attempts.
 *
 * @param ctx Pointer to PisstvppContext (may be partially initialized)
 *
 * @note Safe to call on:
 *   - Fully initialized context
 *   - Partially initialized context (skips uninitialized subsystems)
 *   - NULL pointer (no-op)
 *   - Already-cleaned-up context (no-op)
 * @note Idempotent: calling multiple times is safe
 */
void pisstvpp_context_cleanup(PisstvppContext *ctx);

/**
 * @brief Check if all required subsystems are initialized
 *
 * Performs a quick validation that the context is ready for use by
 * confirming all required initialization steps completed.
 *
 * @param ctx Pointer to PisstvppContext to check
 * @return 1 if fully initialized, 0 if any subsystem is missing
 *
 * @note This is primarily a safety check; in normal flow, successful
 *       pisstvpp_context_init() guarantees this will return 1
 */
int pisstvpp_context_is_valid(const PisstvppContext *ctx);

/**
 * @brief Get configuration from context (const access)
 *
 * Provides read-only access to the application configuration. All modules
 * should access config parameters through the context to ensure consistency.
 *
 * @param ctx Context to query
 * @return Pointer to const PisstvppConfig (guaranteed non-NULL)
 *
 * @note Return value is const; modules must request changes through
 *       a formal config update mechanism (not yet implemented)
 */
const PisstvppConfig* pisstvpp_context_get_config(const PisstvppContext *ctx);

/**
 * @brief Print context state to output for debugging
 *
 * Outputs a human-readable summary of the context state, including:
 * - Configuration settings
 * - Initialization status of each subsystem
 * - Module-specific state information
 *
 * Useful for debugging and verbose output modes.
 *
 * @param ctx Context to display
 *
 * @note Outputs to stdout via error_log and similar functions
 */
void pisstvpp_context_print_state(const PisstvppContext *ctx);

/**
 * @brief Get opaque pointer to image module state
 *
 * Provides access to image processing module state for qualified callers.
 * The type is opaque; only image.c should cast and use this pointer.
 *
 * @param ctx Context to query
 * @return Opaque pointer to image state (may be NULL if not initialized)
 *
 * @note Module state structure is defined in image.c; definition not
 *       exposed in header to prevent circular dependencies
 */
PisstvppImageState* pisstvpp_context_get_image_state(PisstvppContext *ctx);

/**
 * @brief Get opaque pointer to SSTV module state
 *
 * Provides access to SSTV encoding module state for qualified callers.
 * The type is opaque; only sstv.c should cast and use this pointer.
 *
 * @param ctx Context to query
 * @return Opaque pointer to SSTV state (may be NULL if not initialized)
 *
 * @note Module state structure is defined in sstv.c; definition not
 *       exposed in header to prevent circular dependencies
 */
PisstvppSSTVState* pisstvpp_context_get_sstv_state(PisstvppContext *ctx);

/**
 * @brief Get opaque pointer to audio encoder state
 *
 * Provides access to audio encoder module state for qualified callers.
 * The type is opaque; only audio_encoder.c should cast and use this pointer.
 *
 * @param ctx Context to query
 * @return Opaque pointer to audio state (may be NULL if not initialized)
 *
 * @note Module state structure is defined in audio_encoder.c; definition
 *       not exposed in header to prevent circular dependencies
 */
PisstvppAudioState* pisstvpp_context_get_audio_state(PisstvppContext *ctx);

#endif // PISSTVPP2_CONTEXT_H
