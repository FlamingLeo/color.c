// converter functions for converting colors from one model to another
//
// mainly to and from rgb, which makes conversion a tiny bit slower
// but doesn't explode code size
#ifndef CONVERTER_H
#define CONVERTER_H

#include "types.h"

hex_t rgb_to_hex(rgb_t *rgb);
rgb_t hex_to_rgb(hex_t hex);

cmyk_t rgb_to_cmyk(rgb_t *rgb);
hsl_t rgb_to_hsl(rgb_t *rgb);
hsv_t rgb_to_hsv(rgb_t *rgb);

rgb_t cmyk_to_rgb(cmyk_t *cmyk);
rgb_t hsl_to_rgb(hsl_t *hsl);
rgb_t hsv_to_rgb(hsv_t *hsv);

#endif