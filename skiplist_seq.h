#ifndef SKIPLIST_SEQ_H
#define SKIPLIST_SEQ_H

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <search.h>
#include <time.h>
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
} SkipList;

// Thread-safe random state
static struct random_data rand_state;
static char statebuf[64];
static int rand_initialized = 0;

static void skiplist_init_random(void) {
    if (!rand_initialized) {
        memset(&rand_state, 0, sizeof(rand_state));
        memset(statebuf, 0, sizeof(statebuf));
        initstate_r((unsigned int)time(NULL), statebuf, sizeof(statebuf), &rand_state);
        rand_initialized = 1;
    }
}

static int skiplist_random_level(void) {
    skiplist_init_random();
    int32_t r;
    random_r(&rand_state, &r);
    int level = 0;
    while (((double)(r & INT_MAX) / INT_MAX) < SKIPLIST_P && level < SKIPLIST_MAX_LEVEL - 1) {
        level++;
        random_r(&rand_state, &r);
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
    SkipList* sl = (SkipList*)malloc(sizeof(SkipList));
    if (!sl) { fprintf(stderr, "Memory allocation failed for skiplist\n"); exit(EXIT_FAILURE); }
    sl->level = 0;
    sl->header = skiplist_create_node(SKIPLIST_MAX_LEVEL - 1, INT_MIN, NULL);
    return sl;
}

static void *skiplist_search(SkipList *list, int key) {
    SkipListNode *current = list->header;
    for (int i = list->level; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->key < key)
            current = current->forward[i];
    }
    current = current->forward[0];
    if (current && current->key == key) return current->value;
    return NULL;
}

static bool skiplist_insert(SkipList *list, int key, void *value) {
    SkipListNode *update[SKIPLIST_MAX_LEVEL];
    SkipListNode *current = list->header;
    for (int i = list->level; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->key < key)
            current = current->forward[i];
        update[i] = current;
    }
    current = current->forward[0];
    if (current && current->key == key) return false;

    int new_level = skiplist_random_level();
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
    return true;
}

static bool skiplist_erase(SkipList *list, int key) {
    SkipListNode *update[SKIPLIST_MAX_LEVEL];
    SkipListNode *current = list->header;
    for (int i = list->level; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->key < key)
            current = current->forward[i];
        update[i] = current;
    }
    current = current->forward[0];
    if (!current || current->key != key) return false;

    for (int i = 0; i <= list->level; i++) {
        if (update[i]->forward[i] != current) break;
        update[i]->forward[i] = current->forward[i];
    }
    free(current->forward);
    free(current);

    while (list->level > 0 && list->header->forward[list->level] == NULL)
        list->level--;

    return true;
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

#endif // SKIPLIST_SEQ_H