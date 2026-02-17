/**
 * @file modes_martin.c
 * @brief Martin M1 and M2 mode definitions and registration
 *
 * @author SlowFrame Contributors
 * @version 2.1.0
 * @date February 2026
 */

#include "modes_martin.h"
#include "slowframe_sstv.h"
#include "error.h"

/*
 * NOTE: encode_frame function pointers now use registry-backed
 * dispatch wrappers defined in slowframe_sstv.c.
 */

/**
 * @brief Martin M1 mode definition
 *
 * - Resolution: 320x256
 * - Duration: 114.286 seconds
 * - VIS Code: 44 (0x2C)
 * - Pixel time: 457.6 μs
 * - Color: RGB (separate scan lines)
 */
static const mode_definition_t martin_m1 = {
    .code = "m1",
    .name = "Martin 1",
    .vis_code = 44,
    .width = 320,
    .height = 256,
    .duration_sec = 114.286,
    .is_color = 1,
    .source = "native",
    .encode_frame = sstv_encode_martin_m1,
    .mmsstv_mode_enum = -1
};

/**
 * @brief Martin M2 mode definition
 *
 * - Resolution: 320x256
 * - Duration: 58.286 seconds
 * - VIS Code: 40 (0x28)
 * - Pixel time: 228.8 μs
 * - Color: RGB (separate scan lines, faster than M1)
 */
static const mode_definition_t martin_m2 = {
    .code = "m2",
    .name = "Martin 2",
    .vis_code = 40,
    .width = 320,
    .height = 256,
    .duration_sec = 58.286,
    .is_color = 1,
    .source = "native",
    .encode_frame = sstv_encode_martin_m2,
    .mmsstv_mode_enum = -1
};

/**
 * @brief Register Martin modes with the registry
 */
void modes_martin_register(mode_registry_t *reg) {
    if (!reg) {
        error_log(SLOWFRAME_ERR_ARG_INVALID, "NULL registry in modes_martin_register");
        return;
    }

    int result = mode_registry_add(reg, &martin_m1);
    if (result != 0) {
        error_log(SLOWFRAME_ERR_SSTV_INIT, 
                  "Failed to register Martin M1 (error %d)", result);
    }

    result = mode_registry_add(reg, &martin_m2);
    if (result != 0) {
        error_log(SLOWFRAME_ERR_SSTV_INIT,
                  "Failed to register Martin M2 (error %d)", result);
    }
}
