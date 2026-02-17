/**
 * @file modes_robot.c  
 * @brief Robot mode definitions and registration
 *
 * @author SlowFrame Contributors
 * @version 2.1.0
 * @date February 2026
 */

#include "modes_robot.h"
#include "slowframe_sstv.h"
#include "error.h"

/*
 * NOTE: encode_frame function pointers now use registry-backed
 * dispatch wrappers defined in slowframe_sstv.c.
 */

/**
 * @brief Robot 36 mode definition
 *
 * - Resolution: 320x240
 * - Duration: 36.0 seconds
 * - VIS Code: 8 (0x08)
 * - Color: YUV (fast mode)
 */
static const mode_definition_t robot_r36 = {
    .code = "r36",
    .name = "Robot 36",
    .vis_code = 8,
    .width = 320,
    .height = 240,
    .duration_sec = 36.0,
    .is_color = 1,
    .source = "native",
    .encode_frame = sstv_encode_robot_r36,
    .mmsstv_mode_enum = -1
};

/**
 * @brief Robot 72 mode definition
 *
 * - Resolution: 320x240
 * - Duration: 72.0 seconds
 * - VIS Code: 12 (0x0C)
 * - Color: YUV (higher quality than R36)
 */
static const mode_definition_t robot_r72 = {
    .code = "r72",
    .name = "Robot 72",
    .vis_code = 12,
    .width = 320,
    .height = 240,
    .duration_sec = 72.0,
    .is_color = 1,
    .source = "native",
    .encode_frame = sstv_encode_robot_r72,
    .mmsstv_mode_enum = -1
};

/**
 * @brief Robot B&W 24 mode definition
 *
 * - Resolution: 320x240
 * - Duration: 24.0 seconds
 * - VIS Code: 9 (0x09)
 * - Color: Monochrome (black & white)
 */
static const mode_definition_t robot_bw24 = {
    .code = "bw24",
    .name = "Robot B&W 24",
    .vis_code = 9,
    .width = 320,
    .height = 240,
    .duration_sec = 24.0,
    .is_color = 0,
    .source = "native",
    .encode_frame = sstv_encode_robot_bw24,
    .mmsstv_mode_enum = -1
};

/**
 * @brief Register Robot modes with the registry
 */
void modes_robot_register(mode_registry_t *reg) {
    if (!reg) {
        error_log(SLOWFRAME_ERR_ARG_INVALID, "NULL registry in modes_robot_register");
        return;
    }

    int result = mode_registry_add(reg, &robot_r36);
    if (result != 0) {
        error_log(SLOWFRAME_ERR_SSTV_INIT,
                  "Failed to register Robot 36 (error %d)", result);
    }

    result = mode_registry_add(reg, &robot_r72);
    if (result != 0) {
        error_log(SLOWFRAME_ERR_SSTV_INIT,
                  "Failed to register Robot 72 (error %d)", result);
    }

    result = mode_registry_add(reg, &robot_bw24);
    if (result != 0) {
        error_log(SLOWFRAME_ERR_SSTV_INIT,
                  "Failed to register Robot B&W 24 (error %d)", result);
    }
}
