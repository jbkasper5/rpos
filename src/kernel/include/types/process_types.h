#ifndef __PROCESS_TYPES_H__
#define __PROCESS_TYPES_H__

#include "macros.h"
#include "types/datastructures_types.h"
#include "types/disk_types.h"
#include "types/filesystem_types.h"

#define MAX_PROCESSES 64
#define MAX_OPEN_FILES 32

enum PROC_STATE{
    PROCESS_BLOCKED,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_TERMINATED,
};

enum WAIT_STATE{
    WAITING_ALL = -1,
    WAITING_NONE = 0
};

enum EXIT_STATUS{
    EX_OK           = 0,      /* successful termination */
    EX__BASE        = 64,      /* base value for error messages */
    EX_USAGE        = 64,      /* command line usage error */
    EX_DATAERR      = 65,      /* data format error */
    EX_NOINPUT      = 66,      /* cannot open input */
    EX_NOUSER       = 67,      /* addressee unknown */
    EX_NOHOST       = 68,      /* host name unknown */
    EX_UNAVAILABLE  = 69,      /* service unavailable */
    EX_SOFTWARE     = 70,      /* internal software error */
    EX_OSERR        = 71,      /* system error (e.g., can't fork) */
    EX_OSFILE       = 72,      /* critical OS file missing */
    EX_CANTCREAT    = 73,      /* can't create (user) output file */
    EX_IOERR        = 74,      /* input/output error */
    EX_TEMPFAIL     = 75,      /* temp failure; user is invited to retry */
    EX_PROTOCOL     = 76,      /* remote error in protocol */
    EX_NOPERM       = 77,      /* permission denied */
    EX_CONFIG       = 78,      /* configuration error */
};

typedef struct pcb_s {
    u64 kernel_stack;

    u64 ttbr;

    enum PROC_STATE state;
    enum EXIT_STATUS exit_status;

    u32 pid;
    u32 waiting_on;

    list_head_t proclist;
    list_head_t runqueue;

    struct pcb_s* parent;
    list_head_t children;
    list_head_t siblings;

    directory* cwd;

    file_t* fds[MAX_OPEN_FILES];
    // file structure information
    // thread information
    // signal information
    // timer information
}pcb_t;

#endif
