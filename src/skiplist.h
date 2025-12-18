#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <stdbool.h>

#define MAX_LEVEL 16      // Maximum levels in the skip list
#define PROBABILITY 0.5   // Probability of level promotion

// Node structure
typedef struct SkipListNode {
    int key;
    struct SkipListNode **forward; // Forward pointers for each level
} SkipListNode;

// Skip list structure
typedef struct SkipList {
    int level;            // Current maximum level of the skip list
    int element_count;    // Number of elements in the skip list
    SkipListNode *header; // Pointer to the header node
} SkipList;

// Function prototypes
SkipList *create_skiplist();
void free_skiplist(SkipList *list);
SkipListNode *create_node(int level, int key);
int random_level(int seed);
bool insert(SkipList *list, int key, int seed);
bool erase(SkipList *list, int key);
bool contains(SkipList *list, int key);
int get_element_count(SkipList *list); // Function to get the element count

#endif // SKIPLIST_H
