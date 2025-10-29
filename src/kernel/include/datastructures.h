#ifndef __DATASTRUCTURES_H__
#define __DATASTRUCTURES_H__

#include "macros.h"

typedef struct pqnode_s{
    int priority;
    void* element;
} pqnode_t;

typedef struct pq_s{
    pqnode_t* heap;
    int size;           // physical size
    int items;          // logical size
} pq_t;

pq_t* pq_init();
void pq_destroy();

void pq_add(pq_t* pq, uint64_t element);
void pq_pop(pq_t* pq);

void _pq_sink(pq_t* pq);
void _pq_swim(pq_t* pq);

#endif