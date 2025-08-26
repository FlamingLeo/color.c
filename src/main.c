#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cli.h"
#include "converter.h"
#include "parser.h"
#include "printer.h"
#include "utility.h"

const char* pangrams[] = {
    "Sphinx of black quartz, judge my vow",
    "How quickly daft jumping zebras vex!",
    "Pack my box with five dozen liquor jugs",
    "Watch Jeopardy!, Alex Trebek's fun TV quiz game",
    "The quick brown fox jumps over the lazy dog"
};
const size_t pangrams_size = ARRAY_LENGTH(pangrams);


// usage: <progname> [options] color
int main(int argc, char **argv) {
    const char* progname = argv[0];
    if (argc < 2) ERROR_EXIT("at least one argument must be passed (-h or color)");

    // parse command line options and exit on failure
    prog_opts_t opts;
    color_t color, colorD, colorC;
    bool color_set = false;

    parse_cli_args(argc, argv, progname, &opts, &color, &colorD, &colorC, &color_set);

    // require a main color unless it was already provided
    if (!color_set) ERROR_EXIT("invalid syntax, color must be specified");

    const char *reset_default = "\x1b[0m";

    // buffers for possible colors (main, distance, contrast)
    char bgbuf[C_STR_BUFSIZE]  = { 0 }, fgbuf[C_STR_BUFSIZE]  = { 0 },
         bgbufD[C_STR_BUFSIZE] = { 0 }, fgbufD[C_STR_BUFSIZE] = { 0 },
         bgbufC[C_STR_BUFSIZE] = { 0 }, fgbufC[C_STR_BUFSIZE] = { 0 };

    // determine initial color preview width and height
    int cwidth = opts.cwidth;
    int cheightorig = (cwidth >> 1) + (cwidth % 2);

    // main loop
    //
    // prints up to 3 color blocks (main, distance, contrast)
    // and sets up buffers for each block
    const  color_t  *sequence[3];
    char  *bgptrs[3], *fgptrs[3];
    size_t seqn = 0;

                         sequence[seqn] = &color;  bgptrs[seqn] = bgbuf;  fgptrs[seqn++] = fgbuf;
    if (opts.distance) { sequence[seqn] = &colorD; bgptrs[seqn] = bgbufD; fgptrs[seqn++] = fgbufD; }
    if (opts.contrast) { sequence[seqn] = &colorC; bgptrs[seqn] = bgbufC; fgptrs[seqn++] = fgbufC; }

    int xkeys = opts.distance + opts.contrast;
    int tkeys = seqn + xkeys;

    if (opts.json) printf("{\n");

    for (size_t si = 0; si < seqn; ++si) {
        const color_t *cptr = sequence[si];
        const char    *label;

        if      (cptr == &color)  label = "main";
        else if (cptr == &colorD) label = "distanceColor";
        else                      label = "contrastColor";

        print_color(cptr, &opts, label, ((tkeys - (si + 1)) > 0), bgptrs[si], fgptrs[si], cwidth, cheightorig, reset_default);
        if (si != seqn - 1) printf("\n");
    }

    
    // compute and show color difference
    if (opts.distance) {
        double d_rgb2   = (opts.cdiff == CDIFF_RGB   || opts.cdiff == CDIFF_ALL) ? dist2_rgb(&color.rgb, &colorD.rgb)                         : 0.0;
        double d_wrgb2  = (opts.cdiff == CDIFF_WRGB  || opts.cdiff == CDIFF_ALL) ? weighted_dist2_rgb(&color.rgb, &colorD.rgb, W_R, W_G, W_B) : 0.0;
        double d_oklab2 = (opts.cdiff == CDIFF_OKLAB || opts.cdiff == CDIFF_ALL) ? dist2_oklab(&color.oklab, &colorD.oklab)                   : 0.0;

        if (opts.json) {
            printf("  \"distance\": { ");
            if      (opts.cdiff == CDIFF_RGB)   { printf("\"rgb2\": %.*f ", opts.dplaces, d_rgb2); }
            else if (opts.cdiff == CDIFF_WRGB)  { printf("\"wrgb2\": %.*f ", opts.dplaces, d_wrgb2); }
            else if (opts.cdiff == CDIFF_OKLAB) { printf("\"oklab2\": %.*f ", opts.dplaces, d_oklab2); }
            else                                { printf("\"rgb2\": %.*f, \"wrgb2\": %.*f, \"oklab2\": %.*f ", opts.dplaces, d_rgb2, opts.dplaces, d_wrgb2, opts.dplaces, d_oklab2); }
            printf("}%s\n", (opts.contrast ? "," : ""));
        } else {
            const char *reset_tail = (opts.mapping == TC_NONE) ? "" : reset_default;
            printf("\nDistance between %s %s %s%06x%s and %s %s %s%06x%s:\n",
                   bgbuf,  reset_tail, fgbuf,  color.hex,  reset_tail,
                   bgbufD, reset_tail, fgbufD, colorD.hex, reset_tail);

            if (opts.cdiff == CDIFF_RGB   || opts.cdiff == CDIFF_ALL) { printf("RGB (Squared) : %.*f\n", opts.dplaces, d_rgb2); }
            if (opts.cdiff == CDIFF_WRGB  || opts.cdiff == CDIFF_ALL) { printf("RGB (Weighted): %.*f\n", opts.dplaces, d_wrgb2); }
            if (opts.cdiff == CDIFF_OKLAB || opts.cdiff == CDIFF_ALL) { printf("Oklab         : %.*f\n", opts.dplaces, d_oklab2); }
        }
    }

    // compute and show contrast between two colors, alternating as foreground and background colors
    if (opts.contrast) {
        double LA = relative_luminance_rgb(&color.rgb);
        double LB = relative_luminance_rgb(&colorC.rgb);
        double lighter = (LA > LB) ? LA : LB;
        double darker  = (LA > LB) ? LB : LA;
        double ratio   = (lighter + 0.05) / (darker + 0.05);

        bool pass_AA       = (ratio >= 4.5);
        bool pass_AA_large = (ratio >= 3.0);
        bool pass_AAA      = (ratio >= 7.0);

        srand(time(NULL)); // random pangram
        int rn = rand() % pangrams_size;

        if (opts.json) {
            printf("  \"contrast\": { \"ratio\": %.*f, \"AA\": %s, \"AA_large\": %s, \"AAA\": %s }\n", opts.dplaces, ratio, pass_AA ? "true" : "false", pass_AA_large ? "true" : "false", pass_AAA ? "true" : "false");
        } else {
            const char *reset_tail = (opts.mapping == TC_NONE) ? "" : reset_default;
            printf("\nContrast between %s %s %s%06x%s and %s %s %s%06x%s:\n",
                   bgbuf,  reset_tail, fgbuf,  color.hex,  reset_tail,
                   bgbufC, reset_tail, fgbufC, colorC.hex, reset_tail);

            if (opts.mapping != TC_NONE) {
                printf("%s%s  %s  %s\n", bgbuf, fgbufC, pangrams[rn], reset_default);
                printf("%s%s  %s  %s\n", bgbufC, fgbuf, pangrams[rn], reset_default);
            }
            printf("Contrast ratio: %.*f\n", opts.dplaces, ratio);
            printf("WCAG AA (normal): %s\n", pass_AA ? "PASS" : "FAIL");
            printf("WCAG AA (large) : %s\n", pass_AA_large ? "PASS" : "FAIL");
            printf("WCAG AAA        : %s\n", pass_AAA ? "PASS" : "FAIL");
        }
    }

    if (opts.json) printf("}\n");

    return 0;
}
