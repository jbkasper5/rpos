#include "datastructures.h"

#define PARENT(i) (((i) - 1) / 2)
#define LCHILD(i) ((2 * (i)) + 1)
#define RCHILD(i) ((2 * (i)) + 2)

pq_t* pq_init(){
    return NULL;
}

void pq_destroy(){
    return;
}

void pq_add(pq_t* pq, uint64_t priority, uintptr_t element){
    if(pq->items >= pq->size) return; // or realloc
    pq->heap[pq->items].priority = priority;
    pq->heap[pq->items].element = element;
    _pq_swim(pq, pq->items++);
}


pqnode_t pq_pop(pq_t* pq){
    pqnode_t node = { .priority = 0, .element = 0 };
    if(!pq->items) return node;
    node = pq->heap[0];

    // copy last item into index 0
    _pq_swap(pq, 0, --pq->items);

    // sink
    _pq_sink(pq, 0);
    return node;
}

pqnode_t pq_peek(pq_t* pq){
    return pq->heap[0];
}

void _pq_sink(pq_t* pq, int idx) {
    int n = pq->items;
    while (1) {
        int lchild = LCHILD(idx);
        int rchild = RCHILD(idx);
        int largest = idx;

        if (lchild < n && pq->heap[lchild].priority > pq->heap[largest].priority)
            largest = lchild;
        if (rchild < n && pq->heap[rchild].priority > pq->heap[largest].priority)
            largest = rchild;

        if (largest == idx)
            break;

        _pq_swap(pq, idx, largest);
        idx = largest;
    }
}

void _pq_swim(pq_t* pq, int idx) {
    while (idx > 0) {
        int parent_idx = PARENT(idx);
        if (pq->heap[parent_idx].priority >= pq->heap[idx].priority)
            break;
        _pq_swap(pq, parent_idx, idx);
        idx = parent_idx;
    }
}

void _pq_swap(pq_t* pq, int idx1, int idx2){
    pqnode_t temp = pq->heap[idx1];
    pq->heap[idx1] = pq->heap[idx2];
    pq->heap[idx2] = temp;
}
