CC = gcc
CFLAGS = -Wall -Werror -std=c99

TARGET = mysh

SRC = mysh.c parser.c executor.c builtins.c wildcard.c utils.c
OBJ = mysh.o parser.o executor.o builtins.o wildcard.o utils.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

mysh.o: mysh.c mysh.h
	$(CC) $(CFLAGS) -c mysh.c

parser.o: parser.c mysh.h
	$(CC) $(CFLAGS) -c parser.c

executor.o: executor.c mysh.h
	$(CC) $(CFLAGS) -c executor.c

builtins.o: builtins.c mysh.h
	$(CC) $(CFLAGS) -c builtins.c

wildcard.o: wildcard.c mysh.h
	$(CC) $(CFLAGS) -c wildcard.c

utils.o: utils.c mysh.h
	$(CC) $(CFLAGS) -c utils.c

clean:
	rm -f $(TARGET) $(OBJ)
