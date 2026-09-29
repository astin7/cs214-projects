#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "traverse.h"
#include "tokenizer.h"
#include "compare.h"

#define SUFFIX ".txt"

static int is_hidden_name(const char *path){
    const char *name = strrchr(path, '/');
    if (name == NULL) {
        name = path;
    }
    else{
        name++;
    }

    return name[0] == '.';
}

static int has_suffix(const char *name, const char *suffix)
{
    int name_len = strlen(name);
    int suffix_len = strlen(suffix);

    if (name_len < suffix_len) {
        return 0;
    }

    return strcmp(name + name_len - suffix_len, suffix) == 0;
}

static void add_file(const char *path)
{
    word_list_t *list = malloc(sizeof(word_list_t));
    if (list == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    wl_init(list, path);
    process_file(path, list);
    add_file_distribution(list);
}

static void traverse_directory(const char *path){
    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror(path);
        return;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        int path_len = strlen(path);
        int name_len = strlen(entry->d_name);

        char *child_path = malloc(path_len + name_len + 2);
        if (child_path == NULL) {
            perror("malloc");
            closedir(dir);
            exit(EXIT_FAILURE);
        }

        strcpy(child_path, path);
        child_path[path_len] = '/';
        strcpy(child_path + path_len + 1, entry->d_name);

        struct stat sb;
        if (stat(child_path, &sb) < 0) {
            perror(child_path);
            free(child_path);
            continue;
        }

        if (S_ISDIR(sb.st_mode)) {
            traverse_directory(child_path);
        }
        else if (S_ISREG(sb.st_mode) && has_suffix(entry->d_name, SUFFIX)) {
            add_file(child_path);
        }

        free(child_path);
    }

    closedir(dir);
}

void traverse_path(const char *path)
{
    struct stat sb;

    if (is_hidden_name(path)) {
        return;
    }

    if (stat(path, &sb) < 0) {
        perror(path);
        return;
    }

    if (S_ISREG(sb.st_mode)) {
        add_file(path);
    }
    else if (S_ISDIR(sb.st_mode)){
        traverse_directory(path);
    }
}
