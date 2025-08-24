// string to color model parser
#ifndef PARSER_H
#define PARSER_H

#include "types.h"

// master parser: tries parsers in order
// takes as parameters the input string to be parsed and a color_t out parameter
//
// this is the only publicly shown endpoint
// see src/parser.c for internal parser implementations
//
// returns 1 if the string could be parsed and writes the result to *out
// returns 0 if the string could not be parsed and does nothing with *out
int parse_color(const char *in, color_t *out);

// print a formatted list of all named css colors
// if l is 0, the result is a prettified table consisting of "colorsample name #rrggbb" separated by whitespace
// if l is 1, the result is a csv-like output "name,#rrggbb"
void list_css_colors(int l);

#endif