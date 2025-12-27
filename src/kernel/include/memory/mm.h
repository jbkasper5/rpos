#ifndef __MM_H__
#define __MM_H__

#define KSTACK      (1 << 22)

// set the user stack up at the top of the 2nd GiB of memory, growing downwards
// subtract 16 so we don't have to allocate an entire page for 1 byte that won't be used
#define USTACK  ((1ULL << 31) - 16)

#ifndef __ASSEMBLER__
void memzero(unsigned long src, unsigned int n);
#endif

#endif