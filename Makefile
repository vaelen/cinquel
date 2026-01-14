CC = gcc
CFLAGS = -std=c89 -pedantic -Wall -Wextra

wordle: wordle.c words.c
	$(CC) $(CFLAGS) wordle.c -o wordle

clean:
	rm -f wordle

.PHONY: clean
