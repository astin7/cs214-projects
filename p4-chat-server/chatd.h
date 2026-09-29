#ifndef CHATD_H
#define CHATD_H

#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>

#define MAX_CLIENTS 100

#define MAX_NAME_LEN 32
#define MAX_STATUS_LEN 64
#define MAX_MSG_LEN 80
#define MAX_BUFFER 1024

#define PROTOCOL_VERSION "1"

#define ERR_UNREADABLE 0
#define ERR_NAME_IN_USE 1
#define ERR_UNKNOWN_RECIPIENT 2
#define ERR_ILLEGAL_CHAR 3
#define ERR_TOO_LONG 4
#define ERR_BAD_STATE 5

typedef struct {
    int fd;
    int named;
    char name[MAX_NAME_LEN + 1];
    char status[MAX_STATUS_LEN + 1];
} client_t;

void add_client(int fd);
void remove_client(int fd);
client_t *get_client_by_fd(int fd);
client_t *get_client_by_name(const char *name);
client_t *get_client_at(int index);
int is_name_taken(const char *name);

int handle_message(int fd, const char *msg);

int write_all(int fd, const char *buf, size_t len);
int valid_name(const char *name);
int valid_text(const char *text, int max_len);

#endif
