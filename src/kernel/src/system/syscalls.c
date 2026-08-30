#include "system/syscalls.h"
#include "system/scheduler.h"
#include "io/kprintf.h"
#include "io/gpio.h"
#include "utils/timer.h"
#include "macros.h"
#include "io/lcd.h"
#include "memory/mem.h"
#include "uabi/rpos/ioctls.h"
#include "uabi/rpos/fb.h"
#include "memory/mmap.h"
#include "synchronization/mutex.h"
#include "filesystem/disk.h"
#include "uabi/rpos/errno.h"
#include "filesystem/elf.h"
#include "memory/memprofiler.h"


void* cacheable_page = NULL;

u64 handle_syscall(u64 x0, u64 x1, u64 x2, u64 x3, u64 x4, u64 x5, u64 syscall_number){
    // DEBUG("Syscall Number: %d\n", syscall_number);

    if(syscall_table[syscall_number]){
        return syscall_table[syscall_number](x0, x1, x2, x3, x4, x5);
    }else{
        return -1;
    }
}

u64 sys_write(u64 fd, u64 buf, u64 count, u64, u64, u64){
    // write a buffer to a file descriptor
    // get fd
    pcb_t* current = get_current();
    if(current->fds[fd] && current->fds[fd]->file_ops->write){
        return current->fds[fd]->file_ops->write(current->fds[fd], buf, count);
    }
    return 0;
}

u64 sys_read(u64 fd, u64 buf, u64 count, u64, u64, u64){
    // deschedule the process until a key press is received over the IRQ
    // deschedule();

    // once we get back here, it means the process was woken by the IRQ handler for the keyboard
    pcb_t* current = get_current();

    // read data from the file descriptor
    if(current->fds[fd] && current->fds[fd]->file_ops->read){
        return current->fds[fd]->file_ops->read(current->fds[fd], buf, count);
    }
}

u64 sys_nanosleep(u64 ns, u64, u64, u64, u64, u64){
    // slep
    timer_nanosleep(ns);
    deschedule();
    return 0;
}

u64 sys_getpid(u64, u64, u64, u64, u64, u64){
    pcb_t* current = get_current();
    if(current && current->pid > 0) return current->pid;
    else return -1;
}

u64 sys_getppid(u64, u64, u64, u64, u64, u64){
    pcb_t* parent = get_current()->parent;
    if(parent && parent->pid > 0) return parent->pid;
    else return -1;
}

u64 sys_clock_gettime(u64 clock, u64 kernel_timespec, u64, u64, u64, u64){
    return 0;
}

u64 sys_mmap(u64 addr, u64 len, u64 prot, u64 flags, u64 fd, u64 offset){
    // TODO: look up the requested memory via the FD
    // TODO: actually use the addr instead of ignoring it like a bum

    
    
    map_pages(frame.fb, va_to_pa(frame.fb), 376, MAP_KERNEL, (u64) L0_TABLE);
    return 0;
}

u64 sys_munmap(u64 addr, u64 len, u64, u64, u64, u64){
    // unmap a chunk of memory starting from addr
    return 0;
}

u64 sys_execve(u64 path, u64 argv, u64 envp, u64, u64, u64){
    char* p = (char*) path;
    // read binary from path
    // load binary into program memory
    // pass in arguments and environment variables
    // jump back to ELF-defined entry point
    // if invalid, return -1
    file_t* f = open(path, NULL);
    if(f){
        // begin ELF parsing
        readelf(f);

        profile(&(memprofiler_cfg){ .pid = get_current()->pid });
        
        return;
    }else{
        return -ENOEXEC;
    }
}

u64 sys_pulse_led(u64 pin_num, u64 turn_on, u64, u64, u64, u64){
    pulse(pin_num, !turn_on);
    return 0;
}

u64 sys_io_setup(u64, u64, u64, u64, u64, u64){
    return 0;
}

u64 sys_getcwd(u64 buffer, u64 size, u64, u64, u64, u64){
    void* thing = (void*)rootfs.root_inode;
    return 0;
}

u64 sys_exit(u64 status, u64, u64, u64, u64, u64){
    INFO("Current running process number: %d\n", get_current()->pid);
    reap();
    return SYS_SUCCESS;
}

u64 sys_exit_group(u64 status, u64, u64, u64, u64, u64){
    INFO("Current running process number: %d\n", get_current()->pid);
    reap();
    return SYS_SUCCESS;
}

u64 sys_get_framebuffer(u64, u64, u64, u64, u64, u64){
    // return frame.fb;
    return 0;
}

u64 sys_open(u64 path, u64 flags, u64, u64, u64, u64){
    file_t* fd = (file_t*) check_vfs((char*) path);
    if(fd){
        // requested filedescriptor is a device/from the VFS
        pcb_t* current = get_current();
        file_t* new_fd = (file_t*) kmalloc(sizeof(file_t));
        memcpy(new_fd, fd, sizeof(file_t));
        int fd_index = fd_alloc(current, new_fd);
        current->fds[fd_index] = new_fd;

        // invoke the open procedure for the filedescriptor
        if(new_fd->file_ops && new_fd->file_ops->open) new_fd->file_ops->open(new_fd);
        return fd_index;
    }else{
        // requested file exists on disk
        file_t* f = open((const char*) path, flags);
        if(!f){
            ERROR("Could not open file.\n");
            return -ENOENT;   
        }else{
            pcb_t* current = get_current();
            int fd_index = fd_alloc(current, f);
            current->fds[fd_index] = f;

            if(f->file_ops && f->file_ops->open) f->file_ops->open(f);
            return fd_index;
        }
    }
    return -ENOENT;
}

u64 sys_ioctl(u64 fd, u64 cmd, u64 arg, u64, u64, u64){
    INFO("Handling IOCTL Request: fd=%d, cmd=0x%x, arg=0x%x\n", fd, cmd, arg);
    pcb_t* current = get_current();  
    if(current->fds[fd] && current->fds[fd]->file_ops->ioctl){
        return current->fds[fd]->file_ops->ioctl(current->fds[fd], cmd, arg);
    }
}

u64 sys_getc(u64, u64, u64, u64, u64, u64){
    char c = uart_getc();
    uart_putc(c);
    return c;
}

u64 sys_clone3(u64 cl_args, u64 size, u64, u64, u64, u64){
    return 0;
}

u64 sys_pipe2(u64 fd_rets, u64 flags, u64, u64, u64, u64){
    // allocate a pipe buffer of 64KiB (16 pages)
    u64* pipe = buddy_alloc(16 << PAGE_SHIFT);

    // create 2 new file descriptors from the process's file descriptor list
    // read

    u64* return_fds = (u64*)fd_rets;

    // return_fds[0] = read_end;
    // return_fds[1] = write_end;

    return 0;
}

u64 sys_fork(u64, u64, u64, u64, u64, u64){  
    // also now need to clone the kstack from the old to the new process
    pcb_t* current = get_current();

    // procalloc
    pcb_t* newproc = clone_active_proc();

    // add cloned process to the scheduler
    add_to_schedule(newproc);

    int pid = newproc->pid;

    // add the new child into the parent's children list
    list_add(&newproc->siblings, &current->children);


    DEBUG("Parent process memprofile: \n");
    profile(&(memprofiler_cfg){ .pid = current->pid });

    DEBUG("New child process memprofile: \n");
    profile(&(memprofiler_cfg){ .pid = pid });

    // return child pid
    return pid;
}

u64 sys_test_mutex(u64, u64, u64, u64, u64, u64){

    mutex_t* m = cacheable_page;
    if(!cacheable_page){
        cacheable_page = buddy_alloc(PAGE_SIZE);
        map(cacheable_page, va_to_pa(cacheable_page), 0, MAP_READ | MAP_WRITE | MAP_CACHE | MAP_KERNEL, L0_TABLE);
        m = (mutex_t*)cacheable_page;
        m->owner = MUTEX_UNLOCKED;
        INIT_LIST_HEAD(&m->wait_list.head);
    }
    
    mutex_acquire(m);

    pcb_t* current = get_current();
    int pid = current->pid;

    INFO("Process %d acquired the mutex. Spinning for a lil while...\n", pid);

    timer_nanosleep(1000000000);
    deschedule();

    mutex_release(m);
}

u64 sys_waitid(u64 pid, u64, u64, u64, u64, u64){
    pcb_t* current = get_current();

    if(list_empty(&current->children)){
        return ECHILD;
    }

    pcb_t* child = NULL;
    bool found = FALSE;
    INFO("Checking children for pid %d...\n", pid);
    for (list_head_t* p = current->children.next; p != &current->children; p = p->next) {
        child = list_entry(p, pcb_t, siblings);
        INFO("Child found: %d\n", child->pid);
        if(child->pid == pid){
            found = TRUE;
            break;
        }
    }

    if(found){
        current->waiting_on = pid;
        while(TRUE){
            if(child->state == PROCESS_TERMINATED){
                break;
            }
            deschedule();  
        }
    }
}