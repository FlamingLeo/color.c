// converter functions for converting colors from one model to another
//
// mainly to and from rgb, which makes conversion a tiny bit slower
// but doesn't explode code size
//
// comments omitted, function names should be self-explanatory
#ifndef CONVERTER_H
#define CONVERTER_H

#include "types.h"

hex_t rgb_to_hex(const rgb_t *rgb);
rgb_t hex_to_rgb(const hex_t hex);

cmyk_t rgb_to_cmyk(const rgb_t *rgb);
hsl_t rgb_to_hsl(const rgb_t *rgb);
hsv_t rgb_to_hsv(const rgb_t *rgb);

rgb_t cmyk_to_rgb(const cmyk_t *cmyk);
rgb_t hsl_to_rgb(const hsl_t *hsl);
rgb_t hsv_to_rgb(const hsv_t *hsv);

rgb_t ansi256_idx_to_rgb(int idx);
rgb_t ansi16_idx_to_rgb(int idx);
int rgb_to_ansi256_idx(const rgb_t *rgb);
int rgb_to_ansi16_idx(const rgb_t *rgb);
int ansi16_idx_to_sgr_fg(int idx);
int ansi16_idx_to_sgr_bg(int idx);

#endif