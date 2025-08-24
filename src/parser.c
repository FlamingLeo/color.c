#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "converter.h"
#include "parser.h"

#define COPY_OR_RETURN(_dst,_src) do { int n = snprintf(_dst, sizeof(_dst), "%s", _src); if (n < 0) return 0; if ((size_t)n >= sizeof(_dst)) return 0; } while (0)

// all 140 named css colors
// source: https://github.com/bahamas10/css-color-names/
static const named_t css_colors[] = {
    { "aliceblue", 0xf0f8ff },{ "antiquewhite", 0xfaebd7 },{ "aqua", 0x00ffff },{ "aquamarine", 0x7fffd4 },{ "azure", 0xf0ffff },
    { "beige", 0xf5f5dc },{ "bisque", 0xffe4c4 },{ "black", 0x000000 },{ "blanchedalmond", 0xffebcd },{ "blue", 0x0000ff },
    { "blueviolet", 0x8a2be2 },{ "brown", 0xa52a2a },{ "burlywood", 0xdeb887 },{ "cadetblue", 0x5f9ea0 },{ "chartreuse", 0x7fff00 },
    { "chocolate", 0xd2691e },{ "coral", 0xff7f50 },{ "cornflowerblue", 0x6495ed },{ "cornsilk", 0xfff8dc },{ "crimson", 0xdc143c },
    { "cyan", 0x00ffff },{ "darkblue", 0x00008b },{ "darkcyan", 0x008b8b },{ "darkgoldenrod", 0xb8860b },{ "darkgray", 0xa9a9a9 },
    { "darkgreen", 0x006400 },{ "darkgrey", 0xa9a9a9 },{ "darkkhaki", 0xbdb76b },{ "darkmagenta", 0x8b008b },{ "darkolivegreen", 0x556b2f },
    { "darkorange", 0xff8c00 },{ "darkorchid", 0x9932cc },{ "darkred", 0x8b0000 },{ "darksalmon", 0xe9967a },{ "darkseagreen", 0x8fbc8f },
    { "darkslateblue", 0x483d8b },{ "darkslategray", 0x2f4f4f },{ "darkslategrey", 0x2f4f4f },{ "darkturquoise", 0x00ced1 },{ "darkviolet", 0x9400d3 },
    { "deeppink", 0xff1493 },{ "deepskyblue", 0x00bfff },{ "dimgray", 0x696969 },{ "dimgrey", 0x696969 },{ "dodgerblue", 0x1e90ff },
    { "firebrick", 0xb22222 },{ "floralwhite", 0xfffaf0 },{ "forestgreen", 0x228b22 },{ "fuchsia", 0xff00ff },{ "gainsboro", 0xdcdcdc },
    { "ghostwhite", 0xf8f8ff },{ "goldenrod", 0xdaa520 },{ "gold", 0xffd700 },{ "gray", 0x808080 },{ "green", 0x008000 },
    { "greenyellow", 0xadff2f },{ "grey", 0x808080 },{ "honeydew", 0xf0fff0 },{ "hotpink", 0xff69b4 },{ "indianred", 0xcd5c5c },
    { "indigo", 0x4b0082 },{ "ivory", 0xfffff0 },{ "khaki", 0xf0e68c },{ "lavenderblush", 0xfff0f5 },{ "lavender", 0xe6e6fa },
    { "lawngreen", 0x7cfc00 },{ "lemonchiffon", 0xfffacd },{ "lightblue", 0xadd8e6 },{ "lightcoral", 0xf08080 },{ "lightcyan", 0xe0ffff },
    { "lightgoldenrodyellow", 0xfafad2 },{ "lightgray", 0xd3d3d3 },{ "lightgreen", 0x90ee90 },{ "lightgrey", 0xd3d3d3 },{ "lightpink", 0xffb6c1 },
    { "lightsalmon", 0xffa07a },{ "lightseagreen", 0x20b2aa },{ "lightskyblue", 0x87cefa },{ "lightslategray", 0x778899 },{ "lightslategrey", 0x778899 },
    { "lightsteelblue", 0xb0c4de },{ "lightyellow", 0xffffe0 },{ "lime", 0x00ff00 },{ "limegreen", 0x32cd32 },{ "linen", 0xfaf0e6 },
    { "magenta", 0xff00ff },{ "maroon", 0x800000 },{ "mediumaquamarine", 0x66cdaa },{ "mediumblue", 0x0000cd },{ "mediumorchid", 0xba55d3 },
    { "mediumpurple", 0x9370db },{ "mediumseagreen", 0x3cb371 },{ "mediumslateblue", 0x7b68ee },{ "mediumspringgreen", 0x00fa9a },{ "mediumturquoise", 0x48d1cc },
    { "mediumvioletred", 0xc71585 },{ "midnightblue", 0x191970 },{ "mintcream", 0xf5fffa },{ "mistyrose", 0xffe4e1 },{ "moccasin", 0xffe4b5 },
    { "navajowhite", 0xffdead },{ "navy", 0x000080 },{ "oldlace", 0xfdf5e6 },{ "olive", 0x808000 },{ "olivedrab", 0x6b8e23 },
    { "orange", 0xffa500 },{ "orangered", 0xff4500 },{ "orchid", 0xda70d6 },{ "palegoldenrod", 0xeee8aa },{ "palegreen", 0x98fb98 },
    { "paleturquoise", 0xafeeee },{ "palevioletred", 0xdb7093 },{ "papayawhip", 0xffefd5 },{ "peachpuff", 0xffdab9 },{ "peru", 0xcd853f },
    { "pink", 0xffc0cb },{ "plum", 0xdda0dd },{ "powderblue", 0xb0e0e6 },{ "purple", 0x800080 },{ "rebeccapurple", 0x663399 },
    { "red", 0xff0000 },{ "rosybrown", 0xbc8f8f },{ "royalblue", 0x4169e1 },{ "saddlebrown", 0x8b4513 },{ "salmon", 0xfa8072 },
    { "sandybrown", 0xf4a460 },{ "seagreen", 0x2e8b57 },{ "seashell", 0xfff5ee },{ "sienna", 0xa0522d },{ "silver", 0xc0c0c0 },
    { "skyblue", 0x87ceeb },{ "slateblue", 0x6a5acd },{ "slategray", 0x708090 },{ "slategrey", 0x708090 },{ "snow", 0xfffafa },
    { "springgreen", 0x00ff7f },{ "steelblue", 0x4682b4 },{ "tan", 0xd2b48c },{ "teal", 0x008080 },{ "thistle", 0xd8bfd8 },
    { "tomato", 0xff6347 },{ "turquoise", 0x40e0d0 },{ "violet", 0xee82ee },{ "wheat", 0xf5deb3 },{ "white", 0xffffff },
    { "whitesmoke", 0xf5f5f5 },{ "yellow", 0xffff00 },{ "yellowgreen", 0x9acd32 },
};

// squared weighted distance between two rgb colors (components 0..255)
static inline double weighted_dist2_rgb(const rgb_t *a, const rgb_t *b, double wr, double wg, double wb) {
    double dr = (double)a->r - (double)b->r;
    double dg = (double)a->g - (double)b->g;
    double db = (double)a->b - (double)b->b;
    return wr * dr * dr + wg * dg * dg + wb * db * db;
}

// find idx of closest named color using weighted squared rgb distance
// technically, doing it with rgb isn't the best (even weighted), but imo it's a fast and good enough way of doing it for this small sample size
static int closest_named_weighted_rgb_index(const rgb_t *in) {
    if (!in) return -1;
    size_t n = sizeof(css_colors) / sizeof(css_colors[0]);

    // source: https://www.compuphase.com/cmetric.htm
    const double wr = 0.22216091748149788;
    const double wg = 0.4288860259783791;
    const double wb = 0.34895305654012304;

    double best_score = 1.79769e+308; // DBL_MAX
    int best_idx = -1;

    for (size_t i = 0; i < n; ++i) {
        rgb_t named = hex_to_rgb(css_colors[i].hex);
        double d = weighted_dist2_rgb(in, &named, wr, wg, wb);
        if (d < best_score) {
            best_score = d;
            best_idx = (int)i;
        }
    }
    return best_idx;
}


// helper function to normalize string in-place by removing whitespaces and converting upper- to lowercase
// the result is guaranteed to be shorter or equal in length to the input
static inline void norm(char *s) {
    if (!s) return;

    char *dst = s;
    for (char *src = s; *src; ++src) {
        unsigned char uc = (unsigned char)*src;
        if (!isspace(uc)) *dst++ = (char)tolower(uc);
    }
    *dst = '\0';
}

// internal parsers: return 1 on success and set the out parameters out->{r,g,b}
//                   return 0 on failure
// destructive!      parsers destroy input string, watch out!

// NAMED (CSS): any valid css named color
static inline int parse_named(char *s, color_t *out) {
    if (!s || !*s) return 0;

    size_t n = sizeof(css_colors) / sizeof(css_colors[0]);
    for (size_t i = 0; i < n; ++i) {
        if (strcmp(s, css_colors[i].name) == 0) {
            hex_t v = css_colors[i].hex;
            out->hex   = v;
            out->rgb   = hex_to_rgb(v);
            out->cmyk  = rgb_to_cmyk(&out->rgb);
            out->hsl   = rgb_to_hsl(&out->rgb);
            out->hsv   = rgb_to_hsv(&out->rgb);
            out->named = css_colors[i];
            return 1;
        }
    }
    return 0;
}

// HEX: "#rrggbb", "0xrrggbb", "xrrggbb", "rrggbb", "hex(...)" incl. shorthand variants
static inline int parse_hex(char *s, color_t *out) {
    char *p = s;

    if (strncmp(p, "hex(", 4) == 0) {
        p += 4;
        size_t L = strlen(p);
        if (L && p[L - 1] == ')') p[L - 1] = 0;
        else return 0;
    }

    if (*p == '#') ++p;                           // #rrggbb or #rgb
    else if (p[0] == '0' && p[1] == 'x') p += 2;  // 0xrrggbb or 0xrgb
    else if (*p == 'x') ++p;                      // xrrggbb or xrgb

    size_t len = strlen(p);
    unsigned v = 0;

    if (len == 3) {
        // expand shorthand
        char buf[7];
        for (int i = 0; i < 3; ++i) {
            if (!isxdigit((unsigned char)p[i])) return 0;
            buf[2 * i]     = p[i];
            buf[2 * i + 1] = p[i];
        }
        buf[6] = '\0';
        if (sscanf(buf, "%6x", &v) != 1) return 0;
    } else if (len == 6) {
        // validate standard hex
        for (size_t i = 0; i < 6; ++i) if (!isxdigit((unsigned char)p[i])) return 0;
        if (sscanf(p, "%6x", &v) != 1) return 0;
    } else return 0; // invalid length

    out->hex   = v;
    out->rgb   = hex_to_rgb(v);
    out->cmyk  = rgb_to_cmyk(&out->rgb);
    out->hsl   = rgb_to_hsl(&out->rgb);
    out->hsv   = rgb_to_hsv(&out->rgb);
    out->named = css_colors[closest_named_weighted_rgb_index(&out->rgb)];
    return 1;
}


// RGB: "rgb(r,g,b)", "(r,g,b)", "r,g,b"
// ints 0..255 or floats 0..1
static inline int parse_rgb(char *s, color_t *out) {
    // don't confuse with hsl/hsv
    if (strchr(s, '%')) return 0;

    char *p = s;
    if (strncmp(p, "rgb(", 4) == 0) {
        p += 4;
        size_t L = strlen(p);
        if (L && p[L - 1] == ')') p[L - 1] = 0; else return 0;
    }

    int    a,  b,  c;
    double fa, fb, fc;
    int    n = 0; 
    if (sscanf(p, "%d,%d,%d%n", &a, &b, &c, &n) == 3 && p[n] == '\0') {
        // integers (0,0,0 - 255,255,255)
        if (a >= 0 && a <= 255 && b >= 0 && b <= 255 && c >= 0 && c <= 255) {
            out->rgb   = (rgb_t){ .r = a, .g = b, .b = c };
            out->hex   = rgb_to_hex(&out->rgb);
            out->cmyk  = rgb_to_cmyk(&out->rgb);
            out->hsl   = rgb_to_hsl(&out->rgb);
            out->hsv   = rgb_to_hsv(&out->rgb);
            out->named = css_colors[closest_named_weighted_rgb_index(&out->rgb)];
            return 1;
        }
    } else if (sscanf(p, "%lf,%lf,%lf%n", &fa, &fb, &fc, &n) == 3 && p[n] == '\0') {
        // floats (0.0,0.0,0.0 - 1.0,1.0,1.0)
        if (fa >= 0.0 && fa <= 1.0 && fb >= 0.0 && fb <= 1.0 && fc >= 0.0 && fc <= 1.0) {
            out->rgb = (rgb_t){ .r = (int)round(fa * 255.0), 
                                .g = (int)round(fb * 255.0), 
                                .b = (int)round(fc * 255.0) };
            out->hex   = rgb_to_hex(&out->rgb);
            out->cmyk  = rgb_to_cmyk(&out->rgb);
            out->hsl   = rgb_to_hsl(&out->rgb);
            out->hsv   = rgb_to_hsv(&out->rgb);
            out->named = css_colors[closest_named_weighted_rgb_index(&out->rgb)];
            return 1;
        }
    }
    return 0;
}

// CMYK: "cmyk(c%,m%,y%,k%)", "cmyk(c,m,y,k)", "c%,m%,y%,k%", "c,m,y,k"
// percent or 0..1 or 0..100
static inline int parse_cmyk(char *s, color_t *out) {
    char *p = s;
    if (strncmp(p, "cmyk(", 5) == 0) {
        p += 5;
        size_t L = strlen(p);
        if (L && p[L - 1] == ')') p[L - 1] = 0; else return 0;
    }

    double c, m, y, k;
    int    n = 0;
    if (sscanf(p, "%lf%%,%lf%%,%lf%%,%lf%%%n", &c, &m, &y, &k, &n) == 4 && p[n] == '\0') {
        // percent form: divide by 100 to get normalized value
        if (c > 100.0 || m > 100.0 || y > 100.0 || k > 100.0 || 
            c <   0.0 || m <   0.0 || y <   0.0 || k <   0.0) return 0;

        c /= 100.0;
        m /= 100.0;
        y /= 100.0;
        k /= 100.0;
    } else if (sscanf(p, "%lf,%lf,%lf,%lf%n", &c, &m, &y, &k, &n) == 4 && p[n] == '\0') {
        // no explicit %: assume any number in [0,1] is already normalized, otherwise assume missing % (convenience)
        if (c > 100.0 || m > 100.0 || y > 100.0 || k > 100.0 || 
            c <   0.0 || m <   0.0 || y <   0.0 || k <   0.0) return 0;

        if (c > 1.0) c /= 100.0;
        if (m > 1.0) m /= 100.0;
        if (y > 1.0) y /= 100.0;
        if (k > 1.0) k /= 100.0;
    } else return 0;

    out->cmyk  = (cmyk_t){ .c = c, .m = m, .y = y, .k = k };
    out->rgb   = cmyk_to_rgb(&out->cmyk);
    out->hex   = rgb_to_hex(&out->rgb);
    out->hsl   = rgb_to_hsl(&out->rgb);
    out->hsv   = rgb_to_hsv(&out->rgb);
    out->named = css_colors[closest_named_weighted_rgb_index(&out->rgb)];
    return 1;
}

// HSL: "hsl(h,s%,l%)", "hsl(h,s,l)"
// h in deg, s/l in percent or 0..1
static inline int parse_hsl(char *s, color_t *out)  {
    // quick reject inputs without necessary prefix
    if (strncmp(s, "hsl(", 4) != 0) return 0;

    char *p = s + 4;
    size_t L = strlen(p);
    if (L && p[L - 1] == ')') p[L - 1] = 0; else return 0;

    double h, sat, l;
    int    n = 0;
    if (sscanf(p, "%lf,%lf%%,%lf%%%n", &h, &sat, &l, &n) == 3 && p[n] == '\0') {
        // percent form: divide by 100 to get normalized value
        if (sat > 100.0 || l > 100.0 || 
            sat <   0.0 || l <   0.0) return 0;

        sat /= 100.0;
        l   /= 100.0;
    } else if (sscanf(p, "%lf,%lf,%lf%n", &h, &sat, &l, &n) == 3 && p[n] == '\0') {
        // no explicit %: assume any number in [0,1] is already normalized, otherwise assume missing % (convenience)
        if (sat > 100.0 || l > 100.0 || 
            sat <   0.0 || l <   0.0) return 0;

        if (sat > 1) sat /= 100.0;
        if (l > 1)   l   /= 100.0;
    } else return 0;

    out->hsl   = (hsl_t){ .h = fmod(h, 360.0), .sat = sat, .l = l };
    out->rgb   = hsl_to_rgb(&out->hsl);
    out->hex   = rgb_to_hex(&out->rgb);
    out->cmyk  = rgb_to_cmyk(&out->rgb);
    out->hsv   = rgb_to_hsv(&out->rgb);
    out->named = css_colors[closest_named_weighted_rgb_index(&out->rgb)];
    return 1;
}

// HSV: "hsv(h,s%,v%)", "hsv(h,s,v)", "h,s%,v%"
// bare requires (!) '%' for s and v
static inline int parse_hsv(char *s, color_t *out) {
    double h, sat, v;
    int    n = 0;
    if (strncmp(s, "hsv(", 4) == 0) {
        char *p = s + 4;
        size_t L = strlen(p);
        if (L && p[L-1] == ')') p[L-1] = 0; else return 0;

        if (sscanf(p, "%lf,%lf%%,%lf%%%n", &h, &sat, &v, &n) == 3 && p[n] == '\0') {
            // try percent form inside prefix
            if (sat > 100.0 || v > 100.0 ||
                sat <   0.0 || v <   0.0) return 0;

            sat /= 100.0;
            v   /= 100.0;
        }
        else if (sscanf(p, "%lf,%lf,%lf%n", &h, &sat, &v, &n) == 3 && p[n] == '\0') {
            // otherwise, do the same as in the previous parsers...
            if (sat > 100.0 || v > 100.0 ||
                sat <   0.0 || v <   0.0) return 0;
            
            if (sat > 1.0) sat /= 100.0;
            if (v   > 1.0) v   /= 100.0;
        } else return 0;
    } else {
        // bare form: require the percent-sign style h,s%,v%
        if (sscanf(s, "%lf,%lf%%,%lf%%%n", &h, &sat, &v, &n) == 3 && s[n] == '\0') {
            if (sat > 100.0 || v > 100.0 ||
                sat <   0.0 || v <   0.0) return 0;

            sat /= 100.0;
            v   /= 100.0;
        }
        else return 0; // bare non-percent triples are NOT accepted
    }

    out->hsv   = (hsv_t){ .h = fmod(h, 360.0), .sat = sat, .v = v };
    out->rgb   = hsv_to_rgb(&out->hsv);
    out->hex   = rgb_to_hex(&out->rgb);
    out->cmyk  = rgb_to_cmyk(&out->rgb);
    out->hsl   = rgb_to_hsl(&out->rgb);
    out->named = css_colors[closest_named_weighted_rgb_index(&out->rgb)];
    return 1;
}

// list of parser functions to iterate through
static parse_fn parsers[] = {parse_named, parse_hex, parse_rgb, parse_cmyk, parse_hsl, parse_hsv};

// public api
int parse_color(const char *in, color_t *out) {
    if (!in || !out) return 0;

    // copy and normalize input
    char s[STR_BUFSIZE];
    snprintf(s, sizeof(s), "%s", in);
    norm(s);

    // check color format by iterating through known parsers
    size_t nparsers = sizeof(parsers) / sizeof(parsers[0]);
    for (size_t i = 0; i < nparsers; ++i) {
        color_t tmp = { 0 };

        // create new mutable copy to be destroyed by current parser
        char t[STR_BUFSIZE];
        snprintf(t, sizeof(t), "%s", s);

        // found match, stop
        if (parsers[i](t, &tmp)) {
            *out = tmp;
            return 1;
        }
    }

    // string could not be parsed by any parser in list
    return 0;
}

void list_css_colors(int l) {
    size_t n = sizeof(css_colors) / sizeof(css_colors[0]);

    if (!l)
        for (size_t i = 0; i < n; ++i) {
            rgb_t rgb = hex_to_rgb(css_colors[i].hex);
            printf("\033[48;2;%d;%d;%dm   "C_RESET " %-21s#%06x\n", rgb.r, rgb.g, rgb.b, css_colors[i].name, css_colors[i].hex);
        }
    else {
        printf("name,color\n");
        for  (size_t i = 0; i < n; ++i) printf("%s,%06x\n", css_colors[i].name, css_colors[i].hex);
    }
}

// TODO: alpha
// TODO: lab space, CI76 or some better metric