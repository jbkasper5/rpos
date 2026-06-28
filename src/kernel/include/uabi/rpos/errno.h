#ifndef __ERRNO_H__
#define __ERRNO_H__

// An attempt was made to perform an operation limited to processes with appropriate privileges or to the owner of a file or other resources
#define EPERM           0x01

// A component of a specified pathname did not exist, or the pathname was an empty string.
#define ENOENT          0x02

// No process could be found corresponding to that specified by the given process ID.
#define ESRCH           0x03

// An asynchronous signal (such as SIGINT or SIGQUIT) was caught by the process during the execution of an interruptible function
#define EINTR           0x04

// Some physical input or output error occurred
#define EIO             0x05

// Input or output on a special file referred to a device that did not exist, or made a request beyond the limits of the device
#define ENXIO           0x06

// The number of bytes used for the argument and environment list of the new process exceeded the limit NCARGS
#define E2BIG           0x07

// A request was made to execute a file that, although it has the appropriate permissions, was not in the format required for an executable file
#define ENOEXEC         0x08

// A file descriptor argument was out of range, referred to no open file, or a read (write) request was made to a file that was only open for writing (reading)
#define EBADF           0x09

// A wait or waitpid function was executed by a process that had no existing or unwaited-for child processes.
#define ECHILD          0x0A
#define EAGAIN          0x0B
#define EWOULDBLOCK     EAGAIN

// The new process image required more memory than was allowed by the hardware or by system-imposed memory management constraints
#define ENOMEM          0x0C

// An attempt was made to access a file in a way forbidden by its file access permissions.
#define EACCES          0x0D

// The system detected an invalid address in attempting to use an argument of a call
#define EFAULT          0x0E

// A block device operation was attempted on a non-block device or file
#define ENOTBLK         0x0F

// An attempt to use a system resource which was in use at the time in a manner which would have conflicted with the request.
#define EBUSY           0x10

// An existing file was mentioned in an inappropriate context, for instance, as the new link name in a link function.
#define EEXIST          0x11

// A hard link to a file on another file system was attempted
#define EXDEV           0x12

// An attempt was made to apply an inappropriate function to a device, for example, trying to read a write-only device such as a printer
#define ENODEV          0x13

// A component of the specified pathname existed, but it was not a directory, when a directory was expected
#define ENOTDIR         0x14

// An attempt was made to open a directory with write mode specified.
#define EISDIR          0x15

// Some invalid argument was supplied. (For example, specifying an undefined signal to a signal or kill function)
#define EINVAL          0x16

// Maximum number of file descriptors allowable on the system has been reached and a requests for an open cannot be satisfied until at least one has been closed
#define ENFILE          0x17

// <As released, the limit on the number of open files per process is 64.> Getdtablesize(2) will obtain the current limit.
#define EMFILE          0x18

// A control function (see ioctl(2)) was attempted for a file or special device for which the operation was inappropriat
#define ENOTTY          0x19

// The new process was a pure procedure (shared text) file which was open for writing by another process, or while the pure procedure file was being executed an open call requested write access
#define ETXTBSY         0x1A

// The size of a file exceeded the maximum (about 2.1E9 bytes on some filesystems including UFS, 1.8E19 bytes on HFS+ and others)
#define EFBIG           0x1B

// A write to an ordinary file, the creation of a directory or symbolic link, or the creation of a directory entry failed because no more disk blocks were available on the file system, or the allocation of an inode for a newly created file failed because no more inodes were available on the file system.
#define ENOSPC          0x1C

// An lseek function was issued on a socket, pipe or FIFO.
#define ESPIPE          0x1D

// An attempt was made to modify a file or directory was made on a file system that was read-only at the time
#define EROFS           0x1E

// Maximum allowable hard links to a single file has been exceeded (limit of 32767 hard links per file).
#define EMLINK          0x1F

// A write on a pipe, socket or FIFO for which there is no process to read the data
#define EPIPE           0x20

// A numerical input argument was outside the defined domain of the mathematical function.
#define EDOM            0x21

// A numerical result of the function was too large to fit in the available space (perhaps exceeded precision)
#define ERANGE          0x22
#define EDEADLK         0x23
#define EDEADLOCK       EDEADLK
#define ENAMETOOLONG    0x24
#define ENOLCK          0x25
#define ENOSYS          0x26
#define ENOTEMPTY       0x27
#define ELOOP           0x28

/* 0x29 reserved */

#define ENOMSG          0x2A
#define EIDRM           0x2B
#define ECHRNG          0x2C
#define EL2NSYNC        0x2D
#define EL3HLT          0x2E
#define EL3RST          0x2F
#define ELNRNG          0x30
#define EUNATCH         0x31
#define ENOCSI          0x32
#define EL2HLT          0x33
#define EBADE           0x34
#define EBADR           0x35
#define EXFULL          0x36
#define ENOANO          0x37
#define EBADRQC         0x38
#define EBADSLT         0x39

/* 0x3A reserved */

#define EBFONT          0x3B
#define ENOSTR          0x3C
#define ENODATA         0x3D
#define ETIME           0x3E
#define ENOSR           0x3F
#define ENONET          0x40
#define ENOPKG          0x41
#define EREMOTE         0x42
#define ENOLINK         0x43
#define EADV            0x44
#define ESRMNT          0x45
#define ECOMM           0x46
#define EPROTO          0x47
#define EMULTIHOP       0x48
#define EDOTDOT         0x49
#define EBADMSG         0x4A
#define EOVERFLOW       0x4B
#define ENOTUNIQ        0x4C
#define EBADFD          0x4D
#define EREMCHG         0x4E
#define ELIBACC         0x4F
#define ELIBBAD         0x50
#define ELIBSCN         0x51
#define ELIBMAX         0x52
#define ELIBEXEC        0x53
#define EILSEQ          0x54
#define ERESTART        0x55
#define ESTRPIPE        0x56
#define EUSERS          0x57
#define ENOTSOCK        0x58
#define EDESTADDRREQ    0x59
#define EMSGSIZE        0x5A
#define EPROTOTYPE      0x5B
#define ENOPROTOOPT     0x5C
#define EPROTONOSUPPORT 0x5D
#define ESOCKTNOSUPPORT 0x5E
#define EOPNOTSUPP      0x5F
#define ENOTSUP         EOPNOTSUPP
#define EPFNOSUPPORT    0x60
#define EAFNOSUPPORT    0x61
#define EADDRINUSE      0x62
#define EADDRNOTAVAIL   0x63
#define ENETDOWN        0x64
#define ENETUNREACH     0x65
#define ENETRESET       0x66
#define ECONNABORTED    0x67
#define ECONNRESET      0x68
#define ENOBUFS         0x69
#define EISCONN         0x6A
#define ENOTCONN        0x6B
#define ESHUTDOWN       0x6C
#define ETOOMANYREFS    0x6D
#define ETIMEDOUT       0x6E
#define ECONNREFUSED    0x6F
#define EHOSTDOWN       0x70
#define EHOSTUNREACH    0x71
#define EALREADY        0x72
#define EINPROGRESS     0x73
#define ESTALE          0x74
#define EUCLEAN         0x75
#define ENOTNAM         0x76
#define ENAVAIL         0x77
#define EISNAM          0x78
#define EREMOTEIO       0x79
#define EDQUOT          0x7A
#define ENOMEDIUM       0x7B
#define EMEDIUMTYPE     0x7C
#define ECANCELED       0x7D
#define ENOKEY          0x7E
#define EKEYEXPIRED     0x7F
#define EKEYREVOKED     0x80
#define EKEYREJECTED    0x81
#define EOWNERDEAD      0x82
#define ENOTRECOVERABLE 0x83
#define ERFKILL         0x84
#define EHWPOISON       0x85

#endif /* __ERRNO_H__ */