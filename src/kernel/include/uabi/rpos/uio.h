#ifndef __UABI_UIO_H__
#define __UABI_UIO_H__

#include <stdint.h>

#define UIO_MAXIOV  1024

struct iovec {
    void*    iov_base;
    uint64_t iov_len;
};

#endif