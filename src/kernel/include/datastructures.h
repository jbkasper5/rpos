#ifndef __DATASTRUCTURES_H__
#define __DATASTRUCTURES_H__

#include "macros.h"

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

void pq_add(pq_t* pq, int priority, uintptr_t element);
pqnode_t pq_pop(pq_t* pq);
pqnode_t pq_peek(pq_t* pq);

void _pq_swap(pq_t* pq, int idx1, int idx2);
void _pq_sink(pq_t* pq, int idx);
void _pq_swim(pq_t* pq, int idx);

#endif