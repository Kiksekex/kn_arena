#include <stdio.h>
#include <string.h>

#define KN_ARENA_IMPLEMENTATION
#include "kn_arena.h"


int main(void)
{
	Arena *a_tmp = kn_arena_init(&kn_arena_set_allocator_std, 8, 256);
	char *str = kn_arena_alloc(a_tmp, strlen("Hello Arenas!") + 1);
	strcpy(str, "Hello Arenas!");
	puts(str);
	printf("count=%zu, strlen=%zu\n", a_tmp->begin->count, strlen(str));
	kn_arena_reset(a_tmp);
	puts("str reset");
	printf("count=%zu, strlen=%zu\n", a_tmp->begin->count, strlen(str));
	kn_arena_free(a_tmp);
	
	return EXIT_SUCCESS;
}
