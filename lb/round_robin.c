#include "../types.h"

Backend* get_next_backend(BackendPool* pool) {
    if (pool -> count == 0) return NULL;

    int start = pool -> current;

    while (1) {
        Backend *b = &pool -> backends[pool -> current];

        pool -> current = (pool -> current + 1) % pool -> count;

        if (b -> is_healthy) {
            return b;
        }

        // Avoiding Infinite Loop
        if (pool -> current == start) {
            return NULL;
        }
    }
}
