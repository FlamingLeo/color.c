// string to color model parser
#ifndef PARSER_H
#define PARSER_H

#include "utility.h"
#include "types.h"

// find closest named color using weighted squared rgb distance for currently chosen name set
named_t closest_named_weighted_rgb(const rgb_t *in);

// master parser: tries parsers in order
// takes as parameters the input string to be parsed and a color_t out parameter
//
// this is the only publicly shown endpoint
// see src/parser.c for internal parser implementations
//
// returns 1 if the string could be parsed and writes the result to *out
// returns 0 if the string could not be parsed and does nothing with *out
int parse_color(const char *in, color_t *out);

// master parser for parsing (at most) two colors from an input string, where the colors in the input string
// are separated by whitespace and the substrings for the individual colors themselves may also contain whitespace
//
// the function internally uses the single color master parser while continuously building longer and longer pairs
// (separated by whitespace) to try and find the largest possible pairs of numbers
// this is done instead of immediately choosing the first result for cases like rgb vs. cmyk, where the latter would result in
// shorter, incorrect pairs being formed
// 
// e.g. "255, 255, 255 100, 0, 100, 0" results in out0 being rgb(255,255,255) and out1 being cmyk(100%,0%,100%,0%)
//      using the second approach, out0 would be rgb(255,255,255) and out2 would be rgb(100,0,100) (if we accept trailing garbage)
//
// special case 1: two shorthand hex numbers would be concatenated to form a single hex value  (e.g. "abc def" would result in hex color "abcdef")
// special case 2: two named colors would be concatenated to form a single named color (e.g. "lime green" would result in named color "limegreen")
// 
// both of these cases are handled as such:
//   when building a candidate color string from multiple tokens, the parser checks if all individual tokens also parse as valid colors
//   if yes, we reject the merged interpretation, otherwise we accept it
//   either way, we keep scanning afterwards to find the longest match to still correctly handle cases like the rbg-cmyk ambiguity
//
// this design choice was made for two reasons:
// 1. allow options to require a single color as a parameter (e.g. distance, contrast) such that they may be
//    placed anywhere, including inbetween other options or as the final option (e.g. -f 2 -d <color1> -x ... <color2>)
//    instead of having both parameters one after the other (e.g. -d <color1> <color2> -x ... <what would we do with the obligatory color then?>)
// 2. allow usage to be as intuitive as possible (typical CLI patterns like <val1> <val2> for doing something with two values)
//    while also allowing users to still use whitespaces in singular colors (could be preference, could be copy-pasted...)
//
//    however, word of advice: you really shouldn't get used to using whitespaces in arguments / parameters, 
//    most programs don't support it and when they do, it's just bad practice
//
//    yes, using a simple separator like ; or | would have been far easier to implement but functionality would have been impaired
//
// returns the number of colors successfully parsed (0, 1, 2)
// it is then up to the caller to validate this (for instance, the caller may only expect 1 out of 2)
int parse_color2(const char *in, color_t *out0, color_t *out1);

// print a formatted list of all named colors (css or xkcd, based on choice)
// if l is 0, the result is a prettified table consisting of "colorsample name #rrggbb" separated by whitespace
// if l is 1, the result is a csv-like output "name,#rrggbb"
//
// if the mode is not truecolor, csv output is printed
void list_colors(int l, color_cap_t mode);

// use xkcd color names instead of css (updates internal pointer and size)
void use_xkcd();

#endif