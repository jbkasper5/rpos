
/*
    x0 - address to perform the atomic swap on
    x1 - the value to attempt to store in the address
*/

.globl atomic_swap
atomic_swap:
1:  ldaxr   w2, [x0]        // load-acquire-exclusive: read *ptr, tag the address
    stlxr   w3, w1, [x0]    // store-release-exclusive: try to write w1
    cbnz    w3, 1b          // w3 nonzero = monitor lost (race) -> retry
    mov     w0, w2          // return old value
    ret