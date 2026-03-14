#ifndef SKIPLIST_LAZY_H
#define SKIPLIST_LAZY_H

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <search.h>
#include <time.h>
#include <stdio.h>
#include <omp.h>

#define SKIPLIST_MAX_LEVEL 16
#define SKIPLIST_P 0.5

typedef struct SkipListNode {
    int key;
    void *value;
    struct SkipListNode **forward;
    volatile bool marked;
    volatile bool fullyLinked;
    int topLevel;
    omp_lock_t lock;
} SkipListNode;

typedef struct SkipList {
    SkipListNode *head;
    SkipListNode *tail;
} SkipList;

#define _LAZY_MAX_THREADS 64
static struct random_data _lazy_rand_states[_LAZY_MAX_THREADS];
static char               _lazy_statebufs[_LAZY_MAX_THREADS][64];
static int                _lazy_rng_initialized[_LAZY_MAX_THREADS];

static int _lazy_random_level(void) {
    int tid = omp_get_thread_num();
    if (tid >= _LAZY_MAX_THREADS) tid = 0;
    if (!_lazy_rng_initialized[tid]) {
        memset(&_lazy_rand_states[tid], 0, sizeof(_lazy_rand_states[tid]));
        memset(_lazy_statebufs[tid],    0, sizeof(_lazy_statebufs[tid]));
        initstate_r((unsigned int)(time(NULL) ^ ((unsigned)tid * 2654435761u)),
                    _lazy_statebufs[tid], sizeof(_lazy_statebufs[tid]),
                    &_lazy_rand_states[tid]);
        _lazy_rng_initialized[tid] = 1;
    }
    int32_t r;
    random_r(&_lazy_rand_states[tid], &r);
    int level = 0;
    while (((double)(r & INT_MAX) / INT_MAX) < SKIPLIST_P &&
           level < SKIPLIST_MAX_LEVEL - 1) {
        level++;
        random_r(&_lazy_rand_states[tid], &r);
    }
    return level;
}


static SkipListNode *_lazy_create_node(int key, void *value, int levels) {
    SkipListNode *node = (SkipListNode *)malloc(sizeof(SkipListNode));
    if (!node) { fprintf(stderr, "Memory allocation failed\n"); exit(EXIT_FAILURE); }
    node->forward = (SkipListNode **)malloc((levels + 1) * sizeof(SkipListNode *));
    if (!node->forward) { free(node); fprintf(stderr, "Memory allocation failed\n"); exit(EXIT_FAILURE); }
    node->key = key;
    node->value = value;
    node->marked = false;
    node->fullyLinked = false;
    node->topLevel = levels;
    omp_init_lock(&node->lock);
    return node;
}

static void _lazy_destroy_node(SkipListNode *node) {
    omp_destroy_lock(&node->lock);
    free(node->forward);
    free(node);
}

/* Used by both insert and erase to release predecessor locks */
static void _lazy_unlock_levels(SkipListNode **preds, int highLock) {
    SkipListNode *helper = NULL;
    for (int l = 0; l <= highLock; ++l) {
        if (preds[l] != helper)
            omp_unset_lock(&preds[l]->lock);
        helper = preds[l];
    }
}

static int _lazy_find(SkipList *list, int key,
                      SkipListNode **preds, SkipListNode **succs) {
    int foundLevel = -1;
    SkipListNode *pred = list->head;
    for (int l = SKIPLIST_MAX_LEVEL; l >= 0; --l) {
        SkipListNode *curr = pred->forward[l];
        while (curr->key < key) {
            pred = curr;
            curr = pred->forward[l];
        }
        if (foundLevel == -1 && curr->key == key)
            foundLevel = l;
        preds[l] = pred;
        succs[l] = curr;
    }
    return foundLevel;
}


static SkipList *skiplist_create(void) {
    SkipList *list = (SkipList *)malloc(sizeof(SkipList));
    if (!list) { fprintf(stderr, "Memory allocation failed\n"); exit(EXIT_FAILURE); }
    list->head = _lazy_create_node(INT_MIN, NULL, SKIPLIST_MAX_LEVEL);
    list->tail = _lazy_create_node(INT_MAX, NULL, SKIPLIST_MAX_LEVEL);
    list->head->fullyLinked = true;
    list->tail->fullyLinked = true;
    /* zero tail's forward pointers so destroy can safely detect the end */
    for (int i = 0; i <= SKIPLIST_MAX_LEVEL; ++i)
        list->tail->forward[i] = NULL;
    for (int i = 0; i <= SKIPLIST_MAX_LEVEL; ++i)
        list->head->forward[i] = list->tail;
    return list;
}

static void skiplist_destroy(SkipList *list) {
    SkipListNode *curr = list->head;
    while (curr != NULL) {
        SkipListNode *next = (curr == list->tail) ? NULL : curr->forward[0];
        _lazy_destroy_node(curr);
        curr = next;
    }
    free(list);
}

static void *skiplist_search(SkipList *list, int key) {
    SkipListNode *preds[SKIPLIST_MAX_LEVEL + 1];
    SkipListNode *succs[SKIPLIST_MAX_LEVEL + 1];
    int foundLevel = _lazy_find(list, key, preds, succs);
    if (foundLevel >= 0 && succs[foundLevel]->fullyLinked && !succs[foundLevel]->marked)
        return succs[foundLevel]->value;
    return NULL;
}

static bool skiplist_insert(SkipList *list, int key, void *value) {
    int topLevel = _lazy_random_level();
    SkipListNode *preds[SKIPLIST_MAX_LEVEL + 1];
    SkipListNode *succs[SKIPLIST_MAX_LEVEL + 1];

    while (true) {
        int foundLevel = _lazy_find(list, key, preds, succs);
        if (foundLevel >= 0) {
            SkipListNode *found = succs[foundLevel];
            if (!found->marked) {
                while (!found->fullyLinked) {}
                return false; /* already exists */
            }
            continue;
        }

        int highLock = -1;
        SkipListNode *helper = NULL;
        bool valid = true;

        for (int l = 0; valid && (l <= topLevel); ++l) {
            SkipListNode *pred = preds[l];
            SkipListNode *succ = succs[l];
            if (pred != helper) {
                omp_set_lock(&pred->lock);
                highLock = l;
                helper = pred;
            }
            valid = !pred->marked && !succ->marked && pred->forward[l] == succ;
        }

        if (!valid) {
            _lazy_unlock_levels(preds, highLock);
            continue;
        }

        SkipListNode *newNode = _lazy_create_node(key, value, topLevel);
        for (int l = 0; l <= topLevel; ++l) {
            newNode->forward[l] = succs[l];
            preds[l]->forward[l] = newNode;
        }
        newNode->fullyLinked = true;
        _lazy_unlock_levels(preds, highLock);
        return true;
    }
}

static bool skiplist_erase(SkipList *list, int key) {
    SkipListNode *victim = NULL;
    bool marked = false;
    int topLevel = -1;
    SkipListNode *preds[SKIPLIST_MAX_LEVEL + 1];
    SkipListNode *succs[SKIPLIST_MAX_LEVEL + 1];

    while (true) {
        int foundLevel = _lazy_find(list, key, preds, succs);
        if (marked ||
            (foundLevel >= 0 &&
             succs[foundLevel]->fullyLinked &&
             !succs[foundLevel]->marked)) {

            if (!marked) {
                victim = succs[foundLevel];
                topLevel = victim->topLevel;
                omp_set_lock(&victim->lock);
                if (victim->marked) {
                    omp_unset_lock(&victim->lock);
                    return false;
                }
                victim->marked = true;
                marked = true;
            }

            int highLock = -1;
            SkipListNode *helper = NULL;
            bool valid = true;

            for (int l = 0; valid && (l <= topLevel); ++l) {
                SkipListNode *pred = preds[l];
                SkipListNode *succ = succs[l];
                if (pred != helper) {
                    omp_set_lock(&pred->lock);
                    highLock = l;
                    helper = pred;
                }
                valid = !pred->marked && pred->forward[l] == succ;
            }

            if (!valid) {
                _lazy_unlock_levels(preds, highLock);
                continue;
            }

            for (int l = topLevel; l >= 0; --l)
                preds[l]->forward[l] = victim->forward[l];

            omp_unset_lock(&victim->lock);
            _lazy_unlock_levels(preds, highLock);
            return true;
        } else {
            return false;
        }
    }
}

#endif /* SKIPLIST_LAZY_H */