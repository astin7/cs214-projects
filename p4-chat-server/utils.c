#include "chatd.h"

int write_all(int fd, const char *buf, size_t len) {
    size_t total = 0;
    ssize_t n;

    while (total < len){
        n = write(fd, buf + total, len - total);
        if (n <= 0) {
            return -1;
        }
        total += n;
    }
    return 0;
}

int valid_name(const char *name) {
    int len = strlen(name);
    if (len < 1 || len > MAX_NAME_LEN) {
        return 0;
    }

    for (int i = 0; i < len; i++) {
        char c = name[i];
        if (!( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' )) {
            return 0;
        }
    }
    return 1;
}

int valid_text(const char *text, int max_len) {
    int len = strlen(text);
    if (len < 0 || len > max_len) {
        return 0;
}
    for (int i = 0; i < len; i++) {
        unsigned char c = text[i];
        if (c < 32 || c > 126) {
            return 0;
        }
    }
    return 1;
}
