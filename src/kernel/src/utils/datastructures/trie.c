#include "utils/datastructures.h"
#include "memory/kmalloc.h"

trie* trie_init(){
    trie* t = (trie*)kmalloc(sizeof(trie));
    t->keys = 0;
    t->head = NULL;
    return t;
}

static trie_node* alloc_trie_node(char c){
    trie_node* node = (trie_node*) kmalloc(sizeof(trie_node));
    if(!node) return NULL;

    node->c = c;
    node->next = NULL;
    node->down = NULL;
    node->value = NULL;
    return node;
}

static trie_node* trie_add_down(const char* key_remainder, uint64_t value){
    char* cp = key_remainder;
    char c = *cp;

    trie_node* startnode = alloc_trie_node(c);
    if(!startnode) return NULL;

    trie_node* currnode = startnode;
    c = *(++cp);
    while(c){
        currnode->down = alloc_trie_node(c);
        if(!currnode->down) return NULL;

        currnode = currnode->down;
        c = *(++cp);
    }
    currnode->value = value;
    return startnode;
}

static trie_node* trie_get_down(trie_node* node, char c){
    while(node){
        if(node->c < c){
            node = node->next;
            continue;
        }else if(node->c == c){
            return node->down;
        }else{
            return NULL;
        }
    }
}

void trie_add(trie* t, const char* key, uint64_t value){
    if(!t->keys){
        trie_node* result = trie_add_down(key, value);
        if(result){
            t->keys++;
            t->head = result;
        }
    }
}

void trie_remove(trie* t, const char* key){
    return;
}

uint64_t trie_get(trie* t, const char* key){
    if(!t->keys) return NULL;
    char* cp = key;
    char c = *cp;

    trie_node* node = t->head;
    trie_node* prevnode = node;
    while(c){
        // skip over the '/' character in dev paths to save space
        // if(*cp == '/'){
        //     cp++;
        //     continue;
        // }

        if(!node) return NULL;

        node = trie_get_down(node, c);
        c = *(++cp);

        if(!node && !c) return prevnode->value;

        prevnode = node;
    }

    return NULL;
}