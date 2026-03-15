# Concurrent Skip List Implementations

Four header-only skip list implementations. Drop the `.h` into your project and include it, and you are done.

## Implementations

| Header | Strategy 
|---|---|
| `skiplist_seq.h` | Sequential (single-thread) |
| `skiplist_gl.h` | Global lock (low parallelism) |
| `skiplist_lazy.h` | Lazy locking |
| `skiplist_free.h` | Lock-free (CAS + atomic mark bits) |

## API

All four implementations expose the same interface:

```c
SkipList *skiplist_create(void);
void      skiplist_destroy(SkipList *list);
bool      skiplist_insert(SkipList *list, int key, void *value);
bool      skiplist_erase(SkipList *list, int key);
void     *skiplist_search(SkipList *list, int key);
```

- `skiplist_insert` returns `true` if inserted, `false` if key already existed.
- `skiplist_erase` returns `true` if removed, `false` if key was not found.
- `skiplist_search` returns the stored `value` pointer, or `NULL` if not found.

## Usage

```c
#include "skiplist_lazy.h"   // or any of the four headers

int main(void) {
    SkipList *sl = skiplist_create();

    skiplist_insert(sl, 42, "hello");
    skiplist_insert(sl, 99, "world");

    char *val = (char *)skiplist_search(sl, 42); // "hello"

    skiplist_erase(sl, 42);

    skiplist_destroy(sl);
}
```

Because all functions are `static`, you can only include **one** skiplist header per `.c` file. If you need two implementations in the same binary, put each in its own `.c` file.

## Building

```bash
# Sequential
gcc -O2 -std=c11 your_file.c

# The rest
gcc -O2 -std=c11 -fopenmp your_file.c

```

## Notes
Only works on Linux, as Windows does not allow the use of initstate_r.
There could be bugs in the code, so please feel free point them out.
## Configuration

At the top of each header:

```c
#define SKIPLIST_MAX_LEVEL 16   // maximum height of a node (default 16)
#define SKIPLIST_P         0.5  // probability of level promotion (default 0.5)
```

Feel free to change these.

## References
The implementation of the Lazy and Lock-Free Skiplists in based on Chapter 15 of *The Art of Multiprocessor Programming* by **Nir Shavit & Maurice Herlihy**
