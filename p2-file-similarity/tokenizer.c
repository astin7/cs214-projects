#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include "tokenizer.h"

#define BUFSIZE 256

static void add_char(char **word, int *len, int *cap, char ch)
{
    if (*len + 1 >= *cap) {
        if (*cap == 0){
            *cap = 16;
        }
        else {
            *cap *= 2;
        }

        *word = realloc(*word, *cap);
        if (*word == NULL) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
    }

    (*word)[*len] = ch;
    (*len)++;
}

static void finish_word(word_list_t *list, char **word, int *len){
    if (*len == 0){
        return;
    }

    (*word)[*len] = '\0';
    wl_add(list, *word);
    *len = 0;
}

void process_file(const char *path, word_list_t *list)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror(path);
        return;
    }

    char buf[BUFSIZE];
    char *word = NULL;
    int len = 0;
    int cap = 0;
    int bytes;
    int i;

    while ((bytes = read(fd, buf, BUFSIZE)) > 0) {
        for (i = 0; i < bytes; i++) {
            unsigned char ch = (unsigned char) buf[i];

            if (isspace(ch)) {
                finish_word(list, &word, &len);
            }
            else if (isalnum(ch) || ch == '-') {
                add_char(&word, &len, &cap, tolower(ch));
            }
            else {
            }
        }
    }

    if (bytes < 0) {
        perror(path);
    }

    finish_word(list, &word, &len);

    free(word);
    close(fd);
}
