#include <assert.h> // debug checks only, shouldn't be needed in prod.
#include <math.h>
#include "converter.h"

// there's no checks for correct input in these functions (e.g. rgb in range [0,255] each)
// we assume the parser has done a good job before filtering / modifying the values beforehand

// safety clamp for potential small float deviations
static inline double clamp01(double x) {
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
}

// formulas mostly from rapidtables:
//   https://www.rapidtables.com/convert/color/
//
// other more general information from wikipedia:
//   https://en.wikipedia.org/wiki/RGB_color_model
//   https://en.wikipedia.org/wiki/CMYK_color_model
//   https://en.wikipedia.org/wiki/HSL_and_HSV
//   https://en.wikipedia.org/wiki/Web_colors

hex_t rgb_to_hex(rgb_t *rgb) {
    assert(rgb);
    return ((hex_t)rgb->r << 16) | ((hex_t)rgb->g << 8) | (hex_t)rgb->b;
}

rgb_t hex_to_rgb(hex_t hex) {
    return (rgb_t){ .r = (hex >> 16) & 0xFF,
                    .g =  (hex >> 8) & 0xFF,
                    .b =         hex & 0xFF };
}

cmyk_t rgb_to_cmyk(rgb_t *rgb) {
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

        cmyk.c = clamp01(cmyk.c);
        cmyk.m = clamp01(cmyk.m);
        cmyk.y = clamp01(cmyk.y);
    }

    return cmyk;
}

hsl_t rgb_to_hsl(rgb_t *rgb) {
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

hsv_t rgb_to_hsv(rgb_t *rgb) {
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

rgb_t cmyk_to_rgb(cmyk_t *cmyk) {
    assert(cmyk);
    return (rgb_t){ .r = (int)round(255 * (1 - cmyk->c) * (1 - cmyk->k)),
                    .g = (int)round(255 * (1 - cmyk->m) * (1 - cmyk->k)),
                    .b = (int)round(255 * (1 - cmyk->y) * (1 - cmyk->k)) };
}

rgb_t hsl_to_rgb(hsl_t *hsl) {
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

rgb_t hsv_to_rgb(hsv_t *hsv) {
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