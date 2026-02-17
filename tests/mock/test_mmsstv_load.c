/**
 * @file test_mmsstv_load.c
 * @brief Simple test program to verify MMSSTV library loading
 */

#include <stdio.h>
#include <stdlib.h>
#include "mmsstv_loader.h"
#include "mmsstv_adapter.h"

int main(void) {
    printf("Testing MMSSTV library loading...\n\n");
    
    // Test 1: Loader
    printf("=== Test 1: MMSSTV Loader ===\n");
    mmsstv_library_t *lib = mmsstv_loader_init();
    if (!lib) {
        fprintf(stderr, "ERROR: mmsstv_loader_init() returned NULL\n");
        return 1;
    }
    
    printf("Loader initialized: %s\n", lib ? "YES" : "NO");
    printf("Library loaded: %s\n", mmsstv_loader_is_available(lib) ? "YES" : "NO");
    
    if (mmsstv_loader_is_available(lib)) {
        const char *version = mmsstv_loader_get_version(lib);
        const char *path = mmsstv_loader_get_path(lib);
        int mode_count = mmsstv_loader_get_mode_count(lib);
        
        printf("Library path: %s\n", path ? path : "unknown");
        printf("Library version: %s\n", version ? version : "unknown");
        printf("Mode count: %d\n", mode_count);
        
        // List modes
        if (mode_count > 0) {
            printf("\nModes:\n");
            for (int i = 0; i < mode_count; i++) {
                const mmsstv_mode_t *mode = mmsstv_loader_get_mode(lib, i);
                if (mode) {
                    printf("  %d. %s (%s) - VIS: 0x%02X - %dx%d\n",
                           i + 1, mode->code, mode->name, mode->vis_code,
                           mode->width, mode->height);
                }
            }
        }
    } else {
        char status[256];
        mmsstv_loader_get_status(lib, status, sizeof(status));
        printf("Status: %s\n", status);
    }
    
    // Test 2: Adapter
    printf("\n=== Test 2: MMSSTV Adapter ===\n");
    mmsstv_adapter_t *adapter = mmsstv_adapter_init();
    if (!adapter) {
        fprintf(stderr, "ERROR: mmsstv_adapter_init() returned NULL\n");
        mmsstv_loader_destroy(lib);
        return 1;
    }
    
    printf("Adapter initialized: YES\n");
    printf("Adapter available: %s\n", mmsstv_adapter_is_available(adapter) ? "YES" : "NO");
    printf("Adapter mode count: %d\n", mmsstv_adapter_get_mode_count(adapter));
    
    // Cleanup
    mmsstv_adapter_destroy(adapter);
    mmsstv_loader_destroy(lib);
    
    printf("\n✓ Test complete\n");
    return 0;
}
