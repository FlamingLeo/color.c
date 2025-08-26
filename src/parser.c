#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "converter.h"
#include "parser.h"
#include "tables.h"
#include "utility.h"

#define COPY_OR_RETURN(_dst,_src) do { int n = snprintf(_dst, sizeof(_dst), "%s", _src); if (n < 0) return 0; if ((size_t)n >= sizeof(_dst)) return 0; } while (0)

// internal name array selection
static const named_t *names    = css_colors;
static       size_t names_size = css_colors_size;

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

// helper function to determine if all digits in a string are hex digits
static inline bool all_hexdigits(const char *ptr, size_t len) {
    for (size_t i = 0; i < len; ++i) if (!isxdigit((unsigned char)ptr[i])) return false;
    return true;
}

// internal parsers: return 1 on success and set the out parameters out->{r,g,b}
//                   return 0 on failure
// destructive!      parsers destroy input string, watch out!

// NAMED: any valid named color (either css or xkcd)
//
// css is parsed first, then xkcd
// if both contain a named struct with the same name, the css variant is chosen first
//
// note: the "named" struct of the out parameter will still be the best approximation based on the current choice of colors
//       so, if we use css colors but input an xkcd color, we will still get the closest approximation to a named css color
static inline int parse_named(char *s, color_t *out) {
    if (!s || !*s) return 0;

    size_t i;

    // iterate through css colors...
    for (i = 0; i < css_colors_size; ++i) {
        if (strcmp(s, css_colors[i].name) == 0) {
            hex_t v    = css_colors[i].hex;
            out->hex   = v;
            out->rgb   = hex_to_rgb(v);
            out->cmyk  = rgb_to_cmyk(&out->rgb);
            out->hsl   = rgb_to_hsl(&out->rgb);
            out->hsv   = rgb_to_hsv(&out->rgb);
            out->named = closest_named_weighted_rgb(&out->rgb);
            return 1;
        }
    }

    // ... then xkcd, if we haven't found anything...
    for (i = 0; i < xkcd_colors_size; ++i) {
        if (strcmp(s, xkcd_colors[i].name) == 0) {
            hex_t v    = xkcd_colors[i].hex;
            out->hex   = v;
            out->rgb   = hex_to_rgb(v);
            out->cmyk  = rgb_to_cmyk(&out->rgb);
            out->hsl   = rgb_to_hsl(&out->rgb);
            out->hsv   = rgb_to_hsv(&out->rgb);
            out->named = closest_named_weighted_rgb(&out->rgb);
            return 1;
        }
    }

    // ...then we REALLY haven't found anything
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
named_t closest_named_weighted_rgb(const rgb_t *in) {
    double best_score = 1e300;
    size_t best_idx   = 0;

    for (size_t i = 0; i < names_size; ++i) {
        rgb_t named = hex_to_rgb(names[i].hex);
        double d = weighted_dist2_rgb(in, &named, W_R, W_G, W_B);
        if (d < best_score) {
            best_score = d;
            best_idx = i;
        }
    }

    named_t closest = names[best_idx];
    closest.diff = best_score;
    return closest;
}

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

int parse_color2(const char *in, color_t *out0, color_t *out1) {
    if (!in || !out0 || !out1) return 0;

    const char *p      = in; // current place in input string
    int         parsed = 0;  // counts how many colors were successfully parsed

    // run logic for both colors
    for (int cidx = 0; cidx < 2; ++cidx) {
        while (*p && isspace((unsigned char)*p)) p++;
        if    (!*p) break;

        const char *lsp               = NULL;  // last success pointer
        color_t best_tmp              = { 0 }; // best / longest candidate found so far
        char buf[STR_BUFSIZE]; buf[0] = '\0';  // candidate strings will be built here

        const char *inner   = p;
        int         tc      = 0; // number of whitespace-separated words appended into buf

        // extend candidate word by word
        // word = run of non-space characters
        while (*inner) {
            while (*inner && isspace((unsigned char)*inner)) inner++;
            if (!*inner) break;

            const char *q = inner;
            while (*q && !isspace((unsigned char)*q)) q++;
            size_t wordlen = (size_t)(q - inner);
            if (wordlen == 0) break;

            // ensure room in buf or break to prevent buffer overflow
            size_t len = strlen(buf);
            if (len + (len ? 1 : 0) + wordlen >= sizeof(buf)) break;

            // append word with single space separator if buf non-empty
            if (len) { buf[len] = ' '; buf[len + 1] = '\0'; len++; }
            memcpy(buf + len, inner, wordlen); buf[len + wordlen] = '\0';
            tc++;

            // try to parse the current candidate
            color_t tmp;
            if (parse_color(buf, &tmp)) {
                // if this candidate was formed by concatenating >=2 tokens, check whether every individual token also parses
                // if yes, prefer the earlier success, otherwise accept the merged candidate
                bool all = false;
                if (tc < 2) {
                    // accept single-token candidate
                    lsp      = q;
                    best_tmp = tmp;
                } else {
                    // scan tokens from p up to q and test each one individually
                    const char *t      = p;
                    bool        tok_ok = true;
                    char        tokenbuf[STR_BUFSIZE];

                    while (t < q) {
                        while (t < q && isspace((unsigned char)*t)) t++;
                        if (t >= q) break;

                        // same logic as before
                        const char *tend = t;
                        while (tend < q && !isspace((unsigned char)*tend)) tend++;
                        size_t tlen = (size_t)(tend - t);
                        if (tlen == 0 || tlen >= sizeof(tokenbuf)) { tok_ok = false; break; }
                        memcpy(tokenbuf, t, tlen); tokenbuf[tlen] = '\0';

                        color_t single_tmp;
                        if (!parse_color(tokenbuf, &single_tmp)) { tok_ok = false; break; }

                        t = tend; // advance to next token
                    } // end scanning tokens

                    // if not all tokens parsed individually, merged candidate is legitimate, because some token alone would not parse
                    // otherwise, reject candidate since every token parses individually and prefer earlier success we recorded
                    all = tok_ok;
                    if (!all) { lsp = q; best_tmp = tmp; }
                }
            }
            inner = q; // advance to next word
        } // end extending candidate

        // no parsed color found starting at p
        if (!lsp) break;

        // commit parsed color into appropriate out parameter
        if (cidx == 0) *out0 = best_tmp;
        else           *out1 = best_tmp;
        parsed++;

        // advance p to after the consumed words for the committed color
        p = lsp;
    } // end for both colors

    return parsed;
}


void list_colors(int l, color_cap_t mode) {
    if (!l && mode == TC_TRUECOLOR)
        for (size_t i = 0; i < names_size; ++i) {
            rgb_t rgb = hex_to_rgb(names[i].hex);
            printf("\033[48;2;%d;%d;%dm    \x1b[0m %-21s#%06x\n", rgb.r, rgb.g, rgb.b, names[i].name, names[i].hex);
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