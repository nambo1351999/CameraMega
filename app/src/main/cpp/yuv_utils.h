#ifndef YUV_UTILS_H
#define YUV_UTILS_H

#include <cstdint>

void RotatePlane16(const uint16_t *src, uint16_t *dst, int width, int height,
                   int rotation);

#endif 
