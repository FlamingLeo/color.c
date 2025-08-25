#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "utility.h"

static int strstrci(const char *hay, const char *needle) {
    if (!hay || !needle) return 0;
    size_t nlen = strlen(needle);
    for (; *hay; ++hay) if (strncasecmp(hay, needle, nlen) == 0) return 1;
    return 0;
}

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

const char *tcolor_tostr(color_cap_t c) {
    switch (c) {
        case TC_NONE:      return "none";
        case TC_16:        return "16";
        case TC_256:       return "256";
        case TC_TRUECOLOR: return "truecolor";
        default:           return "unknown";
    }
}

color_cap_t detect_terminal_color() {
    // not a tty: no color
    if (!isatty(STDOUT_FILENO)) return TC_NONE;

    // respect NO_COLOR envvar (https://no-color.org/)
    if (getenv("NO_COLOR")) return TC_NONE;

    // check COLORTERM envvar
    const char *colorterm = getenv("COLORTERM");
    if (colorterm && (strstr(colorterm, "truecolor") || strstr(colorterm, "24bit") || strstrci(colorterm, "24bit"))) return TC_TRUECOLOR;

    // check TERM envvar for corresponding string
    // note: on my machine, there's a discrepancy between COLORTERM (truecolor) and TERM (xterm-256color)
    //       this is why COLORTERM is preferred, but keep in mind that results and correctness may vary
    const char *term = getenv("TERM");
    if (term) {
        if (strstr(term, "256color"))                           return TC_256;
        if (strstr(term, "truecolor") || strstr(term, "24bit")) return TC_TRUECOLOR;
    }

    // assume no color for safety if no above check passes
    // the user may still attempt to force a color mapping either way
    return TC_NONE;
}

bool cmp_rgb(const rgb_t *a, const rgb_t *b) { return a->r == b->r && a->g == b->g && a->b == b->b; }