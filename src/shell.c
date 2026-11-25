#include <ctype.h>
#include "shell.h"
#include <dirent.h>
#include <limits.h>

// Background job list
Job jobs[MAX_JOBS];
int job_count = 0;


// Add a background job
void add_job(pid_t pid, char* command) {

    if (job_count >= MAX_JOBS) {
        printf("Maximum background jobs reached.\n");
        return;
    }

    jobs[job_count].pid = pid;

    strncpy(jobs[job_count].command, command, MAX_LEN - 1);
    jobs[job_count].command[MAX_LEN - 1] = '\0';

    job_count++;

    printf("[%d] %d running in background\n", job_count, pid);
}


// Remove a completed job
void remove_job(pid_t pid) {

    for (int i = 0; i < job_count; i++) {

        if (jobs[i].pid == pid) {

            for (int j = i; j < job_count - 1; j++) {
                jobs[j] = jobs[j + 1];
            }

            job_count--;
            return;
        }
    }
}


// Display background jobs
void print_jobs(void) {

    if (job_count == 0) {
        printf("No background jobs.\n");
        return;
    }

    for (int i = 0; i < job_count; i++) {

        printf("[%d] %d Running %s\n",
               i + 1,
               jobs[i].pid,
               jobs[i].command);
    }
}


// Check completed background processes
void check_background_jobs(void) {

    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {

        for (int i = 0; i < job_count; i++) {

            if (jobs[i].pid == pid) {

                printf("\n[%d] %d completed: %s\n",
                       i + 1,
                       pid,
                       jobs[i].command);

                remove_job(pid);
                break;
            }
        }
    }
}


// Tokenize commands
char** tokenize(char* cmdline) {

    if (cmdline == NULL || cmdline[0] == '\0') {
        return NULL;
    }

    char** arglist = calloc(MAXARGS + 1, sizeof(char*));

    if (arglist == NULL) {
        perror("malloc");
        return NULL;
    }

    int count = 0;
    char* cp = cmdline;

    while (*cp != '\0') {

        while (*cp == ' ' || *cp == '\t') {
            cp++;
        }

        if (*cp == '\0') {
            break;
        }

        if (count >= MAXARGS) {
            fprintf(stderr, "Too many arguments.\n");
            break;
        }

        // Special operators
        if (*cp == '<' || *cp == '>' ||
            *cp == '|' || *cp == '&') {

            char token[2] = {*cp, '\0'};

            arglist[count] = strdup(token);

            cp++;
            count++;
            continue;
        }

        char* start = cp;

        while (*cp != '\0' &&
               *cp != ' ' && *cp != '\t' &&
               *cp != '<' && *cp != '>' &&
               *cp != '|' && *cp != '&') {

            cp++;
        }

        int len = cp - start;

        arglist[count] = malloc(len + 1);

        if (arglist[count] == NULL) {
            perror("malloc");

            for (int i = 0; i < count; i++) {
                free(arglist[i]);
            }

            free(arglist);
            return NULL;
        }

        strncpy(arglist[count], start, len);
        arglist[count][len] = '\0';

        count++;
    }

    if (count == 0) {
        free(arglist);
        return NULL;
    }

    arglist[count] = NULL;

    return arglist;
}


// Display command history
void print_history(void) {

    HIST_ENTRY** entries = history_list();

    if (entries == NULL) {
        printf("No commands in history.\n");
        return;
    }

    for (int i = 0; entries[i] != NULL; i++) {

        printf("%d  %s\n",
               history_base + i,
               entries[i]->line);
    }
}


// Handle built-in commands

int handle_builtin(char** arglist) {

    if (arglist == NULL || arglist[0] == NULL) {
        return 0;
    }

    // Exit
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
            perror("cd");
        }

        return 1;
    }

    // Help
    else if (strcmp(arglist[0], "help") == 0) {

        printf("Built-in commands:\n");
        printf("  exit       - Exit shell\n");
        printf("  cd         - Change directory\n");
        printf("  help       - Display help\n");
        printf("  jobs       - Display background jobs\n");
        printf("  history    - Display command history\n");
printf("  set        - Display shell variables\n");
        return 1;
    }

    // Jobs
    else if (strcmp(arglist[0], "jobs") == 0) {

        print_jobs();
        return 1;
    }

    // History
    else if (strcmp(arglist[0], "history") == 0) {

        print_history();
        return 1;
    }

    return 0;
}


// Command completion generator
char* command_generator(const char* text, int state) {

    static char* path_copy = NULL;
    static char* saveptr;
    static char* current_dir;
    static DIR* dir;

    static const char* builtins[] = {
        "exit", "cd", "help", "jobs", "history", NULL
    };

    static int builtin_index;

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

    while (builtins[builtin_index] != NULL) {

        const char* name = builtins[builtin_index++];

        if (strncmp(name, text, strlen(text)) == 0) {
            return strdup(name);
        }
    }

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


// Readline completion
char** shell_completion(const char* text, int start, int end) {

    (void)end;

    if (start == 0) {
        return rl_completion_matches(text, command_generator);
    }

    return rl_completion_matches(text, rl_filename_completion_function);
}
/* Shell variable storage */

typedef struct Variable {
    char *name;
    char *value;
    struct Variable *next;
} Variable;

static Variable *variable_list = NULL;


/* Store or update a variable */

void store_variable(char *name, char *value) {

    Variable *current = variable_list;

    while (current != NULL) {

        if (strcmp(current->name, name) == 0) {

            free(current->value);
            current->value = strdup(value);
            return;
        }

        current = current->next;
    }

    Variable *new_variable = malloc(sizeof(Variable));

    if (new_variable == NULL) {
        perror("malloc");
        return;
    }

    new_variable->name = strdup(name);
    new_variable->value = strdup(value);

    new_variable->next = variable_list;
    variable_list = new_variable;
}


/* Detect and process assignments */

int variable_assignment(char *command) {

    char *equals = strchr(command, '=');

    if (equals == NULL)
        return 0;

    if (equals == command)
        return 0;

    for (char *p = command; p < equals; p++) {

        if (isspace((unsigned char)*p))
            return 0;
    }

    char *name = strndup(command, equals - command);

    for (char *p = name; *p; p++) {

        if (!(isalnum((unsigned char)*p) || *p == '_')) {
            free(name);
            return 0;
        }
    }

    if (!(isalpha((unsigned char)name[0]) || name[0] == '_')) {
        free(name);
        return 0;
    }

    char *value = equals + 1;

    while (isspace((unsigned char)*value))
        value++;

    size_t len = strlen(value);

    if (len >= 2 &&
        ((value[0] == '"' && value[len - 1] == '"') ||
         (value[0] == '\'' && value[len - 1] == '\''))) {

        value[len - 1] = '\0';
        value++;
    }

    store_variable(name, value);

    free(name);

    return 1;
}


/* Expand variables such as $MSG */

void expand_variables(char **arglist) {

    for (int i = 0; arglist[i] != NULL; i++) {

        if (arglist[i][0] != '$')
            continue;

        char *name = arglist[i] + 1;

        Variable *current = variable_list;

        while (current != NULL) {

            if (strcmp(current->name, name) == 0) {

                char *replacement = strdup(current->value);

                if (replacement != NULL) {

                    free(arglist[i]);
                    arglist[i] = replacement;
                }

                break;
            }

            current = current->next;
        }

        if (current == NULL) {

            free(arglist[i]);
            arglist[i] = strdup("");
        }
    }
}


/* Display all stored variables */

void print_variables(void) {

    Variable *current = variable_list;

    while (current != NULL) {

        printf("%s=%s\n",
               current->name,
               current->value);

        current = current->next;
    }
}

