#include "mysh.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void write_err(const char *s) {
    write(STDERR_FILENO, s, strlen(s));
}

static int is_special(char c){
    return c == '<' || c == '>' || c == '|';
}

static void init_command(command_t *cmd) {
    int i;
    cmd->input_file = NULL;
    cmd->output_file = NULL;

    for (i = 0; i < MAX_ARGS; i++) {
        cmd->argv[i] = NULL;
    }
}

int tokenize(char *line, char **tokens) {
    int count;
    char *p;
    char *start;

    count = 0;
    p = line;

    while (*p != '\0'){
        while (*p == ' ' || *p == '\t' || *p == '\n') {
            p++;
        }

        if (*p == '\0') {
            break;
        }

        if (*p == '#') {
            break;
        }

        if (count >= MAX_TOKENS) {
            break;
        }

        if (is_special(*p)) {
            tokens[count] = p;
            count++;

            p++;
            if (*p != '\0') {
                *p = '\0';
                p++;
            }
            continue;
        }

        start = p;
        while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n' && *p != '#' && !is_special(*p)) {
            p++;
        }

        tokens[count] = start;
        count++;

        if (*p == '#') {
            *p = '\0';
            break;
        }

        if (is_special(*p)) {
            *p = '\0';
            continue;
        }

        if (*p != '\0') {
            *p = '\0';
            p++;
        }
    }
    return count;
}

job_t *parse_tokens(char **tokens, int num_tokens) {
    job_t *job;
    int i;
    int cmd_index;
    int arg_index;

    if (num_tokens == 0) {
        return NULL;
    }

    job = malloc(sizeof(job_t));
    if (job == NULL) {
        write_err("malloc failed\n");
        return NULL;
    }

    for (i = 0; i < MAX_TOKENS; i++) {
        init_command(&job->commands[i]);
    }

    job->num_commands = 1;
    cmd_index = 0;
    arg_index = 0;

    for (i = 0; i < num_tokens; i++) {
        if (strcmp(tokens[i], "|") == 0) {
            if (arg_index == 0) {
                write_err("syntax error\n");
                free(job);
                return NULL;
            }

            job->commands[cmd_index].argv[arg_index] = NULL;
            cmd_index++;

            if (cmd_index >= MAX_TOKENS) {
                write_err("too many commands\n");
                free(job);
                return NULL;
            }

            job->num_commands++;
            arg_index = 0;
            continue;
        }

        if (strcmp(tokens[i], "<") == 0) {
            if (i + 1 >= num_tokens) {
                write_err("syntax error\n");
                free(job);
                return NULL;
            }

            if (job->commands[cmd_index].input_file != NULL) {
                write_err("syntax error\n");
                free(job);
                return NULL;
            }

            i++;
            if (strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0 ||
                strcmp(tokens[i], "|") == 0) {
                write_err("syntax error\n");
                free(job);
                return NULL;
            }

            job->commands[cmd_index].input_file = tokens[i];
            continue;
        }

        if (strcmp(tokens[i], ">") == 0) {
            if (i + 1 >= num_tokens) {
                write_err("syntax error\n");
                free(job);
                return NULL;
            }

            if (job->commands[cmd_index].output_file != NULL) {
                write_err("syntax error\n");
                free(job);
                return NULL;
            }

            i++;
            if (strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0 ||
                strcmp(tokens[i], "|") == 0) {
                write_err("syntax error\n");
                free(job);
                return NULL;
            }

            job->commands[cmd_index].output_file = tokens[i];
            continue;
        }

        if (arg_index >= MAX_ARGS - 1) {
            write_err("too many arguments\n");
            free(job);
            return NULL;
        }

        job->commands[cmd_index].argv[arg_index] = tokens[i];
        arg_index++;
    }

    if (arg_index == 0) {
        write_err("syntax error\n");
        free(job);
        return NULL;
    }

    job->commands[cmd_index].argv[arg_index] = NULL;
    return job;
}

void free_job(job_t *job){
    if (job != NULL) {
        free(job);
    }
}
