#include SKIPLIST_HEADER
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

// Static random state so level generation is actually random across calls
static struct random_data rand_state;
static char statebuf[32];
static int rand_initialized = 0;

static void init_rand(int seed) {
    if (!rand_initialized) {
        memset(&rand_state, 0, sizeof(struct random_data));
        memset(statebuf, 0, sizeof(statebuf));
        initstate_r(seed, statebuf, 32, &rand_state);
        rand_initialized = 1;
    }
}

// Create a new node
SkipListNode *create_node(int level, int key, int value) {
    SkipListNode *node = (SkipListNode *)malloc(sizeof(SkipListNode));
    node->key = key;
    node->value = value;
    node->forward = (SkipListNode **)malloc((level + 1) * sizeof(SkipListNode *));
    for (int i = 0; i <= level; i++) {
        node->forward[i] = NULL;
    }
    return node;
}

// Create an empty skip list
SkipList *create_skiplist(int seed) {
    SkipList *list = (SkipList *)malloc(sizeof(SkipList));
    list->level = 0;
    list->header = create_node(MAX_LEVEL, INT_MIN, 0);
    list->prefilled_count = 0;
    list->inserted_count = 0;
    list->deleted_count = 0;
    init_rand(seed);
    return list;
}

// Generate a random level for a new node
int random_level() {
    int choice;
    random_r(&rand_state, &choice);
    int level = 0;
    while (((double)(choice & RAND_MAX) / RAND_MAX) < PROBABILITY && level < MAX_LEVEL) {
        random_r(&rand_state, &choice);
        level++;
    }
    return level;
}

// Insert a key-value pair into the skip list
bool insert(SkipList *list, int key, int value) {
    SkipListNode *update[MAX_LEVEL + 1];
    SkipListNode *current = list->header;

    for (int i = list->level; i >= 0; i--) {
        while (current->forward[i] != NULL && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];

    // Key already exists: do not insert, return false
    if (current != NULL && current->key == key) {
        return false;
    }

    int new_level = random_level();
    if (new_level > list->level) {
        for (int i = list->level + 1; i <= new_level; i++) {
            update[i] = list->header;
        }
        list->level = new_level;
    }

    SkipListNode *new_node = create_node(new_level, key, value);
    for (int i = 0; i <= new_level; i++) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }
    list->inserted_count++;
    return true;
}

// Delete a key from the skip list
bool erase(SkipList *list, int key) {
    SkipListNode *update[MAX_LEVEL + 1];
    SkipListNode *current = list->header;

    for (int i = list->level; i >= 0; i--) {
        while (current->forward[i] != NULL && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];

    if (current != NULL && current->key == key) {
        for (int i = 0; i <= list->level; i++) {
            if (update[i]->forward[i] != current) {
                break;
            }
            update[i]->forward[i] = current->forward[i];
        }

        free(current->forward);
        free(current);

        while (list->level > 0 && list->header->forward[list->level] == NULL) {
            list->level--;
        }
        list->deleted_count++;
        return true;
    }

    return false;
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

// Get the number of elements currently in the skip list
int get_element_count(SkipList *list) {
    int count = 0;
    SkipListNode *current = list->header->forward[0];
    while (current != NULL) {
        count++;
        current = current->forward[0];
    }
    return count;
}

// Free the entire skip list
void destroy_skiplist (SkipList *list) {
    SkipListNode *current = list->header;
    while (current != NULL) {
        SkipListNode *next = current->forward[0];
        free(current->forward);
        free(current);
        current = next;
    }
    free(list);
}