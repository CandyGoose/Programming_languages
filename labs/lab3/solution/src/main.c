#include "../include/bmp_io.h"
#include "../include/transformations.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <source-image> <transformed-image>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Открываем исходное изображение для чтения
    FILE* in = fopen(argv[1], "rb");
    if (!in) {
        perror("Ошибка при открытии исходного изображения");
        return EXIT_FAILURE;
    }

    // Считываем исходное изображение из файла
    struct image* img = NULL;
    enum read_status read_result = from_bmp(in, &img);
    fclose(in);

    if (read_result != READ_OK) {
        fprintf(stderr, "Ошибка чтения исходного изображения: %d\n", read_result);
        if (img) {
            destroy_image(img);
        }
        return EXIT_FAILURE;
    }

    // Выполняем поворот изображения
    struct image* rotated_img = rotate(img);
    if (!rotated_img) {
        fprintf(stderr, "Ошибка при создании нового изображения\n");
        destroy_image(img);
        return EXIT_FAILURE;
    }

    // Открываем новый файл для записи повернутого изображения
    FILE* out = fopen(argv[2], "wb");
    if (!out) {
        perror("Ошибка при открытии нового изображения");
        destroy_image(img);
        destroy_image(rotated_img);
        return EXIT_FAILURE;
    }

    // Записываем повернутое изображение в файл
    enum write_status write_result = to_bmp(out, rotated_img);
    fclose(out);

    if (write_result != WRITE_OK) {
        fprintf(stderr, "Ошибка при записи нового изображения: %d\n", write_result);
        destroy_image(img);
        destroy_image(rotated_img);
        return EXIT_FAILURE;
    }

    // Освобождаем память, выделенную для изображений
    destroy_image(img);
    destroy_image(rotated_img);

    printf("Поворот изображения прошел успешно!\n");
    return EXIT_SUCCESS;
}
