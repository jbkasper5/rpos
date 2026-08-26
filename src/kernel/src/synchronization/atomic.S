
/*
    x0 - address to perform the atomic swap on
    x1 - the value to attempt to store in the address
*/

.globl atomic_swap
atomic_swap:
1:  ldaxr   x2, [x0]        // load-acquire-exclusive: read *ptr, tag the address
    stlxr   w3, x1, [x0]    // store-release-exclusive: try to write w1
    cbnz    w3, 1b          // w3 nonzero = monitor lost (race) -> retry
    mov     x0, x2          // return old value
    ret