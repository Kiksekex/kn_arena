all: clean hello_arena run

hello_arena:
	gcc -march=native -g -O0 -Wall -Wextra -fsanitize=address,undefined hello_arena.c -o hello_arena

run: hello_arena
	./hello_arena

clean:
	rm -f hello_arena
