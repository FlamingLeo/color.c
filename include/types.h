// typedefs and macro definitions
#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

#define MIN(a,b)              ((a) < (b) ? (a) : (b))
#define MAX(a,b)              ((a) > (b) ? (a) : (b))
#define CLAMP(_n, _l, _r)     ((_n) < (_l) ? (_l) : ((_n) > (_r) ? (_r) : (_n)))
#define ERROR_EXIT(_fmt, ...) do { fprintf(stderr, "error: " _fmt "\n\n", ##__VA_ARGS__); print_usage(stderr, progname); exit(EXIT_FAILURE); } while(0)

#define ZERO_THRESH 1e-12
#define STR_BUFSIZE   128
#define C_STR_BUFSIZE 20
#define C_COL_BUFSIZE 64

// source: https://en.wikipedia.org/wiki/YCbCr#ITU-R_BT.601_conversion
#define W_R 0.299
#define W_G 0.587
#define W_B 0.114

#define NULLSTR  "<NULL>"

// used in tests (assume color supported, cba disabling now)
#define C_RESET  "\x1b[0m"
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

// terminal color options enum
typedef enum {
    TC_NONE      = 0,
    TC_16        = 16,
    TC_256       = 256,
    TC_TRUECOLOR = 16777216
} color_cap_t;

// printing context
typedef struct {
    int  cheight;      // height of the color preview rectangle
    int  cwidth;       // width of the color preview rectangle
    const char *reset; // reset escape code (empty string for no color)
    bool txtclr;       // plain text (no color)?
    char *bgbufptr;    // pointer to char buffer containing background color ANSI escape code
    char *fgbufptr;    // pointer to char buffer containing foreground color ANSI escape code
} print_ctx_t;

// program options container
typedef struct {
    color_cap_t mapping;    // terminal color mode
    bool        cwset;      // did we set the color preview width manually via the command line?
    int         cwidth;     // color preview width
    int         dplaces;    // number of decimal places printed for floats (rounding via printf)
    bool        webfmt;     // display colors in css format (e.g. "rgb(255,255,255)" instead of "255,255,255")?
    bool        txtclr;     // should the text be colored aswell?
    bool        json;       // print out in json format?
    char       *conversion; // pointer into argv
    bool        distance;   // should we do distance calculation between two colors?
} prog_opts_t;

#endif