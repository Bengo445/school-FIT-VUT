all: main

.PHONY: run clean

main: flows.c
	cc -std=c11 -Wall -Wextra -Werror -pedantic flows.c -o flows -lm

run: main
	./main $(ARGS)

clean:
	rm -f main main-*
