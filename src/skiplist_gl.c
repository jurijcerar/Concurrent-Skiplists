#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <omp.h>
#include "skiplist3.h"

// Function to create a new node
SkipListNode* create_node(int level, int key, int value) {
    SkipListNode* node = (SkipListNode*)malloc(sizeof(SkipListNode));
    if (!node) {
        fprintf(stderr, "Memory allocation failed for node\n");
        exit(EXIT_FAILURE);
    }
    node->key = key;
    node->value = value;
    node->forward = (SkipListNode**)calloc((level + 1), sizeof(SkipListNode*));
    if (!node->forward) {
        fprintf(stderr, "Memory allocation failed for forward pointers\n");
        free(node);
        exit(EXIT_FAILURE);
    }
    return node;
}

// Function to create a skiplist
SkipList* create_skiplist() {
    SkipList* sl = (SkipList*)malloc(sizeof(SkipList));
    if (!sl) {
        fprintf(stderr, "Memory allocation failed for skiplist\n");
        exit(EXIT_FAILURE);
    }
    sl->level = 0;
    sl->header = create_node(MAX_LEVEL, INT_MIN, 0);
    pthread_mutex_init(&sl->lock, NULL);
    sl->prefilled_count = 0;
    sl->inserted_count = 0;
    sl->deleted_count = 0;
    return sl;
}

// Random level generator
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

// Insert function
bool insert(SkipList* sl, int key, int value) {
    SkipListNode* update[MAX_LEVEL + 1];
    SkipListNode* x;
    bool inserted = true;

    #pragma omp critical
    {
        x = sl->header;
        for (int i = sl->level; i >= 0; i--) {
            while (x->forward[i] != NULL && x->forward[i]->key < key) {
                x = x->forward[i];
            }
            update[i] = x;
        }

        x = x->forward[0];

        if (x != NULL && x->key == key) {
            x->value = value; // Update value
            inserted = false;
        } else {
            int new_level = random_level();
            if (new_level > sl->level) {
                for (int i = sl->level + 1; i <= new_level; i++) {
                    update[i] = sl->header;
                }
                sl->level = new_level;
            }

            SkipListNode* new_node = create_node(new_level, key, value);
            for (int i = 0; i <= new_level; i++) {
                new_node->forward[i] = update[i]->forward[i];
                update[i]->forward[i] = new_node;
            }

            sl->inserted_count++; // Increment inserted count
        }
    }

    return inserted;
}

// Contains function
bool contains(SkipList* sl, int key) {
    SkipListNode* x = sl->header;
    bool result = false;

    #pragma omp critical
    {
        for (int i = sl->level; i >= 0; i--) {
            while (x->forward[i] != NULL && x->forward[i]->key < key) {
                x = x->forward[i];
            }
        }
        x = x->forward[0];
        result = (x != NULL && x->key == key);
    }

    return result;
}

// Erase function
bool erase(SkipList* sl, int key) {
    SkipListNode* update[MAX_LEVEL + 1];
    SkipListNode* x;
    bool found = false;

    #pragma omp critical
    {
        x = sl->header;
        for (int i = sl->level; i >= 0; i--) {
            while (x->forward[i] != NULL && x->forward[i]->key < key) {
                x = x->forward[i];
            }
            update[i] = x;
        }

        x = x->forward[0];

        if (x != NULL && x->key == key) {
            for (int i = 0; i <= sl->level; i++) {
                if (update[i]->forward[i] != x) break;
                update[i]->forward[i] = x->forward[i];
            }

            free(x->forward);
            free(x);

            while (sl->level > 0 && sl->header->forward[sl->level] == NULL) {
                sl->level--;
            }

            sl->deleted_count++; // Increment deleted count
            found = true;
        }
    }

    return found;
}

// Prefill function
void prefill(SkipList* sl, int count) {
    pthread_mutex_lock(&sl->lock);
    for (int i = 0; i < count; i++) {
        int key = rand();
        int value = rand();
        if (insert(sl, key, value)) {
            sl->prefilled_count++;
        }
    }
    pthread_mutex_unlock(&sl->lock);
}

// Validation function
bool validate(SkipList* sl) {
    int expected_count = sl->prefilled_count + sl->inserted_count - sl->deleted_count;
    int actual_count = 0;

    SkipListNode* x = sl->header->forward[0];
    while (x != NULL) {
        actual_count++;
        x = x->forward[0];
    }

    if (expected_count != actual_count) {
        fprintf(stderr, "Validation failed: Expected count = %d, Actual count = %d\n", expected_count, actual_count);
        return false;
    }

    printf("Validation passed: Expected count matches Actual count (%d).\n", actual_count);
    return true;
}

// Function to print the skiplist (for debugging)
void print_skiplist(SkipList* sl) {
    for (int i = sl->level; i >= 0; i--) {
        SkipListNode* x = sl->header->forward[i];
        printf("Level %d: ", i);
        while (x != NULL) {
            printf("(%d, %d) ", x->key, x->value);
            x = x->forward[i];
        }
        printf("\n");
    }
}

// Free the entire skip list
void free_skiplist(SkipList* list) {
    SkipListNode* current, *next;

    #pragma omp critical
    {
        current = list->header;
        while (current != NULL) {
            next = current->forward[0];
            free(current->forward);
            free(current);
            current = next;
        }
        pthread_mutex_destroy(&list->lock); // Destroy mutex
        free(list);
    }
}

int get_element_count(SkipList *list) {
    int count = 0;
    SkipListNode *current = list->header->forward[0];
    
    while (current != NULL) {
        count++;
        current = current->forward[0];
    }
    
    return count;
}

