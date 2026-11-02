#ifndef KN_INCLUDE_KN_ARENA_H
#define KN_INCLUDE_KN_ARENA_H

#include <stddef.h>
#include <stdint.h>

#ifndef KN_ARENA_DEF
#ifdef KN_ARENA_STATIC
#define KN_ARENA_DEF static
#else
#define KN_ARENA_DEF extern
#endif
#endif


#define KN_ARENA_MIN_ALIGNMENT 8

typedef struct Region Region;

struct Region {
	Region *next;
	size_t count;
	size_t capacity;
	uintptr_t *data;
};

typedef struct {
	struct _allocator *allocator;
	Region *begin, *end;
} Arena;

struct _allocator {
	Region *(*new_region)(Arena *arena, size_t capacity);
	void *(*arena_alloc)(Arena *arena, size_t size);
	void (*arena_reset)(Arena *arena);
	void (*arena_free)(Arena *arena);
	size_t alignment;
	int extends;
};

// declarations

KN_ARENA_DEF Arena *kn_arena_init(void *allocator_fptr, size_t alignment, size_t capacity);

KN_ARENA_DEF void *kn_arena_alloc(Arena *arena, size_t size);

KN_ARENA_DEF void kn_arena_reset(Arena *arena);

// free's the entire arena and sets the arena ptr to NULL
KN_ARENA_DEF void kn_arena_free(Arena *arena);

// only used with init
KN_ARENA_DEF void kn_arena_set_allocator_std(Arena *arena, size_t align);

// only used with init
KN_ARENA_DEF void kn_arena_set_allocator_aligned(Arena *arena, size_t align);

#endif // KN_INCLUDE_KN_ARENA_H

/////////// Implementation /////////////////////////////

#define KN_ARENA_IMPLEMENTATION
#ifdef KN_ARENA_IMPLEMENTATION
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <nmmintrin.h>

// INFO: we are wrapping our allocator functions to have the same interface/api no matter the allocator used

static Region *kn_new_region(Arena *arena, size_t capacity) {
	return arena->allocator->new_region(arena, capacity);
}

KN_ARENA_DEF Arena *kn_arena_init(void *allocator_fptr, size_t alignment, size_t capacity) {
	Arena *out = malloc(sizeof(Arena));
	assert(out);
	size_t _alignment = ((alignment >= KN_ARENA_MIN_ALIGNMENT) && ((_mm_popcnt_u64(alignment)) == 1)) ? alignment : 64;
	size_t misalignment = capacity & (_alignment - 1);
	size_t _capacity = (misalignment == 0) ? capacity : capacity + _alignment - misalignment;

	void (*fptr)(Arena *arena, size_t align);
	fptr = allocator_fptr;

	fptr(out, _alignment);
	out->begin = out->allocator->new_region(out, _capacity);
	out->end = out->begin;
	return out;
}

KN_ARENA_DEF void *kn_arena_alloc(Arena *arena, size_t size) {
	void *result = arena->allocator->arena_alloc(arena, size);
	if (arena->allocator->extends == 0) return result;
	if (result != NULL) return result;
	
	arena->end->next = kn_new_region(arena, arena->begin->capacity);
	arena->end = arena->end->next;
	result = arena->allocator->arena_alloc(arena, size);

	return result;
}

KN_ARENA_DEF void kn_arena_reset(Arena *arena) {
	arena->allocator->arena_reset(arena);
}

static void kn_arena_region_free(Region *region) {
	free(region->data);
	region->data = NULL;
	free(region);
	region = NULL;
}

KN_ARENA_DEF void kn_arena_free(Arena *arena) {
	arena->allocator->arena_free(arena);
}

// allocator implementations 

static Region *kn_new_region_std(Arena *arena, size_t capacity) {
	Region *new = malloc(sizeof(Region));
	assert(new);

	new->next = NULL;
	new->count = 0;
	new->capacity = capacity;
	new->data = aligned_alloc(arena->allocator->alignment, capacity);
	assert(new->data);

	return new;
}

static void kn_arena_free_std(Arena *arena) {
	Region *next = arena->begin;
	Region *current = NULL;
	while (next != NULL) {
		current = next;
		next = current->next;
		kn_arena_region_free(current);
	}
	arena->begin = NULL;
	arena->end = NULL;
}

static void *kn_arena_alloc_std(Arena *arena, size_t size) {
	assert(arena);
	assert(arena->begin);

	Region *region = arena->begin;
	if ((region->capacity - region->count) < size ) return NULL;

	void *result = region->data + region->count;
	region->count += size;

	return result;
}

static void *kn_arena_alloc_aligned(Arena *arena, size_t size) {
	assert(arena);
	assert(arena->begin);

	size_t misalignment = size & (arena->allocator->alignment - 1);
	size_t _size = (misalignment == 0) ? size : size + arena->allocator->alignment - misalignment;

	Region *region = arena->begin;
	if ((region->capacity - region->count) < _size ) return NULL;

	void *result = region->data + region->count;
	region->count += _size;

	return result;
}

static void kn_arena_reset_std(Arena *arena) {
	arena->begin->count = 0;
}

// TODO: static void kn_arena_resize_std(Arena *arena, size_t new_size) {
// }

KN_ARENA_DEF void kn_arena_set_allocator_std(Arena *arena, size_t align) {
	arena->allocator->new_region = &kn_new_region_std;
	arena->allocator->arena_alloc = &kn_arena_alloc_std;
	arena->allocator->arena_reset = &kn_arena_reset_std;
	arena->allocator->arena_free = &kn_arena_free_std;
	arena->allocator->alignment = align;
	arena->allocator->extends = 1;
}

KN_ARENA_DEF void kn_arena_set_allocator_aligned(Arena *arena, size_t align) {
	arena->allocator->new_region = &kn_new_region_std;
	arena->allocator->arena_alloc = &kn_arena_alloc_aligned;
	arena->allocator->arena_reset = &kn_arena_reset_std;
	arena->allocator->arena_free = &kn_arena_free_std;
	arena->allocator->alignment = align;
	arena->allocator->extends = 0;
}

#endif // KN_ARENA_IMPLEMENTATION
