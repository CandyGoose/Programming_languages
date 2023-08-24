#include "../include/image.h"
#include <stdlib.h>

struct image* create_image(uint64_t width, uint64_t height) {
    struct image* img = (struct image*)malloc(sizeof(struct image));
    if (!img) {
        return NULL; // Не удалось выделить память
    }

    img->width = width;
    img->height = height;

    img->data = (struct pixel*)malloc(width * height * sizeof(struct pixel));
    if (!img->data) {
        free(img);
        return NULL; // Не удалось выделить память
    }

    return img;
}

void destroy_image(struct image* img) {
    if (!img)
        return;
    free(img->data);
    free(img);
}
