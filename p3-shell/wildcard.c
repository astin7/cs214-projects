#include "mysh.h"

#include <dirent.h>
#include <stdlib.h>
#include <string.h>

static int has_wildcard(char *s) {
    if (s == NULL) {
        return 0;
    }

    while (*s != '\0') {
        if (*s == '*') {
            return 1;
        }
        s++;
    }
    return 0;
}

static int match_pattern(char *pattern, char *name){
    char *star;
    int prefix_len;
    int suffix_len;
    int name_len;

    star = strchr(pattern, '*');
    if (star == NULL) {
        return strcmp(pattern, name) == 0;
    }

    prefix_len = star - pattern;
    suffix_len = strlen(star + 1);
    name_len = strlen(name);

    if (name_len < prefix_len + suffix_len) {
        return 0;
    }

    if (prefix_len > 0) {
        if (strncmp(pattern, name, prefix_len) != 0) {
            return 0;
        }
    }

    if (suffix_len > 0) {
        if (strcmp(name + name_len - suffix_len, star + 1) != 0) {
            return 0;
        }
    }

    return 1;
}

static int split_pattern(char *token, char **dir_part, char **file_part) {
    char *slash;
    int dir_len;

    slash = strrchr(token, '/');

    if (slash == NULL) {
        *dir_part = copy_string(".");
        *file_part = copy_string(token);
    }
    else {
        dir_len = slash - token;

        if (dir_len == 0) {
            *dir_part = copy_string("/");
        }
        else {
            *dir_part = malloc(dir_len + 1);
            if (*dir_part == NULL) {
                return 0;
            }

            strncpy(*dir_part, token, dir_len);
            (*dir_part)[dir_len] = '\0';
        }

        *file_part = copy_string(slash + 1);
    }

    if (*dir_part == NULL || *file_part == NULL) {
        if (*dir_part != NULL) {
            free(*dir_part);
        }
        if (*file_part != NULL) {
            free(*file_part);
        }
        return 0;
    }

    return 1;
}

static char *join_path(char *dir_part, char *name) {
    char *result;

    if (strcmp(dir_part, ".") == 0) {
        return copy_string(name);
    }

    if (strcmp(dir_part, "/") == 0) {
        result = malloc(strlen(name) + 2);
        if (result == NULL) {
            return NULL;
        }

        result[0] = '/';
        strcpy(result + 1, name);
        return result;
    }

    result = malloc(strlen(dir_part) + strlen(name) + 2);
    if (result == NULL) {
        return NULL;
    }

    strcpy(result, dir_part);
    strcat(result, "/");
    strcat(result, name);

    return result;
}

static void sort_strings(char **arr, int count) {
    int i;
    int j;
    char *temp;

    for (i = 0; i < count - 1; i++) {
        for (j = i + 1; j < count; j++) {
            if (strcmp(arr[i], arr[j]) > 0) {
                temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
    }
}

static void free_strings(char **arr, int count) {
    int i;

    for (i = 0; i < count; i++) {
        free(arr[i]);
    }
}

int expand_wildcards(command_t *cmd) {
    char *new_argv[MAX_ARGS];
    int new_argc;
    int i;

    new_argc = 0;

    for (i = 0; i < MAX_ARGS; i++) {
        new_argv[i] = NULL;
    }

    for (i = 0; cmd->argv[i] != NULL; i++) {
        char *token;

        token = cmd->argv[i];

        if (!has_wildcard(token)) {
            if (new_argc >= MAX_ARGS - 1) {
                free_strings(new_argv, new_argc);
                return 1;
            }

            new_argv[new_argc] = copy_string(token);
            if (new_argv[new_argc] == NULL) {
                free_strings(new_argv, new_argc);
                return 1;
            }

            new_argc++;
        }
        else{
            char *dir_part;
            char *file_part;
            DIR *dir;
            struct dirent *entry;
            char *matches[MAX_ARGS];
            int match_count;
            int starts_with_star;
            int j;

            dir_part = NULL;
            file_part = NULL;
            match_count = 0;

            for (j = 0; j < MAX_ARGS; j++) {
                matches[j] = NULL;
            }

            if (!split_pattern(token, &dir_part, &file_part)) {
                free_strings(new_argv, new_argc);
                return 1;
            }

            dir = opendir(dir_part);
            if (dir == NULL) {
                if (new_argc >= MAX_ARGS - 1) {
                    free(dir_part);
                    free(file_part);
                    free_strings(new_argv, new_argc);
                    return 1;
                }

                new_argv[new_argc] = copy_string(token);
                free(dir_part);
                free(file_part);

                if (new_argv[new_argc] == NULL) {
                    free_strings(new_argv, new_argc);
                    return 1;
                }

                new_argc++;
                continue;
            }

            starts_with_star = (file_part[0] == '*');

            while ((entry = readdir(dir)) != NULL) {
                char *full_name;

                if (starts_with_star && entry->d_name[0] == '.') {
                    continue;
                }

                if (!match_pattern(file_part, entry->d_name)) {
                    continue;
                }

                if (match_count >= MAX_ARGS - 1) {
                    break;
                }

                full_name = join_path(dir_part, entry->d_name);
                if (full_name == NULL) {
                    closedir(dir);
                    free(dir_part);
                    free(file_part);
                    free_strings(matches, match_count);
                    free_strings(new_argv, new_argc);
                    return 1;
                }

                matches[match_count] = full_name;
                match_count++;
            }

            closedir(dir);
            free(dir_part);
            free(file_part);

            if (match_count == 0) {
                if (new_argc >= MAX_ARGS - 1) {
                    free_strings(new_argv, new_argc);
                    return 1;
                }

                new_argv[new_argc] = copy_string(token);
                if (new_argv[new_argc] == NULL) {
                    free_strings(new_argv, new_argc);
                    return 1;
                }

                new_argc++;
            }
            else {
                sort_strings(matches, match_count);

                for (j = 0; j < match_count; j++){
                    if (new_argc >= MAX_ARGS - 1) {
                        free_strings(matches + j, match_count - j);
                        free_strings(new_argv, new_argc);
                        return 1;
                    }

                    new_argv[new_argc] = matches[j];
                    new_argc++;
                }
            }
        }
    }

    new_argv[new_argc] = NULL;

    for (i = 0; i < MAX_ARGS; i++) {
        cmd->argv[i] = NULL;
    }

    for (i = 0; i <= new_argc; i++) {
        cmd->argv[i] = new_argv[i];
    }

    return 0;
}

void free_expanded_wildcards(command_t *cmd) {
    int i;

    for (i = 0; cmd->argv[i] != NULL; i++) {
        free(cmd->argv[i]);
        cmd->argv[i] = NULL;
    }
}
