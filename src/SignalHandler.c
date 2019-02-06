#include "SignalHandler.h"
#include "shell.h"
#include <stdio.h>
#include <sys/wait.h>

void ctrlCHandler(int i)
{
  printf("\n");
  prompt();
}

void childHandler(int i)
{
  int   status;
  pid_t pid;
  while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0)
  {
    handleStatus(status);
    if (pid != 0)
    {
      printf("Child death signaled and treated (%d)\n", pid);
      printf("\n");
      prompt();
    }
  }
}
