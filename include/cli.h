// function for working with the terminal interface
#ifndef CLI_H
#define CLI_H

#include "types.h"

// try to detect supported terminal color mode automatically using environment variables
color_cap_t detect_terminal_color();

// parse command-line options and possibly set color/colorD. returns 0 on success or 1 if ERROR_EXIT invoked
int parse_cli_args(int argc, char **argv, const char *progname_param, prog_opts_t *opts, color_t *color, color_t *colorD, bool *color_set);

#endif