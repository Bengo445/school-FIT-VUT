all: main

CC = gcc
override CFLAGS += -std=c11 -Wall -Wextra -Werror -Wno-unused-variable -Wno-unused-parameter -Wno-unused-result -Wno-unknown-pragmas -pedantic -lm

.PHONY: run clean

main: keyfilter.c
	$(CC) $(CFLAGS) -O0 $^ -o keyfilter

run: main
	./main $(ARGS)

clean:
	rm -f main main-*
