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

static trie_node* trie_add_down(const char* key_remainder, u64 value){
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

void trie_add(trie* t, const char* key, u64 value){
    if(!t->keys){
        trie_node* result = trie_add_down(key, value);
        if(result){
            t->keys++;
            t->head = result;
        }
        return;
    }

    trie_node* node = t->head;
    trie_node* parent = NULL;
    const char* cp = key;
    char c = *cp;

    while(c){
        trie_node* cur = node;
        trie_node* prev = NULL;

        // search for c in the current level
        while(cur && cur->c < c){
            prev = cur;
            cur = cur->next;
        }

        if(cur && cur->c == c){
            // found matching node, go down
            parent = cur;
            node = cur->down;
            c = *(++cp);
        } else {
            // character not found, insert a new chain for the rest of the key
            trie_node* new_node = trie_add_down(cp, value);
            if(!new_node) return;

            if(!prev){
                // insert before cur at this level
                new_node->next = cur;
                if(parent) parent->down = new_node;
                else t->head = new_node;
            } else {
                // insert after prev
                new_node->next = cur;
                prev->next = new_node;
            }
            t->keys++;
            return;
        }
    }

    // reached end of key, set value on current node
    if(parent) parent->value = value;
}

void trie_remove(trie* t, const char* key){
    return;
}

u64 trie_get(trie* t, const char* key){
    if(!t->keys) return NULL;
    char* cp = key;
    char c = *cp;

    trie_node* node = t->head;
    trie_node* prevnode = node;
    while(c){

        if(!node) return NULL;

        node = trie_get_down(node, c);
        c = *(++cp);

        if(!node && !c) return prevnode->value;

        prevnode = node;
    }

    return NULL;
}