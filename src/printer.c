#include <stdarg.h>
#include "converter.h"
#include "printer.h"
#include "utility.h"

void print_usage(FILE* stream, const char *progname) { fprintf(stream, "usage: %s [-c <model>] [-d <color>] [-f <n>] [-h] [-j] [-l [0|1]] [-m <map>] [-p] [-w <n>] [-W] [-x] <color>\nsee readme or help for a list of valid formats\n", progname); }

void print_help(const char* progname) {
    printf("color - a color printing (and conversion) tool for true color terminals\n\n");
    print_usage(stdout, progname);
    printf("\noptions:\n"
           "  -c <model>: only show the conversion of the chosen color to the specified model, then exit\n"
           "  -d <color>: choose a color to compute the difference with\n"
           "  -f <0..5> : choose the maximum amount of decimal places to print (default: 2)\n"
           "              0 rounds the numbers to the nearest integer\n"
           "  -h        : show this help text and exit\n"
           "  -j        : print output in json format\n"
           "  -l [0 | 1]: show a list of currently supported named colors and exit (default: 0)\n"
           "     - 0: human-readable format with sample, name and hex color\n"
           "     - 1: csv output with headers \"name\", \"color\", no sample\n"
           "  -m <map>  : map terminal color to \"0\", \"16\", \"256\" or \"truecolor\" output (default: your terminal's color mode)\n"
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

void print_color_line(print_ctx_t *ctx, const char *label, const char *fmt, ...) {
    const char *left_bg = (ctx->cheight > 0) ? ctx->bgbufptr : ""; ctx->cheight--;

    printf("%s%*s%s%s%s%-5s ", left_bg, ctx->cwidth, ctx->cwidth > 0 ? " " : "", ctx->reset, ctx->cwidth > 0 ? "   " : "", ctx->txtclr ? ctx->fgbufptr : "", label);

    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    printf("%s\n", ctx->reset);
}

void print_color_line_empty(print_ctx_t *ctx) {
    const char *left_bg = (ctx->cheight > 0) ? ctx->bgbufptr : ""; ctx->cheight--;
    printf("%s%*s%s\n", left_bg, ctx->cwidth, ctx->cwidth > 0 ? " " : "", ctx->reset);
}

bool print_color(const color_t *colorptr, const prog_opts_t *opts,   bool is_main,      char       *bgbufptr, 
                 char          *fgbufptr, int                cwidth, int  cheight_orig, const char *reset_default) {
    int cheight_local = cheight_orig;
    const char *reset = reset_default;

    // make sure sgr strings exist for the trailing "distance between..." line
    int ansiidx = map_rgb_to_sgr_strings(opts->mapping, &colorptr->rgb,
                                         bgbufptr, C_STR_BUFSIZE,
                                         fgbufptr, C_STR_BUFSIZE);

    // prepare textual representations (declared but not necessarily filled)
    char rgb[C_COL_BUFSIZE], hex[C_COL_BUFSIZE], cmyk[C_COL_BUFSIZE], hsl[C_COL_BUFSIZE], hsv[C_COL_BUFSIZE], named[STR_BUFSIZE];

    // populate only needed buffers in conversion-only mode
    // json mode: buffers not needed
    if (opts->conversion) {
        if (!opts->json) {
            if      (strcasecmp_own(opts->conversion, "rgb"))   { fmt_color_strings(colorptr, opts->webfmt, opts->dplaces, rgb, sizeof(rgb), NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);     printf("%s\n", rgb); }
            else if (strcasecmp_own(opts->conversion, "hex"))   { fmt_color_strings(colorptr, opts->webfmt, opts->dplaces, NULL, 0, hex, sizeof(hex), NULL, 0, NULL, 0, NULL, 0, NULL, 0);     printf("%s\n", hex); }
            else if (strcasecmp_own(opts->conversion, "cmyk"))  { fmt_color_strings(colorptr, opts->webfmt, opts->dplaces, NULL, 0, NULL, 0, cmyk, sizeof(cmyk), NULL, 0, NULL, 0, NULL, 0);   printf("%s\n", cmyk); } 
            else if (strcasecmp_own(opts->conversion, "hsl"))   { fmt_color_strings(colorptr, opts->webfmt, opts->dplaces, NULL, 0, NULL, 0, NULL, 0, hsl, sizeof(hsl), NULL, 0, NULL, 0);     printf("%s\n", hsl); }
            else if (strcasecmp_own(opts->conversion, "hsv"))   { fmt_color_strings(colorptr, opts->webfmt, opts->dplaces, NULL, 0, NULL, 0, NULL, 0, NULL, 0, hsv, sizeof(hsv), NULL, 0);     printf("%s\n", hsv); }
            else if (strcasecmp_own(opts->conversion, "named")) { fmt_color_strings(colorptr, opts->webfmt, opts->dplaces, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, named, sizeof(named)); printf("%s\n", named); }
        } else {
            printf("  \"%s\" : { ", (is_main ? "main" : "distance"));
            if      (strcasecmp_own(opts->conversion, "rgb"))   printf("\"rgb\": { \"r\": %d, \"g\": %d, \"b\": %d }",                            colorptr->rgb.r, colorptr->rgb.g, colorptr->rgb.b);
            else if (strcasecmp_own(opts->conversion, "hex"))   printf("\"hex\": \"#%06x\"",                                                      colorptr->hex);
            else if (strcasecmp_own(opts->conversion, "cmyk"))  printf("\"cmyk\": { \"c\": %.*f, \"m\": %.*f, \"y\": %.*f, \"k\": %.*f }",        opts->dplaces, colorptr->cmyk.c, opts->dplaces, colorptr->cmyk.m, opts->dplaces, colorptr->cmyk.y, opts->dplaces, colorptr->cmyk.k);
            else if (strcasecmp_own(opts->conversion, "hsl"))   printf("\"hsl\": { \"h\": %.*f, \"s\": %.*f, \"l\": %.*f }",                      opts->dplaces, colorptr->hsl.h, opts->dplaces, colorptr->hsl.sat, opts->dplaces, colorptr->hsl.l);
            else if (strcasecmp_own(opts->conversion, "hsv"))   printf("\"hsv\": { \"h\": %.*f, \"s\": %.*f, \"v\": %.*f }",                      opts->dplaces, colorptr->hsv.h, opts->dplaces, colorptr->hsv.sat, opts->dplaces, colorptr->hsv.v);
            else if (strcasecmp_own(opts->conversion, "named")) printf("\"named\": { \"name\": \"%s\", \"hex\": \"#%06x\", \"wsqrdist\": %.*f }", colorptr->named.name, colorptr->named.hex, opts->dplaces, colorptr->named.diff);
            printf(" }%s\n", (opts->distance ? "," : ""));
        }
        return true;
    } // end conversion mode

    // don't display color preview in zero color mode
    if (opts->mapping == TC_NONE) { cwidth = 0; cheight_local = 0; reset = ""; }

    // fill all string buffers now that we're not in conversion mode anymore
    fmt_color_strings(colorptr, opts->webfmt, opts->dplaces,
                      rgb,   sizeof(rgb),
                      hex,   sizeof(hex),
                      cmyk,  sizeof(cmyk),
                      hsl,   sizeof(hsl),
                      hsv,   sizeof(hsv),
                      named, sizeof(named));

    if (opts->json) {
        printf("  \"%s\": {\n"
               "    \"rgb\": { \"r\": %d, \"g\": %d, \"b\": %d },\n"
               "    \"hex\": \"#%06x\",\n"
               "    \"cmyk\": { \"c\": %.*f, \"m\": %.*f, \"y\": %.*f, \"k\": %.*f },\n"
               "    \"hsl\": { \"h\": %.*f, \"s\": %.*f, \"l\": %.*f },\n"
               "    \"hsv\": { \"h\": %.*f, \"s\": %.*f, \"v\": %.*f },\n"
               "    \"named\": { \"name\": \"%s\", \"hex\": \"#%06x\", \"wsqrdist\": %.*f }\n"
               "  }%s\n",
               (is_main ? "main" : "distance"),
               colorptr->rgb.r, colorptr->rgb.g, colorptr->rgb.b,
               colorptr->hex,
               opts->dplaces, colorptr->cmyk.c, opts->dplaces, colorptr->cmyk.m, opts->dplaces, colorptr->cmyk.y, opts->dplaces, colorptr->cmyk.k,
               opts->dplaces, colorptr->hsl.h, opts->dplaces, colorptr->hsl.sat, opts->dplaces, colorptr->hsl.l,
               opts->dplaces, colorptr->hsv.h, opts->dplaces, colorptr->hsv.sat, opts->dplaces, colorptr->hsv.v,
               colorptr->named.name, colorptr->named.hex, opts->dplaces, colorptr->named.diff, (opts->distance ? "," : ""));
        return false;
    } // end normal json mode

    // print preview and color info
    print_ctx_t ctx;
    ctx.cheight  = cheight_local; ctx.cwidth   = cwidth;
    ctx.reset    = reset;         ctx.txtclr   = opts->txtclr;
    ctx.bgbufptr = bgbufptr;      ctx.fgbufptr = fgbufptr;

    print_color_line(&ctx, "RGB  :", "%s", rgb);
    print_color_line(&ctx, "Hex  :", "%s", hex);
    print_color_line(&ctx, "CMYK :", "%s", cmyk);
    print_color_line(&ctx, "HSL  :", "%s", hsl);
    print_color_line(&ctx, "HSV  :", "%s", hsv);
    print_color_line_empty(&ctx);
    print_color_line(&ctx, "Named:", "%s", named);

    // print whether or not the color corresponds exactly to a valid 16- or 256-color mapping
    if (opts->mapping == TC_16) {
        rgb_t mapped = ansi16_idx_to_rgb(ansiidx);  bool match = cmp_rgb(&colorptr->rgb, &mapped);
        if (match) print_color_line(&ctx, "Maps to 16 colors?", "%s", "YES");
        else       print_color_line(&ctx, "Maps to 16 colors?", "NO (closest: %s%06x)", opts->webfmt ? "#" : "", rgb_to_hex(&mapped));
    }
    if (opts->mapping == TC_256) {
        rgb_t mapped = ansi256_idx_to_rgb(ansiidx); bool match = cmp_rgb(&colorptr->rgb, &mapped);
        if (match) print_color_line(&ctx, "Maps to 256 colors?", "%s", "YES");
        else       print_color_line(&ctx, "Maps to 256 colors?", "NO (closest: %s%06x)", opts->webfmt ? "#" : "", rgb_to_hex(&mapped));
    }

    // print remaining left-sample rows
    for (; ctx.cheight > 0; --ctx.cheight) printf("%s%*s%s\n", bgbufptr, cwidth, " ", reset);

    return false;
}