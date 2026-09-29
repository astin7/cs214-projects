#ifndef WORDLIST_H
#define WORDLIST_H

typedef struct word_node {
    char *word;
    int count;
    struct word_node *next;
} word_node_t;

typedef struct {
    word_node_t *head;
    int total_words;
    int unique_words;
    char *path;
} word_list_t;

void wl_init(word_list_t *list, const char *path);

word_node_t *wl_find(word_list_t *list, const char *word);

void wl_add(word_list_t *list, const char *word);

void wl_destroy(word_list_t *list);

#endif
