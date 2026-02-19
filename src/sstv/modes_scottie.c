/**
 * @file modes_scottie.c
 * @brief Scottie mode definitions and registration
 *
 * @author SlowFrame Contributors
 * @version 2.1.0
 * @date February 2026
 */

#include "modes_scottie.h"
#include "slowframe_sstv.h"
#include "error.h"

/*
 * NOTE: encode_frame function pointers now use registry-backed
 * dispatch wrappers defined in slowframe_sstv.c.
 */

/**
 * @brief Scottie S1 mode definition
 *
 * - Resolution: 320x256
 * - Duration: 110.080 seconds
 * - VIS Code: 60 (0x3C)
 * - Pixel time: 432.0 μs
 * - Color: RGB (Green-Red-Blue scan order)
 */
static const mode_definition_t scottie_s1 = {
    .code = "s1",
    .name = "Scottie 1",
    .vis_code = 60,
    .width = 320,
    .height = 256,
    .duration_sec = 110.080,
    .is_color = 1,
    .source = "native",
    .encode_frame = sstv_encode_scottie_s1,
    .mmsstv_mode_enum = -1
};

/**
 * @brief Scottie S2 mode definition
 *
 * - Resolution: 320x256
 * - Duration: 71.680 seconds
 * - VIS Code: 56 (0x38)
 * - Pixel time: 275.2 µs
 * - Color: RGB (faster than S1)
 */
static const mode_definition_t scottie_s2 = {
    .code = "s2",
    .name = "Scottie 2",
    .vis_code = 56,
    .width = 320,
    .height = 256,
    .duration_sec = 71.680,
    .is_color = 1,
    .source = "native",
    .encode_frame = sstv_encode_scottie_s2,
    .mmsstv_mode_enum = -1
};

/**
 * @brief Scottie DX mode definition
 *
 * - Resolution: 320x256
 * - Duration: 268.800 seconds
 * - VIS Code: 76 (0x4C)
 * - Pixel time: 1080.0 µs
 * - Color: RGB (high quality, slow speed)
 */
static const mode_definition_t scottie_dx = {
    .code = "sdx",
    .name = "Scottie DX",
    .vis_code = 76,
    .width = 320,
    .height = 256,
    .duration_sec = 268.800,
    .is_color = 1,
    .source = "native",
    .encode_frame = sstv_encode_scottie_sdx,
    .mmsstv_mode_enum = -1
};

/**
 * @brief Register Scottie modes with the registry
 */
void modes_scottie_register(mode_registry_t *reg) {
    if (!reg) {
        error_log(SLOWFRAME_ERR_ARG_INVALID, "NULL registry in modes_scottie_register");
        return;
    }

    int result = mode_registry_add(reg, &scottie_s1);
    if (result != 0) {
        error_log(SLOWFRAME_ERR_SSTV_INIT,
                  "Failed to register Scottie S1 (error %d)", result);
    }

    result = mode_registry_add(reg, &scottie_s2);
    if (result != 0) {
        error_log(SLOWFRAME_ERR_SSTV_INIT,
                  "Failed to register Scottie S2 (error %d)", result);
    }

    result = mode_registry_add(reg, &scottie_dx);
    if (result != 0) {
        error_log(SLOWFRAME_ERR_SSTV_INIT,
                  "Failed to register Scottie DX (error %d)", result);
    }
}
