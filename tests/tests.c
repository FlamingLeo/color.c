// test_color.c
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "parser.h"

// terminal output: column widths
#define TEST_W_STATUS 6
#define TEST_W_ID     28
#define TEST_W_INPUT  26
#define TEST_W_EXPECT 34
#define TEST_W_ACTUAL 34

// tolerance for floating point comparisons (CMYK/HSL/HSV)
#define EPS 1e-6

// struct containing test case information
typedef struct test_case_t {
    const char *id;       // test name
    const char *input;    // input to be tested
    bool expect_ok;       // should parse succeed?
    color_t expected;     // full expected color_t (only used if expect_ok==true)
} test_case_t;

// array of test cases (generated using external python script)
//
// successes must contain full color structs
// fails may be empty, they're not checked anyway
//
// note: without -Wno-missing-braces, this code generates a LOT of warnings, but it should work fine
static test_case_t tests[] = {
    { "null-invalid", NULL, false, (color_t){ 0 } },
    { "normalize-valid-01", " RGB ( 255 , 0 , 0 ) ", true, (color_t){ .hex = 0xff0000, .rgb = { .r = 255, .g = 0, .b = 0 }, .cmyk = { .c = 0.000000, .m = 1.000000, .y = 1.000000, .k = 0.000000 }, .hsl = { .h = 0.000000, .sat = 1.000000, .l = 0.500000 }, .hsv = { .h = 0.000000, .sat = 1.000000, .v = 1.000000 } } },
    { "normalize-valid-02", "255 , 0 , 0", true, (color_t){ .hex = 0xff0000, .rgb = { .r = 255, .g = 0, .b = 0 }, .cmyk = { .c = 0.000000, .m = 1.000000, .y = 1.000000, .k = 0.000000 }, .hsl = { .h = 0.000000, .sat = 1.000000, .l = 0.500000 }, .hsv = { .h = 0.000000, .sat = 1.000000, .v = 1.000000 } } },
    { "normalize-valid-03", "rGb( 0.5 , 0.5 , 0.5 )", true, (color_t){ .hex = 0x808080, .rgb = { .r = 128, .g = 128, .b = 128 }, .cmyk = { .c = 0.000000, .m = 0.000000, .y = 0.000000, .k = 0.498039 }, .hsl = { .h = 0.000000, .sat = 0.000000, .l = 0.501961 }, .hsv = { .h = 0.000000, .sat = 0.000000, .v = 0.501961 } } },
    { "hex-valid-01", "#abcdef", true, (color_t){ .hex = 0xabcdef, .rgb = { .r = 171, .g = 205, .b = 239 }, .cmyk = { .c = 0.284519, .m = 0.142259, .y = 0.000000, .k = 0.062745 }, .hsl = { .h = 210.000000, .sat = 0.680000, .l = 0.803922 }, .hsv = { .h = 210.000000, .sat = 0.284519, .v = 0.937255 } } },
    { "hex-valid-02", "0x123456", true, (color_t){ .hex = 0x123456, .rgb = { .r = 18, .g = 52, .b = 86 }, .cmyk = { .c = 0.790698, .m = 0.395349, .y = 0.000000, .k = 0.662745 }, .hsl = { .h = 210.000000, .sat = 0.653846, .l = 0.203922 }, .hsv = { .h = 210.000000, .sat = 0.790698, .v = 0.337255 } } },
    { "hex-valid-03", "xc0ffee", true, (color_t){ .hex = 0xc0ffee, .rgb = { .r = 192, .g = 255, .b = 238 }, .cmyk = { .c = 0.247059, .m = 0.000000, .y = 0.066667, .k = 0.000000 }, .hsl = { .h = 163.809524, .sat = 1.000000, .l = 0.876471 }, .hsv = { .h = 163.809524, .sat = 0.247059, .v = 1.000000 } } },
    { "hex-valid-04", "badbed", true, (color_t){ .hex = 0xbadbed, .rgb = { .r = 186, .g = 219, .b = 237 }, .cmyk = { .c = 0.215190, .m = 0.075949, .y = 0.000000, .k = 0.070588 }, .hsl = { .h = 201.176471, .sat = 0.586207, .l = 0.829412 }, .hsv = { .h = 201.176471, .sat = 0.215190, .v = 0.929412 } } },
    { "hex-valid-05", "hex(#decade)", true, (color_t){ .hex = 0xdecade, .rgb = { .r = 222, .g = 202, .b = 222 }, .cmyk = { .c = 0.000000, .m = 0.090090, .y = 0.000000, .k = 0.129412 }, .hsl = { .h = 300.000000, .sat = 0.232558, .l = 0.831373 }, .hsv = { .h = 300.000000, .sat = 0.090090, .v = 0.870588 } } },
    { "hex-invalid-triplet", "#abc", true, (color_t){ .hex = 0xaabbcc, .rgb = { .r = 170, .g = 187, .b = 204 }, .cmyk = { .c = 0.166667, .m = 0.083333, .y = 0.000000, .k = 0.200000 }, .hsl = { .h = 210.000000, .sat = 0.250000, .l = 0.733333 }, .hsv = { .h = 210.000000, .sat = 0.166667, .v = 0.800000 } } },
    { "hex-invalid-characters", "gg0000", false, (color_t){ 0 } },
    { "hex-invalid-notclosed", "hex(0xf1dd1e", false, (color_t){ 0 } },
    { "hex-invalid-negative-1", "-0xdeface", false, (color_t){ 0 } },
    { "hex-invalid-negative-2", "0x-600613", false, (color_t){ 0 } },
    { "hex-invalid-negative-3", "-ca7713", false, (color_t){ 0 } },
    { "malformed-hextra-1", "#fef1f0fum", false, (color_t){ 0 } },
    { "malformed-hextra-2", "0xdeadbeef", false, (color_t){ 0 } },
    { "malformed-hextra-3", "hex(f0cacc)ia", false, (color_t){ 0 } },
    { "rgb-valid-01", "10,20,30", true, (color_t){ .hex = 0x0a141e, .rgb = { .r = 10, .g = 20, .b = 30 }, .cmyk = { .c = 0.666667, .m = 0.333333, .y = 0.000000, .k = 0.882353 }, .hsl = { .h = 210.000000, .sat = 0.500000, .l = 0.078431 }, .hsv = { .h = 210.000000, .sat = 0.666667, .v = 0.117647 } } },
    { "rgb-valid-02", "rgb(0,128,255)", true, (color_t){ .hex = 0x0080ff, .rgb = { .r = 0, .g = 128, .b = 255 }, .cmyk = { .c = 1.000000, .m = 0.498039, .y = 0.000000, .k = 0.000000 }, .hsl = { .h = 209.882353, .sat = 1.000000, .l = 0.500000 }, .hsv = { .h = 209.882353, .sat = 1.000000, .v = 1.000000 } } },
    { "rgb-invalid-above-255", "255,255,256", false, (color_t){ 0 } },
    { "rgb-invalid-below-0", "-1,0,0", false, (color_t){ 0 } },
    { "rgb-invalid-formatting", "(255,0,0)", false, (color_t){ 0 } },
    { "rgb-invalid-percentages", "rgb(100%,0%,0%)", false, (color_t){ 0 } },
    { "rgb-invalid-notclosed", "rgb(0,123,123", false, (color_t){ 0 } },
    { "rgb-invalid-extra", "rgb(255,0,0)more", false, (color_t){ 0 } },
    { "rgb-invalid-extra-2", "255,128,16more", false, (color_t){ 0 } },
    { "rgb-float-valid-01", "0.0,0.0,0.0", true, (color_t){ .hex = 0x000000, .rgb = { .r = 0, .g = 0, .b = 0 }, .cmyk = { .c = 0.000000, .m = 0.000000, .y = 0.000000, .k = 1.000000 }, .hsl = { .h = 0.000000, .sat = 0.000000, .l = 0.000000 }, .hsv = { .h = 0.000000, .sat = 0.000000, .v = 0.000000 } } },
    { "rgb-float-valid-02", "1.0,1.0,1.0", true, (color_t){ .hex = 0xffffff, .rgb = { .r = 255, .g = 255, .b = 255 }, .cmyk = { .c = 0.000000, .m = 0.000000, .y = 0.000000, .k = 0.000000 }, .hsl = { .h = 0.000000, .sat = 0.000000, .l = 1.000000 }, .hsv = { .h = 0.000000, .sat = 0.000000, .v = 1.000000 } } },
    { "rgb-float-valid-03", "0.5,0.5,0.5", true, (color_t){ .hex = 0x808080, .rgb = { .r = 128, .g = 128, .b = 128 }, .cmyk = { .c = 0.000000, .m = 0.000000, .y = 0.000000, .k = 0.498039 }, .hsl = { .h = 0.000000, .sat = 0.000000, .l = 0.501961 }, .hsv = { .h = 0.000000, .sat = 0.000000, .v = 0.501961 } } },
    { "rgb-float-valid-04", "0.5,1,0", true, (color_t){ .hex = 0x80ff00, .rgb = { .r = 128, .g = 255, .b = 0 }, .cmyk = { .c = 0.498039, .m = 0.000000, .y = 1.000000, .k = 0.000000 }, .hsl = { .h = 89.882353, .sat = 1.000000, .l = 0.500000 }, .hsv = { .h = 89.882353, .sat = 1.000000, .v = 1.000000 } } },
    { "rgb-float-invalid-mixed", "121,0.6,121", false, (color_t){ 0 } },
    { "rgb-float-invalid-range", "10.3,54.2,33.3", false, (color_t){ 0 } },
    { "rgb-float-invalid-negative", "-0.5,0,0", false, (color_t){ 0 } },
    { "cmyk-valid-01", "cmyk(0%,100%,100%,0%)", true, (color_t){ .hex = 0xff0000, .rgb = { .r = 255, .g = 0, .b = 0 }, .cmyk = { .c = 0.000000, .m = 1.000000, .y = 1.000000, .k = 0.000000 }, .hsl = { .h = 0.000000, .sat = 1.000000, .l = 0.500000 }, .hsv = { .h = 0.000000, .sat = 1.000000, .v = 1.000000 } } },
    { "cmyk-valid-02", "cmyk(0,1,1,0)", true, (color_t){ .hex = 0xff0000, .rgb = { .r = 255, .g = 0, .b = 0 }, .cmyk = { .c = 0.000000, .m = 1.000000, .y = 1.000000, .k = 0.000000 }, .hsl = { .h = 0.000000, .sat = 1.000000, .l = 0.500000 }, .hsv = { .h = 0.000000, .sat = 1.000000, .v = 1.000000 } } },
    { "cmyk-valid-03", "0,100,100,0", true, (color_t){ .hex = 0xff0000, .rgb = { .r = 255, .g = 0, .b = 0 }, .cmyk = { .c = 0.000000, .m = 1.000000, .y = 1.000000, .k = 0.000000 }, .hsl = { .h = 0.000000, .sat = 1.000000, .l = 0.500000 }, .hsv = { .h = 0.000000, .sat = 1.000000, .v = 1.000000 } } },
    { "cmyk-valid-04", "cmyk(50%,50%,50%,50%)", true, (color_t){ .hex = 0x404040, .rgb = { .r = 64, .g = 64, .b = 64 }, .cmyk = { .c = 0.500000, .m = 0.500000, .y = 0.500000, .k = 0.500000 }, .hsl = { .h = 0.000000, .sat = 0.000000, .l = 0.250980 }, .hsv = { .h = 0.000000, .sat = 0.000000, .v = 0.250980 } } },
    { "cmyk-invalid-zero", "cmyk(0,0,0)", false, (color_t){ 0 } },
    { "cmyk-invalid-negative", "cmyk(-1,0,0)", false, (color_t){ 0 } },
    { "cmyk-invalid-range", "cmyk(101,0,0)", false, (color_t){ 0 } },
    { "cmyk-invalid-mixed", "cmyk(50,60%,70)", false, (color_t){ 0 } },
    { "cmyk-invalid-notclosed", "cmyk(0,1,1,0", false, (color_t){ 0 } },
    { "cmyk-invalid-extra", "cmyk(0,1,1,0)more", false, (color_t){ 0 } },
    { "hsl-valid-01", "hsl(0,100%,50%)", true, (color_t){ .hex = 0xff0000, .rgb = { .r = 255, .g = 0, .b = 0 }, .cmyk = { .c = 0.000000, .m = 1.000000, .y = 1.000000, .k = 0.000000 }, .hsl = { .h = 0.000000, .sat = 1.000000, .l = 0.500000 }, .hsv = { .h = 0.000000, .sat = 1.000000, .v = 1.000000 } } },
    { "hsl-valid-02", "hsl(120,100%,50%)", true, (color_t){ .hex = 0x00ff00, .rgb = { .r = 0, .g = 255, .b = 0 }, .cmyk = { .c = 1.000000, .m = 0.000000, .y = 1.000000, .k = 0.000000 }, .hsl = { .h = 120.000000, .sat = 1.000000, .l = 0.500000 }, .hsv = { .h = 120.000000, .sat = 1.000000, .v = 1.000000 } } },
    { "hsl-valid-03", "hsl(240,100%,50%)", true, (color_t){ .hex = 0x0000ff, .rgb = { .r = 0, .g = 0, .b = 255 }, .cmyk = { .c = 1.000000, .m = 1.000000, .y = 0.000000, .k = 0.000000 }, .hsl = { .h = 240.000000, .sat = 1.000000, .l = 0.500000 }, .hsv = { .h = 240.000000, .sat = 1.000000, .v = 1.000000 } } },
    { "hsl-valid-04", "hsl(360,100%,50%)", true, (color_t){ .hex = 0xff0000, .rgb = { .r = 255, .g = 0, .b = 0 }, .cmyk = { .c = 0.000000, .m = 1.000000, .y = 1.000000, .k = 0.000000 }, .hsl = { .h = 360.000000, .sat = 1.000000, .l = 0.500000 }, .hsv = { .h = 0.000000, .sat = 1.000000, .v = 1.000000 } } },
    { "hsl-valid-05", "hsl(120,1,50)", true, (color_t){ .hex = 0x00ff00, .rgb = { .r = 0, .g = 255, .b = 0 }, .cmyk = { .c = 1.000000, .m = 0.000000, .y = 1.000000, .k = 0.000000 }, .hsl = { .h = 120.000000, .sat = 1.000000, .l = 0.500000 }, .hsv = { .h = 120.000000, .sat = 1.000000, .v = 1.000000 } } },
    { "hsl-valid-06", "hsl(240,100,0.5)", true, (color_t){ .hex = 0x0000ff, .rgb = { .r = 0, .g = 0, .b = 255 }, .cmyk = { .c = 1.000000, .m = 1.000000, .y = 0.000000, .k = 0.000000 }, .hsl = { .h = 240.000000, .sat = 1.000000, .l = 0.500000 }, .hsv = { .h = 240.000000, .sat = 1.000000, .v = 1.000000 } } },
    { "hsl-valid-07", "hsl(0,0.5,0.5)", true, (color_t){ .hex = 0xbf4040, .rgb = { .r = 191, .g = 64, .b = 64 }, .cmyk = { .c = 0.000000, .m = 0.664921, .y = 0.664921, .k = 0.250980 }, .hsl = { .h = 0.000000, .sat = 0.500000, .l = 0.500000 }, .hsv = { .h = 0.000000, .sat = 0.664921, .v = 0.749020 } } },
    { "hsl-valid-08", "hsl(0,0,0)", true, (color_t){ .hex = 0x000000, .rgb = { .r = 0, .g = 0, .b = 0 }, .cmyk = { .c = 0.000000, .m = 0.000000, .y = 0.000000, .k = 1.000000 }, .hsl = { .h = 0.000000, .sat = 0.000000, .l = 0.000000 }, .hsv = { .h = 0.000000, .sat = 0.000000, .v = 0.000000 } } },
    { "hsl-valid-09", "hsl(-360,100%,100%)", true, (color_t){ .hex = 0xffffff, .rgb = { .r = 255, .g = 255, .b = 255 }, .cmyk = { .c = 0.000000, .m = 0.000000, .y = 0.000000, .k = 0.000000 }, .hsl = { .h = -360.000000, .sat = 1.000000, .l = 1.000000 }, .hsv = { .h = 0.000000, .sat = 0.000000, .v = 1.000000 } } },
    { "hsl-invalid-negative-1", "hsl(-360,0,-1)", false, (color_t){ 0 } },
    { "hsl-invalid-negative-2", "hsl(360,100%,-50%)", false, (color_t){ 0 } },
    { "hsl-invalid-mixed", "hsl(120,100%,50)", false, (color_t){ 0 } },
    { "hsl-invalid-notclosed", "hsl(240,100%,50%", false, (color_t){ 0 } },
    { "hsl-invalid-extra-1", "hsl(0,100%,50%)more", false, (color_t){ 0 } },
    { "hsl-invalid-extra-2", "hsl(0,0.5,0.5)more", false, (color_t){ 0 } },
    { "hsv-valid-01", "hsv(0,100%,100%)", true, (color_t){ .hex = 0xff0000, .rgb = { .r = 255, .g = 0, .b = 0 }, .cmyk = { .c = 0.000000, .m = 1.000000, .y = 1.000000, .k = 0.000000 }, .hsl = { .h = 0.000000, .sat = 1.000000, .l = 0.500000 }, .hsv = { .h = 0.000000, .sat = 1.000000, .v = 1.000000 } } },
    { "hsv-valid-02", "hsv(120,100%,100%)", true, (color_t){ .hex = 0x00ff00, .rgb = { .r = 0, .g = 255, .b = 0 }, .cmyk = { .c = 1.000000, .m = 0.000000, .y = 1.000000, .k = 0.000000 }, .hsl = { .h = 120.000000, .sat = 1.000000, .l = 0.500000 }, .hsv = { .h = 120.000000, .sat = 1.000000, .v = 1.000000 } } },
    { "hsv-valid-03", "hsv(240,100%,100%)", true, (color_t){ .hex = 0x0000ff, .rgb = { .r = 0, .g = 0, .b = 255 }, .cmyk = { .c = 1.000000, .m = 1.000000, .y = 0.000000, .k = 0.000000 }, .hsl = { .h = 240.000000, .sat = 1.000000, .l = 0.500000 }, .hsv = { .h = 240.000000, .sat = 1.000000, .v = 1.000000 } } },
    { "hsv-valid-04", "hsv(120,100%,50%)", true, (color_t){ .hex = 0x008000, .rgb = { .r = 0, .g = 128, .b = 0 }, .cmyk = { .c = 1.000000, .m = 0.000000, .y = 1.000000, .k = 0.498039 }, .hsl = { .h = 120.000000, .sat = 1.000000, .l = 0.250980 }, .hsv = { .h = 120.000000, .sat = 1.000000, .v = 0.500000 } } },
    { "hsv-valid-05", "hsv(360,100%,100%)", true, (color_t){ .hex = 0xff0000, .rgb = { .r = 255, .g = 0, .b = 0 }, .cmyk = { .c = 0.000000, .m = 1.000000, .y = 1.000000, .k = 0.000000 }, .hsl = { .h = 0.000000, .sat = 1.000000, .l = 0.500000 }, .hsv = { .h = 360.000000, .sat = 1.000000, .v = 1.000000 } } },
    { "hsv-valid-06", "0,100%,50%", true, (color_t){ .hex = 0x800000, .rgb = { .r = 128, .g = 0, .b = 0 }, .cmyk = { .c = 0.000000, .m = 1.000000, .y = 1.000000, .k = 0.498039 }, .hsl = { .h = 0.000000, .sat = 1.000000, .l = 0.250980 }, .hsv = { .h = 0.000000, .sat = 1.000000, .v = 0.500000 } } },
    { "hsv-valid-07", "-360,100%,50%", true, (color_t){ .hex = 0x800000, .rgb = { .r = 128, .g = 0, .b = 0 }, .cmyk = { .c = 0.000000, .m = 1.000000, .y = 1.000000, .k = 0.498039 }, .hsl = { .h = 0.000000, .sat = 1.000000, .l = 0.250980 }, .hsv = { .h = -360.000000, .sat = 1.000000, .v = 0.500000 } } },
    { "hsv-valid-08", "hsv(-360,100%,100%)", true, (color_t){ .hex = 0xff0000, .rgb = { .r = 255, .g = 0, .b = 0 }, .cmyk = { .c = 0.000000, .m = 1.000000, .y = 1.000000, .k = 0.000000 }, .hsl = { .h = 0.000000, .sat = 1.000000, .l = 0.500000 }, .hsv = { .h = -360.000000, .sat = 1.000000, .v = 1.000000 } } },
    { "hsv-invalid-nopercentage", "120,1,0.5", false, (color_t){ 0 } },
    { "hsv-invalid-mixed", "hsv(120,100%,50)", false, (color_t){ 0 } },
    { "hsv-invalid-negative", "hsv(-120,100%,-100%)", false, (color_t){ 0 } },
    { "hsv-invalid-notclosed", "hsv(-120,100%,-100%", false, (color_t){ 0 } },
    { "hsv-invalid-extra-1", "hsv(0,100%,50%)more", false, (color_t){ 0 } },
    { "hsv-invalid-extra-2", "hsv(0,1,0.5)more", false, (color_t){ 0 } },
    { "hsv-invalid-extra-3", "0,100%,50%more", false, (color_t){ 0 } },
    { "malformed-garbage-1", "garbage", false, (color_t){ 0 } },
    { "malformed-garbage-2", "1,2", false, (color_t){ 0 } },
    { NULL, NULL, 0, (color_t){ 0 } }
};

// safely verify double equality
static inline bool almost_equal(double a, double b) { return fabs(a - b) <= EPS; }

// formatted output helpers
static inline void print_col(const char *s, int width) {
    if (!s) s = NULLSTR;
    printf("%-*s", width, s);
}

static inline void print_header() {
    printf(C_BOLD "%-*s %-*s %-*s %-*s %-*s\n" C_RESET,
           TEST_W_STATUS, "result",
           TEST_W_ID,     "id",
           TEST_W_INPUT,  "input",
           TEST_W_EXPECT, "expected",
           TEST_W_ACTUAL, "actual");
}

static inline void format_color_brief(char *buf, size_t bufsz, const color_t *c) {
    // only output hex, rgb to avoid cluttering output
    snprintf(buf, bufsz, "hex(0x%06x) rgb(%d,%d,%d)", (hex_t)c->hex, c->rgb.r, c->rgb.g, c->rgb.b);
}

static void print_result_row(const test_case_t *t, bool passed, int rc, const color_t *out, const color_t *expected) {
    if (passed) printf(C_GREEN "%-*s " C_RESET, TEST_W_STATUS, "PASS");
    else        printf(C_RED   "%-*s " C_RESET, TEST_W_STATUS, "FAIL");

    printf("%s%-*s ", passed ? C_LGREEN : C_LRED, TEST_W_ID, t->id ? t->id : NULLSTR);

    printf(passed ? C_GREEN : C_RED);
    print_col(t->input ? t->input : NULLSTR, TEST_W_INPUT);
    printf(C_RESET " ");

    char expectbuf[STR_BUFSIZE];
    if (t->expect_ok) format_color_brief(expectbuf, sizeof(expectbuf), expected);
    else              snprintf(expectbuf, sizeof(expectbuf), "parse failed");

    printf(passed ? C_LGREEN : C_LRED);
    print_col(expectbuf, TEST_W_EXPECT);
    printf(C_RESET " ");

    char actualbuf[STR_BUFSIZE];
    if (rc == 1) format_color_brief(actualbuf, sizeof(actualbuf), out);
    else         snprintf(actualbuf, sizeof(actualbuf), "parse failed");

    printf(passed ? C_GREEN : C_RED);
    print_col(actualbuf, TEST_W_ACTUAL);
    printf(C_RESET " \n");
}

// run a test case
static bool run_test_case(const test_case_t *t) {
    color_t out = { 0 };
    int rc = parse_color(t->input, &out);

    bool pass = true;
    if (t->expect_ok) {
        if (rc != 1) pass = false;
        else {
            // compare integer fields exactly (rgb, hex)
            if (out.rgb.r != t->expected.rgb.r ||
                out.rgb.g != t->expected.rgb.g ||
                out.rgb.b != t->expected.rgb.b) pass = false;
            if (out.hex != t->expected.hex)     pass = false;

            // CMYK
            if (!almost_equal(out.cmyk.c, t->expected.cmyk.c) ||
                !almost_equal(out.cmyk.m, t->expected.cmyk.m) ||
                !almost_equal(out.cmyk.y, t->expected.cmyk.y) ||
                !almost_equal(out.cmyk.k, t->expected.cmyk.k)) pass = false;

            // HSL
            if (!almost_equal(out.hsl.h, t->expected.hsl.h) ||
                !almost_equal(out.hsl.sat, t->expected.hsl.sat) ||
                !almost_equal(out.hsl.l, t->expected.hsl.l)) pass = false;

            // HSV
            if (!almost_equal(out.hsv.h, t->expected.hsv.h) ||
                !almost_equal(out.hsv.sat, t->expected.hsv.sat) ||
                !almost_equal(out.hsv.v, t->expected.hsv.v)) pass = false;
        }
    } else { if (rc != 0) pass = false; }

    print_result_row(t, pass, rc, &out, &t->expected);
    return pass;
}

// run and log all tests
// return 0 if all tests passed or 1 otherwise
int main(void) {
    print_header();
    int total = 0, passed = 0;
    for (const test_case_t *t = tests; t->id != NULL; ++t, ++total) passed += run_test_case(t); // yes this is standard compliant
    printf("\n%d / %d tests passed\n", passed, total);
    return !(passed == total);
}
