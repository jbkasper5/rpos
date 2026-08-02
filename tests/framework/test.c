#include "test.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>

#define MAX_TESTS 1024

static struct {
    const char *suite;
    const char *name;
    test_fn_t   fn;
} g_tests[MAX_TESTS];
static int g_ntests = 0;

/* shared with host_sys.c's panic(), which also longjmps here */
jmp_buf g_test_jmp;
int     g_test_failed = 0;

void register_test(const char *suite, const char *name, test_fn_t fn) {
    if (g_ntests < MAX_TESTS) {
        g_tests[g_ntests].suite = suite;
        g_tests[g_ntests].name  = name;
        g_tests[g_ntests].fn    = fn;
        g_ntests++;
    }
}

/* color only when the stream is a terminal, unless overridden by env */
static int color_on(int fd) {
    if (getenv("NO_COLOR"))    return 0;
    if (getenv("FORCE_COLOR")) return 1;
    return isatty(fd);
}
#define ANSI_RED   "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_DIM   "\033[2m"
#define ANSI_RESET "\033[0m"

void test_failf(const char *file, int line, const char *fmt, ...) {
    int c = color_on(fileno(stderr));
    fprintf(stderr, "%s    FAIL %s:%d: ", c ? ANSI_RED : "", file, line);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "%s\n", c ? ANSI_RESET : "");
    g_test_failed = 1;
    longjmp(g_test_jmp, 1);
}

/*
 * Usage: run_tests [substring-filter]
 * Runs every registered test whose "suite" or "name" contains the filter
 * (all tests if no filter). Exit code is nonzero iff any test failed.
 *   NO_COLOR=1     force plain output
 *   FORCE_COLOR=1  force colored output even when piped
 */
int main(int argc, char **argv) {
    const char *filter = (argc > 1) ? argv[1] : NULL;
    int passed = 0, failed = 0, skipped = 0;

    int color = color_on(fileno(stdout));
    const char *RED   = color ? ANSI_RED   : "";
    const char *GREEN = color ? ANSI_GREEN : "";
    const char *DIM   = color ? ANSI_DIM   : "";
    const char *RESET = color ? ANSI_RESET : "";

    for (int i = 0; i < g_ntests; i++) {
        if (filter &&
            !strstr(g_tests[i].suite, filter) &&
            !strstr(g_tests[i].name, filter)) {
            skipped++;
            continue;
        }

        g_test_failed = 0;
        printf("%s[ RUN  ]%s %s.%s\n", DIM, RESET, g_tests[i].suite, g_tests[i].name);

        if (setjmp(g_test_jmp) == 0) {
            g_tests[i].fn();
        }

        if (g_test_failed) {
            printf("%s[ FAIL ]%s %s.%s\n", RED, RESET, g_tests[i].suite, g_tests[i].name);
            failed++;
        } else {
            printf("%s[ PASS ]%s %s.%s\n", GREEN, RESET, g_tests[i].suite, g_tests[i].name);
            passed++;
        }
    }

    printf("\n==== %s%d passed%s, %s%d failed%s",
           GREEN, passed, RESET,
           (failed ? RED : DIM), failed, RESET);
    if (skipped) printf(", %d filtered out", skipped);
    printf(" ====\n");

    return failed ? 1 : 0;
}
