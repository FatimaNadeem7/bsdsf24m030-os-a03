#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#include <readline/readline.h>
#include <readline/history.h>

#define MAX_LEN 512
#define MAXARGS 64
#define MAX_JOBS 100

#define PROMPT "FCIT> "

typedef struct {
    pid_t pid;
    char command[MAX_LEN];
} Job;

// Tokenization and execution
char** tokenize(char* cmdline);
int execute(char** arglist, int background, char* command);
void execute_line(char* cmdline);

// Built-in commands
int handle_builtin(char** arglist);
void print_history(void);

// Readline autocomplete
char** shell_completion(const char* text, int start, int end);

// Background jobs
void add_job(pid_t pid, char* command);
void remove_job(pid_t pid);
void check_background_jobs(void);
void print_jobs(void);

#endif
