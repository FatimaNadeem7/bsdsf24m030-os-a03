#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {

        // Check whether the user entered a history command (!n)
        if (cmdline[0] == '!') {

            char* endptr;
            long n = strtol(cmdline + 1, &endptr, 10);

            // Check if the entered history number is valid
            if (cmdline[1] == '\0' || *endptr != '\0' || n < 1) {
                printf("Invalid history command.\n");
                free(cmdline);
                continue;
            }

            // Retrieve the requested command
            if (n > history_count) {
                printf("History: command not found.\n");
                free(cmdline);
                continue;
            }

            char* previous_command = get_history_command((int)n);

            if (previous_command == NULL) {
                printf("History: command not found.\n");
                free(cmdline);
                continue;
            }

            // Replace !n with the actual previous command
            char* new_cmdline = strdup(previous_command);

            free(cmdline);
            cmdline = new_cmdline;

            printf("%s\n", cmdline);
        }

        // Save the actual command in history
        if (cmdline[0] != '\0') {
            add_history(cmdline);
        }

        // Tokenize the command
        if ((arglist = tokenize(cmdline)) != NULL) {

            if (handle_builtin(arglist) == 0) {
                execute(arglist);
            }

            // Free allocated argument memory
            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }

            free(arglist);
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");

    return 0;
}
