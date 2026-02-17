/**
 * @file modes_scottie.h
 * @brief Scottie S1, S2, and SDX SSTV mode definitions
 *
 * Scottie modes are RGB-based color SSTV modes with different scan line
 * ordering compared to Martin (Green-Red-Blue).
 *
 * @author SlowFrame Contributors
 * @version 2.1.0
 * @date February 2026
 */

#ifndef MODES_SCOTTIE_H
#define MODES_SCOTTIE_H

#include "sstv/mode_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register all Scottie mode definitions with the registry
 *
 * Registers:
 * - Scottie S1 (320x256, 110.080s, VIS 0x3C/60)
 * - Scottie S2 (320x256, 71.680s, VIS 0x38/56)
 * - Scottie DX (320x256, 268.800s, VIS 0x4C/76)
 *
 * @param reg Mode registry to populate
 */
void modes_scottie_register(mode_registry_t *reg);

#ifdef __cplusplus
}
#endif

#endif /* MODES_SCOTTIE_H */
