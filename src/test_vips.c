#include <stdio.h>
#include <stdlib.h>
#include <vips/vips.h>

int main(int argc, char *argv[]) {
    printf("VIPS init...\n");
    if (VIPS_INIT(argv[0])) {
        vips_error_exit(NULL);
    }
    printf("VIPS initialized\n");
    printf("Loading test.png...\n");
    
    VipsImage *image = vips_image_new_from_file("test.png", NULL);
    if (!image) {
        printf("Error: %s\n", vips_error_buffer());
        vips_shutdown();
        return 1;
    }
    printf("Loaded: %d x %d, %d bands\n", image->Xsize, image->Ysize, image->Bands);
    
    g_object_unref(image);
    vips_shutdown();
    printf("OK\n");
    return 0;
}
