#ifndef __MM_H__
#define __MM_H__

#define LOW_MEMORY      (1 << 22)

#ifndef __ASSEMBLER__
void memzero(unsigned long src, unsigned int n);
#endif

#endif