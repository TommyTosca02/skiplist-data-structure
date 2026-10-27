#include "skiplist.h"
#include <stdlib.h>

#ifdef SKIPLIST_COUNT_WORK
static skiplist_work_t work;

void skiplist_reset_work() {
    work.traversals = 0;
    work.data_allocations = 0;
}

skiplist_work_t skiplist_get_work() {
    return work;
}

#define COUNT_TRAVERSAL() (work.traversals++)
#define COUNT_DATA_ALLOCATION() (work.data_allocations++)
#else
#define COUNT_TRAVERSAL() ((void)0)
#define COUNT_DATA_ALLOCATION() ((void)0)
#endif

// this part (up to #endif) is made by Gemini
// we have to find a way to execute _BitScanForward64() in case we are on windows, and __builtin_ctzll() in case we are on Linux/Mac
#if defined(_MSC_VER)
#include <intrin.h>
static inline int count_trailing_zeros64(uint64_t v) {
    unsigned long index;
    return _BitScanForward64(&index, v) ? (int)index : 64;
}
#else
static inline int count_trailing_zeros64(uint64_t v) {
    return v ? __builtin_ctzll(v) : 64; 
}
#endif

//also this function is made by Gemini
uint64_t xorshift64(uint64_t* state) {
    uint64_t x = *state;
    if (x == 0) x = 0x853c49e6748fea9bULL; 
    x ^= x << 13; 
    x ^= x >> 7;  
    x ^= x << 17; 
    *state = x; 
    return x; 
}

//the remaining part of skiplist.c is written by me

int skiplist_random_height(uint64_t* rng_state, int max_height) {
    uint64_t rand = xorshift64(rng_state);
    if (rand == 0) return max_height; 
    int zeroCount = count_trailing_zeros64(rand);
    int height = (int)zeroCount + 1;
    if(height > max_height)  height = max_height;
    return height;
}

static skiplist_node_t* create_node(int64_t key, int height) {
    skiplist_node_t* node = (skiplist_node_t*)malloc(sizeof(skiplist_node_t) + height * sizeof(skiplist_node_t*));
    if (!node) return NULL;
    node->key = key;
    node->height = height;

    for (int i = 0; i < height; i++) {
        node->next[i] = NULL;
    }
    
    return node;
}

skiplist_t* create_skiplist(uint64_t seed)  {
    skiplist_t* skiplist = (skiplist_t*)malloc(sizeof(skiplist_t)); 
    if(!skiplist) return NULL;
    skiplist->max_height = SKIPLIST_MAX_HEIGHT;

    if(seed == 0)
        skiplist->rng_state = 0x123456789ABCDEF0ULL;
    else
        skiplist->rng_state = seed;

    skiplist->head = create_node(0, SKIPLIST_MAX_HEIGHT);

    if(!skiplist->head) {
        free(skiplist);
        return NULL;
    }
    return skiplist;
}

void free_skiplist(skiplist_t* skiplist) {
    if(!skiplist) return;
    skiplist_node_t* current_node = skiplist->head;

    while(current_node) {
        skiplist_node_t* next_node = current_node->next[0]; 
        free(current_node);
        current_node = next_node;
    }

    free(skiplist);
}

bool skiplist_search(const skiplist_t* skiplist, int64_t key) {
    if(!skiplist) return false;
    if(!skiplist->head) return false;

    skiplist_node_t* current_node = skiplist->head;

    for(int i=skiplist->max_height-1; i>=0; i--) {
        while(current_node->next[i] && current_node->next[i]->key < key) {
            current_node = current_node->next[i];
            COUNT_TRAVERSAL();
        }
    }
    current_node = current_node->next[0];
    if (current_node) COUNT_TRAVERSAL();

    if(current_node && current_node->key == key)
        return true;
    else
        return false;
}

bool skiplist_insert(skiplist_t* skiplist, int64_t key) {
    if(!skiplist) return false;
    if(!skiplist->head) return false;

    skiplist_node_t* current_node = skiplist->head;
    skiplist_node_t* update[SKIPLIST_MAX_HEIGHT];

    for(int i=skiplist->max_height-1; i>=0; i--) {
        while(current_node->next[i] && current_node->next[i]->key < key) {
            current_node = current_node->next[i];
            COUNT_TRAVERSAL();
        }
        update[i] = current_node;
    }

    if(current_node->next[0] && current_node->next[0]->key == key)
        return false;

    int height = skiplist_random_height(&(skiplist->rng_state), SKIPLIST_MAX_HEIGHT);
    skiplist_node_t* new_node = create_node(key, height);
    if (!new_node) return false;
    COUNT_DATA_ALLOCATION();

    for(int i=0; i < new_node->height; i++) {
        new_node->next[i] = update[i]->next[i];
        update[i]->next[i] = new_node;
    }

    return true;
}

void skiplist_split(skiplist_t* skiplist, int64_t pivot, skiplist_t** left_split, skiplist_t** right_split) {
    if (!skiplist || !skiplist->head || !left_split || !right_split) return;

    *left_split = create_skiplist(skiplist->rng_state);
    if (!*left_split) return;
    *right_split = create_skiplist(skiplist->rng_state);
    if (!*right_split) {
        free_skiplist(*left_split);
        *left_split = NULL;
        return;
    }

    skiplist_node_t* left_cut[SKIPLIST_MAX_HEIGHT];
    skiplist_node_t* current_node = skiplist->head;

    for(int i=skiplist->max_height-1; i>=0; i--) {
        while(current_node->next[i] && current_node->next[i]->key <= pivot) {
            current_node = current_node->next[i];
            COUNT_TRAVERSAL();
        }
        left_cut[i] = current_node;
    }

    for(int i=0; i<skiplist->max_height; i++) {
        (*right_split)->head->next[i] = left_cut[i]->next[i];

        if(left_cut[i] != skiplist->head) {
            (*left_split)->head->next[i] = skiplist->head->next[i];
            left_cut[i]->next[i] = NULL; 
        } else {
            (*left_split)->head->next[i] = NULL;
        }

        skiplist->head->next[i] = NULL; 
    }

}

skiplist_t* skiplist_union(skiplist_t* skiplist_1, skiplist_t* skiplist_2) {
    if (!skiplist_1 && !skiplist_2) return NULL;
    if (!skiplist_1) return skiplist_2;
    if (!skiplist_2) return skiplist_1;

    uint64_t seed = skiplist_1->rng_state;

    skiplist_t* result = create_skiplist(seed);
    if (!result) return NULL;

    skiplist_node_t* last[SKIPLIST_MAX_HEIGHT];
    skiplist_node_t* current_node = skiplist_1->head;

    for (int i = SKIPLIST_MAX_HEIGHT - 1; i >= 0; i--) {
        while (current_node->next[i]) {
            current_node = current_node->next[i];
            COUNT_TRAVERSAL();
        }
        last[i] = current_node;
    }

    for(int i = 0; i < SKIPLIST_MAX_HEIGHT; i++) {
        if(last[i] == skiplist_1->head) {
            result->head->next[i] = skiplist_2->head->next[i];
        }
        else {
            result->head->next[i] = skiplist_1->head->next[i];
            last[i]->next[i] = skiplist_2->head->next[i];
        }

        skiplist_1->head->next[i] = NULL;
        skiplist_2->head->next[i] = NULL;
    }

    return result;
}

bool skiplist_check_invariants(const skiplist_t* skiplist)  {
    if (!skiplist || !skiplist->head || skiplist->head->height != SKIPLIST_MAX_HEIGHT) 
        return false;

    skiplist_node_t* current_node = skiplist->head->next[0];

    while(current_node) {
        if(current_node->height < 1 || current_node->height > SKIPLIST_MAX_HEIGHT)
            return false;

        for(int i=0; i<current_node->height; i++) {
            if(current_node->next[i]) {
                if(current_node->key >= current_node->next[i]->key)
                    return false;
                if(current_node->next[i]->height < i+1)
                    return false;
            }
        }
        current_node = current_node->next[0];
    }

    return true;
}

void skiplist_split_rebuilding(skiplist_t* skiplist, int64_t pivot, skiplist_t** left_split, skiplist_t** right_split) {
    if (!skiplist || !skiplist->head || !left_split || !right_split) return;

    *left_split = create_skiplist(skiplist->rng_state);
    if (!*left_split) return;
    *right_split = create_skiplist(skiplist->rng_state);
        if (!*right_split) {
        free_skiplist(*left_split);
        *left_split = NULL;
        return;
    }

    skiplist_node_t* current_node = skiplist->head->next[0];
    if (current_node) COUNT_TRAVERSAL();
    skiplist_node_t* temp = NULL;

    while(current_node) {
        if(current_node->key <= pivot)
            skiplist_insert(*left_split, current_node->key);
        else
            skiplist_insert(*right_split, current_node->key);
        temp = current_node;
        current_node = current_node->next[0];
        if (current_node) COUNT_TRAVERSAL();
        free(temp);
    }

    for (int i = 0; i < SKIPLIST_MAX_HEIGHT; i++) {
        skiplist->head->next[i] = NULL;
    }
}