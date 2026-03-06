CC = gcc
CFLAGS = -std=c89 -pedantic -Wall -Wextra

cinquel: cinquel.c words.c
	$(CC) $(CFLAGS) cinquel.c -o cinquel

clean:
	rm -f cinquel

.PHONY: clean
