#include "skiplist4.h"

// Create a new node
Node* createNode(int key, int levels) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->key = key;
    node->next = (Node**)malloc((levels +1)  * sizeof(Node*));
    node->marked = false;
    node->fullyLinked = false;
    node->topLevel = levels;
    omp_init_lock(&node->lock);
    return node;    
}

// Destroy a node
void destroyNode(Node* node) {
    omp_destroy_lock(&node->lock);
    free(node->next);
    free(node);
}

// Create a new skip list
SkipList* create_skiplist(void) {
    SkipList* list = (SkipList*)malloc(sizeof(SkipList));
    list->head = createNode(INT_MIN, MAX_LEVEL);
    list->tail = createNode(INT_MAX, MAX_LEVEL);

    list->head->fullyLinked = true;

    for (int i = 0; i <= MAX_LEVEL; ++i) {
        list->head->next[i] = list->tail;
    }

    return list;
}

// Destroy a skip list
void destroySkipList(SkipList* list) {
    Node* curr = list->head;
    while (curr) {
        Node* next = curr->next[0];
        destroyNode(curr);
        curr = next;
    }
    free(list);
}

// Generate a random level for a new node
int random_level(int seed) {

    struct random_data rand_state;
    int choice;
    char statebuf[32];
    bzero(&rand_state, sizeof(struct random_data));
    bzero(&statebuf,   sizeof(statebuf));
    initstate_r(seed, statebuf, 32, &rand_state);
    random_r(&rand_state, &choice);

    int level = 0;
    while (((double)choice / RAND_MAX) < 0.5 && level < MAX_LEVEL) {
        level++;
    }
    return level;

    /*int level = 0;
    while ((rand() % 2) == 0 && level < MAX_LEVEL) {
        level++;
    }
    return level;*/

}

// Find method for SkipList
int find(SkipList* list, int key, Node** preds, Node** succs) {
    int foundLevel = -1;
    Node* pred = list->head;
    for (int l = MAX_LEVEL; l >= 0; --l) {
        Node* curr = pred->next[l];
        while (curr->key < key) {
            pred = curr;
            curr = pred->next[l];
        }
        if (foundLevel == -1 && curr->key == key) {
            foundLevel = l;
        }
        preds[l] = pred;
        succs[l] = curr;
    }
    return foundLevel;
}

// Contains method for SkipList
bool contains(SkipList* list, int key) {
    Node* preds[MAX_LEVEL + 1];
    Node* succs[MAX_LEVEL + 1];
    int foundLevel = find(list, key, preds, succs);
    return (foundLevel >= 0 && succs[foundLevel]->fullyLinked && !succs[foundLevel]->marked);
}

// Remove method for SkipList
bool erase(SkipList* list, int key) {
    Node* victim = NULL;
    bool marked = false;
    int topLevel = -1;
    Node* preds[MAX_LEVEL + 1];
    Node* succs[MAX_LEVEL + 1];

    while (true) {
        int foundLevel = find(list, key, preds, succs);
        if (marked || (foundLevel >= 0 && succs[foundLevel]->fullyLinked && !succs[foundLevel]->marked)) {
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
            Node* pred;
            Node* succ;
            Node* helper = NULL;
            bool valid = true;
            for (int l = 0; valid && (l <= topLevel); ++l) {
                pred = preds[l];
                succ = succs[l];
                if (pred != helper) {
                    omp_set_lock(&pred->lock);
                    highLock = l;
                    helper = pred;
                }
                valid = !pred->marked && pred->next[l] == succ;
            }
            if (!valid) {
                unlockLevels(preds, highLock);
                continue;
            }

            for (int l = topLevel; l >= 0; --l) {
                preds[l]->next[l] = victim->next[l];
            }

            omp_unset_lock(&victim->lock);
            unlockLevels(preds, highLock);

            #pragma omp atomic
            list->deleted_count++; // Increment deleted count

            return true;
        } else {
            return false;
        }
    }
}


// Unlock levels helper function
void unlockLevels(Node** preds, int highLock) {
    Node* helper = NULL;
    for (int l = 0; l <= highLock; ++l) {
        if (preds[l] != helper) {
            omp_unset_lock(&preds[l]->lock);
        }
        helper = preds[l];
    }
}

// Add method for SkipList
bool insert(SkipList* list, int key, int seed) {
    int topLevel = random_level(seed);
    Node* preds[MAX_LEVEL + 1];
    Node* succs[MAX_LEVEL + 1];

    while (true) {
        int foundLevel = find(list, key, preds, succs);

        if (foundLevel >= 0) {
            Node* found = succs[foundLevel];
            if (!found->marked) {
                while (!found->fullyLinked) {}
                return false;
            }
            continue;
        }

        int highLock = -1;
        Node* pred;
        Node* succ;
        Node* helper = NULL;
        bool valid = true;

        for (int l = 0; valid && (l <= topLevel); ++l) {
            pred = preds[l];
            succ = succs[l];
            if (pred != helper) {
                omp_set_lock(&pred->lock);
                highLock = l;
                helper = pred;
            }

            valid = !pred->marked && !succ->marked && pred->next[l] == succ;
        }

        if (!valid) {
            unlockLevels(preds, highLock);
            continue;
        }

        Node* newNode = createNode(key, topLevel);
        for (int l = 0; l <= topLevel; ++l) {
            newNode->next[l] = succs[l];
            preds[l]->next[l] = newNode;
        }
        newNode->fullyLinked = true;

        unlockLevels(preds, highLock);

        #pragma omp atomic
        list->inserted_count++; // Increment inserted count

        return true;
    }
}

bool validate_skiplist(SkipList* list) {
    int expected_count = list->inserted_count - list->deleted_count;
    int actual_count = get_element_count(list);

    if (expected_count != actual_count) {
        fprintf(stderr, "Validation failed: Expected count = %d, Actual count = %d\n", expected_count, actual_count);
        return false;
    }

    printf("Validation passed: Expected count matches Actual count (%d).\n", actual_count);
    return true;
}

int get_element_count(SkipList* list) {
    int count = 0;
    Node* current = list->head->next[0]; // Start from the first real node

    while (current != list->tail) {
        if (!current->marked) { // Count only unmarked nodes
            count++;
        }
        current = current->next[0];
    }

    return count;
}
