#include <assert.h> // debug checks only, shouldn't be needed in prod.
#include <math.h>
#include <stddef.h>
#include "converter.h"
#include "parser.h"
#include "utility.h"

// there's no checks for correct input in these functions (e.g. rgb in range [0,255] each)
// we assume the parser has done a good job before filtering / modifying the values beforehand

// ansi 16 palette
static const rgb_t ansi16_rgb[16] = {
    {0,0,0},       {128,0,0},   {0,128,0},   {128,128,0},
    {0,0,128},     {128,0,128}, {0,128,128}, {192,192,192},
    {128,128,128}, {255,0,0},   {0,255,0},   {255,255,0},
    {0,0,255},     {255,0,255}, {0,255,255}, {255,255,255}
};

// 6 cube levels used by xterm 256
static const int cube_levels[6] = {0, 95, 135, 175, 215, 255};

// formulas mostly from rapidtables:
//   https://www.rapidtables.com/convert/color/
//
// other more general information from wikipedia:
//   https://en.wikipedia.org/wiki/RGB_color_model
//   https://en.wikipedia.org/wiki/CMYK_color_model
//   https://en.wikipedia.org/wiki/HSL_and_HSV
//   https://en.wikipedia.org/wiki/Web_colors

hex_t rgb_to_hex(const rgb_t *rgb) {
    assert(rgb);
    return ((hex_t)rgb->r << 16) | ((hex_t)rgb->g << 8) | (hex_t)rgb->b;
}

rgb_t hex_to_rgb(const hex_t hex) {
    return (rgb_t){ .r = (hex >> 16) & 0xFF,
                    .g =  (hex >> 8) & 0xFF,
                    .b =         hex & 0xFF };
}

cmyk_t rgb_to_cmyk(const rgb_t *rgb) {
    assert(rgb);

    cmyk_t cmyk;
    double r = rgb->r / 255.0;
    double g = rgb->g / 255.0;
    double b = rgb->b / 255.0;

    double mrgb = MAX(MAX(r,g),b);
    cmyk.k = 1.0 - mrgb;

    if (cmyk.k >= 1.0 - ZERO_THRESH) cmyk.c = cmyk.m = cmyk.y = 0.0;
    else {
        double denom = 1.0 - cmyk.k;
        cmyk.c = (1.0 - r - cmyk.k) / denom;
        cmyk.m = (1.0 - g - cmyk.k) / denom;
        cmyk.y = (1.0 - b - cmyk.k) / denom;

        // safety clamps
        cmyk.c = CLAMP(cmyk.c, 0.0, 1.0);
        cmyk.m = CLAMP(cmyk.m, 0.0, 1.0);
        cmyk.y = CLAMP(cmyk.y, 0.0, 1.0);
    }

    return cmyk;
}

hsl_t rgb_to_hsl(const rgb_t *rgb) {
    assert(rgb);

    hsl_t hsl;
    double r = rgb->r / 255.0;
    double g = rgb->g / 255.0;
    double b = rgb->b / 255.0;

    double cmax = MAX(MAX(r,g),b);
    double cmin = MIN(MIN(r,g),b);
    double delta = cmax - cmin;

    // lightness
    hsl.l = (cmax + cmin) / 2.0;

    // achromatic -> hue and saturation 0
    if (delta < ZERO_THRESH) { hsl.h = hsl.sat = 0.0; return hsl; }

    // saturation
    double denom = 1.0 - fabs(2.0 * (hsl.l) - 1.0);
    if (denom <= ZERO_THRESH) hsl.sat = 0.0;
    else {
        hsl.sat = delta / denom; 
        if (hsl.sat > 1.0) hsl.sat = 1.0; // example: 251,251,255 -> sat very slightly over 1.0
    }

    // hue in [0,360)
    double hp;
    if      (fabs(cmax - r) < ZERO_THRESH) hp = (g - b) / delta;        // cmax = r'
    else if (fabs(cmax - g) < ZERO_THRESH) hp = (b - r) / delta + 2.0;  // cmax = g'
    else                                   hp = (r - g) / delta + 4.0;  // cmax = b'
    hp *= 60.0;
    
    // wrap
    if (hp < 0.0)       hp += 360.0;
    while (hp >= 360.0) hp -= 360.0;
    hsl.h = hp;

    return hsl;
}

hsv_t rgb_to_hsv(const rgb_t *rgb) {
    assert(rgb);

    hsv_t hsv;
    double r = rgb->r / 255.0;
    double g = rgb->g / 255.0;
    double b = rgb->b / 255.0;

    double cmax = MAX(MAX(r,g),b);
    double cmin = MIN(MIN(r,g),b);
    double delta = cmax - cmin;

    // value
    hsv.v = cmax;

    // achromatic -> hue and saturation 0
    if (delta < ZERO_THRESH) { hsv.h = hsv.sat = 0.0; return hsv; }

    // saturation
    hsv.sat = (cmax <= ZERO_THRESH) ? 0.0 : (delta / cmax);
    if (hsv.sat > 1.0) hsv.sat = 1.0;

    // hue in [0,360)
    double hp;
    if      (fabs(cmax - r) < ZERO_THRESH) hp = (g - b) / delta;       // cmax = r'
    else if (fabs(cmax - g) < ZERO_THRESH) hp = (b - r) / delta + 2.0; // cmax = g'
    else                                   hp = (r - g) / delta + 4.0; // cmax = b'
    
    // wrap
    hp *= 60.0;
    if (hp < 0.0) hp += 360.0;
    while (hp >= 360.0) hp -= 360.0;
    hsv.h = hp;

    return hsv;
}

rgb_t cmyk_to_rgb(const cmyk_t *cmyk) {
    assert(cmyk);
    return (rgb_t){ .r = (int)round(255 * (1 - cmyk->c) * (1 - cmyk->k)),
                    .g = (int)round(255 * (1 - cmyk->m) * (1 - cmyk->k)),
                    .b = (int)round(255 * (1 - cmyk->y) * (1 - cmyk->k)) };
}

rgb_t hsl_to_rgb(const hsl_t *hsl) {
    assert(hsl);

    double C = (1.0 - fabs(2.0 * hsl->l - 1.0)) * hsl->sat;
    double X = C * (1.0 - fabs(fmod(hsl->h / 60.0, 2.0) - 1.0));
    double m = hsl->l - C / 2.0;

    // h must be in [0,360)
    double r, g, b;
    double hp = fmod(hsl->h, 360.0);
    if (hp < 0.0) hp += 360.0;

    if (hp < 60.0)       { r = C;   g = X;   b = 0.0; } // [  0,  60)
    else if (hp < 120.0) { r = X;   g = C;   b = 0.0; } // [ 60, 120)
    else if (hp < 180.0) { r = 0.0; g = C;   b = X;   } // [120, 180)
    else if (hp < 240.0) { r = 0.0; g = X;   b = C;   } // [180, 240)
    else if (hp < 300.0) { r = X;   g = 0.0; b = C;   } // [240, 300)
    else                 { r = C;   g = 0.0; b = X;   } // [300, 360)

    return (rgb_t){ .r = (int)round((r + m) * 255),
                    .g = (int)round((g + m) * 255),
                    .b = (int)round((b + m) * 255) };
}

rgb_t hsv_to_rgb(const hsv_t *hsv) {
    assert(hsv);
    
    double C = hsv->v * hsv->sat;
    double X = C * (1.0 - fabs(fmod(hsv->h / 60.0, 2.0) - 1.0));
    double m = hsv->v - C;

    double r, g, b;
    double hp = fmod(hsv->h, 360.0);
    if (hp < 0.0) hp += 360.0;

    if (hp < 60.0)       { r = C;   g = X;   b = 0.0; } // [  0,  60)
    else if (hp < 120.0) { r = X;   g = C;   b = 0.0; } // [ 60, 120)
    else if (hp < 180.0) { r = 0.0; g = C;   b = X;   } // [120, 180)
    else if (hp < 240.0) { r = 0.0; g = X;   b = C;   } // [180, 240)
    else if (hp < 300.0) { r = X;   g = 0.0; b = C;   } // [240, 300)
    else                 { r = C;   g = 0.0; b = X;   } // [300, 360)

    return (rgb_t){ .r = (int)round((r + m) * 255),
                    .g = (int)round((g + m) * 255),
                    .b = (int)round((b + m) * 255) };
}

rgb_t ansi256_idx_to_rgb(int idx) {
    assert(idx <= 255);

    // system colors (approximate mappings to cube / grayscale could be used, commonly standardized)
    if (idx < 16) return ansi16_rgb[idx];

    // grayscale colors
    if (idx >= 232) {
        int gi = idx - 232;
        int v = 8 + gi * 10;
        return (rgb_t){ .r = v, .g = v, .b = v };
    }

    // rest: 6x6x6 cube
    int ci = idx - 16;
    int r6 = ci / 36;
    int g6 = (ci % 36) / 6;
    int b6 = ci % 6;

    return (rgb_t){ .r = cube_levels[r6], .g = cube_levels[g6], .b = cube_levels[b6] };
}

rgb_t ansi16_idx_to_rgb(int idx) { return ansi16_rgb[idx]; }

int rgb_to_ansi256_idx(const rgb_t *rgb) {
    int    best   = 0;
    double best_d = 1e300;
    for (int idx = 0; idx < 256; ++idx) {
        rgb_t p = ansi256_idx_to_rgb(idx);

        double rlin  = (double)(rgb->r) / 255.0, glin  = (double)(rgb->g) / 255.0, blin  = (double)(rgb->b) / 255.0;
        double prlin = (double)(p.r)    / 255.0, pglin = (double)(p.g)    / 255.0, pblin = (double)(p.b)    / 255.0;
        double dr    = rlin - prlin,             dg    = glin - pglin,             dbi   = blin - pblin;
        double d     = dr*dr                           + dg*dg                           + dbi*dbi;

        if (d < best_d) { best_d = d; best = idx; }
    }
    return best;
}

int rgb_to_ansi16_idx(const rgb_t *rgb) {
    int best      = 0;
    double best_d = 1e300;
    for (int i = 0; i < 16; ++i) {
        double d = dist2_rgb(rgb, &ansi16_rgb[i]);
        if (d < best_d) { best_d = d; best = i; }
    }
    return best;
}

int ansi16_idx_to_sgr_fg(int idx) {
    if (idx < 0) idx = 0;
    if (idx < 8) return 30 + idx;
    return 90 + (idx - 8);
}

int ansi16_idx_to_sgr_bg(int idx) {
    if (idx < 0) idx = 0;
    if (idx < 8) return 40 + idx;
    return 100 + (idx - 8);
}