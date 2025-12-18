#ifndef SKIPLIST_HEADER
#error "SKIPLIST_HEADER must be defined to include the appropriate skiplist header."
#endif

#include SKIPLIST_HEADER

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <omp.h>
#include <string.h>
#include <limits.h>

// ENUMS 
enum strategy { RANDOM, UNIQUE, DETERMINISTIC };
enum key_range_type { DISJOINT, PER_THREAD, COMMON };

// STRUCTS
struct counters {
    int all_operations;
    int succ_operations;
    int succ_insert;
    int succ_delete;
    int succ_contains;
    int all_insert;
    int all_delete;
    int all_contains;
};
struct bench_result {
    float time;
    struct counters reduced_counters;
};

int global_index = 0;
#pragma omp threadprivate(global_index)

// HELPER FUNCTIONS
int generate_key(int start_range, int end_range, enum strategy st, int seed, int *arr) {
    struct random_data rand_state;
    int choice;
    char statebuf[32];
    bzero(&rand_state, sizeof(struct random_data));
    bzero(&statebuf, sizeof(statebuf));
    initstate_r(seed, statebuf, 32, &rand_state);
    int key = 0;

    switch (st) {
        case RANDOM:
            random_r(&rand_state, &choice);
            key = (choice % (end_range - start_range + 1)) + start_range;
            break;
        case UNIQUE:
            if (global_index + start_range > end_range) {
                global_index = 0;
            }
            key = arr[global_index];
            global_index++;
            break;
        case DETERMINISTIC:
            if (global_index + start_range > end_range) {
                global_index = 0;
            }
            key = global_index + start_range;
            global_index++;
            break;
    }

    return key;
}

void fillAndShuffleRow(int *row, int start, int end, int size) {
    if (size != (end - start + 1)) {
        printf("Error: The size of the row does not match the range size.\n");
        return;
    }

    for (int i = 0; i < size; i++) {
        row[i] = start + i;
    }

    for (int i = size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = row[i];
        row[i] = row[j];
        row[j] = temp;
    }
}

void divide_range(int threads, int **ranges, int start, int end, enum key_range_type krt) {
    int range_size = end - start;

    if (krt == DISJOINT) {
        int chunk = range_size / threads;
        for (int i = 0; i < threads; i++) {
            ranges[i][0] = start + i * chunk;
            ranges[i][1] = (i == threads - 1) ? end : start + (i + 1) * chunk;
        }
    } else if (krt == PER_THREAD) {
        int chunk = range_size / (threads + 1);
        for (int i = 0; i < threads; i++) {
            ranges[i][0] = start + i * chunk;
            ranges[i][1] = start + (i + 2) * chunk;
            if (ranges[i][1] > end) ranges[i][1] = end;
        }
    } else if (krt == COMMON) {
        for (int i = 0; i < threads; i++) {
            ranges[i][0] = start;
            ranges[i][1] = end;
        }
    } else {
        fprintf(stderr, "Invalid mode\n");
    }
}

// Print Results Function
void print_results(struct counters data) {
    printf("\n--- Benchmark Results ---\n");
    printf("Total Operations: %d/%d\n", data.succ_operations, data.all_operations);

    printf("  Inserts: %d/%d\n", data.succ_insert, data.all_insert);
    printf("  Deletes: %d/%d\n", data.succ_delete, data.all_delete);
    printf("  Contains: %d/%d\n", data.succ_contains, data.all_contains);

    printf("-------------------------\n");
}

// Validation Function
void bench_validate_skiplist(SkipList *sl, int successful_prefill, int successful_inserts, int successful_deletes) {
    int expected_count = successful_prefill + successful_inserts - successful_deletes;
    int actual_count = get_element_count(sl);

    int test_key = INT_MAX - 1; // Use a unique key unlikely to conflict with existing keys
    bool failed_deletion = false;

    // Check if the test key is already in the skiplist
    if (contains(sl, test_key)) {
        // If already present, attempt to delete it and check behavior
        if (!erase(sl, test_key)) {
            fprintf(stderr, "Validation failed: Unable to delete the already present test key %d.\n", test_key);
            failed_deletion = true;
        }

        if (erase(sl, test_key)) {
            fprintf(stderr, "Validation failed: Test key %d was deleted more than once.\n", test_key);
            failed_deletion = true;
        }
    } else {
        // If not present, insert the test key
        if (!insert(sl, test_key, 12345)) {
            fprintf(stderr, "Validation failed: Unable to insert the test key %d.\n", test_key);
            failed_deletion = true;
        }

        // Now delete it and ensure it cannot be deleted again
        if (!erase(sl, test_key)) {
            fprintf(stderr, "Validation failed: Unable to delete the test key %d after insertion.\n", test_key);
            failed_deletion = true;
        }

        if (erase(sl, test_key)) {
            fprintf(stderr, "Validation failed: Test key %d was deleted more than once after insertion.\n", test_key);
            failed_deletion = true;
        }
    }

    // Perform standard element count validation and include deletion validation
    if (expected_count != actual_count || failed_deletion) {
        fprintf(stderr, "Validation failed: ");
        if (expected_count != actual_count) {
            fprintf(stderr, "Expected count = %d, Actual count = %d.\n", expected_count, actual_count);
        }
        if (failed_deletion) {
            fprintf(stderr, "Deletion behavior validation failed.\n");
        }
    } else {
        printf("Validation passed: Expected count matches Actual count (%d). Test key behavior (insertion and deletion) is correct.\n", actual_count);
    }
}



// BENCHMARK
struct counters random_bench1(struct SkipList *sl, int seed, double duration, int *operations, int start_range, int end_range, int *arr, enum strategy strat) {
    #pragma omp barrier
    struct random_data rand_state;
    int choice;
    char statebuf[32];
    bzero(&rand_state, sizeof(struct random_data));
    bzero(&statebuf, sizeof(statebuf));
    initstate_r(seed, statebuf, 32, &rand_state);

    struct counters data = {.all_operations = 0,
                            .succ_insert = 0,
                            .succ_delete = 0,
                            .succ_contains = 0,
                            .all_delete = 0,
                            .all_contains = 0,
                            .succ_operations = 0};

    double tic = omp_get_wtime();
    int i = 0;
    while (duration > (omp_get_wtime() - tic)) {
        int key = generate_key(start_range, end_range, strat, seed + i, arr);
        random_r(&rand_state, &choice);

        if (choice % 100 < operations[0]) {
            if (insert(sl, key, seed)) {
                data.succ_insert++;
                data.succ_operations++;
            }
            data.all_insert++;
            data.all_operations++;
        } else if (choice % 100 < operations[0] + operations[1] && choice % 100 >= operations[0]) {
            if (erase(sl, key)) {
                data.succ_delete++;
                data.succ_operations++;
            }
            data.all_operations++;
            data.all_delete++;
        } else {
            if (contains(sl, key)) {
                data.succ_contains++;
                data.succ_operations++;
            }
            data.all_operations++;
            data.all_contains++;
        }
        i++;
    }
    return data;
}

struct bench_result small_bench(int threads, int prefill, double duration, int seed, int *operations, int start_range, int end_range, enum key_range_type krt, enum strategy strat) {
    struct bench_result result;
    result.reduced_counters = (struct counters) {
        .all_operations = 0,
        .succ_insert = 0,
        .succ_delete = 0,
        .succ_contains = 0,
        .all_delete = 0,
        .all_contains = 0,
        .succ_operations = 0
    };

    struct counters thread_data[threads];
    double tic, toc;
    SkipList *sl = create_skiplist();

    int **ranges = malloc(threads * sizeof(int *));
    for (int i = 0; i < threads; i++) {
        ranges[i] = malloc(2 * sizeof(int));
    }
    divide_range(threads, ranges, start_range, end_range, krt);

    int **random_arr = malloc(threads * sizeof(int *));
    for (int i = 0; i < threads; i++) {
        int range_size = ranges[i][1] - ranges[i][0] + 1;
        random_arr[i] = malloc(range_size * sizeof(int));
        fillAndShuffleRow(random_arr[i], ranges[i][0], ranges[i][1], range_size);
    }

    for (int i = 0; i < prefill; i++) {
        int key = generate_key(start_range, end_range, RANDOM, seed + i, NULL);
        insert(sl, key, seed);
    }

    omp_set_num_threads(threads);
    tic = omp_get_wtime();
    {
        #pragma omp parallel for
        for (int i = 0; i < threads; i++) {
            thread_data[i] = random_bench1(sl, seed, duration, operations, ranges[i][0], ranges[i][1], random_arr[i], strat);
        }
    }
    toc = omp_get_wtime();

    for (int i = 0; i < threads; i++) {
        result.reduced_counters.all_operations += thread_data[i].all_operations;
        result.reduced_counters.succ_insert += thread_data[i].succ_insert;
        result.reduced_counters.succ_delete += thread_data[i].succ_delete;
        result.reduced_counters.succ_contains += thread_data[i].succ_contains;
        result.reduced_counters.all_insert += thread_data[i].all_insert;
        result.reduced_counters.all_delete += thread_data[i].all_delete;
        result.reduced_counters.all_contains += thread_data[i].all_contains;
        result.reduced_counters.succ_operations += thread_data[i].succ_operations;
    }

    for (int i = 0; i < threads; i++) {
        free(random_arr[i]);
        free(ranges[i]);
    }
    free(random_arr);
    free(ranges);

    result.time = toc - tic;
    printf("Benchmark completed in %.2f seconds. Total operations: %d\n",
           result.time, result.reduced_counters.all_operations);

    print_results(result.reduced_counters);

    // Perform validation
    bench_validate_skiplist(sl, prefill, result.reduced_counters.succ_insert, result.reduced_counters.succ_delete);

    printf("Summary: Threads=%d, Duration=%.2f, Total_Operations=%d, Successful_Operations=%d, "
       "Insert_Success=%.6f, Delete_Success=%.6f, Contains_Success=%.6f, Throughput=%.6f\n",
       threads, result.time, 
       result.reduced_counters.all_operations, result.reduced_counters.succ_operations,
       (result.reduced_counters.all_insert > 0 ? 
           (double)result.reduced_counters.succ_insert / result.reduced_counters.all_insert : 0.0),
       (result.reduced_counters.all_delete > 0 ? 
           (double)result.reduced_counters.succ_delete / result.reduced_counters.all_delete : 0.0),
       (result.reduced_counters.all_contains > 0 ? 
           (double)result.reduced_counters.succ_contains / result.reduced_counters.all_contains : 0.0),
       (result.reduced_counters.all_operations / result.time));

    return result;
}

int main(int argc, char *argv[]) {
    int threads = 1, experiments = 3, prefill = 0, operations[3] = {10, 10, 80};
    int seed = 12345, key_range[2] = {0, 5000};
    double duration = 1.0;
    enum strategy strat = RANDOM;
    enum key_range_type range_type = COMMON;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) threads = atoi(argv[++i]);
        else if (strcmp(argv[i], "--experiments") == 0 && i + 1 < argc) experiments = atoi(argv[++i]);
        else if (strcmp(argv[i], "--duration") == 0 && i + 1 < argc) duration = atof(argv[++i]);
        else if (strcmp(argv[i], "--operations") == 0 && i + 3 < argc) {
            operations[0] = atoi(argv[++i]);
            operations[1] = atoi(argv[++i]);
            operations[2] = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--key_range") == 0 && i + 2 < argc) {
            key_range[0] = atoi(argv[++i]);
            key_range[1] = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--strategy") == 0 && i + 1 < argc) {
            if (strcmp(argv[++i], "RANDOM") == 0) strat = RANDOM;
            else if (strcmp(argv[i], "UNIQUE") == 0) strat = UNIQUE;
            else if (strcmp(argv[i], "DETERMINISTIC") == 0) strat = DETERMINISTIC;
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) seed = atoi(argv[++i]);
        else if (strcmp(argv[i], "--prefill") == 0 && i + 1 < argc) prefill = atoi(argv[++i]);
        else if (strcmp(argv[i], "--range_type") == 0 && i + 1 < argc) {
            if (strcmp(argv[++i], "DISJOINT") == 0) range_type = DISJOINT;
            else if (strcmp(argv[i], "PER_THREAD") == 0) range_type = PER_THREAD;
            else if (strcmp(argv[i], "COMMON") == 0) range_type = COMMON;
        } else {
            printf("Unknown parameter: %s\n", argv[i]);
            return 1;
        }
    }

    printf("Running with the following parameters:\n");
    printf("  Threads: %d\n  Experiments: %d\n  Duration: %.2f\n", threads, experiments, duration);
    printf("  Operations: %d%% insert, %d%% delete, %d%% contains\n", operations[0], operations[1], operations[2]);
    printf("  Key Range: [%d, %d]\n  Strategy: %d\n  Prefill: %d\n  Range Type: %d\n",
           key_range[0], key_range[1], strat, prefill, range_type);

    struct bench_result results[experiments];
    for (int i = 0; i < experiments; i++) {
        printf("\nRunning experiment %d/%d...\n", i + 1, experiments);
        results[i] = small_bench(threads, prefill, duration, seed, operations, key_range[0], key_range[1], range_type, strat);
    }

    return 0;
}
