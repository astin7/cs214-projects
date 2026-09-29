#include "chatd.h"

static int send_err(int fd, int code, const char *text) {
    char body[1024];
    char msg[1200];
    int body_len;
    int msg_len;

    snprintf(body, sizeof(body), "%d|%s|", code, text);
    body_len = strlen(body);
    msg_len = snprintf(msg, sizeof(msg), "1|ERR|%d|%s", body_len, body);

    if (msg_len < 0 || msg_len >= (int)sizeof(msg)) {
        return -1;
    }
    return write_all(fd, msg, msg_len);
}

static int send_msg(int fd, const char *sender, const char *recipient, const char *text) {
    char body[16384];
    char msg[17000];
    int body_len;
    int msg_len;

    snprintf(body, sizeof(body), "%s|%s|%s|", sender, recipient, text);
    body_len = strlen(body);
    msg_len = snprintf(msg, sizeof(msg), "1|MSG|%d|%s", body_len, body);

    if (msg_len < 0 || msg_len >= (int)sizeof(msg)) {
        return -1;
    }
    return write_all(fd, msg, msg_len);
}

static int split_prefix(const char *msg, char *version, char *code, int *body_len, const char **body_start) {
    const char *p1;
    const char *p2;
    const char *p3;
    char lenbuf[16];
    int i;

    p1 = strchr(msg, '|');
    if (p1 == NULL) {
        return -1;
    }

    p2 = strchr(p1 + 1, '|');
    if (p2 == NULL) {
        return -1;
    }

    p3 = strchr(p2 + 1, '|');
    if (p3 == NULL) {
        return -1;
    }

    if (p1 - msg >= 16) {
        return -1;
    }
    strncpy(version, msg, p1 - msg);
    version[p1 - msg] = '\0';

    if (p2 - (p1 + 1) != 3) {
        return -1;
    }
    strncpy(code, p1 + 1, 3);
    code[3] = '\0';

    if (p3 - (p2 + 1) <= 0 || p3 - (p2 + 1) >= (int)sizeof(lenbuf)) {
        return -1;
    }
    strncpy(lenbuf, p2 + 1, p3 - (p2 + 1));
    lenbuf[p3 - (p2 + 1)] = '\0';

    for (i = 0; lenbuf[i] != '\0'; i++) {
        if (lenbuf[i] < '0' || lenbuf[i] > '9') {
            return -1;
        }
    }

    *body_len = atoi(lenbuf);
    *body_start = p3 + 1;

    return 0;
}

static int parse_one_field(const char *body, char *field, int field_size) {
    size_t len;

    len = strlen(body);
    if (len < 1 || body[len - 1] != '|') {
        return -1;
    }

    if ((int)(len - 1) >= field_size) {
        return -1;
    }

    strncpy(field, body, len - 1);
    field[len - 1] = '\0';
    return 0;
}

static int parse_msg_fields(const char *body, char *sender, char *recipient, char *text) {
    const char *p1;
    const char *p2;
    size_t text_len;

    p1 = strchr(body, '|');
    if (p1 == NULL) {
        return -1;
    }

    p2 = strchr(p1 + 1, '|');
    if (p2 == NULL) {
        return -1;
    }

    if (p1 - body >= MAX_NAME_LEN + 2) {
        return -1;
    }
    strncpy(sender, body, p1 - body);
    sender[p1 - body] = '\0';

    if (p2 - (p1 + 1) >= MAX_NAME_LEN + 2) {
        return -1;
    }
    strncpy(recipient, p1 + 1, p2 - (p1 + 1));
    recipient[p2 - (p1 + 1)] = '\0';

    text_len = strlen(p2 + 1);
    if (text_len < 1 || (p2 + 1)[text_len - 1] != '|') {
        return -1;
    }

    if ((int)(text_len - 1) > MAX_MSG_LEN) {
        return -2;
    }

    strncpy(text, p2 + 1, text_len - 1);
    text[text_len - 1] = '\0';

    return 0;
}

static int client_is_named(client_t *client) {
    return client != NULL && client->named;
}

static int handle_nam(client_t *client, const char *body) {
    char name[MAX_BUFFER];

    if (client == NULL) {
        return -1;
    }

    if (client->named) {
        send_err(client->fd, ERR_BAD_STATE, "Already named");
        return 0;
    }

    if (parse_one_field(body, name, sizeof(name)) < 0) {
        send_err(client->fd, ERR_UNREADABLE, "Unreadable");
        return -1;
    }

    if ((int)strlen(name) > MAX_NAME_LEN) {
        send_err(client->fd, ERR_TOO_LONG, "Too long");
        return 0;
    }

    if (!valid_name(name)) {
        send_err(client->fd, ERR_ILLEGAL_CHAR, "Illegal character");
        return 0;
    }

    if (is_name_taken(name)) {
        send_err(client->fd, ERR_NAME_IN_USE, "Name in use");
        return 0;
    }

    strcpy(client->name, name);
    client->named = 1;
    client->status[0] = '\0';

    send_msg(client->fd, "#all", client->name, "Welcome to the chat!");
    return 0;
}

static int handle_set(client_t *client, const char *body) {
    char status[MAX_BUFFER];
    char announcement[256];
    int len;
    int i;

    if (client == NULL) {
        return -1;
    }

    if (!client_is_named(client)) {
        send_err(client->fd, ERR_BAD_STATE, "Choose a name first");
        return 0;
    }

    if (parse_one_field(body, status, sizeof(status)) < 0) {
        send_err(client->fd, ERR_UNREADABLE, "Unreadable");
        return -1;
    }

    len = strlen(status);
    if (len > MAX_STATUS_LEN) {
        send_err(client->fd, ERR_TOO_LONG, "Too long");
        return 0;
    }

    if (!valid_text(status, MAX_STATUS_LEN)) {
        send_err(client->fd, ERR_ILLEGAL_CHAR, "Illegal character");
        return 0;
    }

    strcpy(client->status, status);

    if (len > 0) {
        snprintf(announcement, sizeof(announcement), "%s is now \"%s\"", client->name, client->status);

        for (i = 0; i < MAX_CLIENTS; i++) {
            client_t *other = get_client_at(i);
            if (other != NULL && other->fd != 0 && other->named) {
                send_msg(other->fd, "#all", "#all", announcement);
            }
        }
    }
    return 0;
}

static int handle_who(client_t *client, const char *body) {
    char target[64];
    char response[16384];
    int i;
    int first;

    if (client == NULL){
        return -1;
    }

    if (!client_is_named(client)) {
        send_err(client->fd, ERR_BAD_STATE, "Choose a name first");
        return 0;
    }

    if (parse_one_field(body, target, sizeof(target)) < 0) {
        send_err(client->fd, ERR_UNREADABLE, "Unreadable");
        return -1;
    }

    if (strcmp(target, "#all") == 0) {
        response[0] = '\0';
        first = 1;

        for (i = 0; i < MAX_CLIENTS; i++) {
            client_t *other = get_client_at(i);
            if (other != NULL && other->fd != 0 && other->named) {
                if (!first) {
                    strcat(response, "\n");
                }

                strcat(response, other->name);
                if (strlen(other->status) > 0) {
                    strcat(response, ": ");
                    strcat(response, other->status);
                }

                first = 0;
            }
        }

        send_msg(client->fd, "#all", client->name, response);
        return 0;
    }
    else {
        client_t *target_client = get_client_by_name(target);
        if (target_client == NULL || !target_client->named) {
            send_err(client->fd, ERR_UNKNOWN_RECIPIENT, "Unknown recipient");
            return 0;
        }

        if (strlen(target_client->status) > 0) {
            snprintf(response, sizeof(response), "%s: %s", target_client->name, target_client->status);
        }
        else {
            snprintf(response, sizeof(response), "No status");
        }

        send_msg(client->fd, "#all", client->name, response);
        return 0;
    }
}

static int handle_msg_type(client_t *client, const char *body) {
    char sender[MAX_NAME_LEN + 1];
    char recipient[MAX_NAME_LEN + 2];
    char text[MAX_MSG_LEN + 1];
    int result;
    int i;

    if (client == NULL) {
        return -1;
    }

    if (!client_is_named(client)) {
        send_err(client->fd, ERR_BAD_STATE, "Choose a name first");
        return 0;
    }

    result = parse_msg_fields(body, sender, recipient, text);
    if (result == -1) {
        send_err(client->fd, ERR_UNREADABLE, "Unreadable");
        return -1;
    }
    if (result == -2) {
        send_err(client->fd, ERR_TOO_LONG, "Too long");
        return 0;
    }

    if (strlen(text) < 1) {
        send_err(client->fd, ERR_TOO_LONG, "Too long");
        return 0;
    }

    if (!valid_text(text, MAX_MSG_LEN)) {
        send_err(client->fd, ERR_ILLEGAL_CHAR, "Illegal character");
        return 0;
    }

    if (strcmp(recipient, "#all") == 0) {
        for (i = 0; i < MAX_CLIENTS; i++) {
            client_t *other = get_client_at(i);
            if (other != NULL && other->fd != 0 && other->named) {
                send_msg(other->fd, client->name, "#all", text);
            }
        }
        return 0;
    }
    else {
        client_t *target = get_client_by_name(recipient);
        if (target == NULL || !target->named) {
            send_err(client->fd, ERR_UNKNOWN_RECIPIENT, "Unknown recipient");
            return 0;
        }

        send_msg(target->fd, client->name, target->name, text);
        return 0;
    }
}

int handle_message(int fd, const char *msg) {
    char version[16];
    char code[4];
    int body_len;
    const char *body;
    client_t *client;

    client = get_client_by_fd(fd);
    if (client == NULL) {
        return -1;
    }

    if (split_prefix(msg, version, code, &body_len, &body) < 0) {
        send_err(fd, ERR_UNREADABLE, "Unreadable");
        return -1;
    }

    if (strcmp(version, PROTOCOL_VERSION) != 0) {
        send_err(fd, ERR_UNREADABLE, "Unreadable");
        return -1;
    }

    if ((int)strlen(body) != body_len) {
        send_err(fd, ERR_UNREADABLE, "Unreadable");
        return -1;
    }

    if (body_len < 1 || body[body_len - 1] != '|') {
        send_err(fd, ERR_UNREADABLE, "Unreadable");
        return -1;
    }

    if (strcmp(code, "NAM") == 0) {
        return handle_nam(client, body);
    }
    else if (strcmp(code, "SET") == 0) {
        return handle_set(client, body);
    } 
    else if (strcmp(code, "WHO") == 0) {
        return handle_who(client, body);
    } 
    else if (strcmp(code, "MSG") == 0) {
        return handle_msg_type(client, body);
    } 
    else {
        send_err(fd, ERR_UNREADABLE, "Unreadable");
        return -1;
    }
}
