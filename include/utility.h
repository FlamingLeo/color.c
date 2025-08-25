// utility functions for color models
#ifndef UTILITY_H
#define UTILITY_H

#include <stdbool.h>
#include "types.h"

// terminal color options
typedef enum {
    TC_NONE      = 0,
    TC_16        = 16,
    TC_256       = 256,
    TC_TRUECOLOR = 16777216
} color_cap_t;

// squared euclidian distance between two rgb colors
double dist2_rgb(const rgb_t *a, const rgb_t *b);

// squared weighted euclidian distance between two rgb colors
double weighted_dist2_rgb(const rgb_t *a, const rgb_t *b, double wr, double wg, double wb);

// string representation of terminal color mode
const char *tcolor_tostr(color_cap_t c);

// try to detect supported terminal color mode automatically using environment variables
color_cap_t detect_terminal_color();

// compare two rgb values and return true if they are equal in r,g and b or false if at least one differs
bool cmp_rgb(const rgb_t *a, const rgb_t *b);

#endif