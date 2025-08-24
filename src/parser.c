#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "converter.h"
#include "parser.h"
#include "tables.h"

#define COPY_OR_RETURN(_dst,_src) do { int n = snprintf(_dst, sizeof(_dst), "%s", _src); if (n < 0) return 0; if ((size_t)n >= sizeof(_dst)) return 0; } while (0)

// internal name array selection
static const named_t *names    = css_colors;
static       size_t names_size = css_colors_size;

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
    // source: https://www.compuphase.com/cmetric.htm
    const double wr = 0.22216091748149788;
    const double wg = 0.4288860259783791;
    const double wb = 0.34895305654012304;

    double best_score = 1.79769e+308; // DBL_MAX
    size_t best_idx = 0;

    for (size_t i = 0; i < names_size; ++i) {
        rgb_t named = hex_to_rgb(names[i].hex);
        double d = weighted_dist2_rgb(in, &named, wr, wg, wb);
        if (d < best_score) {
            best_score = d;
            best_idx = i;
        }
    }

    named_t closest = names[best_idx];
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

// NAMED: any valid named color
static inline int parse_named(char *s, color_t *out) {
    if (!s || !*s) return 0;

    for (size_t i = 0; i < names_size; ++i) {
        if (strcmp(s, names[i].name) == 0) {
            hex_t v = names[i].hex;
            out->hex   = v;
            out->rgb   = hex_to_rgb(v);
            out->cmyk  = rgb_to_cmyk(&out->rgb);
            out->hsl   = rgb_to_hsl(&out->rgb);
            out->hsv   = rgb_to_hsv(&out->rgb);
            out->named = names[i];
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

void list_colors(int l) {
    if (!l)
        for (size_t i = 0; i < names_size; ++i) {
            rgb_t rgb = hex_to_rgb(names[i].hex);
            printf("\033[48;2;%d;%d;%dm   "C_RESET " %-21s#%06x\n", rgb.r, rgb.g, rgb.b, names[i].name, names[i].hex);
        }
    else {
        printf("name,color\n");
        for (size_t i = 0; i < names_size; ++i) printf("%s,%06x\n", names[i].name, names[i].hex);
    }
}

void use_xkcd() {
    names      = xkcd_colors;
    names_size = xkcd_colors_size;
}

// TODO: alpha
// TODO: lab space, CI76 or some better metric