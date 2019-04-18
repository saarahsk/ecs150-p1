#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>

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

char* delete_surrounding_spaces(char* str) {
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

    // delete the spaces around the command line to prevent code below getting confused
    cmdline = delete_surrounding_spaces(cmdline);
    if (strlen(cmdline) == 0) {
        return true;
    }

    // determine if we are dealing with a background job
    if (cmdline[strlen(cmdline) - 1] == '&') {
        cmdline[strlen(cmdline) - 1] = '\0';
        cmd->background = true;
        cmdline = delete_surrounding_spaces(cmdline);
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
            cmdline = delete_surrounding_spaces(cmdline);
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
                cmdline = delete_surrounding_spaces(cmdline);

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
                cmdline = delete_surrounding_spaces(cmdline);

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

void check_children_complete(int* active_jobs) {
    // get the status of any completed children
    while (true) {
        int status = 0;
        pid_t pid = waitpid(-1, &status, WNOHANG);

        if (pid == -1) {
            if (errno == ECHILD) {
                // no more child processes
                return;
            }

            perror("waitpid");
            return;
        }

        if (pid == 0) {
            // no children are complete yet
            break;
        }

        fprintf(stderr, "+ completed '?' [%d]\n", WEXITSTATUS(status));
        *active_jobs -= 1;
    }
}

int main() {
    int active_jobs = 0;

    char input[MAX_LENGTH];
    while (true) {
        if (active_jobs > 0) {
            check_children_complete(&active_jobs);
        }

        char *n1;

        printf("sshell$ ");
        fflush(stdout);

        fgets(input, MAX_LENGTH, stdin);
        
        if (!isatty(STDIN_FILENO)) {
            printf("%s", input);
            fflush(stdout);
        }

        n1 = strchr(input, '\n');

        if(n1){
          *n1 = '\0';
        }

        if (strlen(input) == 0) {
            continue;
        }

        struct command cmd;
        init_command(&cmd);

        bool success = parse_command(input, &cmd);
        if (!success) {
            continue;
        }

        if (cmd.argc == 0) {
            printf("Error: invalid command line\n");
            continue;
        }

        // handle built in commands
        if (strcmp(cmd.argv[0], "exit") == 0) {
            if (active_jobs > 0) {
                fprintf(stderr, "Error: active jobs still running\n");
                continue;
            }

            fprintf(stderr, "Bye...\n");
            return 0;
        }
        else if (strcmp(cmd.argv[0], "pwd") == 0) {
            // use getcwd to get the current working directory
            char cwd[MAX_LENGTH];
            if (getcwd(cwd, MAX_LENGTH) == NULL) {
                perror("pwd");
                return 1;
            }

            // special redirection since we aren't actually forking so have to fake it
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
                // we don't want to accidentally close stdout of the main shell
                fclose(fd);
            }

            printf("+ completed '%s' [0]\n", input);
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

            // chdir changes the working directory of the process
            int ret = chdir(cmd.argv[1]);
            if (ret == -1) {
                perror("cd");
                continue;
            }

            printf("+ completed '%s %s' [0]\n", cmd.argv[0], cmd.argv[1]);
        }
        else {
            // user entered command is not a builtin, fork and run the program
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                return 1;
            }
            else if (pid == 0) {
                if (cmd.output != NULL) {
                    FILE* fd = fopen(cmd.output, "w");
                    if (fd == NULL) {
                        fprintf(stderr, "Error: cannot open output file\n");
                        return 1;
                    }

                    // replace file descriptor for stdout for the child
                    dup2(fileno(fd), fileno(stdout));
                    fclose(fd);
                }

                if (cmd.input != NULL) {
                    FILE* fd = fopen(cmd.input, "r");
                    if (fd == NULL) {
                        fprintf(stderr, "Error: cannot open input file\n");
                        return 1;
                    }

                    // replace file descriptor for stdin for the child
                    dup2(fileno(fd), fileno(stdin));
                    fclose(fd);
                }

                execvp(cmd.argv[0], cmd.argv);
                fprintf(stderr, "Error: command not found\n");
                return 1;
            }
            else {
                active_jobs++;

                if (!cmd.background) {
                    int status = 0;
                    int ret = waitpid(pid, &status, 0);
                    if (ret == -1) {
                        perror("waitpid");
                        return 1;
                    }

                    fprintf(stderr, "+ completed '%s' [%d]\n", input, WEXITSTATUS(status));
                    active_jobs--;
                }
            }
        }

        free_command(&cmd);
    }

    return 0;
}
