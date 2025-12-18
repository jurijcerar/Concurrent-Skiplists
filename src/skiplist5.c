#include "skiplist5.h"

// Helper functions for marking
bool get_flag(uintptr_t p) {
    return p & 1;
}

void* get_pointer(uintptr_t p) {
    return (void*)(p & (UINTPTR_MAX ^ 1));
}

uintptr_t set_flag(uintptr_t p, bool value) {
    return (p & (UINTPTR_MAX ^ 1)) | value;
}

// AtomicMarkableReference functions
void* get(AtomicMarkableReference* amr, bool* marked) {
    uintptr_t p = atomic_load(&amr->ptr);
    *marked = get_flag(p);
    return get_pointer(p);
}

void set(AtomicMarkableReference* amr, void* ref, bool mark) {
    uintptr_t p = (uintptr_t)ref;
    p = set_flag(p, mark);
    atomic_store(&amr->ptr, p);
}

bool attemptMark(AtomicMarkableReference* amr, void* expectedRef, bool newMark) {
    uintptr_t p = (uintptr_t)expectedRef;
    uintptr_t old = atomic_load(&amr->ptr);

    if (get_pointer(old) == expectedRef && !get_flag(old)) {
        uintptr_t newVal = set_flag(p, newMark);
        return atomic_compare_exchange_strong(&amr->ptr, &old, newVal);
    }

    return false;
}

bool compareAndSet(AtomicMarkableReference* amr, void* expectedRef, bool expectedMark, void* newRef, bool newMark) {
    uintptr_t expected = (uintptr_t)expectedRef;
    expected = set_flag(expected, expectedMark);

    uintptr_t new = (uintptr_t)newRef;
    new = set_flag(new, newMark);

    return atomic_compare_exchange_strong(&amr->ptr, &expected, new);
}

void* getReference(AtomicMarkableReference* amr) {
    uintptr_t p = atomic_load(&amr->ptr);
    return get_pointer(p);
}

// Create a new node
Node* createNode(int key, int topLevel) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->key = key;
    newNode->topLevel = topLevel;

    for (int i = 0; i <= topLevel; i++) {
        set((AtomicMarkableReference*)&newNode->next[i], NULL, false);
    }
    return newNode;
}

// Initialize the skip list
SkipList* create_skiplist() {
    SkipList* list = (SkipList*)malloc(sizeof(SkipList));
    Node* head = createNode(INT_MIN, MAX_LEVEL);
    Node* tail = createNode(INT_MAX, MAX_LEVEL);

    for (int i = 0; i <= MAX_LEVEL; i++) {
        set((AtomicMarkableReference*)&head->next[i], tail, false);
    }

    list->head = head;
    list->tail = tail;
    return list;
}

// Generate a random level
int random_level(int seed) {
    struct random_data rand_state;
    char statebuf[32];
    bzero(&rand_state, sizeof(struct random_data));
    bzero(&statebuf, sizeof(statebuf));
    initstate_r(seed, statebuf, sizeof(statebuf), &rand_state);

    int choice;
    random_r(&rand_state, &choice);

    int level = 0;
    while (((double)choice / RAND_MAX) < 0.5 && level < MAX_LEVEL) {
        level++;
        random_r(&rand_state, &choice);
    }
    return level;
}

// Find a node
bool find(SkipList* list, int key, Node* preds[], Node* succs[]) {
    Node* pred = NULL;
    Node* curr = NULL;
    Node* succ = NULL;
    bool marked = false;
    int bottomLevel = 0;

retry:
    while (true) {
        pred = list->head;
        for (int level = MAX_LEVEL; level >= bottomLevel; level--) {
            curr = getReference((AtomicMarkableReference*)&pred->next[level]);
            while (true) {
                succ = get((AtomicMarkableReference*)&curr->next[level], &marked);
                while (marked) {
                    if (!compareAndSet((AtomicMarkableReference*)&pred->next[level], curr, false, succ, false)) {
                        goto retry;
                    }
                    curr = getReference((AtomicMarkableReference*)&pred->next[level]);
                    succ = get((AtomicMarkableReference*)&curr->next[level], &marked);
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
}

// Insert into the skip list
bool insert(SkipList* list, int key, int seed) {
    int topLevel = random_level(seed);
    int bottomLevel = 0;
    Node* preds[MAX_LEVEL + 1];
    Node* succs[MAX_LEVEL + 1];

    while (true) {
        //printf("a");
        bool found = find(list, key, preds, succs);
        if (found) {
            return false; // Key already exists
        }else{
            Node* newNode = createNode(key, topLevel);

            for (int level = bottomLevel; level <= topLevel; level++) {
                Node* succ = succs[level];
                set((AtomicMarkableReference*)&newNode->next[level], succ, false);
            }

            Node* pred = preds[bottomLevel];
            Node* succ = succs[bottomLevel];

            set((AtomicMarkableReference*)&newNode->next[bottomLevel], succ, false);

            if (!compareAndSet((AtomicMarkableReference*)&pred->next[bottomLevel],
                           succ, false, newNode, false)) {
                continue; // Retry insertion
            }

            for (int level = bottomLevel + 1; level <= topLevel; level++) {
                while (true) {
                    pred = preds[level];
                    succ = succs[level];
                    if (compareAndSet((AtomicMarkableReference*)&pred->next[level],
                                    succ, false, newNode, false)) {
                        break;
                    }
                    find(list, key, preds, succs);
                }
            }

            return true;
        }
    }
}

// Erase from the skip list
bool erase(SkipList* list, int key) {
    Node* preds[MAX_LEVEL + 1];
    Node* succs[MAX_LEVEL + 1];
    int bottomLevel = 0;
    Node* succ;

    while (true) {
        //printf("b");
        bool found = find(list, key, preds, succs);
        if (!found) {
            return false; // Key not found
        }else {
            Node* nodeToRemove = succs[bottomLevel];
            for (int level = nodeToRemove->topLevel; level >= bottomLevel + 1; level--) {
                bool marked = false;
                succ = get((AtomicMarkableReference*)&nodeToRemove->next[level], &marked);
                while (!marked) {
                    attemptMark((AtomicMarkableReference*)&nodeToRemove->next[level], succ, true);
                    succ = get((AtomicMarkableReference*)&nodeToRemove->next[level], &marked);
                }
            }


             
            bool marked = false;
            Node* succ = get((AtomicMarkableReference*)&nodeToRemove->next[bottomLevel], &marked);
            while (true) {
                bool isMarked = compareAndSet((AtomicMarkableReference*)&nodeToRemove->next[bottomLevel],
                                succ, false, succ, true);
                succ = get((AtomicMarkableReference*)&succs[bottomLevel]->next[bottomLevel], &marked);
                if(isMarked){
                    find(list, key, preds, succs);
                    return true;
                } else if (marked) {
                    return false;
                }
            }
        }
    }
}

// Check if an element exists
bool contains(SkipList* list, int key) {
    Node* pred = list->head;
    Node* curr = NULL;
    Node* succ = NULL;
    bool marked = false;
    int bottomLevel = 0;

    for (int level = MAX_LEVEL; level >= bottomLevel; level--) {
        curr = getReference((AtomicMarkableReference*)&pred->next[level]);
        while (true) {
            //printf("c");
            succ = get((AtomicMarkableReference*)&curr->next[level], &marked);
            while (marked) {
                curr = getReference((AtomicMarkableReference*)&curr->next[level]);
                succ = get((AtomicMarkableReference*)&curr->next[level], &marked);
            }
            if (curr->key < key) {
                pred = curr;
                curr = succ;
            } else {
                break;
            }
        }
    }
    return (curr->key == key);
}

// Get the element count in the skip list
int get_element_count(SkipList* list) {
    Node* current = list->head;
    int count = 0;

    // Traverse the bottommost level
    current = (Node*)getReference((AtomicMarkableReference*)&current->next[0]);
    while (current != list->tail) {
        count++;
        current = (Node*)getReference((AtomicMarkableReference*)&current->next[0]);
    }

    return count;
}
