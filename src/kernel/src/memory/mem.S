

.section .text


.globl _neon_64_memcpy

# x0 -> destination
# x1 -> source pointer
# x2 -> number of neon copies to perform

# NOTE: this is an unrolled version, doing 64 bytes of copying per iteration
_neon_64_memcpy:
    # copy 16 bytes and post-increment x0/x1
    ld1     {v0.16b}, [x1], #16
    st1     {v0.16b}, [x0], #16

    # copy 16 bytes and post-increment x0/x1
    ld1     {v1.16b}, [x1], #16
    st1     {v1.16b}, [x0], #16

    # copy 16 bytes and post-increment x0/x1
    ld1     {v2.16b}, [x1], #16
    st1     {v2.16b}, [x0], #16

    # copy 16 bytes and post-increment x0/x1
    ld1     {v3.16b}, [x1], #16
    st1     {v3.16b}, [x0], #16

    subs x2, x2, #1

    b.gt _neon_64_memcpy

    ret

.globl _neon_16_memcpy
_neon_16_memcpy:
    # copy 16 bytes and post-increment x0/x1
    ld1     {v0.16b}, [x1], #16
    st1     {v0.16b}, [x0], #16
    subs x2, x2, #1
    b.gt _neon_16_memcpy
    ret


# x0 -> buffer
# x1 -> byte to copy
# x2 -> n_bytes
.globl _neon_8_memset
_neon_8_memset:
    dup v0.16b, w1
_ne8loop:
    st1 {v0.16b}, [x0], #16
    subs x2, x2, #1
    b.gt _ne8loop
    ret

.globl _neon_16_memset
_neon_16_memset:
    dup v0.8h, w1
_ne16loop:
    st1 {v0.8h}, [x0], #16
    subs x2, x2, #1
    b.gt _ne16loop
    ret

.globl _neon_32_memset
_neon_32_memset:
    dup v0.4s, w1
_ne32loop:
    st1 {v0.4s}, [x0], #16
    subs x2, x2, #1
    b.gt _ne32loop
    ret

.globl _neon_64_memset
_neon_64_memset:
    dup v0.2d, x1
_ne64loop:
    st1 {v0.2d}, [x0], #16
    subs x2, x2, #1
    b.gt _ne64loop
    ret