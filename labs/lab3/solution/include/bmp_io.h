#ifndef BMP_IO_H
#define BMP_IO_H

#include "image.h"
#include <stdio.h>

enum read_status {
    READ_OK = 0,
    READ_INVALID_SIGNATURE,
    READ_INVALID_BITS,
    READ_INVALID_HEADER
};

enum read_status from_bmp(FILE* in, struct image** img);
enum write_status {
    WRITE_OK = 0,
    WRITE_ERROR
};

enum write_status to_bmp(FILE* out, const struct image* img);

#endif // BMP_IO_H
