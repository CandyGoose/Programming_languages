#include "../include/transformations.h"

#include <stddef.h>

struct image* rotate(const struct image* source) {
    // Создаем новое изображение с перевернутой шириной и высотой
    struct image* rotated = create_image(source->height, source->width);
    if (!rotated) {
        return NULL;
    }

    // Производим поворот путем копирования пикселей в новое изображение
    for (uint64_t y = 0; y < source->height; ++y) {
        for (uint64_t x = 0; x < source->width; ++x) {
            rotated->data[x * rotated->width + rotated->width - 1 - y] = source->data[y * source->width + x];
        }
    }

    return rotated;
}
