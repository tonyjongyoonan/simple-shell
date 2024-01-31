#include <unistd.h>   // for read() and write()
#include <stdlib.h>   // for malloc, EXIT_SUCCESS
#include <stdbool.h>  // for boolean
#include <string.h>   // for strlen
#include <stdio.h>    // for standard io
#include <sys/wait.h> // for waitpid()
#include <signal.h>   // for signals and handlers

#define MAX_INPUT_SIZE 4096

int count_args(char* cmd) {
  int count = 0;
  for (int i = 0; i < strlen(cmd); i++) {
    if (i == 0) {
      // check if beginning of cmd is start of new word
      if (cmd[i] != ' ' && cmd[i] != '\n' && cmd[i] != '\r' && cmd[i] != '\t') {
        count++;
      }
    } else {
      // check if curr character is the start of a new word
      if (cmd[i] != ' ' && cmd[i] != '\n' && cmd[i] != '\r' && cmd[i] != '\t' && cmd[i] != '\0' && (cmd[i - 1] == ' ' || cmd[i - 1] == '\t' || cmd[i - 1] == '\0')) {
        count++;
      }
    }
  }
  return count;
}

char** parse_args(char* cmd, int numBytes, int numArgs) {
    char** args = (char**) malloc(sizeof(char*) * (numArgs + 1));
    // char* args[] vs char** args -- stack vs heap which is why we use char**
    if (args == NULL) {
      return NULL;
    }
    // iterate through input string and create a string array: args
    int i = 0;
    char* token = strtok(cmd, " \n\r\t"); 
    while (token != NULL) {
      args[i] = token;
      token = strtok(NULL, " \n\r\t"); // gets rid of whitespace and new lines 
      i++;
    }
    args[i] = NULL;
  return args;
}

bool timedOut = false; 
int timeout = 0;
pid_t pid;

void handler(int signo) {
  if (signo == SIGALRM) {
    timedOut = true;
    if (kill(pid, SIGINT) == -1) {
      // pid: fork() returns pid of newly created child, returns 0 to child, BUT it's not the pid of the child, but indicating that it is the child
      perror("kill error");
      exit(EXIT_FAILURE);
    }
    printf("Bwahaha... Tonight, I dine on turtle soup!\n");
  } else if (signo == SIGINT) {
    if (pid == 0) {
      if (write(STDERR_FILENO, "\n", 1) == -1) {
        perror("write error");
        exit(EXIT_FAILURE);
      }
      if (write(STDERR_FILENO, PROMPT, strlen(PROMPT)) == -1) {
        perror("write error");
        exit(EXIT_FAILURE);
      }    
    } else if (kill(pid, SIGINT) == -1) {
      perror("kill error");
      exit(EXIT_FAILURE);
    }
  }
}

int main(int argc, char *argv[]) {      
  // takes in argument timeout (in secs) with default 0
  if (argc == 1) {
    timeout = 0;
  } else if (argc == 2) {
    if (argv[1] != 0) {
      timeout = atoi(argv[1]);
    } else {
      timeout = 0;
    }
  } else {
    perror("should not have more than one argument");
    exit(EXIT_FAILURE);
  }
  while (1) {
    pid = 0;
    if (write(STDERR_FILENO, PROMPT, strlen(PROMPT)) == -1) {
      perror("write error");
      exit(EXIT_FAILURE);
    }
    if (signal(SIGINT, handler) == SIG_ERR) {
      perror("signal error");
      exit(EXIT_FAILURE);
    }
    char cmd[MAX_INPUT_SIZE];
    int numBytes = read(STDIN_FILENO, cmd, MAX_INPUT_SIZE);
    cmd[numBytes] = '\0'; // null-terminate the string
    if (numBytes < 0) {
      perror("read error");
      exit(EXIT_FAILURE);
    } else if (numBytes == 0) {
      return 0;
    }
    int numArgs = count_args(cmd);
    printf("numArgs: %d", numArgs);
    if (numArgs == 0) {
      continue;
    }
    char** args = parse_args(cmd, numBytes, numArgs); // Q: why can't we declare this in child and never free?
    pid = fork();
    if (pid < 0) {
      perror("fork error");
      exit(EXIT_FAILURE);
    }
    if (pid == 0) {
      char* envp[] = {NULL};
      execve(args[0], args, envp);
      perror("execve error");
      exit(EXIT_FAILURE);
    }
    if (pid > 0) {
      timedOut = false;
      if (signal(SIGALRM, handler) == SIG_ERR) {
        perror("signal error");
        exit(EXIT_FAILURE);
      }
      alarm(timeout); // fix this in loop
      int status;
      if (wait(&status) == -1) { // does this work in all cases
        perror("wait error");
        exit(EXIT_FAILURE);
      }
      alarm(0); // cancels alarm to not go off if child process finished
      free(args);
    }
  }
  return 0;
}
