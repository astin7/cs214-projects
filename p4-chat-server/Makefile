CC = gcc
CFLAGS = -Wall -Werror -std=c99

TARGET = chatd
SRC = chatd.c protocol.c clientlist.c utils.c
OBJ = chatd.o protocol.o clientlist.o utils.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

chatd.o: chatd.c chatd.h
	$(CC) $(CFLAGS) -c chatd.c

protocol.o: protocol.c chatd.h
	$(CC) $(CFLAGS) -c protocol.c

clientlist.o: clientlist.c chatd.h
	$(CC) $(CFLAGS) -c clientlist.c

utils.o: utils.c chatd.h
	$(CC) $(CFLAGS) -c utils.c

clean:
	rm -f $(TARGET) $(OBJ)
