#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <stdbool.h>

#define MAX_LEVEL 16 // Maximum levels in the skip list
#define PROBABILITY 0.5 // Probability of level promotion

// Node structure
typedef struct SkipListNode {
    int key;
    int value;
    struct SkipListNode **forward; // Forward pointers for each level
} SkipListNode;

// Skip list structure
typedef struct SkipList {
    int level; // Current maximum level of the skip list
    SkipListNode *header; // Pointer to the header node
    int prefilled_count; // Count of prefilled keys
    int inserted_count; // Count of inserted keys
    int deleted_count; // Count of deleted keys
} SkipList;

// Function prototypes
SkipList *create_skiplist();
void free_skiplist(SkipList *list);
SkipListNode *create_node(int level, int key, int value);
int random_level();
void insert(SkipList *list, int key, int value);
bool erase(SkipList *list, int key);
bool contains(SkipList *list, int key);
void prefill(SkipList *list, int count);
bool validate(SkipList *list);

#endif // SKIPLIST_H
