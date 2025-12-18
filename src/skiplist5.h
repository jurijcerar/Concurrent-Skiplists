#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <limits.h>
#include <stdint.h>
#include <strings.h>
#include <pthread.h>

#define MAX_LEVEL 16

// Define the Node structure
typedef struct Node {
    int key;
    atomic_intptr_t next[MAX_LEVEL + 1];  // Array of atomic pointers
    int topLevel;
} Node;

// Define the SkipList structure
typedef struct SkipList {
    Node* head;
    Node* tail;
} SkipList;

// Define AtomicMarkableReference structure
typedef struct {
    atomic_uintptr_t ptr;  // Atomic pointer
} AtomicMarkableReference;

// Helper function prototypes
bool get_flag(uintptr_t p);
void* get_pointer(uintptr_t p);
uintptr_t set_flag(uintptr_t p, bool value);

// AtomicMarkableReference function prototypes
void* get(AtomicMarkableReference* amr, bool* marked);
void set(AtomicMarkableReference* amr, void* ref, bool mark);
bool attemptMark(AtomicMarkableReference* amr, void* expectedRef, bool newMark);
bool compareAndSet(AtomicMarkableReference* amr, void* expectedRef, bool expectedMark, void* newRef, bool newMark);
void* getReference(AtomicMarkableReference* amr);

// Skip list function prototypes
int random_level(int seed);
SkipList* create_skiplist();
Node* createNode(int key, int topLevel);
bool insert(SkipList* list, int key, int seed);
bool find(SkipList* list, int key, Node* preds[], Node* succs[]);
bool erase(SkipList* list, int key);
bool contains(SkipList* list, int key);
int get_element_count(SkipList* list);


#endif // SKIPLIST_H
