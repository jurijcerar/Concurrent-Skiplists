#include "skiplist.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

// Create a new node
SkipListNode *create_node(int level, int key) {
    SkipListNode *node = (SkipListNode *)malloc(sizeof(SkipListNode));
    node->key = key;
    node->forward = (SkipListNode **)malloc((level + 1) * sizeof(SkipListNode *));
    for (int i = 0; i <= level; i++) {
        node->forward[i] = NULL;
    }
    return node;
}

// Create an empty skip list
SkipList *create_skiplist() {
    SkipList *list = (SkipList *)malloc(sizeof(SkipList));
    list->level = 0;
    list->element_count = 0; // Initialize element count
    list->header = create_node(MAX_LEVEL, INT_MIN); // Header with INT_MIN key
    return list;
}

// Generate a random level for a new node
int random_level(int seed) {
    struct random_data rand_state;
    int choice;
    char statebuf[32];
    bzero(&rand_state, sizeof(struct random_data));
    bzero(&statebuf, sizeof(statebuf));
    initstate_r(seed, statebuf, 32, &rand_state);
    random_r(&rand_state, &choice);

    int level = 0;
    while (((double)choice / RAND_MAX) < PROBABILITY && level < MAX_LEVEL) {
        level++;
    }
    return level;
}

// Insert a key into the skip list
bool insert(SkipList *list, int key, int seed) {
    SkipListNode *update[MAX_LEVEL + 1];
    SkipListNode *current = list->header;

    // Locate the position for the new key
    for (int i = list->level; i >= 0; i--) {
        while (current->forward[i] != NULL && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];

    // Check if the key already exists
    if (current != NULL && current->key == key) {
        return false; // Key already exists
    }

    // Generate a random level for the new node
    int new_level = random_level(seed);
    if (new_level > list->level) {
        for (int i = list->level + 1; i <= new_level; i++) {
            update[i] = list->header;
        }
        list->level = new_level;
    }

    // Create a new node
    SkipListNode *new_node = create_node(new_level, key);
    for (int i = 0; i <= new_level; i++) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }

    // Update element count
    list->element_count++;
    return true; // Insertion successful
}

// Delete a key from the skip list
bool erase(SkipList *list, int key) {
    SkipListNode *update[MAX_LEVEL + 1];
    SkipListNode *current = list->header;

    // Locate the node to delete
    for (int i = list->level; i >= 0; i--) {
        while (current->forward[i] != NULL && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];

    // Key found, proceed to delete
    if (current != NULL && current->key == key) {
        for (int i = 0; i <= list->level; i++) {
            if (update[i]->forward[i] != current) {
                break;
            }
            update[i]->forward[i] = current->forward[i];
        }

        free(current->forward);
        free(current);

        // Adjust list level if necessary
        while (list->level > 0 && list->header->forward[list->level] == NULL) {
            list->level--;
        }

        // Update element count
        list->element_count--;
        return true; // Deletion successful
    }

    return false; // Key not found
}

// Check if a key exists in the skip list
bool contains(SkipList *list, int key) {
    SkipListNode *current = list->header;

    for (int i = list->level; i >= 0; i--) {
        while (current->forward[i] != NULL && current->forward[i]->key < key) {
            current = current->forward[i];
        }
    }

    current = current->forward[0];
    return current != NULL && current->key == key;
}

// Get the element count of the skip list
int get_element_count(SkipList *list) {
    return list->element_count;
}

// Free the entire skip list
void free_skiplist(SkipList *list) {
    SkipListNode *current = list->header;
    while (current != NULL) {
        SkipListNode *next = current->forward[0];
        free(current->forward);
        free(current);
        current = next;
    }
    free(list);
}
