CC = gcc
CFLAGS = -g -std=c99 -Wall

OBJS = main.o wordlist.o tokenizer.o traverse.o compare.o

all: compare

compare: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o compare -lm

main.o: main.c traverse.h compare.h
wordlist.o: wordlist.c wordlist.h
tokenizer.o: tokenizer.c tokenizer.h wordlist.h
traverse.o: traverse.c traverse.h tokenizer.h compare.h
compare.o: compare.c compare.h wordlist.h

clean:
	rm -f *.o compare
