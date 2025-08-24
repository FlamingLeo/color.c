#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "converter.h"
#include "parser.h"

#define COPY_OR_RETURN(_dst,_src) do { int n = snprintf(_dst, sizeof(_dst), "%s", _src); if (n < 0) return 0; if ((size_t)n >= sizeof(_dst)) return 0; } while (0)

// all 148 named css colors
// source: https://github.com/bahamas10/css-color-names/
static const named_t css_colors[] = {
    { "aliceblue"           , 0xf0f8ff, 0.0 }, { "antiquewhite"        , 0xfaebd7, 0.0 }, { "aqua"                , 0x00ffff, 0.0 }, { "aquamarine"          , 0x7fffd4, 0.0 }, 
    { "azure"               , 0xf0ffff, 0.0 }, { "beige"               , 0xf5f5dc, 0.0 }, { "bisque"              , 0xffe4c4, 0.0 }, { "black"               , 0x000000, 0.0 }, 
    { "blanchedalmond"      , 0xffebcd, 0.0 }, { "blue"                , 0x0000ff, 0.0 }, { "blueviolet"          , 0x8a2be2, 0.0 }, { "brown"               , 0xa52a2a, 0.0 }, 
    { "burlywood"           , 0xdeb887, 0.0 }, { "cadetblue"           , 0x5f9ea0, 0.0 }, { "chartreuse"          , 0x7fff00, 0.0 }, { "chocolate"           , 0xd2691e, 0.0 }, 
    { "coral"               , 0xff7f50, 0.0 }, { "cornflowerblue"      , 0x6495ed, 0.0 }, { "cornsilk"            , 0xfff8dc, 0.0 }, { "crimson"             , 0xdc143c, 0.0 }, 
    { "cyan"                , 0x00ffff, 0.0 }, { "darkblue"            , 0x00008b, 0.0 }, { "darkcyan"            , 0x008b8b, 0.0 }, { "darkgoldenrod"       , 0xb8860b, 0.0 }, 
    { "darkgray"            , 0xa9a9a9, 0.0 }, { "darkgreen"           , 0x006400, 0.0 }, { "darkgrey"            , 0xa9a9a9, 0.0 }, { "darkkhaki"           , 0xbdb76b, 0.0 }, 
    { "darkmagenta"         , 0x8b008b, 0.0 }, { "darkolivegreen"      , 0x556b2f, 0.0 }, { "darkorange"          , 0xff8c00, 0.0 }, { "darkorchid"          , 0x9932cc, 0.0 }, 
    { "darkred"             , 0x8b0000, 0.0 }, { "darksalmon"          , 0xe9967a, 0.0 }, { "darkseagreen"        , 0x8fbc8f, 0.0 }, { "darkslateblue"       , 0x483d8b, 0.0 }, 
    { "darkslategray"       , 0x2f4f4f, 0.0 }, { "darkslategrey"       , 0x2f4f4f, 0.0 }, { "darkturquoise"       , 0x00ced1, 0.0 }, { "darkviolet"          , 0x9400d3, 0.0 }, 
    { "deeppink"            , 0xff1493, 0.0 }, { "deepskyblue"         , 0x00bfff, 0.0 }, { "dimgray"             , 0x696969, 0.0 }, { "dimgrey"             , 0x696969, 0.0 }, 
    { "dodgerblue"          , 0x1e90ff, 0.0 }, { "firebrick"           , 0xb22222, 0.0 }, { "floralwhite"         , 0xfffaf0, 0.0 }, { "forestgreen"         , 0x228b22, 0.0 }, 
    { "fuchsia"             , 0xff00ff, 0.0 }, { "gainsboro"           , 0xdcdcdc, 0.0 }, { "ghostwhite"          , 0xf8f8ff, 0.0 }, { "goldenrod"           , 0xdaa520, 0.0 }, 
    { "gold"                , 0xffd700, 0.0 }, { "gray"                , 0x808080, 0.0 }, { "green"               , 0x008000, 0.0 }, { "greenyellow"         , 0xadff2f, 0.0 }, 
    { "grey"                , 0x808080, 0.0 }, { "honeydew"            , 0xf0fff0, 0.0 }, { "hotpink"             , 0xff69b4, 0.0 }, { "indianred"           , 0xcd5c5c, 0.0 }, 
    { "indigo"              , 0x4b0082, 0.0 }, { "ivory"               , 0xfffff0, 0.0 }, { "khaki"               , 0xf0e68c, 0.0 }, { "lavenderblush"       , 0xfff0f5, 0.0 }, 
    { "lavender"            , 0xe6e6fa, 0.0 }, { "lawngreen"           , 0x7cfc00, 0.0 }, { "lemonchiffon"        , 0xfffacd, 0.0 }, { "lightblue"           , 0xadd8e6, 0.0 }, 
    { "lightcoral"          , 0xf08080, 0.0 }, { "lightcyan"           , 0xe0ffff, 0.0 }, { "lightgoldenrodyellow", 0xfafad2, 0.0 }, { "lightgray"           , 0xd3d3d3, 0.0 }, 
    { "lightgreen"          , 0x90ee90, 0.0 }, { "lightgrey"           , 0xd3d3d3, 0.0 }, { "lightpink"           , 0xffb6c1, 0.0 }, { "lightsalmon"         , 0xffa07a, 0.0 }, 
    { "lightseagreen"       , 0x20b2aa, 0.0 }, { "lightskyblue"        , 0x87cefa, 0.0 }, { "lightslategray"      , 0x778899, 0.0 }, { "lightslategrey"      , 0x778899, 0.0 }, 
    { "lightsteelblue"      , 0xb0c4de, 0.0 }, { "lightyellow"         , 0xffffe0, 0.0 }, { "lime"                , 0x00ff00, 0.0 }, { "limegreen"           , 0x32cd32, 0.0 }, 
    { "linen"               , 0xfaf0e6, 0.0 }, { "magenta"             , 0xff00ff, 0.0 }, { "maroon"              , 0x800000, 0.0 }, { "mediumaquamarine"    , 0x66cdaa, 0.0 }, 
    { "mediumblue"          , 0x0000cd, 0.0 }, { "mediumorchid"        , 0xba55d3, 0.0 }, { "mediumpurple"        , 0x9370db, 0.0 }, { "mediumseagreen"      , 0x3cb371, 0.0 }, 
    { "mediumslateblue"     , 0x7b68ee, 0.0 }, { "mediumspringgreen"   , 0x00fa9a, 0.0 }, { "mediumturquoise"     , 0x48d1cc, 0.0 }, { "mediumvioletred"     , 0xc71585, 0.0 }, 
    { "midnightblue"        , 0x191970, 0.0 }, { "mintcream"           , 0xf5fffa, 0.0 }, { "mistyrose"           , 0xffe4e1, 0.0 }, { "moccasin"            , 0xffe4b5, 0.0 }, 
    { "navajowhite"         , 0xffdead, 0.0 }, { "navy"                , 0x000080, 0.0 }, { "oldlace"             , 0xfdf5e6, 0.0 }, { "olive"               , 0x808000, 0.0 }, 
    { "olivedrab"           , 0x6b8e23, 0.0 }, { "orange"              , 0xffa500, 0.0 }, { "orangered"           , 0xff4500, 0.0 }, { "orchid"              , 0xda70d6, 0.0 }, 
    { "palegoldenrod"       , 0xeee8aa, 0.0 }, { "palegreen"           , 0x98fb98, 0.0 }, { "paleturquoise"       , 0xafeeee, 0.0 }, { "palevioletred"       , 0xdb7093, 0.0 }, 
    { "papayawhip"          , 0xffefd5, 0.0 }, { "peachpuff"           , 0xffdab9, 0.0 }, { "peru"                , 0xcd853f, 0.0 }, { "pink"                , 0xffc0cb, 0.0 }, 
    { "plum"                , 0xdda0dd, 0.0 }, { "powderblue"          , 0xb0e0e6, 0.0 }, { "purple"              , 0x800080, 0.0 }, { "rebeccapurple"       , 0x663399, 0.0 }, 
    { "red"                 , 0xff0000, 0.0 }, { "rosybrown"           , 0xbc8f8f, 0.0 }, { "royalblue"           , 0x4169e1, 0.0 }, { "saddlebrown"         , 0x8b4513, 0.0 }, 
    { "salmon"              , 0xfa8072, 0.0 }, { "sandybrown"          , 0xf4a460, 0.0 }, { "seagreen"            , 0x2e8b57, 0.0 }, { "seashell"            , 0xfff5ee, 0.0 }, 
    { "sienna"              , 0xa0522d, 0.0 }, { "silver"              , 0xc0c0c0, 0.0 }, { "skyblue"             , 0x87ceeb, 0.0 }, { "slateblue"           , 0x6a5acd, 0.0 }, 
    { "slategray"           , 0x708090, 0.0 }, { "slategrey"           , 0x708090, 0.0 }, { "snow"                , 0xfffafa, 0.0 }, { "springgreen"         , 0x00ff7f, 0.0 }, 
    { "steelblue"           , 0x4682b4, 0.0 }, { "tan"                 , 0xd2b48c, 0.0 }, { "teal"                , 0x008080, 0.0 }, { "thistle"             , 0xd8bfd8, 0.0 }, 
    { "tomato"              , 0xff6347, 0.0 }, { "turquoise"           , 0x40e0d0, 0.0 }, { "violet"              , 0xee82ee, 0.0 }, { "wheat"               , 0xf5deb3, 0.0 }, 
    { "white"               , 0xffffff, 0.0 }, { "whitesmoke"          , 0xf5f5f5, 0.0 }, { "yellow"              , 0xffff00, 0.0 }, { "yellowgreen"         , 0x9acd32, 0.0 }, 
};

// squared weighted distance between two rgb colors (components 0..255)
static inline double weighted_dist2_rgb(const rgb_t *a, const rgb_t *b, double wr, double wg, double wb) {
    double dr = (double)a->r - (double)b->r;
    double dg = (double)a->g - (double)b->g;
    double db = (double)a->b - (double)b->b;
    return wr * dr * dr + wg * dg * dg + wb * db * db;
}

// find closest named color using weighted squared rgb distance
// technically, doing it with rgb isn't the best (even weighted), but imo it's a fast and good enough way of doing it for this small sample size
static named_t closest_named_weighted_rgb(const rgb_t *in) {
    size_t n = sizeof(css_colors) / sizeof(css_colors[0]);

    // source: https://www.compuphase.com/cmetric.htm
    const double wr = 0.22216091748149788;
    const double wg = 0.4288860259783791;
    const double wb = 0.34895305654012304;

    double best_score = 1.79769e+308; // DBL_MAX
    size_t best_idx = 0;

    for (size_t i = 0; i < n; ++i) {
        rgb_t named = hex_to_rgb(css_colors[i].hex);
        double d = weighted_dist2_rgb(in, &named, wr, wg, wb);
        if (d < best_score) {
            best_score = d;
            best_idx = i;
        }
    }

    named_t closest = css_colors[best_idx];
    closest.diff = best_score;
    return closest;
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
    out->named = closest_named_weighted_rgb(&out->rgb);
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
            out->named = closest_named_weighted_rgb(&out->rgb);
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
            out->named = closest_named_weighted_rgb(&out->rgb);
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
    out->named = closest_named_weighted_rgb(&out->rgb);
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
    out->named = closest_named_weighted_rgb(&out->rgb);
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
    out->named = closest_named_weighted_rgb(&out->rgb);
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