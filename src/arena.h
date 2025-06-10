#ifndef ARENA_H
#define ARENA_H
#include <stddef.h>
#include <stdint.h>

#ifndef ARENA_DEFAULT_ALIGNMENT
#define ARENA_DEFAULT_ALIGNMENT (2 * sizeof(void *))
#endif

struct Arena {
    uint8_t *buffer;
    size_t capacity;
    size_t position;
};

void arena_init(struct Arena *arena, void *buffer, size_t capacity);
void arena_clear(struct Arena *arena);

void *arena_alloc_aligned(struct Arena *arena, size_t size, size_t alignment);
void *arena_alloc(struct Arena *arena, size_t size);
void *arena_alloc_array(struct Arena *arena, size_t count, size_t element_size);

#endif /* ARENA_H */
