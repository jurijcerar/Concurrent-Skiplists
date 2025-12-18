#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <strings.h>
#include <omp.h>

#define MAX_LEVEL 16

// Node struct
typedef struct Node {
    int key;                  // Key stored in the node
    struct Node** next;        // Forward pointers for each level
    volatile bool marked;               // Marked for logical deletion
    volatile bool fullyLinked;          // Node is fully linked
    int topLevel;                     // Level of the node
    omp_lock_t lock;           // OpenMP lock for fine-grained locking
} Node;

// SkipList struct
typedef struct SkipList {
    Node* head;
    Node* tail;
    int inserted_count; // Number of successful insertions
    int deleted_count;  // Number of successful deletions
} SkipList;

// Function prototypes
Node* createNode(int key, int levels);
void destroyNode(Node* node);
SkipList* create_skiplist(void);
void destroySkipList(SkipList* list);
int random_level(int seed);
int find(SkipList* list, int key, Node** preds, Node** succs);
bool contains(SkipList* list, int key);
bool insert(SkipList* list, int key, int seed);
bool erase(SkipList* list, int key);
void unlockLevels(Node** preds, int highLock);

// Validation functions
int get_element_count(SkipList* list);
bool validate_skiplist(SkipList* list);

#endif // SKIPLIST_H
