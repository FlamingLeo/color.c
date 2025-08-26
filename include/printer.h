// functions for printing something to terminal output
#ifndef PRINTER_H
#define PRINTER_H

#include <stdio.h>
#include "types.h"

// print usage string
void print_usage(FILE* stream, const char *progname);

// print help string
void print_help(const char *progname);

// prints a labeled line and decrements height to detect color preview printing borders
// specifically, if the height is negative, we're done printing the color preview
void print_color_line(print_ctx_t *ctx, const char *label, const char *fmt, ...);

// prints an empty color preview line (or nothing if the preview is finished)
void print_color_line_empty(print_ctx_t *ctx);

// print a single color block
//
// this may either be complete color information (all formats), possibly including a color preview,
// or just a single conversion when "-c <model>" is specified
//
// either option also supports json mode where the output is formatted directly as json
//
// returns true if it handled a conversion-only branch, false for a complete block
bool print_color(const color_t *colorptr, const prog_opts_t *opts,
                 const char *json_label, bool json_add_comma,
                 char *bgbufptr, char *fgbufptr,
                 int cwidth, int cheight_orig, const char *reset_default);

#endif