#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


// the maximum length of a command
#define MAX_LENGTH 512
// the maximum number of arguments
#define MAX_ARGS 16


// how to capture the output of exec
// https://stackoverflow.com/questions/7292642/grabbing-output-from-exec
void executeCommand(char command[MAX_ARGS]) {

  int pipe[2];
  pid_t pid;
  int status;

  char foo[4096]; //change to something more meaningful later
  // char *cmd[MAX_LENGTH]= command;

  // if(pipe(link) == -1) {
  //   die("pipe");
  // }


  pid = fork();
  if(pid == 0) {
    // child

    dup2(pipe[1], STDOUT_FILENO);
    close(pipe[0]);
    close(pipe[1]);
    execvp(&command[0], &command);
    perror("execvp");
    exit(1);
  } else if (pid > 0) {
    // parent
    close(pipe[1]);
    int nbytes = read(pipe[0], foo, sizeof(foo));
    printf("output:(%.*s\n)", nbytes, foo);
    waitpid(-1, &status, 0);
    fprintf(stderr, "+ completed '%s' [%d]", command, WEXITSTATUS(status));
  } else {
    perror("fork");
    exit(1);
  }

}


// this function runs the shell
void runShell() {
  while(1) {
    printf("sshell$ ");

    char buf[MAX_LENGTH];

    char *command[MAX_ARGS];
    command[0] = fgets(buf, MAX_LENGTH, stdin);

    executeCommand(*command);

  }
}

int main(int argc, char *argv[]) {
  // main function just runs the shell
  runShell();
  return 0;

}
