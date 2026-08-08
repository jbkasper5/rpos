#include "test.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <signal.h>

/* Is a debugger attached? Under lldb/gdb the per-test SIGALRM watchdog fires
 * while you're paused at a breakpoint (wall-clock keeps ticking), so we turn it
 * off automatically. A genuinely hung test can be interrupted by hand. */
#if defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
static int under_debugger(void) {
    struct kinfo_proc info;
    info.kp_proc.p_flag = 0;
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };
    size_t size = sizeof(info);
    if (sysctl(mib, 4, &info, &size, NULL, 0) != 0) return 0;
    return (info.kp_proc.p_flag & P_TRACED) != 0;
}
#else
static int under_debugger(void) { return 0; }
#endif

static int g_watchdog = 1;   /* per-test SIGALRM watchdog enabled? */

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

/* crash isolation: a buggy TU under test can SIGSEGV/SIGBUS; catch it, mark the
 * test failed, and continue with the rest of the suite instead of aborting */
static sigjmp_buf              g_crash_jmp;
static volatile sig_atomic_t   g_crash_sig;

static void crash_handler(int sig) {
    g_crash_sig = sig;
    siglongjmp(g_crash_jmp, 1);
}

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

void test_set_timeout(unsigned seconds) {
    if (g_watchdog) alarm(seconds);
}

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

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = crash_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);        /* per-test watchdog (see alarm() below) */

    const char *tenv = getenv("TEST_TIMEOUT_SEC");
    unsigned timeout = tenv ? (unsigned)atoi(tenv) : 20;
    if (timeout == 0 || under_debugger()) {
        g_watchdog = 0;
        fprintf(stderr, "(per-test watchdog disabled: %s)\n",
                under_debugger() ? "debugger attached" : "TEST_TIMEOUT_SEC=0");
    }

    for (int i = 0; i < g_ntests; i++) {
        if (filter &&
            !strstr(g_tests[i].suite, filter) &&
            !strstr(g_tests[i].name, filter)) {
            skipped++;
            continue;
        }

        g_test_failed = 0;
        printf("%s[ RUN  ]%s %s.%s\n", DIM, RESET, g_tests[i].suite, g_tests[i].name);

        if (sigsetjmp(g_crash_jmp, 1) != 0) {
            /* a signal fired inside the test (crash or watchdog timeout) */
            alarm(0);
            int ce = color_on(fileno(stderr));
            int timed_out = (g_crash_sig == SIGALRM);
            fprintf(stderr, "%s    %s %s.%s: %s%s\n",
                    ce ? ANSI_RED : "", timed_out ? "TIMEOUT" : "CRASH",
                    g_tests[i].suite, g_tests[i].name,
                    timed_out ? "exceeded time limit (likely hang/cycle)" : strsignal((int)g_crash_sig),
                    ce ? ANSI_RESET : "");
            g_test_failed = 1;
        } else {
            if (g_watchdog) alarm(timeout);
            if (setjmp(g_test_jmp) == 0) g_tests[i].fn();
            if (g_watchdog) alarm(0);
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
