#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "converter.h"
#include "utility.h"
#include "printer.h"

double dist2_rgb(const rgb_t *a, const rgb_t *b) {
    double dr = (double)a->r - (double)b->r;
    double dg = (double)a->g - (double)b->g;
    double db = (double)a->b - (double)b->b;
    return dr * dr + dg * dg + db * db;
}

double weighted_dist2_rgb(const rgb_t *a, const rgb_t *b, double wr, double wg, double wb) {
    double dr = (double)a->r - (double)b->r;
    double dg = (double)a->g - (double)b->g;
    double db = (double)a->b - (double)b->b;
    return wr * dr * dr + wg * dg * dg + wb * db * db;
}

double dist2_oklab(const oklab_t *a, const oklab_t *b) {
    return (a->L - b->L) * (a->L - b->L)
         + (a->a - b->a) * (a->a - b->a)
         + (a->b - b->b) * (a->b - b->b);
}

const char *tcolor_tostr(color_cap_t c) {
    switch (c) {
        case TC_NONE:      return "none";
        case TC_16:        return "16";
        case TC_256:       return "256";
        case TC_TRUECOLOR: return "truecolor";
        default:           return "unknown";
    }
}

bool cmp_rgb(const rgb_t *a, const rgb_t *b) { return a->r == b->r && a->g == b->g && a->b == b->b; }

int safe_atoi(const char *s, const char* progname) {
    if (s == NULL) ERROR_EXIT("null input"); // should not happen

    errno = 0;
    char *endptr;
    long val = strtol(s, &endptr, 10);

    if    (s == endptr)                     ERROR_EXIT("no digits found in input: '%s'", s);
    while (isspace((unsigned char)*endptr)) endptr++; // trailing whitespaces
    if    (*endptr != '\0')                 ERROR_EXIT("invalid trailing characters in input: '%s'", s);

    return (int)val;
}

bool strncat_safe(char *dst, size_t dst_sz, const char *piece, bool add_space) {
    size_t used = strlen(dst);
    size_t piece_len = strlen(piece);
    size_t need = piece_len + (add_space ? 1 : 0);
    if (need > 0 && (dst_sz - used - 1) < need) return false;
    if (add_space && used > 0) {
        strncat(dst, " ", dst_sz - used - 1);
        used += 1;
    }
    strncat(dst, piece, dst_sz - used - 1);
    return true;
}

bool strncasecmp_own(const char *a, const char *b, size_t n) {
    if (n == 0) return true;
    for (; n && *a && *b; ++a, ++b, --n) if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
    if (n == 0) return true;
    return *a == *b;
}

bool strcasecmp_own(const char *a, const char *b) {
    for (; *a && *b; ++a, ++b) if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
    return *a == *b;
}

bool strcasestr_own(const char *hay, const char *needle) {
    if (!hay || !needle) return 0;
    size_t nlen = strlen(needle);
    for (; *hay; ++hay) if (strncasecmp_own(hay, needle, nlen) == 0) return true;
    return false;
}

void fmt_color_strings(const color_t *colorptr, bool webfmt, int dplaces,
                       char *rgb,   size_t rgb_s,
                       char *hex,   size_t hex_s,
                       char *cmyk,  size_t cmyk_s,
                       char *hsl,   size_t hsl_s,
                       char *hsv,   size_t hsv_s,
                       char *oklab, size_t oklab_s,
                       char *oklch, size_t oklch_s,
                       char *named, size_t named_s) {
    if (webfmt) {
        if (rgb && rgb_s)     snprintf(rgb,   rgb_s,   "rgb(%d,%d,%d)",                     colorptr->rgb.r, colorptr->rgb.g, colorptr->rgb.b);
        if (hex && hex_s)     snprintf(hex,   hex_s,   "#%06x",                             colorptr->hex);
        if (cmyk && cmyk_s)   snprintf(cmyk,  cmyk_s,  "cmyk(%.*f%%,%.*f%%,%.*f%%,%.*f%%)", dplaces, colorptr->cmyk.c * 100.0, dplaces, colorptr->cmyk.m * 100.0, dplaces, colorptr->cmyk.y * 100.0, dplaces, colorptr->cmyk.k * 100.0);
        if (hsl && hsl_s)     snprintf(hsl,   hsl_s,   "hsl(%.*f,%.*f%%,%.*f%%)",           dplaces, colorptr->hsl.h, dplaces, colorptr->hsl.sat * 100.0, dplaces, colorptr->hsl.l * 100.0);
        if (hsv && hsv_s)     snprintf(hsv,   hsv_s,   "hsv(%.*f,%.*f%%,%.*f%%)",           dplaces, colorptr->hsv.h, dplaces, colorptr->hsv.sat * 100.0, dplaces, colorptr->hsv.v * 100.0);
        if (oklab && oklab_s) snprintf(oklab, oklab_s, "oklab(%.*f%%,%.*f,%.*f)",           dplaces, colorptr->oklab.L * 100.0, dplaces, colorptr->oklab.a, dplaces, colorptr->oklab.b);
        if (oklch && oklch_s) snprintf(oklch, oklch_s, "oklch(%.*f%%,%.*f%%,%.*f)",         dplaces, colorptr->oklch.L * 100.0, dplaces, colorptr->oklch.c * 100.0, dplaces, colorptr->oklch.h);
        if (named && named_s) snprintf(named, named_s, "%s (#%06x) (dist² %.*f)",           colorptr->named.name, colorptr->named.hex, dplaces, colorptr->named.diff);
    } else {
        if (rgb && rgb_s)     snprintf(rgb,   rgb_s,   "%d,%d,%d",                          colorptr->rgb.r, colorptr->rgb.g, colorptr->rgb.b);
        if (hex && hex_s)     snprintf(hex,   hex_s,   "%06x",                              colorptr->hex);
        if (cmyk && cmyk_s)   snprintf(cmyk,  cmyk_s,  "%.*f%%,%.*f%%,%.*f%%,%.*f%%",       dplaces, colorptr->cmyk.c * 100.0, dplaces, colorptr->cmyk.m * 100.0, dplaces, colorptr->cmyk.y * 100.0, dplaces, colorptr->cmyk.k * 100.0);
        if (hsl && hsl_s)     snprintf(hsl,   hsl_s,   "%.*f,%.*f%%,%.*f%%",                dplaces, colorptr->hsl.h, dplaces, colorptr->hsl.sat * 100.0, dplaces, colorptr->hsl.l * 100.0);
        if (hsv && hsv_s)     snprintf(hsv,   hsv_s,   "%.*f,%.*f%%,%.*f%%",                dplaces, colorptr->hsv.h, dplaces, colorptr->hsv.sat * 100.0, dplaces, colorptr->hsv.v * 100.0);
        if (oklab && oklab_s) snprintf(oklab, oklab_s, "%.*f%%,%.*f,%.*f",                  dplaces, colorptr->oklab.L * 100.0, dplaces, colorptr->oklab.a, dplaces, colorptr->oklab.b);
        if (oklch && oklch_s) snprintf(oklch, oklch_s, "%.*f%%,%.*f%%,%.*f",                dplaces, colorptr->oklch.L * 100.0, dplaces, colorptr->oklch.c * 100.0, dplaces, colorptr->oklch.h);
        if (named && named_s) snprintf(named, named_s, "%s (%06x) (dist² %.*f)",            colorptr->named.name, colorptr->named.hex, dplaces, colorptr->named.diff);
    }
}


int map_rgb_to_sgr_strings(color_cap_t mapping, const rgb_t *rgb_in, char *bgbufptr, size_t bgbufsz, char *fgbufptr, size_t fgbufsz) {
    if (mapping == TC_NONE)      {                                       if (bgbufsz > 0)   bgbufptr[0] = '\0';                                               if (fgbufsz > 0)   fgbufptr[0] = '\0';                                               return -1;  }
    if (mapping == TC_16)        { int idx = rgb_to_ansi16_idx(rgb_in);  snprintf(bgbufptr, bgbufsz, "\x1b[%dm", ansi16_idx_to_sgr_bg(idx));                  snprintf(fgbufptr, fgbufsz, "\x1b[%dm", ansi16_idx_to_sgr_fg(idx));                  return idx; }
    if (mapping == TC_256)       { int idx = rgb_to_ansi256_idx(rgb_in); snprintf(bgbufptr, bgbufsz, "\x1b[48;5;%dm", idx);                                   snprintf(fgbufptr, fgbufsz, "\x1b[38;5;%dm", idx);                                   return idx; }
    if (mapping == TC_TRUECOLOR) {                                       snprintf(bgbufptr, bgbufsz, "\033[48;2;%d;%d;%dm", rgb_in->r, rgb_in->g, rgb_in->b); snprintf(fgbufptr, fgbufsz, "\033[38;2;%d;%d;%dm", rgb_in->r, rgb_in->g, rgb_in->b); return -1;  }
    return -1;
}