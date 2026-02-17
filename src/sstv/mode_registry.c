/**
 * @file mode_registry.c
 * @brief SSTV Mode Registry Implementation
 *
 * Implements a simple, efficient mode registry using linear array storage.
 * For 50-64 modes, linear search is faster than hash tables due to cache locality.
 *
 * ## Design Notes
 * - Fixed capacity of 64 modes (more than enough for 7 native + 43 MMSSTV + future)
 * - Mode definitions are stored as pointers (registry doesn't own the data)
 * - Case-insensitive lookups using strcasecmp
 * - No dynamic reallocation - simple and predictable
 *
 * @author SlowFrame Contributors
 * @version 2.1.0
 * @date February 2026
 */

#include "sstv/mode_registry.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>  /* strcasecmp */
#include <stdio.h>
#include <ctype.h>

/* Maximum number of modes the registry can hold */
#define MODE_REGISTRY_MAX_MODES 64

/**
 * @brief Internal mode registry structure
 */
struct mode_registry_s {
    const mode_definition_t *modes[MODE_REGISTRY_MAX_MODES];
    size_t count;
};

/* Static buffer for mode lists returned by enumeration functions */
static mode_definition_t s_list_buffer[MODE_REGISTRY_MAX_MODES];

/*==============================================================================
 * REGISTRY LIFECYCLE
 *============================================================================*/

mode_registry_t* mode_registry_create(void) {
    mode_registry_t *reg = calloc(1, sizeof(mode_registry_t));
    if (!reg) {
        error_log(SLOWFRAME_ERR_MEMORY_ALLOC, "Failed to allocate mode registry");
        return NULL;
    }
    
    reg->count = 0;
    
    return reg;
}

void mode_registry_free(mode_registry_t *reg) {
    if (!reg) {
        return;
    }
    
    /* Clear the mode pointers (we don't own the mode definitions) */
    memset(reg->modes, 0, sizeof(reg->modes));
    reg->count = 0;
    
    /* Free the registry itself */
    free(reg);
}

/*==============================================================================
 * MODE REGISTRATION
 *============================================================================*/

int mode_registry_add(mode_registry_t *reg, const mode_definition_t *mode) {
    if (!reg || !mode) {
        error_log(SLOWFRAME_ERR_ARG_INVALID, 
                  "mode_registry_add: NULL registry or mode");
        return -1;
    }
    
    if (!mode->code || !mode->name) {
        error_log(SLOWFRAME_ERR_ARG_INVALID,
                  "mode_registry_add: mode missing code or name");
        return -1;
    }
    
    /* Check capacity */
    if (reg->count >= MODE_REGISTRY_MAX_MODES) {
        error_log(SLOWFRAME_ERR_ARG_INVALID,
                  "mode_registry_add: registry full (max %d modes)",
                  MODE_REGISTRY_MAX_MODES);
        return -3;
    }
    
    /* Check for duplicate code */
    for (size_t i = 0; i < reg->count; i++) {
        if (strcasecmp(reg->modes[i]->code, mode->code) == 0) {
            error_log(SLOWFRAME_ERR_ARG_INVALID,
                      "mode_registry_add: duplicate mode code '%s'",
                      mode->code);
            return -2;
        }
    }
    
    /* Add mode to registry */
    reg->modes[reg->count] = mode;
    reg->count++;
    
    return 0;
}

/*==============================================================================
 * MODE LOOKUP
 *============================================================================*/

const mode_definition_t* mode_registry_lookup_by_code(
    const mode_registry_t *reg,
    const char *code
) {
    if (!reg || !code) {
        return NULL;
    }
    
    for (size_t i = 0; i < reg->count; i++) {
        if (strcasecmp(reg->modes[i]->code, code) == 0) {
            return reg->modes[i];
        }
    }
    
    return NULL;
}

const mode_definition_t* mode_registry_lookup_by_name(
    const mode_registry_t *reg,
    const char *name
) {
    if (!reg || !name) {
        return NULL;
    }
    
    for (size_t i = 0; i < reg->count; i++) {
        if (strcasecmp(reg->modes[i]->name, name) == 0) {
            return reg->modes[i];
        }
    }
    
    return NULL;
}

const mode_definition_t* mode_registry_lookup_by_vis(
    const mode_registry_t *reg,
    uint8_t vis_code
) {
    if (!reg) {
        return NULL;
    }
    
    for (size_t i = 0; i < reg->count; i++) {
        if (reg->modes[i]->vis_code == vis_code) {
            return reg->modes[i];
        }
    }
    
    return NULL;
}

/*==============================================================================
 * MODE ENUMERATION
 *============================================================================*/

mode_list_t mode_registry_list_all(const mode_registry_t *reg) {
    mode_list_t list = { NULL, 0 };
    
    if (!reg || reg->count == 0) {
        return list;
    }
    
    /* Copy mode definitions to static buffer */
    for (size_t i = 0; i < reg->count; i++) {
        s_list_buffer[i] = *reg->modes[i];
    }
    
    list.modes = s_list_buffer;
    list.count = reg->count;
    
    return list;
}

int mode_registry_mode_count(const mode_registry_t *reg) {
    if (!reg) {
        return 0;
    }
    
    return (int)reg->count;
}

mode_list_t mode_registry_list_by_source(
    const mode_registry_t *reg,
    const char *source
) {
    mode_list_t list = { NULL, 0 };
    
    if (!reg || !source || reg->count == 0) {
        return list;
    }
    
    /* Filter modes by source */
    size_t match_count = 0;
    for (size_t i = 0; i < reg->count; i++) {
        if (reg->modes[i]->source &&
            strcasecmp(reg->modes[i]->source, source) == 0) {
            s_list_buffer[match_count] = *reg->modes[i];
            match_count++;
        }
    }
    
    if (match_count > 0) {
        list.modes = s_list_buffer;
        list.count = match_count;
    }
    
    return list;
}

/*==============================================================================
 * UTILITY FUNCTIONS
 *============================================================================*/

int mode_registry_has_mode(
    const mode_registry_t *reg,
    const char *code
) {
    return (mode_registry_lookup_by_code(reg, code) != NULL) ? 1 : 0;
}

void mode_registry_print_all(const mode_registry_t *reg) {
    if (!reg) {
        printf("Mode registry: NULL\n");
        return;
    }
    
    if (reg->count == 0) {
        printf("Mode registry: empty\n");
        return;
    }
    
    printf("Mode Registry - %zu modes registered:\n", reg->count);
    printf("%-10s %-20s %5s %9s %7s %8s %s\n",
           "Code", "Name", "VIS", "Size", "Time", "Color", "Source");
    printf("%-10s %-20s %5s %9s %7s %8s %s\n",
           "----", "----", "---", "----", "----", "-----", "------");
    
    for (size_t i = 0; i < reg->count; i++) {
        const mode_definition_t *m = reg->modes[i];
        printf("%-10s %-20s 0x%02X %4ux%-4u %6.1fs %8s %s\n",
               m->code,
               m->name,
               m->vis_code,
               m->width,
               m->height,
               m->duration_sec,
               m->is_color ? "color" : "b&w",
               m->source ? m->source : "unknown");
    }
}
