#ifndef __PIPE_H__
#define __PIPE_H__

#include "memory/virtual_memory.h"
#include "macros.h"

// typedef struct pipe_buffer_s {
//     u8* page;                    // The actual memory page containing data
//     u32 offset;
//     u32 len;             // Location and size of data within the page
//     // const struct pipe_buf_operations *ops; // Table of operations (e.g., release, steal)
//     u32 flags;                   // Buffer-specific flags
//     u64 private;                // Private data for the operations
// } pipe_buffer_t;

typedef struct pipe_data_s{
    // struct mutex mutex;           // Mutex for serializing access
    // wait_queue_head_t rd_wait;    // Wait queue for readers
    // wait_queue_head_t wr_wait;    // Wait queue for writers
    u32 head;            // Pointer to the next slot to write
    u32 tail;            // Pointer to the next slot to read
    u32 ring_size;       // Total number of buffers in the ring
    u32 readers;         // Number of processes reading
    u32 writers;         // Number of processes writing
    u8* buf;             // page buffer
} pipe_data_t;

#endif