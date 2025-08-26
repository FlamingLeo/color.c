#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "converter.h"
#include "parser.h"
#include "printer.h"
#include "utility.h"

// usage: <progname> [options] color
int main(int argc, char **argv) {
    const char* progname = argv[0];
    if (argc < 2) ERROR_EXIT("at least one argument must be passed (-h or color)");

    // parse command line options and exit on failure
    prog_opts_t opts;
    color_t color, colorD;
    bool color_set = false;

    parse_cli_args(argc, argv, progname, &opts, &color, &colorD, &color_set);

    // require a main color unless it was already provided
    if (!color_set) ERROR_EXIT("invalid syntax, color must be specified");

    const char *reset_default = "\x1b[0m";

    // buffers for two possible colors (main and distance)
    char bgbuf[C_STR_BUFSIZE]  = { 0 }, fgbuf[C_STR_BUFSIZE]  = { 0 },
         bgbufD[C_STR_BUFSIZE] = { 0 }, fgbufD[C_STR_BUFSIZE] = { 0 };

    // initial pointers
    const color_t *colorptr = &color;
    char *bgbufptr = bgbuf;
    char *fgbufptr = fgbuf;

    // determine initial color preview width and height
    int cwidth = opts.cwidth;
    int cheightorig = (cwidth >> 1) + (cwidth % 2);
    int cwidthorig = cwidth;

    if (opts.json) printf("{\n");

    // main loop
    while (colorptr) {
        print_color(colorptr, &opts, (colorptr == &color), bgbufptr, fgbufptr, cwidth, cheightorig, reset_default);

        // after output, advance to distance color if requested
        if (colorptr == &color && opts.distance) {
            colorptr = &colorD;
            bgbufptr = bgbufD;
            fgbufptr = fgbufD;
            if (!opts.json && !opts.conversion) printf("\n");
        } else {
            colorptr = NULL;
        }

        // restore widths for next iteration
        cwidth = cwidthorig;
    }

    if (opts.distance) {
        double d2  = dist2_rgb(&color.rgb, &colorD.rgb);
        double wd2 = weighted_dist2_rgb(&color.rgb, &colorD.rgb, W_R, W_G, W_B);
        if (opts.json) printf("  \"dist\": { \"rgb2\": %.*f, \"wrgb2\": %.*f }\n", opts.dplaces, d2, opts.dplaces, wd2);
        else {
            const char *reset_tail = (opts.mapping == TC_NONE) ? "" : reset_default;
            printf("\nDistance between %s %s %s%06x%s and %s %s %s%06x%s:\n", bgbuf,  reset_tail, fgbuf,  color.hex,  reset_tail,
                                                                              bgbufD, reset_tail, fgbufD, colorD.hex, reset_tail);
            printf("Squared : %.*f\nWeighted: %.*f\n", opts.dplaces, d2, opts.dplaces, wd2);
        }
    }

    if (opts.json) printf("}\n");

    return 0;
}
