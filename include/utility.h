// utility functions for color models
#ifndef UTILITY_H
#define UTILITY_H

#include <stdio.h>
#include "types.h"


// squared euclidian distance between two rgb colors
double dist2_rgb(const rgb_t *a, const rgb_t *b);

// squared weighted euclidian distance between two rgb colors
double weighted_dist2_rgb(const rgb_t *a, const rgb_t *b, double wr, double wg, double wb);

// string representation of terminal color mode
const char *tcolor_tostr(color_cap_t c);

// compare two rgb values and return true if they are equal in r,g and b or false if at least one differs
bool cmp_rgb(const rgb_t *a, const rgb_t *b);

// use strtol to convert a string to an integer with error handling
// returns the parsed integer or exits the program if something went wrong
//
// progname is passed to the usage string if something goes wrong
int safe_atoi(const char *s, const char *progname);

// append piece to dst with optional separating space, checking bounds
// returns true on success, false if piece did not fit
bool strncat_safe(char *dst, size_t dst_sz, const char *piece, bool add_space);

// case-insensitive equality without modifying inputs
// returns true if the strings are the same and false if they differ in at least one case-insensitive character
//
// this differs from the usual strcmp or non-standard strcasecmp implementatons, which return an int, where 0 signifies equality
bool strcasecmp_own(const char *a, const char *b);

// case-insensitive equality without modifying inputs for up to n characters
// returns true if the strings are the same and false if they differ in at least one case-insensitive character
//
// again, basically the non-standard strncasecmp but implemented for portability
bool strncasecmp_own(const char *a, const char *b, size_t n);

// case-insensitive strstr
bool strcasestr_own(const char *hay, const char *needle);

// format textual representations for a color into provided buffers
void fmt_color_strings(const color_t *colorptr, bool webfmt, int dplaces,
                              char *rgb,   size_t rgb_s,
                              char *hex,   size_t hex_s,
                              char *cmyk,  size_t cmyk_s,
                              char *hsl,   size_t hsl_s,
                              char *hsv,   size_t hsv_s,
                              char *oklab, size_t oklab_s,
                              char *oklch, size_t oklch_s,
                              char *named, size_t named_s);

// fill bgbufptr and fgbufptr given mapping and rgb
// returns the calculated ansi index for 16 or 256 colors and -1 otherwise
//
// source: https://en.wikipedia.org/wiki/ANSI_escape_code
int map_rgb_to_sgr_strings(color_cap_t mapping, const rgb_t *rgb_in, char *bgbufptr, size_t bgbufsz, char *fgbufptr, size_t fgbufsz);

#endif