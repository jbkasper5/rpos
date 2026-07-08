#ifndef __DATASTRUCTURES_TYPES_H__
#define __DATASTRUCTURES_TYPES_H__

#include "macros.h"

/* ============= PRIORITY QUEUES ============= */
typedef struct pqnode_s{
    u64 priority;
    uintptr_t element; // generic element pointer
} pqnode_t;

typedef struct pq_s{
    pqnode_t* heap;
    int size;           // physical size
    int items;          // logical size
} pq_t;

/* ============= LINKED LISTS ============= */
typedef struct list_head_s{
    struct list_head_s* next;
    struct list_head_s* prev;
} list_head_t;

/* ============= SEARCH TRIES ============= */
typedef struct trie_s{
    struct trie_node_s* head;
    u32 keys;
} trie;

typedef struct trie_node_s{
    struct trie_node_s* next;
    struct trie_node_s* down;
    u64 value;
    char c;
    char reserved[7];
} trie_node;

#endif
