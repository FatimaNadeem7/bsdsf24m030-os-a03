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

