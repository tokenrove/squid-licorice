#pragma once

#include <stdlib.h>
#include <stdbool.h>

typedef void *alloc_bitmap;

extern alloc_bitmap alloc_bitmap_init(size_t count, size_t member_size);
extern void alloc_bitmap_destroy(alloc_bitmap);
extern void *alloc_bitmap_alloc_first_free(alloc_bitmap);
extern bool alloc_bitmap_remove(alloc_bitmap, void *);

struct alloc_bitmap_iterator {
    /* private */
    void *t;
    size_t i, j, n;
    /* public */
    void *(*next)(struct alloc_bitmap_iterator *);
    void (*mark_for_removal)(struct alloc_bitmap_iterator *);
    void (*expunge_marked)(struct alloc_bitmap_iterator *);
};

extern struct alloc_bitmap_iterator alloc_bitmap_iterate(alloc_bitmap);
