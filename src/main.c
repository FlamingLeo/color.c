#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "parser.h"

// ansi escape code format (24-bit)
// source: https://en.wikipedia.org/wiki/ANSI_escape_code
//   ESC[38;2;⟨r⟩;⟨g⟩;⟨b⟩m Select RGB foreground color
//   ESC[48;2;⟨r⟩;⟨g⟩;⟨b⟩m Select RGB background color

#define ERROR_EXIT(fmt, ...)         do { fprintf(stderr, "error: " fmt "\n\n", ##__VA_ARGS__); print_usage(stderr, progname); return 1; } while(0)
#define PRINT_COLOR(label, fmt, ...) do { printf("%s%*s" C_RESET "%s%s%-5s : ", cheight-- > 0 ? bgbuf : "", cwidth, cwidth > 0 ? " " : "", cwidth > 0 ? "   " : "", txtclr ? fgbuf : "", label); printf((fmt), __VA_ARGS__); printf(C_RESET "\n"); } while (0)
#define PRINT_COLOR_EMPTY()          do { printf("%s%*s" C_RESET "\n", cheight-- > 0 ? bgbuf : "", cwidth, cwidth > 0 ? " " : ""); } while (0)
#define DPLACES(_n, _d)              ((((_n) < 10) ? 1 : ((_n) < 100) ? 2 : 3) + (_d))
#define CLAMP(_n, _l, _r)            ((_n) < (_l) ? (_l) : ((_n) > (_r) ? (_r) : (_n)))

// helper print functions
static inline void print_usage(FILE* stream, const char *progname) { fprintf(stream, "usage: %s [-d n] [-w n] [-t] [-x] [-h] color\nsee readme or help for a list of valid formats\n", progname); }
static inline void print_help(const char* progname) {
    printf(C_BOLD "color - a color printing (and conversion) tool for true color terminals\n\n" C_RESET);
    print_usage(stdout, progname);
    printf("\noptions:\n"
           "  -h        : show this help text\n"
           "  -d [0..10]: choose the maximum amount of decimal places to print             (default: 2)\n"
           "  -w [0..25]: choose the width of the left color square to display (h = w / 2) (default: 14)\n"
           "  -t        : disable coloring text output (useful for hard-to-read colors)    (default: true)\n"
           "  -x        : print colors in web format (css)                                 (default: false)\n");
    printf("\nvalid color formats (case-insensitive):\n"
           "  named: any valid named css color (e.g. forestgreen, mediumblue...)\n"
           "  rgb:   rgb(r,g,b)\n"
           "         r,g,b\n"
           "  hex:   #rrggbb\n"
           "         0xrrggbb\n"
           "         xrrggbb\n"
           "         rrggbb\n"
           "         all above variants as shorthand (e.g. #rgb) or surrounded by \"hex(...)\"\n"
           "  cmyk:  cmyk(c%%,m%%,y%%,k%%)\n"
           "         cmyk(c,m,y,k)\n"
           "         c%%,m%%,y%%,k%%\n"
           "         c,m,y,k\n"
           "  hsl:   hsl(h,s%%,l%%)\n"
           "         hsl(h,s,l)\n"
           "  hsv:   hsv(h,s%%,v%%)\n"
           "         hsv(h,s,v)\n"
           "         h,s%%,v%%\n");
#if defined(GIT_HASH) && defined(GIT_BRANCH) && defined(COMPILE_TIME)
    printf("\nhash:   " GIT_HASH
           "\nbranch: " GIT_BRANCH
           "\ntime:   " COMPILE_TIME "\n");
#endif
}

// usage: <progname> [options] color
int main(int argc, char **argv) {
    const char* progname = argv[0];
    if (argc < 2) ERROR_EXIT("at least one argument must be passed (-h or color)");

    // terminal output config variables
    int  cwidth  = 14;   // width of the color display on the left
                         // height is automatically computed as half of the width to (almost) create a square
    int  dplaces = 2;    // number of decimal places to round output to for numbers with decimal values (cmyk, hsl...)
    bool webfmt = false; // display colors in css format
    bool txtclr = true;  // color text output

    // handle command line options
    // i should probably use strtol here but the variables being modified aren't really critical
    int arg = 1;
    while ((argc > arg) && (argv[arg][0] == '-')) {
        if (argv[arg][1] == 'x') webfmt = true;
        if (argv[arg][1] == 't') txtclr = false;
        if (argv[arg][1] == 'w' && argc > arg + 1) { cwidth  = atoi(argv[++arg]); cwidth  = CLAMP(cwidth , 0, 25); }
        if (argv[arg][1] == 'd' && argc > arg + 1) { dplaces = atoi(argv[++arg]); dplaces = CLAMP(dplaces, 0, 10); }
        if (argv[arg][1] == 'h') { print_help(progname); return 0; }
        arg++;
    }
    int cheight = (cwidth >> 1) + (cwidth % 2);

    // once we're past options, we assume the remaining argv strings form a color
    // there may be multiple due to whitespace, e.g. ("r, g, b" -> "r,", "g,", "b")
    // this is why we need to safely concatenate them
    // if we can't, we exit
    char colorbuf[STR_BUFSIZE] = "";
    for(; arg < argc; ++arg) {
        if (strlen(argv[arg]) + 1 > sizeof(colorbuf) - strlen(colorbuf)) ERROR_EXIT("input too long");
        else strncat(colorbuf, argv[arg], sizeof(colorbuf) - strlen(colorbuf) - 1);
    }

    // attempt to parse color
    color_t color = { 0 }; if(!parse_color(colorbuf, &color)) ERROR_EXIT("invalid syntax %s", colorbuf);

    // do the thing
    char bgbuf[C_STR_BUFSIZE]; snprintf(bgbuf, C_STR_BUFSIZE, "\033[48;2;%d;%d;%dm", color.rgb.r, color.rgb.g, color.rgb.b);
    char fgbuf[C_STR_BUFSIZE]; snprintf(fgbuf, C_STR_BUFSIZE, "\033[38;2;%d;%d;%dm", color.rgb.r, color.rgb.g, color.rgb.b);
    
    if (webfmt) {
        PRINT_COLOR("RGB" , "rgb(%d,%d,%d)",                     color.rgb.r, color.rgb.g, color.rgb.b);
        PRINT_COLOR("Hex" , "#%06x",                             color.hex);
        PRINT_COLOR("CMYK", "cmyk(%.*g%%,%.*g%%,%.*g%%,%.*g%%)", DPLACES(color.cmyk.c * 100.0, dplaces), color.cmyk.c * 100.0, DPLACES(color.cmyk.m * 100.0, dplaces), color.cmyk.m * 100.0, DPLACES(color.cmyk.y * 100.0, dplaces), color.cmyk.y * 100.0, DPLACES(color.cmyk.k * 100.0, dplaces), color.cmyk.k * 100.0);
        PRINT_COLOR("HSL" , "hsl(%.*g,%.*g%%,%.*g%%)",           DPLACES(color.hsl.h, dplaces), color.hsl.h, DPLACES(color.hsl.sat * 100.0, dplaces), color.hsl.sat * 100.0, DPLACES(color.hsl.l * 100.0, dplaces), color.hsl.l * 100.0);
        PRINT_COLOR("HSV" , "hsv(%.*g,%.*g%%,%.*g%%)",           DPLACES(color.hsv.h, dplaces), color.hsv.h, DPLACES(color.hsv.sat * 100.0, dplaces), color.hsv.sat * 100.0, DPLACES(color.hsv.v * 100.0, dplaces), color.hsv.v * 100.0);
        PRINT_COLOR_EMPTY();
        PRINT_COLOR("Named" , "%s (#%06x)",                      color.named.name, color.named.hex);
    }
    else {
        PRINT_COLOR("RGB" , "%d,%d,%d",                          color.rgb.r, color.rgb.g, color.rgb.b);
        PRINT_COLOR("Hex" , "%06x",                              color.hex);
        PRINT_COLOR("CMYK", "%.*g%%,%.*g%%,%.*g%%,%.*g%%",       DPLACES(color.cmyk.c * 100.0, dplaces), color.cmyk.c * 100.0, DPLACES(color.cmyk.m * 100.0, dplaces), color.cmyk.m * 100.0, DPLACES(color.cmyk.y * 100.0, dplaces), color.cmyk.y * 100.0, DPLACES(color.cmyk.k * 100.0, dplaces), color.cmyk.k * 100.0);
        PRINT_COLOR("HSL" , "%.*g,%.*g%%,%.*g%%",                DPLACES(color.hsl.h, dplaces), color.hsl.h, DPLACES(color.hsl.sat * 100.0, dplaces), color.hsl.sat * 100.0, DPLACES(color.hsl.l * 100.0, dplaces), color.hsl.l * 100.0);
        PRINT_COLOR("HSV" , "%.*g,%.*g%%,%.*g%%",                DPLACES(color.hsv.h, dplaces), color.hsv.h, DPLACES(color.hsv.sat * 100.0, dplaces), color.hsv.sat * 100.0, DPLACES(color.hsv.v * 100.0, dplaces), color.hsv.v * 100.0);
        PRINT_COLOR_EMPTY();
        PRINT_COLOR("Named" , "%s (%06x)",                       color.named.name, color.named.hex);
    }
    for(; cheight > 0; --cheight) printf("%s%*s" C_RESET "\n", bgbuf, cwidth, " ");

    return 0;
}