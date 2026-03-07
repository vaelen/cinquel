CC = gcc
CFLAGS = -std=c89 -pedantic -Wall -Wextra

cinquel: cinquel.o ui.o util.o
	$(CC) $(CFLAGS) cinquel.o ui.o util.o -o cinquel

cinquel.o: cinquel.c cinquel.h words.c
	$(CC) $(CFLAGS) -c cinquel.c

ui.o: ui.c ui.h cinquel.h
	$(CC) $(CFLAGS) -c ui.c

util.o: util.c util.h
	$(CC) $(CFLAGS) -c util.c

clean:
	rm -f cinquel cinquel.o ui.o util.o

.PHONY: clean
