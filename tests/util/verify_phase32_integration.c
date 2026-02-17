/**
 * @file verify_phase32_integration.c
 * @brief Phase 3.2 Integration Verification
 *
 * Verifies that the mode registry is properly integrated into the
 * SlowFrame context and that all native modes are registered at startup.
 *
 * @author SlowFrame Contributors
 * @version 2.1.0
 * @date February 2026
 */

#include <stdio.h>
#include <string.h>
#include "slowframe_context.h"
#include "slowframe_config.h"
#include "sstv/mode_registry.h"

int main(void) {
    printf("================================================================================\n");
    printf("PHASE 3.2 INTEGRATION VERIFICATION\n");
    printf("================================================================================\n\n");

    /* Initialize context (which creates and populates registry) */
    printf("[1/4] Initializing SlowFrame context...\n");
    SlowframeContext ctx;
    SlowframeConfig config;
    
    /* Use minimal config for verification */
    memset(&config, 0, sizeof(SlowframeConfig));
    strncpy(config.input_file, "dummy.png", sizeof(config.input_file) - 1);
    strncpy(config.output_file, "output.wav", sizeof(config.output_file) - 1);
    strncpy(config.protocol, "m1", sizeof(config.protocol) - 1);
    config.sample_rate = 22050;
    
    int result = slowframe_context_init(&ctx, &config);
    if (result != 0) {
        fprintf(stderr, "❌ FAILED: Could not initialize context (error %d)\n", result);
        return 1;
    }
    printf("✅ Context initialized\n\n");

    /* Get registry from context */
    printf("[2/4] Accessing mode registry from context...\n");
    mode_registry_t *reg = slowframe_context_get_mode_registry(&ctx);
    if (!reg) {
        fprintf(stderr, "❌ FAILED: Registry not found in context\n");
        slowframe_context_cleanup(&ctx);
        return 1;
    }
    printf("✅ Registry accessible\n\n");

    /* Verify all 7 native modes are registered */
    printf("[3/4] Verifying all 7 native modes are registered...\n");
    const char *expected_modes[] = {"m1", "m2", "s1", "s2", "sdx", "r36", "r72"};
    int all_found = 1;
    
    for (int i = 0; i < 7; i++) {
        const mode_definition_t *mode = mode_registry_lookup_by_code(reg, expected_modes[i]);
        if (!mode) {
            fprintf(stderr, "❌ FAILED: Mode %s not found\n", expected_modes[i]);
            all_found = 0;
        } else {
            printf("  ✓ %-4s - %s (VIS 0x%02X, %dx%d, %.1fs)\n",
                   mode->code, mode->name, mode->vis_code,
                   mode->width, mode->height, mode->duration_sec);
        }
    }
    
    if (!all_found) {
        slowframe_context_cleanup(&ctx);
        return 1;
    }
    printf("✅ All 7 native modes registered\n\n");

    /* Verify lookups work by code, name, and VIS */
    printf("[4/4] Testing lookup methods...\n");
    
    /* Lookup by code */
    const mode_definition_t *m1 = mode_registry_lookup_by_code(reg, "m1");
    if (!m1 || strcmp(m1->name, "Martin 1") != 0) {
        fprintf(stderr, "❌ FAILED: Lookup by code\n");
        slowframe_context_cleanup(&ctx);
        return 1;
    }
    printf("  ✓ Lookup by code: m1 -> %s\n", m1->name);
    
    /* Lookup by name */
    const mode_definition_t *s1 = mode_registry_lookup_by_name(reg, "Scottie 1");
    if (!s1 || strcmp(s1->code, "s1") != 0) {
        fprintf(stderr, "❌ FAILED: Lookup by name\n");
        slowframe_context_cleanup(&ctx);
        return 1;
    }
    printf("  ✓ Lookup by name: 'Scottie 1' -> %s\n", s1->code);
    
    /* Lookup by VIS code */
    const mode_definition_t *r36 = mode_registry_lookup_by_vis(reg, 8);
    if (!r36 || strcmp(r36->code, "r36") != 0) {
        fprintf(stderr, "❌ FAILED: Lookup by VIS\n");
        slowframe_context_cleanup(&ctx);
        return 1;
    }
    printf("  ✓ Lookup by VIS: 0x08 -> %s\n", r36->code);
    
    printf("✅ All lookup methods working\n\n");

    /* Cleanup */
    slowframe_context_cleanup(&ctx);

    printf("================================================================================\n");
    printf("✅ PHASE 3.2 INTEGRATION VERIFICATION PASSED\n");
    printf("================================================================================\n");
    printf("\n");
    printf("Summary:\n");
    printf("  • Mode registry successfully integrated into SlowFrame context\n");
    printf("  • All 7 native modes (Martin, Scottie, Robot) registered at startup\n");
    printf("  • Registry accessible via slowframe_context_get_mode_registry()\n");
    printf("  • Mode lookups working by code, name, and VIS code\n");
    printf("  • encode_frame pointers are NULL (Phase 3.4 will implement these)\n");
    printf("\n");
    printf("Next Steps:\n");
    printf("  • Phase 3.3: Add mode validation and error handling\n");
    printf("  • Phase 3.4: Refactor encoding dispatch to use mode registry\n");
    printf("\n");

    return 0;
}
