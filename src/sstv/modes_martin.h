/**
 * @file modes_martin.h
 * @brief Martin M1 and M2 SSTV mode definitions
 *
 * Martin modes are RGB-based color SSTV modes with separate scan lines
 * for each color channel (Green-Blue-Red order).
 *
 * @author SlowFrame Contributors
 * @version 2.1.0
 * @date February 2026
 */

#ifndef MODES_MARTIN_H
#define MODES_MARTIN_H

#include "sstv/mode_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register all Martin mode definitions with the registry
 *
 * Registers:
 * - Martin M1 (320x256, 114.286s, VIS 0x2C/44)
 * - Martin M2 (320x256, 58.286s, VIS 0x28/40)
 *
 * @param reg Mode registry to populate
 */
void modes_martin_register(mode_registry_t *reg);

#ifdef __cplusplus
}
#endif

#endif /* MODES_MARTIN_H */
