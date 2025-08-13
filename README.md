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