#ifndef SKIPLIST_FREE_H
#define SKIPLIST_FREE_H

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
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
    atomic_uintptr_t forward[SKIPLIST_MAX_LEVEL + 1]; /* mark bit packed into LSB */
    int topLevel;
} SkipListNode;

typedef struct SkipList {
    SkipListNode *head;
    SkipListNode *tail;
} SkipList;

#define _FREE_MAX_THREADS 64
static struct random_data _free_rand_states[_FREE_MAX_THREADS];
static char               _free_statebufs[_FREE_MAX_THREADS][64];
static int                _free_rng_initialized[_FREE_MAX_THREADS];

static void _free_init_random_thread(int tid) {
    if (!_free_rng_initialized[tid]) {
        memset(&_free_rand_states[tid], 0, sizeof(_free_rand_states[tid]));
        memset(_free_statebufs[tid],    0, sizeof(_free_statebufs[tid]));
        initstate_r((unsigned int)(time(NULL) ^ ((unsigned)tid * 2654435761u)),
                    _free_statebufs[tid], sizeof(_free_statebufs[tid]),
                    &_free_rand_states[tid]);
        _free_rng_initialized[tid] = 1;
    }
}

static int _free_random_level(void) {
    int tid = omp_get_thread_num();
    if (tid >= _FREE_MAX_THREADS) tid = 0;
    _free_init_random_thread(tid);
    int32_t r;
    random_r(&_free_rand_states[tid], &r);
    int level = 0;
    while (((double)(r & INT_MAX) / INT_MAX) < SKIPLIST_P &&
           level < SKIPLIST_MAX_LEVEL - 1) {
        level++;
        random_r(&_free_rand_states[tid], &r);
    }
    return level;
}

static inline bool _free_get_mark(uintptr_t p) {
    return p & 1;
}
static inline SkipListNode *_free_get_ptr(uintptr_t p) {
    return (SkipListNode *)(p & ~(uintptr_t)1);
}
static inline uintptr_t _free_make(SkipListNode *ref, bool mark) {
    return (uintptr_t)ref | (uintptr_t)mark;
}
static inline SkipListNode *_free_get(atomic_uintptr_t *slot, bool *marked) {
    uintptr_t p = atomic_load_explicit(slot, memory_order_acquire);
    *marked = _free_get_mark(p);
    return _free_get_ptr(p);
}
static inline SkipListNode *_free_get_ref(atomic_uintptr_t *slot) {
    return _free_get_ptr(atomic_load_explicit(slot, memory_order_acquire));
}
static inline void _free_set(atomic_uintptr_t *slot, SkipListNode *ref, bool mark) {
    atomic_store_explicit(slot, _free_make(ref, mark), memory_order_release);
}
static inline bool _free_attempt_mark(atomic_uintptr_t *slot,
                                      SkipListNode *expectedRef, bool newMark) {
    uintptr_t old = _free_make(expectedRef, false); /* only attempt if unmarked */
    uintptr_t newVal = _free_make(expectedRef, newMark);
    return atomic_compare_exchange_strong_explicit(slot, &old, newVal,
                                                   memory_order_acq_rel,
                                                   memory_order_acquire);
}
static inline bool _free_cas(atomic_uintptr_t *slot,
                              SkipListNode *expRef, bool expMark,
                              SkipListNode *newRef, bool newMark) {
    uintptr_t expected = _free_make(expRef, expMark);
    uintptr_t desired  = _free_make(newRef, newMark);
    return atomic_compare_exchange_strong_explicit(slot, &expected, desired,
                                                   memory_order_acq_rel,
                                                   memory_order_acquire);
}


static SkipListNode *_free_create_node(int key, void *value,
                                       int topLevel, SkipListNode *tail) {
    SkipListNode *node = (SkipListNode *)malloc(sizeof(SkipListNode));
    if (!node) { fprintf(stderr, "Memory allocation failed\n"); exit(EXIT_FAILURE); }
    node->key      = key;
    node->value    = value;
    node->topLevel = topLevel;
    for (int i = 0; i <= SKIPLIST_MAX_LEVEL; i++)
        atomic_init(&node->forward[i], _free_make(tail, false));
    return node;
}

static bool _free_find(SkipList *list, int key,
                       SkipListNode **preds, SkipListNode **succs) {
    SkipListNode *pred, *curr, *succ;
    bool marked = false;

retry:
    pred = list->head;
    for (int level = SKIPLIST_MAX_LEVEL; level >= 0; level--) {
        curr = _free_get_ref(&pred->forward[level]);
        while (true) {
            succ = _free_get(&curr->forward[level], &marked);
            while (marked) {
                if (!_free_cas(&pred->forward[level], curr, false, succ, false))
                    goto retry;
                curr = _free_get_ref(&pred->forward[level]);
                succ = _free_get(&curr->forward[level], &marked);
            }
            if (curr->key < key) {
                pred = curr;
                curr = succ;
            } else {
                break;
            }
        }
        preds[level] = pred;
        succs[level] = curr;
    }
    return (curr->key == key);
}


static SkipList *skiplist_create(void) {
    SkipList *list = (SkipList *)malloc(sizeof(SkipList));
    if (!list) { fprintf(stderr, "Memory allocation failed\n"); exit(EXIT_FAILURE); }
    list->head = _free_create_node(INT_MIN, NULL, SKIPLIST_MAX_LEVEL, NULL);
    list->tail = _free_create_node(INT_MAX, NULL, SKIPLIST_MAX_LEVEL, NULL);
    for (int i = 0; i <= SKIPLIST_MAX_LEVEL; i++)
        _free_set(&list->head->forward[i], list->tail, false);
    return list;
}

static void skiplist_destroy(SkipList *list) {
    SkipListNode *curr = _free_get_ref(&list->head->forward[0]);
    free(list->head);
    while (curr != list->tail) {
        SkipListNode *next = _free_get_ref(&curr->forward[0]);
        free(curr);
        curr = next;
    }
    free(list->tail);
    free(list);
}

static void *skiplist_search(SkipList *list, int key) {
    SkipListNode *pred = list->head;
    bool marked = false;
    SkipListNode *curr = NULL;
    SkipListNode *succ = NULL;
    for (int level = SKIPLIST_MAX_LEVEL; level >= 0; level--) {
        curr = _free_get_ref(&pred->forward[level]);
        while (true) {
            succ = _free_get(&curr->forward[level], &marked);
            while (marked) {
                curr = _free_get_ref(&curr->forward[level]);
                succ = _free_get(&curr->forward[level], &marked);
            }
            if (curr->key < key) {
                pred = curr;
                curr = succ;
            } else {
                break;
            }
        }
        if (level == 0)
            return (curr->key == key) ? curr->value : NULL;
    }
    return NULL;    
}

static bool skiplist_insert(SkipList *list, int key, void *value) {
    int topLevel = _free_random_level();
    SkipListNode *preds[SKIPLIST_MAX_LEVEL + 1];
    SkipListNode *succs[SKIPLIST_MAX_LEVEL + 1];

    while (true) {
        if (_free_find(list, key, preds, succs))
            return false;

        SkipListNode *newNode = _free_create_node(key, value, topLevel, list->tail);

        for (int level = 0; level <= topLevel; level++){
            SkipListNode *succ = succs[level];
            _free_set(&newNode->forward[level], succ, false);
        }

        SkipListNode *succ = succs[0];
        SkipListNode *pred = preds[0];
        if (!_free_cas(&pred->forward[0], succ, false, newNode, false)) {
            //free(newNode);
            continue;
        }

        /* Link remaining levels */
        for (int level = 1; level <= topLevel; level++) {
            while (true) {
                SkipListNode *succ = succs[level];
                SkipListNode *pred = preds[level];
                if (_free_cas(&pred->forward[level],
                              succ, false, newNode, false))
                    break;
                _free_find(list, key, preds, succs);
            }
        }
        return true;
    }
}

static bool skiplist_erase(SkipList *list, int key) {
    SkipListNode *preds[SKIPLIST_MAX_LEVEL + 1];
    SkipListNode *succs[SKIPLIST_MAX_LEVEL + 1];

    while (true) {
        if (!_free_find(list, key, preds, succs))
            return false;

        SkipListNode *nodeToRemove = succs[0];

        /* Mark all levels top-down except level 0 */
        for (int level = nodeToRemove->topLevel; level >= 1; level--) {
            bool marked=false;
            SkipListNode *succ = _free_get(&nodeToRemove->forward[level], &marked);
            while (!marked) {
                //_free_attempt_mark(&nodeToRemove->forward[level], succ, true);
                _free_cas(&nodeToRemove->forward[level],
                                   succ, false, succ, true);
                succ = _free_get(&nodeToRemove->forward[level], &marked);
            }
        }

        /* Mark level 0 — linearization point for remove */
        bool marked = false;
        SkipListNode *succ = _free_get(&nodeToRemove->forward[0], &marked);
        while (true) {
            bool iMark = _free_cas(&nodeToRemove->forward[0],
                                   succ, false, succ, true);
            /* Re-read from nodeToRemove (not succs[0] which may be freed) */
            succ = _free_get(&succs[0]->forward[0], &marked);
            if (iMark) {
                _free_find(list, key, preds, succs); /* physically unlink */
                return true;
            } else if (marked) {
                return false; /* another thread won */
            }
        }
    }
}

#endif /* SKIPLIST_FREE_H */