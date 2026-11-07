.section .text

// u32 atomic_increment(void* address, u32 increment)
.globl atomic_increment
atomic_increment:
1:  ldaxr   w2, [x0]        // w2 = old value, tag the address exclusive
    add     w4, w2, w1      // w4 = old + increment
    stlxr   w3, w4, [x0]    // try to store the new value; w3 = 0 on success
    cbnz    w3, 1b          // lost the monitor -> retry
    mov     w0, w2          // return old value (same as ldaddal)
    ret
