#include "arena.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Adapted from https://en.wikipedia.org/wiki/Data_structure_alignment#Computing_padding */
static inline uintptr_t align_forward(uintptr_t offset, uintptr_t align) {
    assert(align != 0 && (align & (align - 1)) == 0);
    return (offset + (align - 1)) & -align;
}

void arena_init(struct Arena *arena, void *buffer, size_t capacity) {
    arena->buffer = buffer;
    arena->capacity = capacity;
    arena->position = 0;
}

void arena_clear(struct Arena *arena) {
    arena->position = 0;
}

void *arena_alloc_aligned(struct Arena *arena, size_t size, size_t align) {
    uintptr_t position = (uintptr_t)arena->buffer + (uintptr_t)arena->position;
    uintptr_t offset = align_forward(position, align) - (uintptr_t)arena->buffer;

    if (offset + size > arena->capacity) {
        /* TODO: Maybe call assert or abort when out of host memory? */
        return NULL;
    }

    void *ptr = arena->buffer + offset;
    arena->position = offset + size;

    /* Using memset may help prevent reading garbage data, but it is slightly slower. Maybe make it
     * optional with a flag? */
    return memset(ptr, 0, size);
}

void *arena_alloc(struct Arena *arena, size_t size) {
    return arena_alloc_aligned(arena, size, ARENA_DEFAULT_ALIGNMENT);
}

void *arena_alloc_array(struct Arena *arena, size_t count, size_t element_size) {
    return arena_alloc(arena, element_size * count);
}
