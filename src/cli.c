#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cli.h"
#include "utility.h"
#include "parser.h"
#include "printer.h"

// helper function to immediately validate conversion
static void validate_conversion(const char *conv, const char *progname) {
    if (!strcasecmp_own(conv, "rgb")   && !strcasecmp_own(conv, "hex")
     && !strcasecmp_own(conv, "cmyk")  && !strcasecmp_own(conv, "hsl")
     && !strcasecmp_own(conv, "hsv")   && !strcasecmp_own(conv, "named")
     && !strcasecmp_own(conv, "oklab") && !strcasecmp_own(conv, "oklch")) ERROR_EXIT("unknown conversion type %s", conv);
}

color_cap_t detect_terminal_color() {
    // not a tty (e.g. file output): no color
    if (!isatty(STDOUT_FILENO)) return TC_NONE;

    // respect NO_COLOR envvar (https://no-color.org/)
    if (getenv("NO_COLOR")) return TC_NONE;

    // check COLORTERM envvar
    const char *colorterm = getenv("COLORTERM");
    if (colorterm && (strstr(colorterm, "truecolor") || strstr(colorterm, "24bit") || strcasestr_own(colorterm, "24bit"))) return TC_TRUECOLOR;

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

void parse_cli_args(int argc, char **argv, const char *progname_param, prog_opts_t *opts, color_t *color, color_t *colorD, bool *color_set) {
    const char *progname = progname_param;

    // initialize defaults
    color_cap_t tmode = detect_terminal_color();
    opts->cwset       = false; opts->cwidth      = 18;    opts->mapping     = tmode;
    opts->dplaces     = 2;     opts->webfmt      = false; opts->txtclr      = true;
    opts->json        = false; opts->conversion  = NULL;  opts->distance    = false;

    int arg = 1;
    while ((argc > arg) && (argv[arg][0] == '-')) {
        // options that alter main program execution
        if      (argv[arg][1] == 'h') { print_help(progname); exit(0); }
        else if (argv[arg][1] == 'l') {
            int lmode = 0;
            if (argc > arg + 1)         lmode = safe_atoi(argv[++arg], progname);
            if (lmode < 0 || lmode > 1) ERROR_EXIT("invalid list mode %d", lmode);
            else                        list_colors(lmode, opts);
            exit(0);
        }

        // flags
        else if (argv[arg][1] == 'j') opts->json = true;
        else if (argv[arg][1] == 'p') opts->txtclr = false;
        else if (argv[arg][1] == 'W') opts->webfmt = true;
        else if (argv[arg][1] == 'x') use_xkcd();

        // options that expect an argument
        else if (argv[arg][1] == 'd' && argc > arg + 1) {
            char buf[STR_BUFSIZE] = "";
            for (++arg; arg < argc && argv[arg][0] != '-'; ++arg) if (!strncat_safe(buf, sizeof(buf), argv[arg], strlen(buf) > 0)) ERROR_EXIT("input too large");

            int end = arg;
            if (end < argc) { if (!parse_color(buf, colorD))             ERROR_EXIT("could not parse -d color %s", buf);                            arg = end - 1; } 
            else {            if (parse_color2(buf, colorD, color) != 2) ERROR_EXIT("could not parse -d color1 color2 %s", buf); *color_set = true; arg = end - 1; }
            opts->distance = true;
        }
        else if (argv[arg][1] == 'c' && argc > arg + 1) { opts->conversion = argv[++arg]; validate_conversion(opts->conversion, progname); }
        else if (argv[arg][1] == 'f' && argc > arg + 1) { opts->dplaces = safe_atoi(argv[++arg], progname); opts->dplaces = CLAMP(opts->dplaces, 0, 5); }
        else if (argv[arg][1] == 'w' && argc > arg + 1) { opts->cwidth  = safe_atoi(argv[++arg], progname); opts->cwidth  = CLAMP(opts->cwidth, 0, 25); opts->cwset = true; }
        else if (argv[arg][1] == 'm' && argc > arg + 1) {
            if (strcasecmp_own(argv[++arg], "truecolor")) opts->mapping = TC_TRUECOLOR;
            else                                          opts->mapping = safe_atoi(argv[arg], progname);

            if (opts->mapping != TC_NONE && opts->mapping != TC_16 && opts->mapping != TC_256 && opts->mapping != TC_TRUECOLOR) ERROR_EXIT("invalid mapping (must be one of: 0, 16, 256, 16777216 / truecolor): %d", opts->mapping);
            if (opts->mapping > tmode) printf("warning: mapping %d (%s) might be unsupported by this terminal (color mode: %s)\n", opts->mapping, tcolor_tostr(opts->mapping), tcolor_tostr(tmode));
        }

        // default behavior
        else ERROR_EXIT("invalid option: %s", argv[arg]);
        arg++;
    }

    // increase width by one to cover extra line added for mapping info
    if (!opts->cwset && opts->mapping != TC_TRUECOLOR) ++opts->cwidth;

    // now concatenate remaining argv into colorbuf
    char colorbuf[STR_BUFSIZE] = "";
    if (arg < argc) {
        for (; arg < argc; ++arg) if (!strncat_safe(colorbuf, sizeof(colorbuf), argv[arg], strlen(colorbuf) > 0)) ERROR_EXIT("input too large");
        
        if (strlen(colorbuf) > 0) {
            if (!parse_color(colorbuf, color)) ERROR_EXIT("invalid syntax %s", colorbuf);
            *color_set = true;
        }
    }

    
}
