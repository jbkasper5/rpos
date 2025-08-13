# rpos

# Using GDB

## Setup
In order to use GDB to debug this project, we need to run QEMU silently without executing the code. To do so, use the `make gdb` rule in the makefile to build the target and configure it for debugging. In a **separate terminal**, we can use gdb via the `aarch64-none-elf-gdb` binary, setting the context to our executable, `exe`:

```
> aarch64-none-elf-gdb exe
```

From there, we need to connect the debugger to the running QEMU instance by doing 

```
(gdb) target remote localhost:1234
```

Alternatively, use the rules in the Makefile to assist with running GDB. In particular, running `make wait` will initialize the kernel in the waiting state, and `make gdb` **in a separate terminal window** will connect GDB to the waiting kernel. From there, standard GDB commands can apply. 

## Multiprocessing

In this project, the `raspi3b` board is configured to have 4 CPU cores, meaning on initialization, every core will begin executing. In order to debug with multiple cores, GDB has a few helpful commands:

+ `info thread` - lists all running threads for the excutable
+ `thread apply all [command]` - applies a GDB command to all threads. You can also specify a thread ID instead of 'all'. 
+ `thread [thread_id]` - will switch the debugger over to the thread specified by `thread_id`



## **SUPER IMPORTANT**

[Here](https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf)


## Interfacing with the PI

In order to view the output from UART, run

```
alias piscreen="screen -L /dev/tty.usbserial-0001 115200"
```

In order to exit out of the screen, type `ctrl+a+\`.

# ARM Knowledge - Calling Conventions

### Calling Conventions
| Argument      | Register                  |
| ------------- | ------------------------- |
| 1st           | `x0`                      |
| 2nd           | `x1`                      |
| 3rd           | `x2`                      |
| 4th           | `x3`                      |
| 5th           | `x4`                      |
| 6th           | `x5`                      |
| 7th           | `x6`                      |
| 8th           | `x7`                      |
| More than 8 → | Passed on stack (aligned) |

### Return Values
| Return Values | Register                  |
| ------------- | ------------------------- |
| 1st           | `x0`                      |
| 2nd           | `x1`                      |


### General Purpose Registers
| Register     | Usage                              | Preserved across calls?         |
| ------------ | ---------------------------------- | ------------------------------- |
| `x0`–`x7`    | Argument / return value            | ❌ No (caller-saved)             |
| `x8`         | Indirect result location / scratch | ❌ No                            |
| `x9`–`x15`   | Temporary (caller-saved)           | ❌ No                            |
| `x19`–`x28`  | Callee-saved (must be preserved)   | ✅ Yes                           |
| `x29`        | Frame pointer (`fp`)               | ✅ Yes                           |
| `x30`        | Link register (`lr`)               | ✅ Yes                           |
| `sp`         | Stack pointer                      | ✅ (implicitly)                  |
| `x16`, `x17` | Intra-procedure-call scratch       | ❌ (reserved for veneer linkage) |


### Useful Instructions

#### 📦 Function / Control Flow

| Instruction | Description                                                      | Notes                   |
| ----------- | ---------------------------------------------------------------- | ----------------------- |
| `bl label`  | Branch with link (calls function, saves return address in `x30`) | —                       |
| `ret`       | Return from function (jumps to `x30`)                            | —                       |
| `b label`   | Unconditional branch                                             | Like `jmp`              |
| `br xN`     | Branch to address in register                                    | Used for indirect calls |

#### 🧮 Arithmetic & Logic

| Instruction       | Description                        |
| ----------------- | ---------------------------------- |
| `add x0, x1, x2`  | `x0 = x1 + x2`                     |
| `sub x0, x1, x2`  | `x0 = x1 - x2`                     |
| `mul x0, x1, x2`  | Multiply                           |
| `udiv x0, x1, x2` | Unsigned divide                    |
| `and x0, x1, x2`  | Bitwise AND                        |
| `orr x0, x1, x2`  | Bitwise OR                         |
| `eor x0, x1, x2`  | Bitwise XOR                        |
| `lsl x0, x1, #n`  | Logical shift left                 |
| `lsr x0, x1, #n`  | Logical shift right (zero-fill)    |
| `asr x0, x1, #n`  | Arithmetic shift right (sign-fill) |

#### 🧾 Comparison & Branching

| Instruction      | Description                    |
| ---------------- | ------------------------------ |
| `cmp x0, x1`     | Compare `x0 - x1` (sets flags) |
| `cbz x0, label`  | Branch if `x0 == 0`            |
| `cbnz x0, label` | Branch if `x0 != 0`            |
| `b.eq label`     | Branch if equal                |
| `b.ne label`     | Branch if not equal            |
| `b.lt label`     | Branch if less than            |
| `b.gt label`     | Branch if greater than         |

#### 📦 Data Movement

| Instruction                    | Description                           |
| ------------------------------ | ------------------------------------- |
| `mov x0, x1`                   | Copy register                         |
| `movz x0, #imm`                | Zero upper bits, set 16-bit immediate |
| `movk x0, #imm, lsl #16/32/48` | Insert 16-bit at position             |


#### 📚 Memory Access

| Instruction      | Description                    |
| ---------------- | ------------------------------ |
| `ldr x0, [x1]`   | Load 64-bit                    |
| `str x0, [x1]`   | Store 64-bit                   |
| `ldrb w0, [x1]`  | Load byte (zero-extend)        |
| `ldrsb x0, [x1]` | Load signed byte (sign-extend) |
| `strb w0, [x1]`  | Store byte                     |

#### 📦 Stack Operations

| Instruction                 | Description                         |
| --------------------------- | ----------------------------------- |
| `stp x29, x30, [sp, #-16]!` | Push frame pointer & return address |
| `ldp x29, x30, [sp], #16`   | Pop frame pointer & return address  |
| `sub sp, sp, #N`            | Allocate N bytes                    |
| `add sp, sp, #N`            | Deallocate N bytes                  |


#### 🔄 Atomics (Exclusive Load/Store & Swap)

| Instruction         | Description                                                     | Notes                                    |
| ------------------- | --------------------------------------------------------------- | ---------------------------------------- |
| `ldxr x0, [x1]`     | **Exclusive load**: load value and tag the address              | Used for atomic operations (lock-free)   |
| `stxr w2, x3, [x1]` | **Exclusive store**: try to store `x3` to `[x1]`, status → `w2` | `w2 = 0` on success, `1` on failure      |
| `swp x0, x1, [x2]`  | **Atomic swap**: store `x1`, return old value in `x0`           | Available in A64 atomic instruction set  |
| `ldaxr`, `stlxr`    | Exclusive Load-Acquire / Store-Release (memory ordering)        | For memory barriers in SMP               |
| `cas x0, x1, [x2]`  | Compare and swap                                                | Atomically replace `[x2]` if equals `x0` |
| `ldar x0, [x1]`     | Load with acquire memory ordering                               | Like `ldr` but with memory barrier       |
| `stlr x0, [x1]`     | Store with release memory ordering                              | —                                        |

#### ⚙️ System Register Access

| Instruction   | Description                           |
| ------------- | ------------------------------------- |
| `mrs x0, reg` | Read system register `reg` into `x0`  |
| `msr reg, x0` | Write `x0` to system register `reg`   |
| `isb`         | Instruction Synchronization Barrier   |
| `dsb sy`      | Data Synchronization Barrier (global) |
| `dmb sy`      | Data Memory Barrier                   |

#### 🔍 Common System Registers

| Register        | Purpose                               |
| --------------- | ------------------------------------- |
| `CurrentEL`     | Current Exception Level               |
| `CNTPCT_EL0`    | Physical counter (read-only)          |
| `CNTP_TVAL_EL0` | Timer interval                        |
| `CNTP_CTL_EL0`  | Timer control                         |
| `DAIF`          | Interrupt mask bits                   |
| `SPSR_EL1`      | Saved processor state for EL1         |
| `ELR_EL1`       | Exception link register (return addr) |
| `MPIDR_EL1`     | Multiprocessor ID (core ID, etc.)     |



### Notes
+ `wN` refers to the lower 32 bits of the 64-bit register `xN`
+ You can also store and load pairs of registers with `stp` and `ldp`, which can be efficient for mass pushing/popping