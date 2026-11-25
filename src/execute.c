#include "shell.h"

void parse_command(
    char** tokens,
    char** args,
    char** input_file,
    char** output_file
) {
    int j = 0;

    *input_file = NULL;
    *output_file = NULL;

    for (int i = 0; tokens[i] != NULL; i++) {

        if (strcmp(tokens[i], "<") == 0) {
            if (tokens[i + 1] != NULL) {
                *input_file = tokens[++i];
            }
        }

        else if (strcmp(tokens[i], ">") == 0) {
            if (tokens[i + 1] != NULL) {
                *output_file = tokens[++i];
            }
        }

        else {
            args[j++] = tokens[i];
        }
    }

    args[j] = NULL;
}

void apply_redirection(char* input_file, char* output_file) {

    if (input_file != NULL) {
        int fd = open(input_file, O_RDONLY);

        if (fd < 0) {
            perror("Input redirection");
            exit(1);
        }

        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (output_file != NULL) {
        int fd = open(
            output_file,
            O_WRONLY | O_CREAT | O_TRUNC,
            0644
        );

        if (fd < 0) {
            perror("Output redirection");
            exit(1);
        }

        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}

int get_status(int status) {

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }

    return 1;
}

int wait_for_process(pid_t pid) {

    int status;

    while (waitpid(pid, &status, 0) == -1) {

        if (errno != EINTR) {
            perror("waitpid");
            return 1;
        }
    }

    return get_status(status);
}

int execute(char** arglist, int background, char* command) {

    int pipe_position = -1;

    for (int i = 0; arglist[i] != NULL; i++) {

        if (strcmp(arglist[i], "|") == 0) {
            pipe_position = i;
            break;
        }
    }

    /* Normal command without pipe */

    if (pipe_position == -1) {

        char* args[MAXARGS + 1];
        char* input_file;
        char* output_file;

        parse_command(
            arglist,
            args,
            &input_file,
            &output_file
        );

        if (args[0] == NULL) {
            return 0;
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {

            apply_redirection(input_file, output_file);

            execvp(args[0], args);

            perror("Command execution failed");
            exit(127);
        }

        if (background) {

            add_job(pid, command);

            return 0;
        }

        return wait_for_process(pid);
    }

    /* Command with one pipe */

    char* left_tokens[MAXARGS + 1];
    char* right_tokens[MAXARGS + 1];

    int i;

    for (i = 0; i < pipe_position; i++) {
        left_tokens[i] = arglist[i];
    }

    left_tokens[i] = NULL;

    int j = 0;

    for (i = pipe_position + 1; arglist[i] != NULL; i++) {
        right_tokens[j++] = arglist[i];
    }

    right_tokens[j] = NULL;

    char* left_args[MAXARGS + 1];
    char* right_args[MAXARGS + 1];

    char* left_input;
    char* left_output;

    char* right_input;
    char* right_output;

    parse_command(
        left_tokens,
        left_args,
        &left_input,
        &left_output
    );

    parse_command(
        right_tokens,
        right_args,
        &right_input,
        &right_output
    );

    int fd[2];

    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid1 = fork();

    if (pid1 < 0) {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        return 1;
    }

    if (pid1 == 0) {

        close(fd[0]);

        dup2(fd[1], STDOUT_FILENO);

        close(fd[1]);

        apply_redirection(left_input, left_output);

        execvp(left_args[0], left_args);

        perror("Command execution failed");
        exit(127);
    }

    pid_t pid2 = fork();

    if (pid2 < 0) {
        perror("fork");

        close(fd[0]);
        close(fd[1]);

        wait_for_process(pid1);

        return 1;
    }

    if (pid2 == 0) {

        close(fd[1]);

        dup2(fd[0], STDIN_FILENO);

        close(fd[0]);

        apply_redirection(right_input, right_output);

        execvp(right_args[0], right_args);

        perror("Command execution failed");
        exit(127);
    }

    close(fd[0]);
    close(fd[1]);

    if (background) {

        add_job(pid1, command);
        add_job(pid2, command);

        return 0;
    }

    wait_for_process(pid1);

    return wait_for_process(pid2);
}
