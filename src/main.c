#include "shell.h"


// Execute multiple commands separated by ;
void execute_line(char* cmdline) {

    char* saveptr;

    char* command = strtok_r(cmdline, ";", &saveptr);

    while (command != NULL) {

        // Remove leading spaces
        while (*command == ' ' || *command == '\t') {
            command++;
        }

        // Ignore empty commands
        if (*command == '\0') {
            command = strtok_r(NULL, ";", &saveptr);
            continue;
        }

        char** arglist = tokenize(command);

        if (arglist == NULL) {
            command = strtok_r(NULL, ";", &saveptr);
            continue;
        }

        int count = 0;

        while (arglist[count] != NULL) {
            count++;
        }

        // Detect trailing &
        int background = 0;

        if (count > 0 &&
            strcmp(arglist[count - 1], "&") == 0) {

            background = 1;

            free(arglist[count - 1]);
            arglist[count - 1] = NULL;

            count--;
        }

        // Ignore an empty command
        if (count > 0) {

            if (handle_builtin(arglist) == 0) {

                execute(arglist, background, command);
            }
        }

        // Free memory
        for (int i = 0; i < count; i++) {
            free(arglist[i]);
        }

        free(arglist);

        command = strtok_r(NULL, ";", &saveptr);
    }
}


int main(void) {

    char* cmdline;

    using_history();

    rl_attempted_completion_function = shell_completion;

    while (1) {

        // Reap completed background processes before prompting
        check_background_jobs();

        cmdline = readline(PROMPT);

        if (cmdline == NULL) {
            break;
        }

        // History command: !n
        if (cmdline[0] == '!') {

            char* endptr;

            long n = strtol(cmdline + 1, &endptr, 10);

            if (cmdline[1] == '\0' ||
                *endptr != '\0' || n < 1) {

                printf("Invalid history command.\n");

                free(cmdline);
                continue;
            }

            HIST_ENTRY* entry = history_get((int)n);

            if (entry == NULL) {

                printf("History: command not found.\n");

                free(cmdline);
                continue;
            }

            char* new_cmdline = strdup(entry->line);

            free(cmdline);

            cmdline = new_cmdline;

            printf("%s\n", cmdline);
        }

        if (cmdline[0] != '\0') {
            add_history(cmdline);
        }

        execute_line(cmdline);

        free(cmdline);
    }

    printf("\nShell exited.\n");

    return 0;
}
