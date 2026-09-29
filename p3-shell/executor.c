#include "mysh.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

static void print_status(int status, int interactive) {
    char buffer[128];

    if (!interactive) {
        return;
    }

    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) != 0) {
            snprintf(buffer, sizeof(buffer), "Exited with status %d\n", WEXITSTATUS(status));
            write_str(STDOUT_FILENO, buffer);
        }
    }
    else if (WIFSIGNALED(status)) {
        snprintf(buffer, sizeof(buffer), "Terminated by signal %d\n", WTERMSIG(status));
        write_str(STDOUT_FILENO, buffer);
    }
}

static int open_input_file(char *name) {
    int fd;

    fd = open(name, O_RDONLY);
    if (fd < 0) {
        perror(name);
    }

    return fd;
}

static int open_output_file(char *name) {
    int fd;

    fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (fd < 0) {
        perror(name);
    }

    return fd;
}

static int open_batch_stdin(void) {
    int fd;

    fd = open("/dev/null", O_RDONLY);
    if (fd < 0) {
        perror("/dev/null");
    }

    return fd;
}

static void child_run_command(command_t *cmd, int in_fd, int out_fd) {
    char *path;
    int status;

    signal(SIGINT, SIG_DFL);

    if (in_fd != STDIN_FILENO) {
        if (dup2(in_fd, STDIN_FILENO) < 0) {
            perror("dup2");
            _exit(1);
        }
        close(in_fd);
    }

    if (out_fd != STDOUT_FILENO) {
        if (dup2(out_fd, STDOUT_FILENO) < 0) {
            perror("dup2");
            _exit(1);
        }
        close(out_fd);
    }

    if (is_builtin(cmd->argv[0])) {
        status = run_builtin(cmd->argv, NULL, STDOUT_FILENO);
        _exit(status);
    }

    path = resolve_program(cmd->argv[0]);
    if (path == NULL) {
        write_str(STDERR_FILENO, cmd->argv[0]);
        write_str(STDERR_FILENO, ": command not found\n");
        _exit(1);
    }

    execv(path, cmd->argv);
    perror(cmd->argv[0]);
    free(path);
    _exit(1);
}

int run_external(char **argv, int in_fd, int out_fd, int interactive) {
    pid_t pid;
    int status;
    command_t cmd;
    int i;

    cmd.input_file = NULL;
    cmd.output_file = NULL;

    for (i = 0; i < MAX_ARGS; i++) {
        cmd.argv[i] = argv[i];
        if (argv[i] == NULL) {
            break;
        }
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        child_run_command(&cmd, in_fd, out_fd);
    }

    if (in_fd != STDIN_FILENO) {
        close(in_fd);
    }
    if (out_fd != STDOUT_FILENO) {
        close(out_fd);
    }

    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    print_status(status, interactive);

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return 0;
    }

    return 1;
}

static int run_builtin_parent(command_t *cmd, int *should_exit, int interactive) {
    int in_fd;
    int out_fd;
    int saved_stdin;
    int saved_stdout;
    int result;
    int status;

    in_fd = STDIN_FILENO;
    out_fd = STDOUT_FILENO;
    saved_stdin = -1;
    saved_stdout = -1;

    if (cmd->input_file != NULL) {
        in_fd = open_input_file(cmd->input_file);
        if (in_fd < 0) {
            return 1;
        }
    }

    if (cmd->output_file != NULL) {
        out_fd = open_output_file(cmd->output_file);
        if (out_fd < 0) {
            if (in_fd != STDIN_FILENO) {
                close(in_fd);
            }
            return 1;
        }
    }

    if (in_fd != STDIN_FILENO) {
        saved_stdin = dup(STDIN_FILENO);
        if (saved_stdin < 0 || dup2(in_fd, STDIN_FILENO) < 0) {
            perror("dup2");
            if (saved_stdin >= 0) {
                close(saved_stdin);
            }
            close(in_fd);
            if (out_fd != STDOUT_FILENO) {
                close(out_fd);
            }
            return 1;
        }
        close(in_fd);
    }

    if (out_fd != STDOUT_FILENO) {
        saved_stdout = dup(STDOUT_FILENO);
        if (saved_stdout < 0 || dup2(out_fd, STDOUT_FILENO) < 0) {
            perror("dup2");
            if (saved_stdout >= 0) {
                close(saved_stdout);
            }
            close(out_fd);
            if (saved_stdin != -1) {
                dup2(saved_stdin, STDIN_FILENO);
                close(saved_stdin);
            }
            return 1;
        }
        close(out_fd);
    }

    result = run_builtin(cmd->argv, should_exit, STDOUT_FILENO);

    if (saved_stdin != -1) {
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);
    }

    if (saved_stdout != -1) {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }

    if (result == 0) {
        status = 0;
    }
    else {
        status = 1 << 8;
    }

    print_status(status, interactive);
    return result;
}

static int execute_single(command_t *cmd, int *should_exit, int interactive) {
    int in_fd;
    int out_fd;
    int result;

    if (cmd->argv[0] == NULL) {
        return 1;
    }

    if (expand_wildcards(cmd) != 0) {
        return 1;
    }

    if (is_builtin(cmd->argv[0])) {
        result = run_builtin_parent(cmd, should_exit, interactive);
        free_expanded_wildcards(cmd);
        return result;
    }

    in_fd = STDIN_FILENO;
    out_fd = STDOUT_FILENO;

    if (cmd->input_file != NULL) {
        in_fd = open_input_file(cmd->input_file);
        if (in_fd < 0) {
            free_expanded_wildcards(cmd);
            return 1;
        }
    }
    else if (!interactive) {
        in_fd = open_batch_stdin();
        if (in_fd < 0) {
            free_expanded_wildcards(cmd);
            return 1;
        }
    }

    if (cmd->output_file != NULL) {
        out_fd = open_output_file(cmd->output_file);
        if (out_fd < 0) {
            if (in_fd != STDIN_FILENO) {
                close(in_fd);
            }
            free_expanded_wildcards(cmd);
            return 1;
        }
    }

    result = run_external(cmd->argv, in_fd, out_fd, interactive);
    free_expanded_wildcards(cmd);
    return result;
}

static void free_pipeline_wildcards(job_t *job, int count) {
    int i;

    for (i = 0; i < count; i++) {
        free_expanded_wildcards(&job->commands[i]);
    }
}

static int execute_pipeline(job_t *job, int *should_exit, int interactive) {
    int num;
    int pipes[MAX_TOKENS][2];
    pid_t pids[MAX_TOKENS];
    int i;
    int j;
    int in_fd;
    int out_fd;
    int status;
    int last_status;

    num = job->num_commands;
    last_status = 1 << 8;

    for (i = 0; i < num; i++) {
        if (job->commands[i].argv[0] == NULL) {
            free_pipeline_wildcards(job, i);
            return 1;
        }

        if (expand_wildcards(&job->commands[i]) != 0) {
            free_pipeline_wildcards(job, i);
            return 1;
        }
    }

    for (i = 0; i < num - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            for (j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            free_pipeline_wildcards(job, num);
            return 1;
        }
    }

    for (i = 0; i < num; i++) {
        if (i == 0) {
            if (job->commands[i].input_file != NULL) {
                in_fd = open_input_file(job->commands[i].input_file);
                if (in_fd < 0) {
                    for (j = 0; j < num - 1; j++) {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }
                    free_pipeline_wildcards(job, num);
                    return 1;
                }
            }
            else if (!interactive) {
                in_fd = open_batch_stdin();
                if (in_fd < 0) {
                    for (j = 0; j < num - 1; j++) {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }
                    free_pipeline_wildcards(job, num);
                    return 1;
                }
            }
            else {
                in_fd = STDIN_FILENO;
            }
        } 
        else {
            in_fd = pipes[i - 1][0];
        }

        if (i == num - 1) {
            if (job->commands[i].output_file != NULL) {
                out_fd = open_output_file(job->commands[i].output_file);
                if (out_fd < 0) {
                    if (in_fd != STDIN_FILENO) {
                        close(in_fd);
                    }
                    for (j = 0; j < num - 1; j++) {
                        if (j != i - 1) {
                            close(pipes[j][0]);
                        }
                        close(pipes[j][1]);
                    }
                    free_pipeline_wildcards(job, num);
                    return 1;
                }
            }
            else {
                out_fd = STDOUT_FILENO;
            }
        }
        else{
            out_fd = pipes[i][1];
        }

        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            if (in_fd != STDIN_FILENO) {
                close(in_fd);
            }
            if (out_fd != STDOUT_FILENO) {
                close(out_fd);
            }
            for (j = 0; j < num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            free_pipeline_wildcards(job, num);
            return 1;
        }

        if (pids[i] == 0) {
            for (j = 0; j < num - 1; j++) {
                if (j != i - 1) {
                    close(pipes[j][0]);
                }
                if (j != i) {
                    close(pipes[j][1]);
                }
            }

            child_run_command(&job->commands[i], in_fd, out_fd);
        }

        if (in_fd != STDIN_FILENO) {
            close(in_fd);
        }
        if (out_fd != STDOUT_FILENO) {
            close(out_fd);
        }
    }

    for (i = 0; i < num; i++) {
        if (waitpid(pids[i], &status, 0) < 0) {
            perror("waitpid");
            free_pipeline_wildcards(job, num);
            return 1;
        }

        if (i == num - 1) {
            last_status = status;
        }
    }

    for (i = 0; i < num; i++) {
        if (job->commands[i].argv[0] != NULL &&
            strcmp(job->commands[i].argv[0], "exit") == 0) {
            if (should_exit != NULL) {
                *should_exit = 1;
            }
        }
    }

    free_pipeline_wildcards(job, num);
    print_status(last_status, interactive);

    if (WIFEXITED(last_status) && WEXITSTATUS(last_status) == 0) {
        return 0;
    }

    return 1;
}

int execute_job(job_t *job, int *should_exit, int interactive) {
    if (job == NULL || job->num_commands <= 0) {
        return 1;
    }

    if (job->num_commands == 1) {
        return execute_single(&job->commands[0], should_exit, interactive);
    }

    return execute_pipeline(job, should_exit, interactive);
}
