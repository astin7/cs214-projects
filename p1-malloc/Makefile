CC = gcc
CFLAGS = -std=c99 -Wall -Werror -g

OBJS = memgrind.o mymalloc.o

memgrind: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

memgrind.o: memgrind.c mymalloc.h
	$(CC) $(CFLAGS) -c $< -o $@

mymalloc.o: mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f memgrind $(OBJS)
