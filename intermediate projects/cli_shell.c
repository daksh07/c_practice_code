/*SUMMARY
First we took a line string using terminal
then we looked for location of \n (Enter) in the string using strcspn (character search position)
we replaced that with null "\0"
then we sliced the line into different words
we used strtok (tokens = slices) to look for empty space " " and slice the string around that space
after slicing the string we saved a pointer to every word into argv 
then we forked a child process to run the command in stored in argv[0] along with arguments in argv[i]
in the child process (pid == 0) we then called execvp to execute the command and arguements
catching errors at every valid point from creating a fork to execvp
while the parent process waited for child process to finish using waitpid
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE 256
#define MAX_ARGS 32

void parse_line(char *line, char *argv[]) {
  // Goal: turn "ls -la\n" into argv = {"ls", "-la", NULL}
  uint8_t i = 0;
  i = strcspn(line, "\n");
  line[i] = '\0';
  i = 0;
  argv[0] = strtok(line, " ");
  while (argv[i] != NULL && i < MAX_ARGS - 1){
    argv[i+1] = strtok(NULL, " ");
    i++;
  }
  // 1. First, strip the trailing newline that fgets() leaves in `line`.
  //    Hint: strcspn(line, "\n") gives you the index of the first '\n'
  //    (or the string's length, if there isn't one) — you can use that
  //    to overwrite it with '\0'.

  // 2. Use strtok(line, " ") to get the first token (the command name).
  //    Store it in argv[0].

  // 3. Loop, calling strtok(NULL, " ") repeatedly to get each
  //    subsequent token, storing each one into the next slot of argv.
  //    strtok returns NULL when there are no more tokens — that's
  //    your loop's stop condition.

  // 4. After the loop, the slot right after the last real argument
  //    must be set to NULL. execvp() (which you'll write next) relies
  //    on this NULL to know where the argument list ends — without
  //    it, execvp will keep reading past your real arguments into
  //    whatever garbage happens to be in memory after them.
}

void execute(char *argv[]) {
   // 1. Call fork(). Store the return value (it's a pid_t) in a variable.
  pid_t p = fork();
  int status;
    // 2. Check the return value:
    //    a. If it's < 0, fork() itself failed (out of resources, etc.) —
    //       print an error (perror("fork") is the idiomatic choice) and
    //       return, since there's no child to do anything with.
    //    b. If it's == 0, you're in the CHILD. Call
    //       execvp(argv[0], argv) here.
    //    c. Otherwise (> 0), you're in the PARENT, and the return value
    //       is the child's PID.
  if (p<0){
    perror("fork fail");
    return;
  }
  if (p == 0){
    execvp(argv[0], argv);
    perror("excvp");
    exit(1);
  }
  if (p>0){
    waitpid(p, &status, WUNTRACED);
  }
    // 3. Inside the child branch (2b): what happens if execvp() itself
    //    fails (e.g. the command doesn't exist)? execvp() only returns
    //    at all if it failed — a successful exec never returns, since
    //    the child's code has already been replaced. So: print an error
    //    (perror("execvp")) and exit the child process — think about
    //    which function ends a process immediately versus using
    //    `return`, which would send this (now-erroring) child straight
    //    back into your shell's main loop as if it were still myshell.

    // 4. Inside the parent branch (2c): call waitpid() on the child's
    //    PID, so the parent blocks until that specific child finishes
    //    before returning from execute() and looping back to the prompt.
}

int main(void) {
  char line[MAX_LINE];

  while (1) {
    printf("myshell> ");
    if (fgets(line, sizeof(line), stdin) == NULL) {
      break;
    }

    char *argv[MAX_ARGS];
    parse_line(line, argv);

    if (argv[0] == NULL) {
      continue;
    }

    execute(argv);
  }

  return 0;
}
