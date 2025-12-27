#ifndef __DATASTRUCTURES_H__
#define __DATASTRUCTURES_H__

#include "macros.h"

/* ============= PRIORITY QUEUES ============= */
typedef struct pqnode_s{
    uint64_t priority;
    uintptr_t element; // generic element pointer
} pqnode_t;

typedef struct pq_s{
    pqnode_t* heap;
    int size;           // physical size
    int items;          // logical size
} pq_t;

pq_t* pq_init();
void pq_destroy();

void pq_add(pq_t* pq, uint64_t priority, uintptr_t element);
pqnode_t pq_pop(pq_t* pq);
pqnode_t pq_peek(pq_t* pq);

void _pq_swap(pq_t* pq, int idx1, int idx2);
void _pq_sink(pq_t* pq, int idx);
void _pq_swim(pq_t* pq, int idx);
/* =========================================== */

/* ============= LINKED LISTS ============= */
typedef struct list_head_s{
    struct list_head_s* next;
    struct list_head_s* prev;
} list_head_t;

#define LIST_HEAD(name) list_head_t name = { &(name), &(name) };
#define list_entry(ptr, type, member) ((type*)((char*)(ptr) - offsetof(type, member)))

static inline void INIT_LIST_HEAD(list_head_t* head) {
    head->next = head;
    head->prev = head;
}

static inline void list_add(list_head_t* newnode, list_head_t* head){
    newnode->next = head->next;
    newnode->prev = head;
    head->next->prev = newnode;
    head->next = newnode;
}

static inline void list_remove(list_head_t *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;

    // Optional: make node “empty” to catch bugs
    node->next = node;
    node->prev = node;
}
static inline int list_empty(list_head_t* head) {
    return head->next == head;
}
/* ======================================== */

#endif