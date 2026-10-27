#include "../src/skiplist.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <limits.h>

void test_insert_and_search() {
    //test empty list
    skiplist_t* skiplist = create_skiplist(42);
    assert(skiplist != NULL);
    assert(skiplist_check_invariants(skiplist) == true); // L'invariante deve passare su lista vuota
    assert(skiplist_search(skiplist, 100) == false);

    //test insert
    assert(skiplist_insert(skiplist, 50) == true);      
    assert(skiplist_check_invariants(skiplist) == true);
    assert(skiplist_search(skiplist, 50) == true);      
    assert(skiplist_search(skiplist, 99) == false);

    //test insert of duplicate
    assert(skiplist_insert(skiplist, 50) == false);
    assert(skiplist_check_invariants(skiplist) == true); 

    //edge cases insert
    assert(skiplist_insert(skiplist, INT64_MIN) == true); 
    assert(skiplist_insert(skiplist, INT64_MAX) == true); 
    assert(skiplist_insert(skiplist, 0) == true);         
    assert(skiplist_insert(skiplist, -1) == true);        
    assert(skiplist_check_invariants(skiplist) == true);

    //test search
    assert(skiplist_search(skiplist, INT64_MIN) == true);
    assert(skiplist_search(skiplist, INT64_MAX) == true);
    assert(skiplist_search(skiplist, 0) == true);
    assert(skiplist_search(skiplist, -1) == true);
    assert(skiplist_search(skiplist, 50) == true);
    assert(skiplist_search(skiplist, 1234567) == false);
    assert(skiplist_check_invariants(skiplist) == true);

    free_skiplist(skiplist);
}

void test_split_structural() {
    skiplist_t* skiplist = create_skiplist(42);

    int64_t keys[8] = {10, 20, 30, 42, 50, 60, 70, 80};
    for (int i = 0; i < 8; i++) {
        skiplist_insert(skiplist, keys[i]);
    }
    assert(skiplist_check_invariants(skiplist) == true);

    void* addr10_orig = (void*)(skiplist->head->next[0]);
    int height10_orig = skiplist->head->next[0]->height;

    skiplist_t* left = NULL;
    skiplist_t* right = NULL;
    skiplist_split(skiplist, 42, &left, &right);

    assert((void*)(left->head->next[0]) == addr10_orig);
    assert(left->head->next[0]->height == height10_orig);

    assert(skiplist_check_invariants(skiplist) == true && skiplist->head->next[0] == NULL);

    free_skiplist(skiplist);
    free_skiplist(left);
    free_skiplist(right);
}

void test_roundtrip() {
    skiplist_t* skiplist = create_skiplist(2026);

    int64_t keys[10] = {-100, -50, 0, 10, 20, 30, 42, 50, 60, 100};
    for (int i = 0; i < 10; i++) {
        skiplist_insert(skiplist, keys[i]);
    }

    skiplist_t *left = NULL, *right = NULL;
    skiplist_split(skiplist, 20, &left, &right);
    skiplist_t* result_union = skiplist_union(left, right);

    assert(result_union != NULL);
    assert(skiplist_check_invariants(result_union) == true);
    //verifichiamo che U contenga tutti gli elementi di partenza
    for (int i = 0; i < 10; i++) {
        assert(skiplist_search(result_union, keys[i]) == true);
    }

    assert(skiplist_check_invariants(skiplist) == true && skiplist->head->next[0] == NULL);
    assert(skiplist_check_invariants(left) == true && left->head->next[0] == NULL);
    assert(skiplist_check_invariants(right) == true && right->head->next[0] == NULL);

    free_skiplist(skiplist);
    free_skiplist(left);
    free_skiplist(right);
    free_skiplist(result_union);
}

//test con 1.000 operazioni miste: 55% inserimento, 30% ricerca, 15% split e union in sequenza
void test_mixed_operations(uint64_t seed) {
    skiplist_t* skiplist = create_skiplist(seed);
    int64_t reference[1000];
    int count = 0;

    for(int i=0; i<1000; i++) {
        int64_t key = (int64_t)xorshift64(&seed);
        uint64_t op = xorshift64(&seed) % 100;

        int pos = 0;
        while (pos < count && reference[pos] < key) {
            pos++;
        }
        //if the key already exists in the reference array
        bool exists = pos < count && reference[pos] == key;

        if(op<55) {
            skiplist_insert(skiplist, key);
            assert(skiplist_check_invariants(skiplist) == true);

            if (!exists) {
                for (int j = count; j > pos; j--)
                    reference[j] = reference[j - 1];

                reference[pos] = key;
                count++;
            }
        } else if(op < 85) {
            skiplist_search(skiplist, key);
            assert(skiplist_check_invariants(skiplist) == true);
        } else {
            //split
            skiplist_t* left = NULL;
            skiplist_t* right = NULL;
            skiplist_split(skiplist, key, &left, &right);
            assert(skiplist_check_invariants(skiplist) == true);
            assert(skiplist_check_invariants(left) == true);
            assert(skiplist_check_invariants(right) == true);

            //union
            skiplist_t* result_union = skiplist_union(left, right);
            assert(skiplist_check_invariants(result_union) == true);

            free_skiplist(skiplist);
            free_skiplist(left);
            free_skiplist(right);

            skiplist = result_union;
        }

        skiplist_node_t* node = skiplist->head->next[0];
        for (int j = 0; j < count; j++) {
            assert(node != NULL && node->key == reference[j]);
            node = node->next[0];
        }
        assert(node == NULL);
    }

    free_skiplist(skiplist);
}

void test_work_counts() {
    int sizes[] = {1024, 4096, 16384, 65536, 262144, 1048576};
    int seeds[] = {42, 234, 1000};

    for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 6; i++) {
            int n = sizes[i];
            int seed = seeds[j];
            skiplist_work_t work;

            printf("\nN=%d seed=%d\n", n, seed);

            skiplist_t* s = create_skiplist(seed);
            for (int k = 0; k < n; k++) {
                skiplist_insert(s, 2 * k);
            }

            //search
            skiplist_reset_work();
            for (int k = 0; k < 500; k++) {
                //k * n / 500 is used to distribute the 500 searches among all the skiplist, and 2* is needed because the skiplist contains only even numbers
                int key = 2*(k * n / 500);
                skiplist_search(s, key);
                skiplist_search(s, key + 1);
            }
            work = skiplist_get_work();
            printf("Search: %.2f traversals\n", (double)work.traversals / 1000);

            //insert
            skiplist_reset_work();
            for (int k = 0; k < 128; k++) {
                int key = 2 * (k * (int64_t)n / 128) + 1;
                skiplist_insert(s, key);
            }
            work = skiplist_get_work();
            printf("Insert: %.2f traversals, %.2f allocations\n", (double)work.traversals / 128, (double)work.data_allocations / 128);
            free_skiplist(s);

            //structural split
            s = create_skiplist(seed);
            for (int k = 0; k < n; k++) {
                skiplist_insert(s, 2 * k);
            }

            skiplist_t *left = NULL, *right = NULL;

            skiplist_reset_work();
            skiplist_split(s, n - 1, &left, &right);
            work = skiplist_get_work();
            printf("Split: %llu traversals, %llu allocations\n", work.traversals, work.data_allocations);

            free_skiplist(s);
            free_skiplist(left);
            free_skiplist(right);

            //union
            skiplist_t* a = create_skiplist(seed);
            skiplist_t* b = create_skiplist(seed + 8);
            for (int k = 0; k < n / 2; k++) {
                skiplist_insert(a, k);
                skiplist_insert(b, n / 2 + k);
            }

            skiplist_reset_work();
            skiplist_t* result = skiplist_union(a, b);
            work = skiplist_get_work();
            printf("Union: %llu traversals\n", work.traversals);

            free_skiplist(a);
            free_skiplist(b);
            free_skiplist(result);

            //rebuilding split
            s = create_skiplist(seed);
            for (int k = 0; k < n; k++) {
                skiplist_insert(s, 2 * k);
            }
            
            left = NULL;
            right = NULL;

            skiplist_reset_work();
            skiplist_split_rebuilding(s, n - 1, &left, &right);
            work = skiplist_get_work();
            printf("Rebuilding: %llu traversals, %llu allocations\n", work.traversals, work.data_allocations);

            free_skiplist(s);
            free_skiplist(left);
            free_skiplist(right);
        }
    }
}

int main() {
    printf("=== STARTING TEST SUITE SKIPLIST ===\n\n");

    printf("Executing test insert and search...");
    test_insert_and_search();
    printf("Ok\n");

    printf("Executing test split...");
    test_split_structural();
    printf("Ok\n");

    printf("Executing test roundtrip...");
    test_roundtrip();
    printf("Ok\n");

    printf("Executing 1.000 mixed operations on 3 seed...\n");
    test_mixed_operations(42);
    test_mixed_operations(234);
    test_mixed_operations(1000);
    printf("ok\n");

    printf("\n=== ALL TESTS PASSED SUCCESSFULLY! ===\n");

    test_work_counts();

    return 0;
}