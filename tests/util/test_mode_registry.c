/**
 * @file test_mode_registry.c
 * @brief Comprehensive test suite for SSTV mode registry
 *
 * Tests all registry operations with realistic mode definitions.
 *
 * @author SlowFrame Contributors
 * @version 2.1.0
 * @date February 2026
 */

#include "sstv/mode_registry.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Test counter */
static int tests_run = 0;
static int tests_passed = 0;

/* Test result macro */
#define TEST(name) \
    do { \
        printf("TEST: %s ... ", name); \
        tests_run++; \
    } while (0)

#define PASS() \
    do { \
        printf("✅ PASS\n"); \
        tests_passed++; \
    } while (0)

#define FAIL(msg, ...) \
    do { \
        printf("❌ FAIL: "); \
        printf(msg, ##__VA_ARGS__); \
        printf("\n"); \
    } while (0)

/* Dummy encode function for testing */
static int dummy_encode(const char *mode_code, uint16_t sample_rate,
                       uint16_t *audio_buffer, uint32_t max_samples) {
    (void)mode_code;
    (void)sample_rate;
    (void)audio_buffer;
    (void)max_samples;
    return 0;
}

/* Sample mode definitions for testing */
static const mode_definition_t martin1 = {
    .code = "m1",
    .name = "Martin 1",
    .vis_code = 0xAC,
    .width = 320,
    .height = 256,
    .duration_sec = 114.286,
    .is_color = 1,
    .source = "native",
    .encode_frame = dummy_encode,
    .mmsstv_mode_enum = -1
};

static const mode_definition_t scottie1 = {
    .code = "s1",
    .name = "Scottie 1",
    .vis_code = 0x3C,
    .width = 320,
    .height = 256,
    .duration_sec = 110.080,
    .is_color = 1,
    .source = "native",
    .encode_frame = dummy_encode,
    .mmsstv_mode_enum = -1
};

static const mode_definition_t robot36 = {
    .code = "r36",
    .name = "Robot 36",
    .vis_code = 0x08,
    .width = 320,
    .height = 240,
    .duration_sec = 36.0,
    .is_color = 1,
    .source = "native",
    .encode_frame = dummy_encode,
    .mmsstv_mode_enum = -1
};

static const mode_definition_t pd120 = {
    .code = "pd120",
    .name = "PD 120",
    .vis_code = 0x63,
    .width = 640,
    .height = 496,
    .duration_sec = 126.0,
    .is_color = 1,
    .source = "mmsstv",
    .encode_frame = NULL,  /* MMSSTV modes don't have native encoders */
    .mmsstv_mode_enum = 15
};

/*==============================================================================
 * TEST CASES
 *============================================================================*/

/**
 * Test: Registry Lifecycle
 */
static void test_lifecycle(void) {
    TEST("registry lifecycle (create/free)");
    
    mode_registry_t *reg = mode_registry_create();
    if (!reg) {
        FAIL("Failed to create registry");
        return;
    }
    
    if (mode_registry_mode_count(reg) != 0) {
        FAIL("New registry should be empty, got count=%d",
             mode_registry_mode_count(reg));
        mode_registry_free(reg);
        return;
    }
    
    mode_registry_free(reg);
    PASS();
}

/**
 * Test: NULL Safety
 */
static void test_null_safety(void) {
    TEST("NULL pointer safety");
    
    /* Free NULL registry should not crash */
    mode_registry_free(NULL);
    
    /* Operations on NULL should return safe values */
    if (mode_registry_mode_count(NULL) != 0) {
        FAIL("mode_count(NULL) should return 0");
        return;
    }
    
    if (mode_registry_lookup_by_code(NULL, "m1") != NULL) {
        FAIL("lookup on NULL registry should return NULL");
        return;
    }
    
    mode_registry_t *reg = mode_registry_create();
    if (mode_registry_lookup_by_code(reg, NULL) != NULL) {
        FAIL("lookup with NULL code should return NULL");
        mode_registry_free(reg);
        return;
    }
    
    mode_registry_free(reg);
    PASS();
}

/**
 * Test: Add Single Mode
 */
static void test_add_single_mode(void) {
    TEST("add single mode");
    
    mode_registry_t *reg = mode_registry_create();
    
    int result = mode_registry_add(reg, &martin1);
    if (result != 0) {
        FAIL("Failed to add mode, error=%d", result);
        mode_registry_free(reg);
        return;
    }
    
    if (mode_registry_mode_count(reg) != 1) {
        FAIL("Expected count=1, got %d", mode_registry_mode_count(reg));
        mode_registry_free(reg);
        return;
    }
    
    mode_registry_free(reg);
    PASS();
}

/**
 * Test: Add Multiple Modes
 */
static void test_add_multiple_modes(void) {
    TEST("add multiple modes");
    
    mode_registry_t *reg = mode_registry_create();
    
    mode_registry_add(reg, &martin1);
    mode_registry_add(reg, &scottie1);
    mode_registry_add(reg, &robot36);
    
    if (mode_registry_mode_count(reg) != 3) {
        FAIL("Expected count=3, got %d", mode_registry_mode_count(reg));
        mode_registry_free(reg);
        return;
    }
    
    mode_registry_free(reg);
    PASS();
}

/**
 * Test: Duplicate Mode Rejection
 */
static void test_duplicate_rejection(void) {
    TEST("duplicate mode rejection");
    
    mode_registry_t *reg = mode_registry_create();
    
    mode_registry_add(reg, &martin1);
    
    /* Try to add same mode again */
    int result = mode_registry_add(reg, &martin1);
    if (result != -2) {
        FAIL("Expected error=-2 for duplicate, got %d", result);
        mode_registry_free(reg);
        return;
    }
    
    /* Count should still be 1 */
    if (mode_registry_mode_count(reg) != 1) {
        FAIL("Expected count=1 after duplicate rejection, got %d",
             mode_registry_mode_count(reg));
        mode_registry_free(reg);
        return;
    }
    
    mode_registry_free(reg);
    PASS();
}

/**
 * Test: Lookup by Code
 */
static void test_lookup_by_code(void) {
    TEST("lookup by code");
    
    mode_registry_t *reg = mode_registry_create();
    mode_registry_add(reg, &martin1);
    mode_registry_add(reg, &scottie1);
    
    const mode_definition_t *found = mode_registry_lookup_by_code(reg, "m1");
    if (!found) {
        FAIL("Failed to find mode 'm1'");
        mode_registry_free(reg);
        return;
    }
    
    if (strcmp(found->name, "Martin 1") != 0) {
        FAIL("Found wrong mode: %s", found->name);
        mode_registry_free(reg);
        return;
    }
    
    /* Test case-insensitive */
    found = mode_registry_lookup_by_code(reg, "M1");
    if (!found) {
        FAIL("Case-insensitive lookup failed");
        mode_registry_free(reg);
        return;
    }
    
    /* Test not found */
    found = mode_registry_lookup_by_code(reg, "notexist");
    if (found) {
        FAIL("Found non-existent mode");
        mode_registry_free(reg);
        return;
    }
    
    mode_registry_free(reg);
    PASS();
}

/**
 * Test: Lookup by Name
 */
static void test_lookup_by_name(void) {
    TEST("lookup by name");
    
    mode_registry_t *reg = mode_registry_create();
    mode_registry_add(reg, &martin1);
    mode_registry_add(reg, &scottie1);
    
    const mode_definition_t *found = 
        mode_registry_lookup_by_name(reg, "Martin 1");
    if (!found) {
        FAIL("Failed to find mode 'Martin 1'");
        mode_registry_free(reg);
        return;
    }
    
    if (strcmp(found->code, "m1") != 0) {
        FAIL("Found wrong mode: %s", found->code);
        mode_registry_free(reg);
        return;
    }
    
    /* Test case-insensitive */
    found = mode_registry_lookup_by_name(reg, "martin 1");
    if (!found) {
        FAIL("Case-insensitive name lookup failed");
        mode_registry_free(reg);
        return;
    }
    
    mode_registry_free(reg);
    PASS();
}

/**
 * Test: Lookup by VIS Code
 */
static void test_lookup_by_vis(void) {
    TEST("lookup by VIS code");
    
    mode_registry_t *reg = mode_registry_create();
    mode_registry_add(reg, &martin1);
    mode_registry_add(reg, &scottie1);
    
    const mode_definition_t *found = 
        mode_registry_lookup_by_vis(reg, 0xAC);
    if (!found) {
        FAIL("Failed to find mode with VIS 0xAC");
        mode_registry_free(reg);
        return;
    }
    
    if (strcmp(found->code, "m1") != 0) {
        FAIL("Found wrong mode: %s", found->code);
        mode_registry_free(reg);
        return;
    }
    
    /* Test not found */
    found = mode_registry_lookup_by_vis(reg, 0xFF);
    if (found) {
        FAIL("Found mode with invalid VIS code");
        mode_registry_free(reg);
        return;
    }
    
    mode_registry_free(reg);
    PASS();
}

/**
 * Test: List All Modes
 */
static void test_list_all(void) {
    TEST("list all modes");
    
    mode_registry_t *reg = mode_registry_create();
    mode_registry_add(reg, &martin1);
    mode_registry_add(reg, &scottie1);
    mode_registry_add(reg, &robot36);
    
    mode_list_t list = mode_registry_list_all(reg);
    
    if (list.count != 3) {
        FAIL("Expected 3 modes, got %zu", list.count);
        mode_registry_free(reg);
        return;
    }
    
    if (!list.modes) {
        FAIL("Mode array is NULL");
        mode_registry_free(reg);
        return;
    }
    
    mode_registry_free(reg);
    PASS();
}

/**
 * Test: List by Source
 */
static void test_list_by_source(void) {
    TEST("list by source filter");
    
    mode_registry_t *reg = mode_registry_create();
    mode_registry_add(reg, &martin1);
    mode_registry_add(reg, &scottie1);
    mode_registry_add(reg, &robot36);
    mode_registry_add(reg, &pd120);  /* MMSSTV mode */
    
    /* List native modes */
    mode_list_t native = mode_registry_list_by_source(reg, "native");
    if (native.count != 3) {
        FAIL("Expected 3 native modes, got %zu", native.count);
        mode_registry_free(reg);
        return;
    }
    
    /* List MMSSTV modes */
    mode_list_t mmsstv = mode_registry_list_by_source(reg, "mmsstv");
    if (mmsstv.count != 1) {
        FAIL("Expected 1 mmsstv mode, got %zu", mmsstv.count);
        mode_registry_free(reg);
        return;
    }
    
    mode_registry_free(reg);
    PASS();
}

/**
 * Test: Has Mode
 */
static void test_has_mode(void) {
    TEST("has_mode check");
    
    mode_registry_t *reg = mode_registry_create();
    mode_registry_add(reg, &martin1);
    
    if (!mode_registry_has_mode(reg, "m1")) {
        FAIL("has_mode returned false for existing mode");
        mode_registry_free(reg);
        return;
    }
    
    if (mode_registry_has_mode(reg, "notexist")) {
        FAIL("has_mode returned true for non-existent mode");
        mode_registry_free(reg);
        return;
    }
    
    mode_registry_free(reg);
    PASS();
}

/**
 * Test: Empty Registry
 */
static void test_empty_registry(void) {
    TEST("empty registry operations");
    
    mode_registry_t *reg = mode_registry_create();
    
    mode_list_t list = mode_registry_list_all(reg);
    if (list.count != 0) {
        FAIL("Empty registry should return empty list");
        mode_registry_free(reg);
        return;
    }
    
    if (mode_registry_lookup_by_code(reg, "m1") != NULL) {
        FAIL("Lookup on empty registry should return NULL");
        mode_registry_free(reg);
        return;
    }
    
    mode_registry_free(reg);
    PASS();
}

/**
 * Test: Mode Definition Completeness
 */
static void test_mode_definition_completeness(void) {
    TEST("mode definition completeness");
    
    mode_registry_t *reg = mode_registry_create();
    mode_registry_add(reg, &martin1);
    
    const mode_definition_t *m = mode_registry_lookup_by_code(reg, "m1");
    if (!m) {
        FAIL("Failed to lookup martin1");
        mode_registry_free(reg);
        return;
    }
    
    /* Verify all fields */
    if (m->width != 320) {
        FAIL("Incorrect width: %u", m->width);
        mode_registry_free(reg);
        return;
    }
    
    if (m->height != 256) {
        FAIL("Incorrect height: %u", m->height);
        mode_registry_free(reg);
        return;
    }
    
    if (m->duration_sec < 114.0 || m->duration_sec > 115.0) {
        FAIL("Incorrect duration: %f", m->duration_sec);
        mode_registry_free(reg);
        return;
    }
    
    if (!m->is_color) {
        FAIL("martin1 should be color mode");
        mode_registry_free(reg);
        return;
    }
    
    if (strcmp(m->source, "native") != 0) {
        FAIL("martin1 should be native source");
        mode_registry_free(reg);
        return;
    }
    
    if (!m->encode_frame) {
        FAIL("martin1 should have encode_frame function");
        mode_registry_free(reg);
        return;
    }
    
    mode_registry_free(reg);
    PASS();
}

/**
 * Test: Print All (visual inspection)
 */
static void test_print_all(void) {
    TEST("print all modes (visual check)");
    
    mode_registry_t *reg = mode_registry_create();
    mode_registry_add(reg, &martin1);
    mode_registry_add(reg, &scottie1);
    mode_registry_add(reg, &robot36);
    mode_registry_add(reg, &pd120);
    
    printf("\n");
    mode_registry_print_all(reg);
    printf("\n");
    
    mode_registry_free(reg);
    PASS();
}

/*==============================================================================
 * MAIN TEST RUNNER
 *============================================================================*/

int main(void) {
    printf("Mode Registry Test Suite - SlowFrame v2.1\n");
    printf("=========================================\n\n");
    
    /* Run all tests */
    test_lifecycle();
    test_null_safety();
    test_add_single_mode();
    test_add_multiple_modes();
    test_duplicate_rejection();
    test_lookup_by_code();
    test_lookup_by_name();
    test_lookup_by_vis();
    test_list_all();
    test_list_by_source();
    test_has_mode();
    test_empty_registry();
    test_mode_definition_completeness();
    test_print_all();
    
    /* Print summary */
    printf("\n=========================================\n");
    printf("Tests Run:    %d\n", tests_run);
    printf("Tests Passed: %d\n", tests_passed);
    printf("Tests Failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("\n✅ ALL TESTS PASSED\n");
        return 0;
    } else {
        printf("\n❌ SOME TESTS FAILED\n");
        return 1;
    }
}
