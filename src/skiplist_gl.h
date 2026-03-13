#ifndef SKIPLIST_GL_H
#define SKIPLIST_GL_H

#include <stdbool.h>
#include <pthread.h>

#define SKIPLIST_MAX_LEVEL 16
#define SKIPLIST_P 0.5

typedef struct SkipListNode {
    int key;
    void *value;
    struct SkipListNode **forward; // Array of pointers for each level
} SkipListNode;

// Skip list structure
typedef struct SkipList {
    int level; // Current maximum level of the skip list
    SkipListNode *header; // Pointer to the header node
    pthread_mutex_t lock; // Mutex for global locking
} SkipList;

SkipList *skiplist_create(void);
void skiplist_destroy(SkipList *list);
bool skiplist_insert(SkipList *list, int key, void *value);
bool skiplist_erase(SkipList *list, int key);
void *skiplist_search(SkipList *list, int key);

#endif // SKIPLIST_GL_H