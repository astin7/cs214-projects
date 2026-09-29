#include "chatd.h"

#include <arpa/inet.h>

#define BACKLOG 10

static int open_listener(const char *port) {
    struct addrinfo hints;
    struct addrinfo *list;
    struct addrinfo *info;
    int listener;
    int yes;
    int error;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    error = getaddrinfo(NULL, port, &hints, &list);
    if (error != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }

    for (info = list; info != NULL; info = info->ai_next) {
        listener = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        if (listener < 0){
            continue;
        }

        yes = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        if (bind(listener, info->ai_addr, info->ai_addrlen) == 0){
            break;
        }

        close(listener);
    }

    freeaddrinfo(list);

    if (info == NULL) {
        return -1;
    }

    if (listen(listener, BACKLOG) < 0) {
        close(listener);
        return -1;
    }
    return listener;
}

static int add_pollfd(struct pollfd *pfds, int *count, int fd) {
    if (*count >= MAX_CLIENTS + 1) {
        return -1;
    }

    pfds[*count].fd = fd;
    pfds[*count].events = POLLIN;
    pfds[*count].revents = 0;
    (*count)++;
    return 0;
}

static void remove_pollfd(struct pollfd *pfds, int *count, int index) {
    pfds[index] = pfds[*count - 1];
    (*count)--;
}

static void disconnect_client(struct pollfd *pfds, int *count, int index) {
    int fd = pfds[index].fd;
    close(fd);
    remove_client(fd);
    remove_pollfd(pfds, count, index);
}

int main(int argc, char *argv[]) {
    int listener;
    struct pollfd pfds[MAX_CLIENTS + 1];
    int nfds;
    int ready;
    int i;

    if (argc != 2) {
        fprintf(stderr, "Usage: ./chatd <port>\n");
        return EXIT_FAILURE;
    }

    listener = open_listener(argv[1]);
    if (listener < 0) {
        perror("open_listener");
        return EXIT_FAILURE;
    }

    pfds[0].fd = listener;
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;
    nfds = 1;

    while (1){
        ready = poll(pfds, nfds, -1);
        if (ready < 0) {
            perror("poll");
            close(listener);
            return EXIT_FAILURE;
        }

        for (i = 0; i < nfds; i++) {
            if (!(pfds[i].revents & POLLIN)) {
                continue;
            }

            if (pfds[i].fd == listener) {
                struct sockaddr_storage remote;
                socklen_t remote_len = sizeof(remote);
                int client_fd;

                client_fd = accept(listener, (struct sockaddr *)&remote, &remote_len);
                if (client_fd < 0) {
                    continue;
                }

                if (add_pollfd(pfds, &nfds, client_fd) < 0) {
                    close(client_fd);
                    continue;
                }

                add_client(client_fd);
            }
            else {
                char buffer[MAX_BUFFER + 1];
                ssize_t bytes_read;
                int result;

                bytes_read = read(pfds[i].fd, buffer, MAX_BUFFER);
                if (bytes_read <= 0) {
                    disconnect_client(pfds, &nfds, i);
                    i--;
                    continue;
                }

                buffer[bytes_read] = '\0';

                result = handle_message(pfds[i].fd, buffer);
                if (result < 0) {
                    disconnect_client(pfds, &nfds, i);
                    i--;
                }
            }
        }
    }

    close(listener);
    return EXIT_SUCCESS;
}
