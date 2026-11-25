#include "shell.h"

#include <ctype.h>

/* Execute normal commands and command chains */

int execute_line(char* cmdline) {

    int last_status = 0;

    char* saveptr;
    char* command;

    char* line = strdup(cmdline);

    if (line == NULL) {
        perror("strdup");
        return 1;
    }

    command = strtok_r(line, ";", &saveptr);

    while (command != NULL) {

        while (isspace((unsigned char)*command)) {
            command++;
        }

        if (*command == '\0') {
            command = strtok_r(NULL, ";", &saveptr);
            continue;
        }

        char original_command[MAX_LEN];

        strncpy(
            original_command,
            command,
            MAX_LEN - 1
        );

        original_command[MAX_LEN - 1] = '\0';

        char** arglist = tokenize(command);

        if (arglist == NULL || arglist[0] == NULL) {
            command = strtok_r(NULL, ";", &saveptr);
            continue;
        }

        int background = 0;

        int count = 0;

        while (arglist[count] != NULL) {
            count++;
        }

        if (count > 0 &&
            strcmp(arglist[count - 1], "&") == 0) {

            background = 1;

            free(arglist[count - 1]);

            arglist[count - 1] = NULL;
        }

        if (handle_builtin(arglist)) {

            last_status = 0;
        }

        else {

            last_status = execute(
                arglist,
                background,
                original_command
            );
        }

        for (int i = 0; arglist[i] != NULL; i++) {
            free(arglist[i]);
        }

        free(arglist);

        command = strtok_r(NULL, ";", &saveptr);
    }

    free(line);

    return last_status;
}


/* Execute the selected conditional block */

void execute_if_block(
    char* condition,
    char* then_block,
    char* else_block
) {

    int status = execute_line(condition);

    if (status == 0) {

        execute_line(then_block);
    }

    else {

        if (else_block != NULL &&
            strlen(else_block) > 0) {

            execute_line(else_block);
        }
    }
}


/* Read and process multiline if statements */

void handle_if_statement(char* first_line) {

    char condition[MAX_LEN];

    char then_block[MAX_LEN * 8] = "";
    char else_block[MAX_LEN * 8] = "";

    strncpy(
        condition,
        first_line + 3,
        MAX_LEN - 1
    );

    condition[MAX_LEN - 1] = '\0';

    int in_else = 0;
    int found_then = 0;
    int found_fi = 0;

    char* line;

    while (1) {

        line = readline("IF> ");

        if (line == NULL) {
            break;
        }

        if (strcmp(line, "then") == 0) {

            found_then = 1;

            free(line);

            continue;
        }

        if (strcmp(line, "else") == 0) {

            in_else = 1;

            free(line);

            continue;
        }

        if (strcmp(line, "fi") == 0) {

            found_fi = 1;

            free(line);

            break;
        }

        if (in_else) {

            strncat(
                else_block,
                line,
                sizeof(else_block) - strlen(else_block) - 2
            );

            strncat(
                else_block,
                ";",
                sizeof(else_block) - strlen(else_block) - 1
            );
        }

        else {

            strncat(
                then_block,
                line,
                sizeof(then_block) - strlen(then_block) - 2
            );

            strncat(
                then_block,
                ";",
                sizeof(then_block) - strlen(then_block) - 1
            );
        }

        free(line);
    }

    if (!found_then || !found_fi) {

        printf("Syntax error: incomplete if statement\n");

        return;
    }

    execute_if_block(
        condition,
        then_block,
        else_block
    );
}


/* Main shell */

int main() {

    char* input;

    using_history();

    rl_attempted_completion_function = shell_completion;

    while (1) {

        check_background_jobs();

        input = readline(PROMPT);

        if (input == NULL) {
            printf("\n");
            break;
        }

        if (strlen(input) == 0) {

            free(input);

            continue;
        }

        add_history(input);

        /* Execute history command such as !1 */

        if (input[0] == '!' &&
            isdigit((unsigned char)input[1])) {

            int number = atoi(input + 1);

            HIST_ENTRY* entry = history_get(number);

            if (entry != NULL) {

                char* history_command = strdup(entry->line);

                printf("%s\n", history_command);

                execute_line(history_command);

                free(history_command);
            }

            else {

                printf("History command not found\n");
            }
        }

        /* Handle if statement */

        else if (strncmp(input, "if ", 3) == 0) {

            handle_if_statement(input);
        }

        /* Execute normal command */

        else {

            execute_line(input);
        }

        free(input);
    }

    return 0;
}
