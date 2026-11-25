#include "shell.h"

#define HISTORY_SIZE 20

// History storage
char* history[HISTORY_SIZE];
int history_count = 0;
int history_start = 0;


// Read command from user
char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);

    char* cmdline = (char*)malloc(sizeof(char) * MAX_LEN);
    int c, pos = 0;

    while ((c = getc(fp)) != EOF) {
        if (c == '\n') break;

        if (pos < MAX_LEN - 1) {
            cmdline[pos++] = c;
        }
    }

    if (c == EOF && pos == 0) {
        free(cmdline);
        return NULL;
    }

    cmdline[pos] = '\0';
    return cmdline;
}


// Convert command into arguments
char** tokenize(char* cmdline) {

    if (cmdline == NULL || cmdline[0] == '\0') {
        return NULL;
    }

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));

    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        bzero(arglist[i], ARGLEN);
    }

    char* cp = cmdline;
    char* start;
    int len;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {

        while (*cp == ' ' || *cp == '\t') {
            cp++;
        }

        if (*cp == '\0') {
            break;
        }

        start = cp;
        len = 0;

        while (*cp != '\0' && *cp != ' ' && *cp != '\t') {
            len++;
            cp++;
        }

        if (len >= ARGLEN) {
            len = ARGLEN - 1;
        }

        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';

        argnum++;
    }

    if (argnum == 0) {
        for (int i = 0; i < MAXARGS + 1; i++) {
            free(arglist[i]);
        }

        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}


// Add command to history
void add_history(const char* cmd) {

    int index;

    if (history_count < HISTORY_SIZE) {
        index = (history_start + history_count) % HISTORY_SIZE;
        history_count++;
    }
    else {
        index = history_start;

        free(history[index]);

        history_start = (history_start + 1) % HISTORY_SIZE;
    }

    history[index] = strdup(cmd);
}


// Display command history
void print_history(void) {

    for (int i = 0; i < history_count; i++) {

        int index = (history_start + i) % HISTORY_SIZE;

        printf("%d  %s\n", i + 1, history[index]);
    }
}


// Retrieve a command from history
char* get_history_command(int n) {

    if (n < 1 || n > history_count) {
        return NULL;
    }

    int index = (history_start + n - 1) % HISTORY_SIZE;

    return history[index];
}


// Handle built-in commands
int handle_builtin(char** arglist) {

    // Exit command
    if (strcmp(arglist[0], "exit") == 0) {

        printf("Exiting shell...\n");
        exit(0);
    }

    // Change directory
    else if (strcmp(arglist[0], "cd") == 0) {

        if (arglist[1] == NULL) {
            printf("Usage: cd <directory>\n");
        }
        else if (chdir(arglist[1]) != 0) {
            perror("cd failed");
        }

        return 1;
    }

    // Help command
    else if (strcmp(arglist[0], "help") == 0) {

        printf("Built-in commands:\n");
        printf("  exit           - Exit the shell\n");
        printf("  cd <directory> - Change directory\n");
        printf("  help           - Display this help message\n");
        printf("  jobs           - Display job control status\n");
        printf("  history        - Display command history\n");

        return 1;
    }

    // Jobs command
    else if (strcmp(arglist[0], "jobs") == 0) {

        printf("Job control not yet implemented.\n");

        return 1;
    }

    // History command
    else if (strcmp(arglist[0], "history") == 0) {

        print_history();

        return 1;
    }

    // Not a built-in command
    return 0;
}
