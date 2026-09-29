#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wordlist.h"

void wl_init(word_list_t *list, const char *path)
{
    list->head = NULL;
    list->total_words = 0;
    list->unique_words = 0;

    if (path != NULL) {
        list->path = malloc(strlen(path) + 1);
        if (list->path == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        strcpy(list->path, path);
    }
    else {
        list->path = NULL;
    }
}

word_node_t *wl_find(word_list_t *list, const char *word)
{
    word_node_t *curr = list->head;

    while (curr != NULL) {
        if (strcmp(curr->word, word) == 0) {
            return curr;
        }
        curr = curr->next;
    }

    return NULL;
}

void wl_add(word_list_t *list, const char *word)
{
    word_node_t *node = wl_find(list, word);

    list->total_words++;

    if (node != NULL) {
        node->count++;
        return;
    }

    node = malloc(sizeof(word_node_t));
    if (node == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    node->word = malloc(strlen(word) + 1);
    if (node->word == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    strcpy(node->word, word);
    node->count = 1;
    node->next = list->head;
    list->head = node;

    list->unique_words++;
}

void wl_destroy(word_list_t *list)
{
    word_node_t *curr = list->head;
    word_node_t *next;

    while (curr != NULL) {
        next = curr->next;
        free(curr->word);
        free(curr);
        curr = next;
    }

    free(list->path);

    list->head = NULL;
    list->path = NULL;
    list->total_words = 0;
    list->unique_words = 0;
}
