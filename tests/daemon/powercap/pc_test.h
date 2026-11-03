/*
 * pc_test.h -- minimal test harness shared by the EARD powercap tests.
 *
 * Three outcomes per check:
 *
 *   CHECK(expr, ...)      must hold; a failure fails the binary.
 *   XFAIL(bug, expr, ...) documents a known defect. The expression states the
 *                         CORRECT behaviour, so today it is expected to be
 *                         false. A false result prints XFAIL and does not fail
 *                         the run; a true result prints XPASS, which means the
 *                         bug was fixed and the XFAIL should become a CHECK.
 *                         XPASS fails the run so that nobody forgets.
 *
 * Every XFAIL names a bug id (e.g. "PC-07") listed in BUGS.md next to this
 * file, so the output can be traced back to the analysis.
 */
#ifndef PC_TEST_H
#define PC_TEST_H

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int pc_failures = 0;
static int pc_passes   = 0;
static int pc_xfails   = 0;
static int pc_xpasses  = 0;

#define CHECK(expr, ...)                         \
    do {                                         \
        if (expr) {                              \
            pc_passes++;                         \
            printf("  PASS  ");                  \
        } else {                                 \
            pc_failures++;                       \
            printf("  FAIL  ");                  \
        }                                        \
        printf(__VA_ARGS__);                     \
        printf("\n");                            \
    } while (0)

#define CHECK_EQ(a, b, ...)                                                    \
    do {                                                                       \
        long long _a = (long long) (a), _b = (long long) (b);                  \
        if (_a == _b) {                                                        \
            pc_passes++;                                                       \
            printf("  PASS  ");                                                \
            printf(__VA_ARGS__);                                               \
            printf("\n");                                                      \
        } else {                                                               \
            pc_failures++;                                                     \
            printf("  FAIL  ");                                                \
            printf(__VA_ARGS__);                                               \
            printf("  (got %lld, expected %lld)\n", _a, _b);                   \
        }                                                                      \
    } while (0)

#define XFAIL(bug, expr, ...)                                                  \
    do {                                                                       \
        if (expr) {                                                            \
            pc_xpasses++;                                                      \
            printf("  XPASS [%s] ", bug);                                      \
        } else {                                                               \
            pc_xfails++;                                                       \
            printf("  XFAIL [%s] ", bug);                                      \
        }                                                                      \
        printf(__VA_ARGS__);                                                   \
        printf("\n");                                                          \
    } while (0)

#define TEST(name) static void name(void)
#define RUN(name)                                   \
    do {                                            \
        printf("\n=== %s ===\n", #name);            \
        name();                                     \
    } while (0)

/*
 * Runs a test in a forked child so that every test starts from a pristine
 * copy of the static state of the code under test (powercap.c,
 * powercap_mgt.c and the plugins keep a lot of file-scope statics and have no
 * reset entry point). The child ships its counters back through a pipe. A
 * child that dies on a signal counts as one failure.
 */
static inline void pc_run_isolated(const char *name, void (*fn)(void))
{
    int p[2];
    const char *only = getenv("PC_ONLY");
    if (only != NULL && strstr(name, only) == NULL)
        return;
    printf("\n=== %s ===\n", name);
    fflush(stdout);
    if (pipe(p) != 0) {
        perror("pipe");
        exit(2);
    }
    pid_t pid = fork();
    if (pid == 0) {
        close(p[0]);
        pc_passes = pc_failures = pc_xfails = pc_xpasses = 0;
        setvbuf(stdout, NULL, _IONBF, 0); /* keep the output of a child that dies */
        alarm(60);
        fn();
        fflush(stdout);
        int c[4] = {pc_passes, pc_failures, pc_xfails, pc_xpasses};
        if (write(p[1], c, sizeof(c)) != sizeof(c))
            _exit(3);
        _exit(0);
    }
    close(p[1]);
    int c[4] = {0}, st = 0;
    ssize_t n  = read(p[0], c, sizeof(c));
    close(p[0]);
    waitpid(pid, &st, 0);
    if (n != sizeof(c) || WIFSIGNALED(st)) {
        printf("  FAIL  %s died (%s)\n", name, WIFSIGNALED(st) ? strsignal(WTERMSIG(st)) : "no result");
        pc_failures++;
        return;
    }
    pc_passes += c[0];
    pc_failures += c[1];
    pc_xfails += c[2];
    pc_xpasses += c[3];
}
#define RUN_ISOLATED(name) pc_run_isolated(#name, name)

static inline int pc_summary(const char *suite)
{
    printf("\n%s: %d passed, %d failed, %d xfail (known bugs), %d xpass\n", suite, pc_passes, pc_failures, pc_xfails,
           pc_xpasses);
    return (pc_failures || pc_xpasses) ? 1 : 0;
}

/*
 * Runs fn() in a child process. Returns the terminating signal, or 0 when the
 * child exited normally. Used to prove crashes (abort/segfault) without
 * killing the test binary.
 */
static inline int pc_crashes(void (*fn)(void))
{
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        /* Silence the child: its output would interleave with ours. */
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        alarm(10);
        fn();
        _exit(0);
    }
    int st = 0;
    waitpid(pid, &st, 0);
    if (WIFSIGNALED(st))
        return WTERMSIG(st);
    return 0;
}

#endif
