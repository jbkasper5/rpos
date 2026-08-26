/*
 * Thin libc bridge. Deliberately uses only builtin types so it can be included
 * by host_env.c (which is compiled with kernel headers and therefore MUST NOT
 * pull in any system header -- see the pid_t note in test.h).
 *
 * The implementation (host_sys.c) is the ONLY harness TU that includes libc
 * headers, and it includes NO kernel headers.
 */
#ifndef HOST_SYS_H
#define HOST_SYS_H

/* page-aligned, demand-zero allocation; terminates the process on failure */
void *host_sys_alloc(unsigned long bytes);

#endif /* HOST_SYS_H */
