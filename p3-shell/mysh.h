#ifndef MYSH_H
#define MYSH_H

#include <unistd.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_TOKENS 100
#define MAX_ARGS 100
#define READ_BUF_SIZE 1024
#define LINE_BUF_SIZE 4096

typedef struct {
    char *argv[MAX_ARGS];
    char *input_file;
    char *output_file;
} command_t;

typedef struct {
    command_t commands[MAX_TOKENS];
    int num_commands;
} job_t;

int tokenize(char *line, char **tokens);
job_t *parse_tokens(char **tokens, int num_tokens);
void free_job(job_t *job);

int execute_job(job_t *job, int *should_exit, int interactive);

int is_builtin(char *cmd);
int run_builtin(char **argv, int *should_exit, int out_fd);

char *resolve_program(char *name);
int run_external(char **argv, int in_fd, int out_fd, int interactive);

int expand_wildcards(command_t *cmd);
void free_expanded_wildcards(command_t *cmd);

void write_str(int fd, const char *s);
char *copy_string(const char *s);

#endif
