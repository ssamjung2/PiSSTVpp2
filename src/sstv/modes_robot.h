/**
 * @file modes_robot.h
 * @brief Robot 36 and 72 SSTV mode definitions
 *
 * Robot modes are YUV-based color SSTV modes with fast transmission times.
 * They use YCrCb color space encoding.
 *
 * @author SlowFrame Contributors
 * @version 2.1.0
 * @date February 2026
 */

#ifndef MODES_ROBOT_H
#define MODES_ROBOT_H

#include "sstv/mode_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register all Robot mode definitions with the registry
 *
 * Registers:
 * - Robot 36 (320x240, 36.0s, VIS 0x08/8)
 * - Robot 72 (320x240, 72.0s, VIS 0x0C/12)
 * - Robot B&W 24 (320x240, 24.0s, VIS 0x09/9)
 *
 * @param reg Mode registry to populate
 */
void modes_robot_register(mode_registry_t *reg);

#ifdef __cplusplus
}
#endif

#endif /* MODES_ROBOT_H */
