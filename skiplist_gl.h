#ifndef SKIPLIST_GL_H
#define SKIPLIST_GL_H

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <search.h>
#include <time.h>
#include <omp.h>
#include <stdio.h>

#define SKIPLIST_MAX_LEVEL 16
#define SKIPLIST_P 0.5

typedef struct SkipListNode {
    int key;
    void *value;
    struct SkipListNode **forward;
} SkipListNode;

typedef struct SkipList {
    int level;
    SkipListNode *header;
    pthread_mutex_t lock;
} SkipList;

#define _GL_MAX_THREADS 64
static struct random_data _gl_rand_states[_GL_MAX_THREADS];
static char               _gl_statebufs[_GL_MAX_THREADS][64];
static int                _gl_rng_initialized[_GL_MAX_THREADS];

static int _gl_random_level(void) {
    int tid = omp_get_thread_num();
    if (tid >= _GL_MAX_THREADS) tid = 0;
    if (!_gl_rng_initialized[tid]) {
        memset(&_gl_rand_states[tid], 0, sizeof(_gl_rand_states[tid]));
        memset(_gl_statebufs[tid],    0, sizeof(_gl_statebufs[tid]));
        initstate_r((unsigned int)(time(NULL) ^ ((unsigned)tid * 2654435761u)),
                    _gl_statebufs[tid], sizeof(_gl_statebufs[tid]),
                    &_gl_rand_states[tid]);
        _gl_rng_initialized[tid] = 1;
    }
    int32_t r;
    random_r(&_gl_rand_states[tid], &r);
    int level = 0;
    while (((double)(r & INT_MAX) / INT_MAX) < SKIPLIST_P &&
           level < SKIPLIST_MAX_LEVEL - 1) {
        level++;
        random_r(&_gl_rand_states[tid], &r);
    }
    return level;
}

static SkipListNode* skiplist_create_node(int level, int key, void *value) {
    SkipListNode* node = malloc(sizeof(SkipListNode));
    if (!node) { fprintf(stderr, "Memory allocation failed\n"); exit(EXIT_FAILURE); }
    node->key = key;
    node->value = value;
    node->forward = calloc(level + 1, sizeof(SkipListNode*));
    if (!node->forward) { free(node); fprintf(stderr, "Memory allocation failed\n"); exit(EXIT_FAILURE); }
    return node;
}

static SkipList* skiplist_create(void) {
    SkipList* list = (SkipList*)malloc(sizeof(SkipList));
    if (!list) { fprintf(stderr, "Memory allocation failed for skiplist\n"); exit(EXIT_FAILURE); }
    list->level = 0;
    list->header = skiplist_create_node(SKIPLIST_MAX_LEVEL - 1, INT_MIN, NULL);
    return list;
}

static void *skiplist_search(SkipList *list, int key) {
    SkipListNode *current = list->header;
    void *result = NULL;

    #pragma omp critical
    {
        for (int i = list->level; i >= 0; i--) {
            while (current->forward[i] && current->forward[i]->key < key)
                current = current->forward[i];
        }
        current = current->forward[0];
        if (current && current->key == key)
            result = current->value;
    }

    return result;
}

static bool skiplist_insert(SkipList *list, int key, void *value) {
    SkipListNode *update[SKIPLIST_MAX_LEVEL];
    SkipListNode *current = list->header;
    bool inserted = true;

    #pragma omp critical
    {
        for (int i = list->level; i >= 0; i--) {
            while (current->forward[i] && current->forward[i]->key < key)
                current = current->forward[i];
            update[i] = current;
        }
        current = current->forward[0];

        if (current != NULL && current->key == key) {
            current->value = value;
            inserted = false;
        } else {
            int new_level = _gl_random_level();
            if (new_level > list->level) {
                for (int i = list->level + 1; i <= new_level; i++)
                    update[i] = list->header;
                list->level = new_level;
            }

            SkipListNode *node = skiplist_create_node(new_level, key, value);
            for (int i = 0; i <= new_level; i++) {
                node->forward[i] = update[i]->forward[i];
                update[i]->forward[i] = node;
            }
        }
    }

    return inserted;
}

static bool skiplist_erase(SkipList *list, int key) {
    SkipListNode *update[SKIPLIST_MAX_LEVEL];
    SkipListNode *current = list->header;
    bool found = false;

    #pragma omp critical
    {
        for (int i = list->level; i >= 0; i--) {
            while (current->forward[i] && current->forward[i]->key < key)
                current = current->forward[i];
            update[i] = current;
        }
        current = current->forward[0];

        if (current != NULL && current->key == key) {
            for (int i = 0; i <= list->level; i++) {
                if (update[i]->forward[i] != current) break;
                update[i]->forward[i] = current->forward[i];
            }
            free(current->forward);
            free(current);

            while (list->level > 0 && list->header->forward[list->level] == NULL)
                list->level--;

            found = true;
        }
    }

    return found;
}

static void skiplist_destroy(SkipList *list) {
    SkipListNode *current = list->header;
    while (current) {
        SkipListNode *next = current->forward[0];
        free(current->forward);
        free(current);
        current = next;
    }
    free(list);
}

#endif // SKIPLIST_GL_H