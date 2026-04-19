.section .text

// u64 atomic_increment(void* address, u64 increment)
.globl atomic_increment
atomic_increment:
    ldaddal w1, w0, [x0]
    ret
