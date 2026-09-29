#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "compare.h"

typedef struct file_node {
    word_list_t *list;
    struct file_node *next;
} file_node_t;

typedef struct {
    word_list_t *a;
    word_list_t *b;
    int combined_words;
    double jsd;
} comparison_t;

static file_node_t *all_files = NULL;

static int get_count(word_list_t *list, const char *word){
    word_node_t *curr = list->head;
    while (curr != NULL) {
        if (strcmp(curr->word, word) == 0){
            return curr->count;
        }
        curr = curr->next;
    }

    return 0;
}

static double get_freq(word_list_t *list, const char *word)
{
    int count = get_count(list, word);

    if (list->total_words == 0) {
        return 0.0;
    }

    return (double) count / (double) list->total_words;
}

static double kld_contribution(double fw, double mean)
{
    if (fw == 0.0) {
        return 0.0;
    }

    return fw * log2(fw / mean);
}

static double compare_two_files(word_list_t *a, word_list_t *b)
{
    double kld_a = 0.0;
    double kld_b = 0.0;
    word_node_t *curr;

    curr = a->head;
    while (curr != NULL) {
        double fa = (double) curr->count / (double) a->total_words;
        double fb = get_freq(b, curr->word);
        double mean = (fa + fb) / 2.0;

        kld_a += kld_contribution(fa, mean);
        kld_b += kld_contribution(fb, mean);

        curr = curr->next;
    }

    curr = b->head;
    while (curr != NULL) {
        if (get_count(a, curr->word) == 0) {
            double fa = 0.0;
            double fb = (double) curr->count / (double) b->total_words;
            double mean = (fa + fb) / 2.0;

            kld_a += kld_contribution(fa, mean);
            kld_b += kld_contribution(fb, mean);
        }

        curr = curr->next;
    }

    return sqrt(0.5 * kld_a + 0.5 * kld_b);
}

static int comparison_cmp(const void *x, const void *y)
{
    const comparison_t *a = x;
    const comparison_t *b = y;
    if (a->combined_words < b->combined_words) {
        return 1;
    }
    if (a->combined_words > b->combined_words) {
        return -1;
    }

    {
        int first = strcmp(a->a->path, b->a->path);
        if (first != 0) {
            return first;
        }
    }

    return strcmp(a->b->path, b->b->path);
}

static void destroy_all_files(void)
{
    file_node_t *curr = all_files;
    file_node_t *next;

    while (curr != NULL) {
        next = curr->next;
        wl_destroy(curr->list);
        free(curr->list);
        free(curr);
        curr = next;
    }

    all_files = NULL;
}

void add_file_distribution(word_list_t *list)
{
    file_node_t *node = malloc(sizeof(file_node_t));
    if (node == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    node->list = list;
    node->next = all_files;
    all_files = node;
}

void compare_all_files(void){
    int n = 0;
    file_node_t *curr = all_files;

    while (curr != NULL) {
        n++;
        curr = curr->next;
    }

    if (n < 2) {
        fprintf(stderr, "Error: fewer than two files to compare\n");
        destroy_all_files();
        exit(EXIT_FAILURE);
    }

    word_list_t **files = malloc(n * sizeof(word_list_t *));
    if (files == NULL) {
        perror("malloc");
        destroy_all_files();
        exit(EXIT_FAILURE);
    }

    curr = all_files;
    int i = 0;
    while (curr != NULL) {
        files[i] = curr->list;
        i++;
        curr = curr->next;
    }

    int num_pairs = n * (n - 1) / 2;
    comparison_t *results = malloc(num_pairs * sizeof(comparison_t));
    if (results == NULL) {
        perror("malloc");
        free(files);
        destroy_all_files();
        exit(EXIT_FAILURE);
    }

    int k = 0;
    int a, b;
    for (a = 0; a < n; a++) {
        for (b = a + 1; b < n; b++) {
            word_list_t *first = files[a];
            word_list_t *second = files[b];

            if (strcmp(first->path, second->path) > 0) {
                word_list_t *temp = first;
                first = second;
                second = temp;
            }

            results[k].a = first;
            results[k].b = second;
            results[k].combined_words = first->total_words + second->total_words;
            results[k].jsd = compare_two_files(first, second);
            k++;
        }
    }

    qsort(results, num_pairs, sizeof(comparison_t), comparison_cmp);

    for (i = 0; i < num_pairs; i++) {
        printf("%.5f %s %s\n",
               results[i].jsd,
               results[i].a->path,
               results[i].b->path);
    }

    free(results);
    free(files);
    destroy_all_files();
}
