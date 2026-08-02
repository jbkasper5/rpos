/*
 * Minimal xUnit-style test framework for the rpos host harness.
 *
 * IMPORTANT: this header pulls in NO system headers. The kernel's scalar.h
 * defines `typedef u16 pid_t`, which collides with the host libc's pid_t, so
 * any TU that mixes kernel headers with <stdio.h>/<stdlib.h> fails to compile.
 * Test files include this header AND kernel headers, so all libc use is pushed
 * behind prototypes (test_failf) whose definitions live in test.c.
 *
 *     TEST(suite, name) {
 *         ASSERT_EQ(some_call(), expected);
 *     }
 */
#ifndef HOST_TEST_H
#define HOST_TEST_H

typedef void (*test_fn_t)(void);

void register_test(const char *suite, const char *name, test_fn_t fn);

/* records the failure, sets the failed flag, and longjmps back to the runner */
void test_failf(const char *file, int line, const char *fmt, ...)
    __attribute__((noreturn, format(printf, 3, 4)));

#define TEST(suite, name)                                                     \
    static void suite##_##name(void);                                         \
    __attribute__((constructor))                                              \
    static void reg_##suite##_##name(void) {                                  \
        register_test(#suite, #name, suite##_##name);                         \
    }                                                                         \
    static void suite##_##name(void)

#define ASSERT_TRUE(c)                                                        \
    do { if (!(c)) test_failf(__FILE__, __LINE__, "expected true: %s", #c); } while (0)

#define ASSERT_FALSE(c)                                                       \
    do { if ((c))  test_failf(__FILE__, __LINE__, "expected false: %s", #c); } while (0)

#define ASSERT_EQ(a, b)                                                       \
    do {                                                                      \
        unsigned long long _a = (unsigned long long)(a);                      \
        unsigned long long _b = (unsigned long long)(b);                      \
        if (_a != _b)                                                         \
            test_failf(__FILE__, __LINE__, "%s == %s  (0x%llx != 0x%llx)",    \
                       #a, #b, _a, _b);                                       \
    } while (0)

#define ASSERT_NE(a, b)                                                       \
    do {                                                                      \
        unsigned long long _a = (unsigned long long)(a);                      \
        unsigned long long _b = (unsigned long long)(b);                      \
        if (_a == _b)                                                         \
            test_failf(__FILE__, __LINE__, "%s != %s  (both 0x%llx)",         \
                       #a, #b, _a);                                           \
    } while (0)

#define ASSERT_GE(a, b)                                                       \
    do {                                                                      \
        unsigned long long _a = (unsigned long long)(a);                      \
        unsigned long long _b = (unsigned long long)(b);                      \
        if (_a < _b)                                                          \
            test_failf(__FILE__, __LINE__, "%s >= %s  (0x%llx < 0x%llx)",     \
                       #a, #b, _a, _b);                                       \
    } while (0)

#define ASSERT_LT(a, b)                                                       \
    do {                                                                      \
        unsigned long long _a = (unsigned long long)(a);                      \
        unsigned long long _b = (unsigned long long)(b);                      \
        if (_a >= _b)                                                         \
            test_failf(__FILE__, __LINE__, "%s < %s  (0x%llx >= 0x%llx)",     \
                       #a, #b, _a, _b);                                       \
    } while (0)

#endif /* HOST_TEST_H */
