#include "shell.h"


// Parse arguments and redirection
int parse_command(char** tokens, int start, int end,
                  char** argv, char** input_file,
                  char** output_file) {

    int argc = 0;

    *input_file = NULL;
    *output_file = NULL;

    for (int i = start; i < end; i++) {

        if (strcmp(tokens[i], "<") == 0) {

            if (i + 1 >= end || *input_file != NULL) {
                fprintf(stderr, "Invalid input redirection.\n");
                return -1;
            }

            *input_file = tokens[++i];
        }

        else if (strcmp(tokens[i], ">") == 0) {

            if (i + 1 >= end || *output_file != NULL) {
                fprintf(stderr, "Invalid output redirection.\n");
                return -1;
            }

            *output_file = tokens[++i];
        }

        else {
            argv[argc++] = tokens[i];
        }
    }

    argv[argc] = NULL;

    if (argc == 0) {
        fprintf(stderr, "Missing command.\n");
        return -1;
    }

    return argc;
}


// Apply redirection
int apply_redirection(char* input_file, char* output_file) {

    int fd;

    if (input_file != NULL) {

        fd = open(input_file, O_RDONLY);

        if (fd < 0) {
            perror("Input file");
            return -1;
        }

        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2");
            close(fd);
            return -1;
        }

        close(fd);
    }

    if (output_file != NULL) {

        fd = open(output_file,
                  O_WRONLY | O_CREAT | O_TRUNC,
                  0644);

        if (fd < 0) {
            perror("Output file");
            return -1;
        }

        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2");
            close(fd);
            return -1;
        }

        close(fd);
    }

    return 0;
}


// Execute command
int execute(char** arglist, int background, char* command) {

    int total = 0;
    int pipe_index = -1;

    while (arglist[total] != NULL) {

        if (strcmp(arglist[total], "|") == 0) {

            if (pipe_index != -1) {
                fprintf(stderr, "Only one pipe is supported.\n");
                return -1;
            }

            pipe_index = total;
        }

        total++;
    }


    // Command without pipe
    if (pipe_index == -1) {

        char* argv[MAXARGS + 1];
        char* input_file;
        char* output_file;

        if (parse_command(arglist, 0, total,
                          argv, &input_file,
                          &output_file) < 0) {
            return -1;
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            return -1;
        }

        if (pid == 0) {

            if (apply_redirection(input_file, output_file) < 0) {
                _exit(1);
            }

            execvp(argv[0], argv);

            perror("Command execution failed");
            _exit(1);
        }

        // Background execution
        if (background) {

            add_job(pid, command);
            return 0;
        }

        // Foreground execution
        int status;

        while (waitpid(pid, &status, 0) < 0) {

            if (errno != EINTR) {
                perror("waitpid");
                break;
            }
        }

        return 0;
    }


    // Pipe handling
    if (pipe_index == 0 || pipe_index == total - 1) {
        fprintf(stderr, "Invalid pipe command.\n");
        return -1;
    }

    char* left_argv[MAXARGS + 1];
    char* right_argv[MAXARGS + 1];

    char* left_input;
    char* left_output;
    char* right_input;
    char* right_output;

    if (parse_command(arglist, 0, pipe_index,
                      left_argv, &left_input,
                      &left_output) < 0) {
        return -1;
    }

    if (parse_command(arglist, pipe_index + 1, total,
                      right_argv, &right_input,
                      &right_output) < 0) {
        return -1;
    }

    int fd[2];

    if (pipe(fd) < 0) {
        perror("pipe");
        return -1;
    }

    pid_t left_pid = fork();

    if (left_pid < 0) {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        return -1;
    }

    if (left_pid == 0) {

        close(fd[0]);

        if (dup2(fd[1], STDOUT_FILENO) < 0) {
            perror("dup2");
            _exit(1);
        }

        close(fd[1]);

        if (apply_redirection(left_input, left_output) < 0) {
            _exit(1);
        }

        execvp(left_argv[0], left_argv);

        perror("Command execution failed");
        _exit(1);
    }


    pid_t right_pid = fork();

    if (right_pid < 0) {

        perror("fork");

        close(fd[0]);
        close(fd[1]);

        waitpid(left_pid, NULL, 0);

        return -1;
    }

    if (right_pid == 0) {

        close(fd[1]);

        if (dup2(fd[0], STDIN_FILENO) < 0) {
            perror("dup2");
            _exit(1);
        }

        close(fd[0]);

        if (apply_redirection(right_input, right_output) < 0) {
            _exit(1);
        }

        execvp(right_argv[0], right_argv);

        perror("Command execution failed");
        _exit(1);
    }

    close(fd[0]);
    close(fd[1]);

    // Background pipe: track both child processes
    if (background) {

        add_job(left_pid, command);
        add_job(right_pid, command);

        return 0;
    }

    // Foreground pipe: wait for both
    waitpid(left_pid, NULL, 0);
    waitpid(right_pid, NULL, 0);

    return 0;
}
