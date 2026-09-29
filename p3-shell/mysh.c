#include "mysh.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>

static void print_prompt(void){
    char cwd[PATH_MAX];
    char prompt[PATH_MAX + 10];
    char *home;
    size_t home_len;

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        write_str(STDOUT_FILENO, "$ ");
        return;
    }

    home = getenv("HOME");
    if (home != NULL) {
        home_len = strlen(home);

        if (strncmp(cwd, home, home_len) == 0) {
            if (cwd[home_len] == '\0') {
                snprintf(prompt, sizeof(prompt), "~$ ");
                write_str(STDOUT_FILENO, prompt);
                return;
            }

            if (cwd[home_len] == '/') {
                snprintf(prompt, sizeof(prompt), "~%s$ ", cwd + home_len);
                write_str(STDOUT_FILENO, prompt);
                return;
            }
        }
    }

    snprintf(prompt, sizeof(prompt), "%s$ ", cwd);
    write_str(STDOUT_FILENO, prompt);
}
static int read_command(int fd, char *line, size_t size) {
    static char buffer[READ_BUF_SIZE];
    static ssize_t buf_len = 0;
    static ssize_t buf_pos = 0;
    ssize_t nread;
    size_t line_len;
    ssize_t i;

    line_len = 0;

    while (1) {
        if (buf_pos >= buf_len) {
            nread = read(fd, buffer, sizeof(buffer));
            if (nread < 0) {
                return -1;
            }
            if (nread == 0) {
                if (line_len == 0) {
                    return 0;
                }
                line[line_len] = '\0';
                return 1;
            }
            buf_len = nread;
            buf_pos = 0;
        }

        for (i = buf_pos; i < buf_len; i++) {
            if (buffer[i] == '\n') {
                line[line_len] = '\0';
                buf_pos = i + 1;
                return 1;
            }

            if (line_len < size - 1) {
                line[line_len] = buffer[i];
                line_len++;
            }
        }

        buf_pos = buf_len;
    }
}

int main(int argc, char *argv[]) {
    int fd;
    int interactive;
    int should_exit;
    int result;
    char line[LINE_BUF_SIZE];
    char *tokens[MAX_TOKENS];
    int num_tokens;
    job_t *job;

    if (argc > 2) {
        write_str(STDERR_FILENO, "Usage: ./mysh [input_file]\n");
        return EXIT_FAILURE;
    }

    if (argc == 2) {
        fd = open(argv[1], O_RDONLY);
        if (fd < 0) {
            perror(argv[1]);
            return EXIT_FAILURE;
        }
        interactive = 0;
    }
    else{
        fd = STDIN_FILENO;
        interactive = isatty(STDIN_FILENO);
    }

    if (interactive) {
        signal(SIGINT, SIG_IGN);
    }

    should_exit = 0;

    if (interactive){
        write_str(STDOUT_FILENO, "Hi! This is my shell!\n");
    }

    while (!should_exit) {
        if (interactive) {
            print_prompt();
        }

        result = read_command(fd, line, sizeof(line));
        if (result < 0) {
            perror("read");
            if (argc == 2) {
                close(fd);
            }
            return EXIT_FAILURE;
        }

        if (result == 0) {
            break;
        }

        num_tokens = tokenize(line, tokens);
        if (num_tokens == 0) {
            continue;
        }

        job = parse_tokens(tokens, num_tokens);
        if (job == NULL) {
            continue;
        }

        execute_job(job, &should_exit, interactive);
        free_job(job);
    }

    if (interactive) {
        write_str(STDOUT_FILENO, "We are now exiting my shell. Adios!\n");
    }

    if (argc == 2) {
        close(fd);
    }

    return EXIT_SUCCESS;
}
