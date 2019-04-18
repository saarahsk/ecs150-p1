#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>

// the maximum length of a command line never exceeds 512 characters.
#define MAX_LENGTH 512
// command will never have more than 16 arguments (name of the program included).
// add one more so we can have a final null if necessary
#define MAX_ARGS 17
#define MAX_ARG_LENGTH 128

struct command {
    int argc;
    char* argv[MAX_ARGS];

    bool background;
    char* input;
    char* output;
};

void init_command(struct command* cmd) {
    cmd->argc = 0;
    cmd->background = false;
    cmd->input = NULL;
    cmd->output = NULL;

    for (int i = 0; i < MAX_ARGS; i++) {
        cmd->argv[i] = NULL;
    }
}

void free_command(struct command* cmd) {
    cmd->argc = 0;
    cmd->background = false;
    cmd->input = NULL;
    cmd->output = NULL;

    for (int i = 0; i < MAX_ARGS - 1; i++) {
        if (cmd->argv[i] != NULL) {
            free(cmd->argv[i]);
            cmd->argv[i] = NULL;
        }
    }
}

void add_argument(char** start, int length, struct command* cmd) {
    cmd->argv[cmd->argc] = malloc(MAX_ARG_LENGTH * sizeof(char));
    strncpy(cmd->argv[cmd->argc], *start, length);
    cmd->argv[cmd->argc][length] = '\0';
    cmd->argc++;
    *start += length;
}

char* trim(char* str) {
    // leading spaces
    while (isspace(*str)) {
        str++;
    }

    // trailing spacesj
    for (int i = strlen(str) - 1; i >= 0; i--) {
        if (!isspace(str[i])) {
            break;
        }

        str[i] = '\0';
    }

    return str;
}

bool parse_command(const char* input, struct command* cmd) {
    char buffer[MAX_LENGTH];
    char* cmdline = buffer;
    strcpy(cmdline, input);

    cmdline = trim(cmdline);
    if (strlen(cmdline) == 0) {
        return true;
    }

    // determine if we are dealing with a background job
    if (cmdline[strlen(cmdline) - 1] == '&') {
        cmdline[strlen(cmdline) - 1] = '\0';
        cmd->background = true;
        cmdline = trim(cmdline);
    }

    // ensure the background job specifier isn't in the middle somewhere
    for (int i = 0; i < strlen(cmdline); i++) {
        if (cmdline[i] == '&') {
            fprintf(stderr, "Error: mislocated background sign\n");
            return false;
        }
    }

    char* end = strchr(cmdline, '\0');
    while (cmd->argc < MAX_ARGS && cmdline < end) {
        if (*cmdline == ' ') {
            // skip all spaces between two words
            cmdline++;
            continue;
        }
        else if (*cmdline == '\'') {
            // quotes are special -- skip until the next quote, even if spaces
            // or other control characters
            cmdline++;
            char* pos = strchr(cmdline, '\'');
            if (pos == NULL) {
                fprintf(stderr, "Error: invalid command line\n");
                return false; // matching end quote not found, invalid cmdline
            }

            add_argument(&cmdline, pos - cmdline, cmd);
            cmdline += 1; // skip the quote
            cmdline = trim(cmdline);
        }
        else {
            // in a regular argument, skip until the next control character
            char* pos = strpbrk(cmdline, " ><");
            if (pos == NULL) {
                pos = end; // no remaining spaces, last argument, go to end
                add_argument(&cmdline, pos - cmdline, cmd);
            }
            else if (*pos == ' ') {
                add_argument(&cmdline, pos - cmdline, cmd);
            }
            else if (*pos == '<') {
                if (pos - cmdline > 0) {
                    add_argument(&cmdline, pos - cmdline, cmd);
                }
                cmdline += 1; // skip the <
                cmdline = trim(cmdline);

                // get filename
                pos = strpbrk(cmdline, " >");
                if (pos == NULL) {
                    pos = end; // no remaining spaces, last argument, go to end
                }

                int arglen = pos - cmdline;
                cmd->input = malloc(MAX_ARG_LENGTH * sizeof(char));
                strncpy(cmd->input, cmdline, arglen);
                cmdline += arglen;

                if (strlen(cmd->input) == 0) {
                    fprintf(stderr, "Error: no input file\n");
                    return false;
                }
            }
            else if (*pos == '>') {
                if (pos - cmdline > 0) {
                    add_argument(&cmdline, pos - cmdline, cmd);
                }
                cmdline += 1; // skip the >
                cmdline = trim(cmdline);

                // get filename
                pos = strpbrk(cmdline, " <");
                if (pos == NULL) {
                    pos = end; // no remaining spaces, last argument, go to end
                }

                int arglen = pos - cmdline;
                cmd->output = malloc(MAX_ARG_LENGTH * sizeof(char));
                strncpy(cmd->output, cmdline, arglen);
                cmdline += arglen;

                if (strlen(cmd->output) == 0) {
                    fprintf(stderr, "Error: no output file\n");
                    return false;
                }
            }
        }
    }

    return true;
}

int main() {
    char buffer[MAX_LENGTH];
    while (true) {

        char *n1;
        printf("sshell$ ");
        fflush(stdout);

        // size_t buffer_size = MAX_LENGTH + 1; // 1 for null character
        // char buffer[buffer_size];
        // memset(buffer, 0, buffer_size);

        // char* buf = buffer;

        // fgets(&buf, MAX_LENGTH, stdin)
        // int chars = getline(&buf, &buffer_size, stdin);

        fgets(buffer, MAX_LENGTH, stdin);

        if (!isatty(STDIN_FILENO)) {
          printf("%s", buffer);
          fflush(stdout);
        }

        n1 = strchr(buffer, '\n');
        if(n1)
          *n1 = '\0';



        if (strlen(buffer) == 0) {
            continue;
        }

        struct command cmd;
        init_command(&cmd);

        bool success = parse_command(buffer, &cmd);
        if (!success) {
            continue;
        }

        if (cmd.argc == 0) {
            printf("Error: invalid command line\n");
            continue;
        }

        // handle built in commands
        if (strcmp(cmd.argv[0], "exit") == 0) {
            fprintf(stderr, "Bye...\n");
            exit(0);
        }
        else if (strcmp(cmd.argv[0], "pwd") == 0) {
            char cwd[MAX_LENGTH];
            if (getcwd(cwd, MAX_LENGTH) == NULL) {
                perror("pwd");
                exit(1);
            }

            FILE* fd = stdout;
            if (cmd.output != NULL) {
                fd = fopen(cmd.output, "w");
                if (fd == NULL) {
                    fprintf(stderr, "Error: cannot open output file\n");
                    continue;
                }
            }

            fprintf(fd, "%s\n", cwd);
            if (cmd.output != NULL) {
                fclose(fd);
            }

            printf("+ completed '%s' [0]\n", buffer);
        }
        else if (strcmp(cmd.argv[0], "cd") == 0) {
            // check to make sure that the directory even exists
            struct stat st;
            if (stat(cmd.argv[1], &st) == -1) {
                fprintf(stderr, "Error: no such directory\n");
                continue;
            }

            // make sure user isn't trying to cd into a file: wtf?
            if (!S_ISDIR(st.st_mode)) {
                fprintf(stderr, "Error: %s is not a directory\n", cmd.argv[1]);
                continue;
            }

            int ret = chdir(cmd.argv[1]);
            if (ret == -1) {
                perror("cd");
                continue;
            }

            printf("+ completed '%s %s' [0]\n", cmd.argv[0], cmd.argv[1]);
        }
        else {
            // user entry is not a builtin, fork and run the program
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(1);
            }
            else if (pid == 0) {
                if (cmd.output != NULL) {
                    FILE* fd = fopen(cmd.output, "w");
                    if (fd == NULL) {
                        fprintf(stderr, "Error: cannot open output file\n");
                        exit(1);
                    }

                    dup2(fileno(fd), fileno(stdout));
                    fclose(fd);
                }

                if (cmd.input != NULL) {
                    FILE* fd = fopen(cmd.input, "r");
                    if (fd == NULL) {
                        fprintf(stderr, "Error: cannot open input file\n");
                        exit(1);
                    }

                    dup2(fileno(fd), fileno(stdin));
                    fclose(fd);
                }

                execvp(cmd.argv[0], cmd.argv);
                fprintf(stderr, "Error: command not found\n");
                exit(1);
            }
            else {
                int status = 0;
                int ret = waitpid(pid, &status, 0);
                if (ret == -1) {
                    perror("waitpid");
                    exit(1);
                }

                fprintf(stderr, "+ completed '%s' [%d]\n", buffer, WEXITSTATUS(status));
            }
        }

        free_command(&cmd);
    }

    return 0;
}
