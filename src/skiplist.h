#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SKIPLIST_MAX_HEIGHT 32

typedef struct {
    uint64_t traversals;
    uint64_t data_allocations;
} skiplist_work_t;

void skiplist_reset_work();
skiplist_work_t skiplist_get_work();

// nodo skiplist
typedef struct skiplist_node {
    int64_t key;
    int height;
    struct skiplist_node* next[];
} skiplist_node_t;

typedef struct {
    skiplist_node_t* head; 
    int max_height;        
    uint64_t rng_state; 
} skiplist_t;

skiplist_t* create_skiplist(uint64_t seed);

void free_skiplist(skiplist_t* skiplist);

uint64_t xorshift64(uint64_t* state);

bool skiplist_search(const skiplist_t* skiplist, int64_t key);
bool skiplist_insert(skiplist_t* skiplist, int64_t key);
void skiplist_split(skiplist_t* skiplist, int64_t pivot, skiplist_t** left_split, skiplist_t** right_split);
skiplist_t* skiplist_union(skiplist_t* skiplist_1, skiplist_t* skiplist_2);

bool skiplist_check_invariants(const skiplist_t* skiplist);

void skiplist_split_rebuilding(skiplist_t* skiplist, int64_t pivot, skiplist_t** left_split, skiplist_t** right_split);

#endif