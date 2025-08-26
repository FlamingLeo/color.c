// function for working with the terminal interface
#ifndef CLI_H
#define CLI_H

#include "types.h"

// try to detect supported terminal color mode automatically using environment variables
color_cap_t detect_terminal_color();

// parse command-line options and possibly set color out parameters
// exit on failure
void parse_cli_args(int argc, char **argv, const char *progname_param, prog_opts_t *opts, color_t *color, color_t *colorD, bool *color_set);

#endif