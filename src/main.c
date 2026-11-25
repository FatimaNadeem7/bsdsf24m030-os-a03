#include "shell.h"

int main() {

    char* cmdline;
    char** arglist;

    // Initialize Readline history
    using_history();
// Enable command and filename Tab completion
rl_attempted_completion_function = shell_completion;
    while ((cmdline = readline(PROMPT)) != NULL) {

        // Handle history command (!n)
        if (cmdline[0] == '!') {

            char* endptr;
            long n = strtol(cmdline + 1, &endptr, 10);

            if (cmdline[1] == '\0' || *endptr != '\0' || n < 1) {
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

        // Add non-empty commands to Readline history
        if (cmdline[0] != '\0') {
            add_history(cmdline);
        }

        // Tokenize the command
        if ((arglist = tokenize(cmdline)) != NULL) {

            if (handle_builtin(arglist) == 0) {
                execute(arglist);
            }

            // Free argument memory
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
