#include "chatd.h"

static client_t clients[MAX_CLIENTS];

void add_client(int fd) {
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == 0) {
            clients[i].fd = fd;
            clients[i].named = 0;
            clients[i].name[0] = '\0';
            clients[i].status[0] = '\0';
            return;
        }
    }
}

void remove_client(int fd) {
    int i;

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == fd) {
            clients[i].fd = 0;
            clients[i].named = 0;
            clients[i].name[0] = '\0';
            clients[i].status[0] = '\0';
            return;
        }
    }
}

client_t *get_client_by_fd(int fd){
    int i;

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == fd) {
            return &clients[i];
        }
    }

    return NULL;
}

client_t *get_client_by_name(const char *name) {
    int i;

    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != 0 && strcmp(clients[i].name, name) == 0) {
            return &clients[i];
        }
    }
    return NULL;
}

client_t *get_client_at(int index) {
    if (index < 0 || index >= MAX_CLIENTS) {
        return NULL;
    }
    return &clients[index];
}

int is_name_taken(const char *name) {
    return get_client_by_name(name) != NULL;
}
