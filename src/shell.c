
#include "shell.h"
#include <dirent.h>
#include <limits.h>

// Generate command completions
char* command_generator(const char* text, int state) {

    static char* path_copy = NULL;
    static char* saveptr;
    static char* current_dir = NULL;
    static DIR* dir = NULL;

    static const char* builtins[] = {
        "exit", "cd", "help", "jobs", "history", NULL
    };

    static int builtin_index = 0;

    if (state == 0) {

        if (dir != NULL) {
            closedir(dir);
            dir = NULL;
        }

        free(path_copy);

        char* path = getenv("PATH");
        path_copy = strdup(path ? path : "");

        saveptr = NULL;
        current_dir = strtok_r(path_copy, ":", &saveptr);
        builtin_index = 0;
    }

    // Complete built-in commands
    while (builtins[builtin_index] != NULL) {

        const char* name = builtins[builtin_index++];

        if (strncmp(name, text, strlen(text)) == 0) {
            return strdup(name);
        }
    }

    // Search executable commands in PATH directories
    while (1) {

        if (dir == NULL) {

            if (current_dir == NULL) {
                return NULL;
            }

            dir = opendir(current_dir);

            if (dir == NULL) {
                current_dir = strtok_r(NULL, ":", &saveptr);
                continue;
            }
        }

        struct dirent* entry;

        while ((entry = readdir(dir)) != NULL) {

            if (strncmp(entry->d_name, text, strlen(text)) == 0) {

                char fullpath[PATH_MAX];

                snprintf(fullpath, sizeof(fullpath),
                         "%s/%s", current_dir, entry->d_name);

                if (access(fullpath, X_OK) == 0) {
                    return strdup(entry->d_name);
                }
            }
        }

        closedir(dir);
        dir = NULL;

        current_dir = strtok_r(NULL, ":", &saveptr);
    }
}

// Handle command and filename completion
char** shell_completion(const char* text, int start, int end) {

    (void)end;

    if (start == 0) {
        return rl_completion_matches(text, command_generator);
    }

    return rl_completion_matches(text, rl_filename_completion_function);
}

// Tokenize command and recognize redirection and pipe operators
char** tokenize(char* cmdline) {

    if (cmdline == NULL || cmdline[0] == '\0') {
        return NULL;
    }

    char** arglist = malloc(sizeof(char*) * (MAXARGS + 1));

    if (arglist == NULL) {
        perror("malloc failed");
        return NULL;
    }

    for (int i = 0; i < MAXARGS + 1; i++) {

        arglist[i] = calloc(ARGLEN, sizeof(char));

        if (arglist[i] == NULL) {
            perror("calloc failed");

            for (int j = 0; j < i; j++) {
                free(arglist[j]);
            }

            free(arglist);
            return NULL;
        }
    }

    char* cp = cmdline;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {

        // Skip spaces and tabs
        if (*cp == ' ' || *cp == '\t') {
            cp++;
            continue;
        }

        // Recognize special operators
        if (*cp == '<' || *cp == '>' || *cp == '|') {

            arglist[argnum][0] = *cp;
            arglist[argnum][1] = '\0';

            cp++;
            argnum++;
            continue;
        }

        // Read normal command or argument
        int len = 0;

        while (*cp != '\0' &&
               *cp != ' ' && *cp != '\t' &&
               *cp != '<' && *cp != '>' && *cp != '|') {

            if (len < ARGLEN - 1) {
                arglist[argnum][len++] = *cp;
            }

            cp++;
        }

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

// Display command history using Readline
void print_history(void) {

    HIST_ENTRY **entries = history_list();

    if (entries == NULL) {
        printf("No commands in history.\n");
        return;
    }

    for (int i = 0; entries[i] != NULL; i++) {
        printf("%d  %s\n", history_base + i, entries[i]->line);
    }
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

    return 0;
}
