#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "parser.h"
#include "converter.h"
#include "utility.h"

// ansi escape code format (24-bit)
// source: https://en.wikipedia.org/wiki/ANSI_escape_code
//   ESC[38;2;⟨r⟩;⟨g⟩;⟨b⟩m Select RGB foreground color
//   ESC[48;2;⟨r⟩;⟨g⟩;⟨b⟩m Select RGB background color

#define ERROR_EXIT(fmt, ...)         do { fprintf(stderr, "error: " fmt "\n\n", ##__VA_ARGS__); print_usage(stderr, progname); return 1; } while(0)
#define PRINT_COLOR(label, fmt, ...) do { printf("%s%*s%s%s%s%-5s ", cheight-- > 0 ? bgbuf : "", cwidth, cwidth > 0 ? " " : "", reset, cwidth > 0 ? "   " : "", txtclr ? fgbuf : "", label); printf((fmt), __VA_ARGS__); printf("%s\n", reset); } while (0)
#define PRINT_COLOR_EMPTY()          do { printf("%s%*s%s\n", cheight-- > 0 ? bgbuf : "", cwidth, cwidth > 0 ? " " : "", reset); } while (0)
#define DPLACES(_n, _d)              ((((_n) < 10) ? 1 : ((_n) < 100) ? 2 : 3) + (_d))

// helper print functions
static inline void print_usage(FILE* stream, const char *progname) { fprintf(stream, "usage: %s [-c <model>] [-f <n>] [-h] [-j] [-l [0|1]] [-m <map>] [-p] [-w <n>] [-W] [-x] <color>\nsee readme or help for a list of valid formats\n", progname); }
static inline void print_help(const char* progname) {
    printf("color - a color printing (and conversion) tool for true color terminals\n\n");
    print_usage(stdout, progname);
    printf("\noptions:\n"
           "  -c <model>: only show the conversion of the chosen color to the specified model, then exit\n"
           "  -f <0..5> : choose the maximum amount of decimal places to print (default: 2)\n"
           "              0 rounds the numbers to the nearest integer\n"
           "  -h        : show this help text and exit\n"
           "  -j        : print output in json format\n"
           "  -l [0 | 1]: show a list of currently supported named colors and exit (default: 0)\n"
           "     - 0: human-readable format with sample, name and hex color\n"
           "     - 1: csv output with headers \"name\", \"color\", no sample\n"
           "  -m <map>  : map terminal color to 0-, 16-, 256- or true color output (default: your terminal's color mode)\n"
           "              you may try and force unsupported terminals render higher color modes\n"
           "  -p        : disable coloring text output (plain, for hard-to-read colors) (default: true)\n"
           "  -w <0..25>: choose the width of the left color square to display (h = w / 2) (default: 14)\n"
           "              0 disabled the color preview\n"
           "  -W        : print colors in web format (css) (default: false)\n"
           "  -x        : use xkcd color names instead of css (default: false)\n"
           "              this option must be set if you want to parse an xkcd color name\n");
    printf("\nvalid color formats (case-insensitive):\n"
           "  named: any valid named css / xkcd color (e.g. forestgreen, mediumblue...)\n"
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
    
    color_cap_t tmode = detect_terminal_color();
    const char *reset = "\x1b[0m";

    // terminal output config variables
    bool cwset          = false; // if the user hasn't chosen a custom width and the mapping isn't truecolor, we print extra information, which the width should handle to look nicer
    int  cwidth         = 14;    // width of the color display on the left
                                 // height is automatically computed as half of the width to (almost) create a square
    color_cap_t mapping = tmode; // 0 / 16 / 256 / 16777216 for colorless / 16-color / 256-color / true color (24-bit) terminals
    int  dplaces        = 2;     // number of decimal places to round output to for numbers with decimal values (cmyk, hsl...)
    bool webfmt         = false; // display colors in css format
    bool txtclr         = true;  // color text output
    bool json           = false; // print output in json format instead of the usual format
    char *conversion    = NULL;  // only display conversion to a specific format

    // handle command line options
    // i should probably use strtol here but the variables being modified aren't really critical
    int arg = 1;
    while ((argc > arg) && (argv[arg][0] == '-')) {
        // special options that change main execution
        if (argv[arg][1] == 'h') { print_help(progname); return 0; }
        if (argv[arg][1] == 'l') { int lmode = 0; if(argc > arg + 1) lmode = atoi(argv[++arg]); if (lmode < 0 || lmode > 1) ERROR_EXIT("invalid list mode %d", lmode); else list_colors(lmode, mapping); return 0; }
        
        // flags
        if (argv[arg][1] == 'j') json   = true;
        if (argv[arg][1] == 'p') txtclr = false;
        if (argv[arg][1] == 'W') webfmt = true;
        if (argv[arg][1] == 'x') use_xkcd();

        // options that require an argument
        if (argv[arg][1] == 'c' && argc > arg + 1) conversion = argv[++arg];
        if (argv[arg][1] == 'f' && argc > arg + 1) { dplaces = atoi(argv[++arg]); dplaces = CLAMP(dplaces, 0, 5);  }
        if (argv[arg][1] == 'm' && argc > arg + 1) { mapping = atoi(argv[++arg]); if (mapping != TC_NONE && mapping != TC_16 && mapping != TC_256 && mapping != TC_TRUECOLOR) ERROR_EXIT("invalid mapping (must be one of: 0, 16, 256, 16777216): %d", mapping); if (mapping > tmode) printf("warning: mapping %d (%s) might be unsupported by this terminal (color mode: %s)\n", mapping, tcolor_tostr(mapping), tcolor_tostr(tmode)); }
        if (argv[arg][1] == 'w' && argc > arg + 1) { cwidth  = atoi(argv[++arg]); cwidth  = CLAMP(cwidth , 0, 25); cwset = true; }
        
        arg++;
    }
    if (!cwset && mapping != TC_TRUECOLOR) ++cwidth;
    int cheight = (cwidth >> 1) + (cwidth % 2);

    // once we're past options, we assume the remaining argv strings form a color
    // there may be multiple due to whitespace, e.g. ("r, g, b" -> "r,", "g,", "b")
    // this is why we need to safely concatenate them
    // if we can't, we exit
    char colorbuf[STR_BUFSIZE] = "";
    for (; arg < argc; ++arg) {
        if (strlen(argv[arg]) + 1 > sizeof(colorbuf) - strlen(colorbuf)) ERROR_EXIT("input too long");
        else strncat(colorbuf, argv[arg], sizeof(colorbuf) - strlen(colorbuf) - 1);
    }

    // attempt to parse color
    color_t color = { 0 }; if(!parse_color(colorbuf, &color)) ERROR_EXIT("invalid syntax %s", colorbuf);

    // copy formatted strings into buffers
    char rgb[C_COL_BUFSIZE], hex[C_COL_BUFSIZE], cmyk[C_COL_BUFSIZE], hsl[C_COL_BUFSIZE], hsv[C_COL_BUFSIZE], named[STR_BUFSIZE];
    if(webfmt) {
        snprintf(rgb,   sizeof(rgb),   "rgb(%d,%d,%d)",                     color.rgb.r, color.rgb.g, color.rgb.b); 
        snprintf(hex,   sizeof(hex),   "#%06x",                             color.hex);
        snprintf(cmyk,  sizeof(cmyk),  "cmyk(%.*g%%,%.*g%%,%.*g%%,%.*g%%)", DPLACES(color.cmyk.c * 100.0, dplaces), color.cmyk.c * 100.0, DPLACES(color.cmyk.m * 100.0, dplaces), color.cmyk.m * 100.0, DPLACES(color.cmyk.y * 100.0, dplaces), color.cmyk.y * 100.0, DPLACES(color.cmyk.k * 100.0, dplaces), color.cmyk.k * 100.0);
        snprintf(hsl,   sizeof(hsl),   "hsl(%.*g,%.*g%%,%.*g%%)",           DPLACES(color.hsl.h, dplaces), color.hsl.h, DPLACES(color.hsl.sat * 100.0, dplaces), color.hsl.sat * 100.0, DPLACES(color.hsl.l * 100.0, dplaces), color.hsl.l * 100.0);
        snprintf(hsv,   sizeof(hsv),   "hsv(%.*g,%.*g%%,%.*g%%)",           DPLACES(color.hsv.h, dplaces), color.hsv.h, DPLACES(color.hsv.sat * 100.0, dplaces), color.hsv.sat * 100.0, DPLACES(color.hsv.v * 100.0, dplaces), color.hsv.v * 100.0);
        snprintf(named, sizeof(named), "%s (#%06x) (dist² %.*g)",           color.named.name, color.named.hex, DPLACES(color.named.diff, dplaces), color.named.diff);
    } else {
        snprintf(rgb,   sizeof(rgb),   "%d,%d,%d",                          color.rgb.r, color.rgb.g, color.rgb.b); 
        snprintf(hex,   sizeof(hex),   "%06x",                              color.hex);
        snprintf(cmyk,  sizeof(cmyk),  "%.*g%%,%.*g%%,%.*g%%,%.*g%%",       DPLACES(color.cmyk.c * 100.0, dplaces), color.cmyk.c * 100.0, DPLACES(color.cmyk.m * 100.0, dplaces), color.cmyk.m * 100.0, DPLACES(color.cmyk.y * 100.0, dplaces), color.cmyk.y * 100.0, DPLACES(color.cmyk.k * 100.0, dplaces), color.cmyk.k * 100.0);
        snprintf(hsl,   sizeof(hsl),   "%.*g,%.*g%%,%.*g%%",                DPLACES(color.hsl.h, dplaces), color.hsl.h, DPLACES(color.hsl.sat * 100.0, dplaces), color.hsl.sat * 100.0, DPLACES(color.hsl.l * 100.0, dplaces), color.hsl.l * 100.0);
        snprintf(hsv,   sizeof(hsv),   "%.*g,%.*g%%,%.*g%%",                DPLACES(color.hsv.h, dplaces), color.hsv.h, DPLACES(color.hsv.sat * 100.0, dplaces), color.hsv.sat * 100.0, DPLACES(color.hsv.v * 100.0, dplaces), color.hsv.v * 100.0);
        snprintf(named, sizeof(named), "%s (%06x) (dist² %.*g)",            color.named.name, color.named.hex, DPLACES(color.named.diff, dplaces), color.named.diff);
    }

    // if the user wants to do conversion, only display the converted model (or error for some unsupported model)
    if (conversion) {
        for(int i = 0; conversion[i]; i++) conversion[i] = tolower(conversion[i]);
        if(strcmp(conversion, "rgb")  == 0) { printf("%s\n", rgb);  return 0; }
        if(strcmp(conversion, "hex")  == 0) { printf("%s\n", hex);  return 0; }
        if(strcmp(conversion, "cmyk") == 0) { printf("%s\n", cmyk); return 0; }
        if(strcmp(conversion, "hsl")  == 0) { printf("%s\n", hsl);  return 0; }
        if(strcmp(conversion, "hsv")  == 0) { printf("%s\n", hsv);  return 0; }
        ERROR_EXIT("unknown conversion type %s", conversion);
    }

    // load background and foreground ansi escape code modifiers
    char bgbuf[C_STR_BUFSIZE] = { 0 }, fgbuf[C_STR_BUFSIZE] = { 0 };

    // map bg and fg colors to 16 or 256 bit palettes if chosen
    if (mapping == TC_NONE)      cwidth = 0, cheight = 0, txtclr = false, reset = "";
    if (mapping == TC_16)        snprintf(bgbuf, sizeof(bgbuf), "\x1b[%dm", ansi16_idx_to_sgr_bg(rgb_to_ansi16_idx(&color.rgb))), snprintf(fgbuf, sizeof(fgbuf), "\x1b[%dm", ansi16_idx_to_sgr_fg(rgb_to_ansi16_idx(&color.rgb)));
    if (mapping == TC_256)       snprintf(bgbuf, sizeof(bgbuf), "\x1b[48;5;%dm", rgb_to_ansi256_idx(&color.rgb)),                 snprintf(fgbuf, sizeof(fgbuf), "\x1b[38;5;%dm", rgb_to_ansi256_idx(&color.rgb));
    if (mapping == TC_TRUECOLOR) snprintf(bgbuf, sizeof(bgbuf), "\033[48;2;%d;%d;%dm", color.rgb.r, color.rgb.g, color.rgb.b),    snprintf(fgbuf, sizeof(fgbuf), "\033[38;2;%d;%d;%dm", color.rgb.r, color.rgb.g, color.rgb.b);
    
    // do the thing
    if (json) {
        printf("{\n"
               "  \"rgb\": { \"r\": %d, \"g\": %d, \"b\": %d },\n"
               "  \"hex\": \"#%06x\",\n"
               "  \"cmyk\": { \"c\": %f, \"m\": %f, \"y\": %f, \"k\": %f },\n"
               "  \"hsl\": { \"h\": %f, \"s\": %f, \"l\": %f },\n"
               "  \"hsv\": { \"h\": %f, \"s\": %f, \"v\": %f },\n"
               "  \"named\": { \"name\": \"%s\", \"hex\": \"#%06x\", \"sqrdist\": %f }\n"
               "}\n", 
               color.rgb.r, color.rgb.g, color.rgb.b,
               color.hex,
               color.cmyk.c, color.cmyk.m, color.cmyk.y, color.cmyk.k,
               color.hsl.h, color.hsl.sat, color.hsl.l,
               color.hsv.h, color.hsv.sat, color.hsv.v,
               color.named.name, color.named.hex, color.named.diff);
    } else {
        PRINT_COLOR("RGB  :" , "%s", rgb);
        PRINT_COLOR("Hex  :" , "%s", hex);
        PRINT_COLOR("CMYK :" , "%s", cmyk);
        PRINT_COLOR("HSL  :" , "%s", hsl);
        PRINT_COLOR("HSV  :" , "%s", hsv);
        PRINT_COLOR_EMPTY();
        PRINT_COLOR("Named:" , "%s", named);
        if (mapping == TC_16)  { rgb_t mapped = ansi16_idx_to_rgb(rgb_to_ansi16_idx(&color.rgb));   bool match = cmp_rgb(&color.rgb, &mapped); if (match) PRINT_COLOR("Maps to 16 colors?",  "%s", "YES"); else PRINT_COLOR("Maps to 16 colors?",  "NO (closest: %s%06x)", webfmt ? "#" : "", rgb_to_hex(&mapped)); }
        if (mapping == TC_256) { rgb_t mapped = ansi256_idx_to_rgb(rgb_to_ansi256_idx(&color.rgb)); bool match = cmp_rgb(&color.rgb, &mapped); if (match) PRINT_COLOR("Maps to 256 colors?", "%s", "YES"); else PRINT_COLOR("Maps to 256 colors?", "NO (closest: %s%06x)", webfmt ? "#" : "", rgb_to_hex(&mapped)); }
        
        // print remaining characters of left sample, if there are any left
        for (; cheight > 0; --cheight) printf("%s%*s%s\n", bgbuf, cwidth, " ", reset);
    }

    return 0;
}