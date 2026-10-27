extern "C" {
    #include "skiplist.h"
}

#include <benchmark/benchmark.h>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <chrono>
#include <cstdlib>

static void BM_search(benchmark::State& state) {
    const int64_t N = state.range(0); 
    const uint64_t seed = state.range(1);
    skiplist_t* skiplist = create_skiplist(seed);

    for (int64_t i = 0; i < N; i++) {
        skiplist_insert(skiplist, i * 2);
    }

    std::vector<int64_t> queries(1000);
    std::mt19937_64 rng(42);
    std::uniform_int_distribution<int64_t> dist(0, N - 1);
    for(int64_t i=0; i < 500; i++)  {
        int64_t index = dist(rng);
        queries[i] = index * 2;
        queries[i + 500] = index * 2 + 1;
    }

    size_t count = 0;
    for (auto _ : state) {
        size_t position = count % 1000;
        int64_t desired_key = queries[position];
        bool found = skiplist_search(skiplist, desired_key);
        benchmark::DoNotOptimize(found); 
        count++;
    }

    free_skiplist(skiplist);
    state.SetComplexityN(N); 
}



static void BM_insert(benchmark::State& state) {
    const int64_t N = state.range(0);
    const uint64_t seed = state.range(1);
    skiplist_t* skiplist = create_skiplist(seed);

    for (int64_t i = 0; i < N; i++)
        skiplist_insert(skiplist, i * 2);

    std::vector<int64_t> keys(4096);
    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<int64_t> dist(0, N - 1);
    for (auto& key : keys)
        key = 2 * dist(rng) + 1;

    int index = 0;
    for (auto _ : state) {
        int64_t key = keys[index % keys.size()];
        index++;

        auto start = std::chrono::steady_clock::now();
        bool inserted = skiplist_insert(skiplist, key);
        auto end = std::chrono::steady_clock::now();

        benchmark::DoNotOptimize(inserted);
        state.SetIterationTime(std::chrono::duration<double>(end - start).count());

        skiplist_node_t* update[SKIPLIST_MAX_HEIGHT];
        skiplist_node_t* current = skiplist->head;

        for (int i = skiplist->max_height - 1; i >= 0; i--) {
            while (current->next[i] && current->next[i]->key < key)
                current = current->next[i];
            update[i] = current;
        }

        skiplist_node_t* node = update[0]->next[0];
        for (int i = 0; i < node->height; i++)
            update[i]->next[i] = node->next[i];
        free(node);
    }

    free_skiplist(skiplist);
    state.SetComplexityN(N);
}


static void BM_split_structural(benchmark::State& state) {
    const int64_t N = state.range(0);
    const uint64_t seed = state.range(1);
    skiplist_t* skiplist = create_skiplist(seed);

    for (int64_t i = 0; i < N; i++)
        skiplist_insert(skiplist, i);

    std::vector<int64_t> pivots(4096);
    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<int64_t> dist(N / 2 - N / 16, N / 2 + N / 16);
    for (auto& pivot : pivots)
        pivot = dist(rng);

    int index = 0;

    for (auto _ : state) {
        int64_t pivot = pivots[index % pivots.size()];
        index++;

        skiplist_node_t* cut[SKIPLIST_MAX_HEIGHT];
        skiplist_node_t* current = skiplist->head;

        for (int i = skiplist->max_height - 1; i >= 0; i--) {
            while (current->next[i] && current->next[i]->key <= pivot)
                current = current->next[i];
            cut[i] = current;
        }

        skiplist_t* left = NULL;
        skiplist_t* right = NULL;

        auto start = std::chrono::steady_clock::now();
        skiplist_split(skiplist, pivot, &left, &right);
        auto end = std::chrono::steady_clock::now();

        benchmark::DoNotOptimize(left);
        benchmark::DoNotOptimize(right);
        state.SetIterationTime(
            std::chrono::duration<double>(end - start).count()
        );

        for (int i = 0; i < SKIPLIST_MAX_HEIGHT; i++) {
            if (cut[i] == skiplist->head) 
                skiplist->head->next[i] = right->head->next[i];
            else {
                skiplist->head->next[i] = left->head->next[i];
                cut[i]->next[i] = right->head->next[i];
            }

            left->head->next[i] = NULL;
            right->head->next[i] = NULL;
        }

        free_skiplist(left);
        free_skiplist(right);
    }

    free_skiplist(skiplist);
    state.SetComplexityN(N);
}


static void BM_split_rebuilding(benchmark::State& state) {
    const int64_t N = state.range(0);
    int64_t pivot = N / 2; 

    for (auto _ : state) {
        const uint64_t seed = state.range(1);
        skiplist_t* skiplist = create_skiplist(seed);
        for (int64_t i = 0; i < N; i++) {
            skiplist_insert(skiplist, i);
        }
        skiplist_t* left = NULL;
        skiplist_t* right = NULL;

        auto start = std::chrono::steady_clock::now();
        skiplist_split_rebuilding(skiplist, pivot, &left, &right);
        auto end = std::chrono::steady_clock::now();

        benchmark::DoNotOptimize(left);
        benchmark::DoNotOptimize(right);

        state.SetIterationTime(
            std::chrono::duration<double>(end - start).count()
        );

        free_skiplist(skiplist);
        free_skiplist(left);
        free_skiplist(right);
    }

    state.SetComplexityN(N);
}




static void BM_union(benchmark::State& state) {
    const int64_t N = state.range(0);
    const uint64_t seed = state.range(1);

    std::vector<skiplist_t*> A(8);
    std::vector<skiplist_t*> B(8);

    // Create 8 coples of indipendent skiplists
    for (int j = 0; j < 8; j++) {
        A[j] = create_skiplist(seed + j);
        B[j] = create_skiplist(seed + j + 8);

        for (int64_t i = 0; i < N / 2; i++) {
            skiplist_insert(A[j], i);
            skiplist_insert(B[j], N / 2 + i);
        }
    }

    int index = 0;

    for (auto _ : state) {
        skiplist_t* oldA = A[index];
        skiplist_t* oldB = B[index];

        auto start = std::chrono::steady_clock::now();
        skiplist_t* result = skiplist_union(oldA, oldB);
        auto end = std::chrono::steady_clock::now();

        benchmark::DoNotOptimize(result);

        state.SetIterationTime(
            std::chrono::duration<double>(end - start).count()
        );

        skiplist_split(result, N / 2 - 1, &A[index], &B[index]);

        free_skiplist(result);
        free_skiplist(oldA);
        free_skiplist(oldB);

        index++;
        if (index == 8) index = 0;
    }

    for (int j = 0; j < 8; j++) {
        free_skiplist(A[j]);
        free_skiplist(B[j]);
    }

    state.SetComplexityN(N);
}


BENCHMARK(BM_search)
    ->ArgsProduct({benchmark::CreateRange(1024, 1048576, 4), {42, 234, 1000}})
    ->Repetitions(3)->Complexity();

BENCHMARK(BM_insert)
    ->ArgsProduct({benchmark::CreateRange(1024, 1048576, 4), {42, 234, 1000}})
    ->UseManualTime()->Repetitions(3)->Complexity();

BENCHMARK(BM_split_structural)
    ->ArgsProduct({benchmark::CreateRange(1024, 1048576, 4), {42, 234, 1000}})
    ->UseManualTime()->Repetitions(3)->Complexity();

BENCHMARK(BM_split_rebuilding)
    ->ArgsProduct({benchmark::CreateRange(1024, 1048576, 4), {42, 234, 1000}})
    ->UseManualTime()->Repetitions(3)->Complexity();

BENCHMARK(BM_union)
    ->ArgsProduct({benchmark::CreateRange(1024, 1048576, 4), {42, 234, 1000}})
    ->UseManualTime()->Repetitions(3)->Complexity();

BENCHMARK_MAIN();