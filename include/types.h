// typedefs and macro definitions
#ifndef TYPES_H
#define TYPES_H

#define ZERO_THRESH 1e-12
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define CLAMP(_n, _l, _r) ((_n) < (_l) ? (_l) : ((_n) > (_r) ? (_r) : (_n)))

#define STR_BUFSIZE   128
#define C_STR_BUFSIZE 20
#define C_COL_BUFSIZE 64

#define NULLSTR  "<NULL>"

// used in tests (assume color supported, cba disabling now)
#define C_BOLD   "\x1b[1m"
#define C_RED    "\x1b[31m"
#define C_LRED   "\x1b[91m"
#define C_GREEN  "\x1b[32m"
#define C_LGREEN "\x1b[92m"

// supported color models
typedef struct { int r, g, b; }                              rgb_t;
typedef struct { double c, m, y, k; }                        cmyk_t;
typedef struct { double h, sat, l; }                         hsl_t;
typedef struct { double h, sat, v; }                         hsv_t;
typedef unsigned                                             hex_t;
typedef struct { const char *name; hex_t hex; double diff; } named_t;

// color struct including all color models
typedef struct { 
    rgb_t   rgb;
    hex_t   hex;
    cmyk_t  cmyk;
    hsl_t   hsl;
    hsv_t   hsv;
    named_t named;
} color_t;

// parser function for parsing a string
//
// returns 1 if the string could be parsed and writes the result to *out
// returns 0 if the string could not be parsed and does nothing with *out
typedef int (*parse_fn)(char *, color_t *);

#endif