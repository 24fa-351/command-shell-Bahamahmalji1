#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h> 

#define MAX_INPUT 1024
#define MAX_ARGS 128

typedef struct EnvVar {
    char name[256];
    char value[256];
    struct EnvVar *next;
} EnvVar;

EnvVar *env_list = NULL;

void handle_cd(char **args);
void handle_pwd();
void handle_set(char **args);
void handle_unset(char **args);
char *expand_variables(char *input);
void execute_command(char *input);

void add_env_var(const char *name, const char *value) {
    EnvVar *new_var = malloc(sizeof(EnvVar));
    strcpy(new_var->name, name);
    strcpy(new_var->value, value);
    new_var->next = env_list;
    env_list = new_var;
}

void remove_env_var(const char *name) {
    EnvVar *current = env_list, *prev = NULL;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            if (prev == NULL) {
                env_list = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

char *get_env_var(const char *name) {
    EnvVar *current = env_list;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current->value;
        }
        current = current->next;
    }
    return NULL;
}

void handle_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "cd: missing argument\n");
        return;
    }
    if (chdir(args[1]) != 0) {
        perror("cd");
    }
}

void handle_pwd() {
    char cwd[MAX_INPUT];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd");
    }
}

void handle_set(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, "set: usage: set VAR VALUE\n");
        return;
    }
    add_env_var(args[1], args[2]);
}

void handle_unset(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "unset: usage: unset VAR\n");
        return;
    }
    remove_env_var(args[1]);
}

char *expand_variables(char *input) {
    static char buffer[MAX_INPUT];
    char *start, *end;
    buffer[0] = '\0';

    while ((start = strchr(input, '$')) != NULL) {
        strncat(buffer, input, start - input);
        end = start + 1;
        while (*end && (isalnum(*end) || *end == '_')) {
            end++;
        }
        char var_name[256];
        strncpy(var_name, start + 1, end - start - 1);
        var_name[end - start - 1] = '\0';
        char *value = get_env_var(var_name);
        if (value) {
            strcat(buffer, value);
        }
        input = end;
    }
    strcat(buffer, input);
    return buffer;
}

void execute_command(char *input) {
    char *expanded_input = expand_variables(input);
    char *args[MAX_ARGS];
    char *token = strtok(expanded_input, " ");
    int arg_count = 0;

    while (token != NULL && arg_count < MAX_ARGS - 1) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;

    if (arg_count == 0) return;

    if (strcmp(args[0], "cd") == 0) {
        handle_cd(args);
    } else if (strcmp(args[0], "pwd") == 0) {
        handle_pwd();
    } else if (strcmp(args[0], "set") == 0) {
        handle_set(args);
    } else if (strcmp(args[0], "unset") == 0) {
        handle_unset(args);
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
        } else {
            perror("fork");
        }
    }
}

int main() {
    char input[MAX_INPUT];

    while (1) {
        printf("xsh# ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
            break;
        }

        execute_command(input);
    }

    return 0;
}
