/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include "shell.h"
#include "SignalHandler.h"
#include "csapp.h"
#include "readcmd.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

void prompt()
{
  printf("\033[0Gshell> ");
  fflush(stdout);
}

void exitShell()
{
  printf("exit\n");
  exit(0);
}

void handleStatus(int status)
{
  if (WTERMSIG(status) == SIGINT)
  {
    printf("\n");
  }
}

int const WRITE = O_CREAT | O_WRONLY;
int const READ = O_RDONLY;
int const NO_FILE = -1;

int openIn(char const* path, int mod)
{
  if (!path)
  {
    return NO_FILE;
  }
  else
  {
    int desc = open(path, mod, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (desc >= 0)
    {
      return desc;
    }
    else
    {
      perror(path);
      exit(1);
    }
  }
}

void callExec(CmdLine cmdL, int no)
{
  char** cmd = cmdL->seq[no];
  signal(SIGINT, SIG_DFL);
  if (execvp(*cmd, cmd) < 0)
  {
    fprintf(stderr, "command not found: %s\n", *cmd);
    exit(1);
  }
}

void execSubCommand(CmdLine cmdL)
{
  pid_t* pids = malloc(sizeof(pid_t) * cmdL->seqSize);

  // No pipe
  if (cmdL->seq[1] == NULL)
  {
    pids[0] = fork();
    if (pids[0] == 0)
    {
      // For the first process in the pipe
      int in = openIn(cmdL->in, READ);
      if (in >= 0)
      {
        dup2(in, STDIN_FILENO);
        close(in);
      }

      // For the last process in the pipe
      int out = openIn(cmdL->out, WRITE);
      if (out >= 0)
      {
        dup2(out, STDOUT_FILENO);
        close(out);
      }
      callExec(cmdL, 0);
    }
  }
  // With pipe(s)
  else
  {
    struct PipeArray
    {
      int p[2];
    }* pipeDesc = malloc(sizeof(struct PipeArray) * cmdL->seqSize - 1);

    pipe(pipeDesc[0].p);

    pids[0] = fork();
    if (pids[0] == 0)
    {
      // Redirect input file for the first process in the pipe
      int in = openIn(cmdL->in, READ);
      if (in >= 0)
      {
        dup2(in, STDIN_FILENO);
        close(in);
      }
      close(pipeDesc[0].p[PIPE_READ]);
      dup2(pipeDesc[0].p[PIPE_WRITE], STDOUT_FILENO);
      close(pipeDesc[0].p[PIPE_WRITE]);
      callExec(cmdL, 0);
    }
    else
    {
      pids[1] = fork();
      if (pids[1] == 0)
      {
        // Redirect output file for the last process in the pipe
        int out = openIn(cmdL->out, WRITE);
        if (out >= 0)
        {
          dup2(out, STDOUT_FILENO);
          close(out);
        }
        close(pipeDesc[0].p[PIPE_WRITE]);
        dup2(pipeDesc[0].p[PIPE_READ], STDIN_FILENO);
        close(pipeDesc[0].p[PIPE_READ]);
        callExec(cmdL, 1);
      }
      else
      {
        close(pipeDesc[0].p[PIPE_READ]);
        close(pipeDesc[0].p[PIPE_WRITE]);
      }
    }
    free(pipeDesc);
  }
  if (!cmdL->bg)
  {
    signal(SIGINT, SIG_IGN);
    int status;
    for (size_t i = 0; i < cmdL->seqSize; i++)
    {
      waitpid(pids[i], &status, 0);
      handleStatus(status);
    }
    signal(SIGINT, ctrlCHandler);
  }
  free(pids);
}

/**
 * @brief process internal commands
 *
 * @param cmd
 */
void processCommands(CmdLine l)
{
  char** cmd = l->seq[0];

  // Internal commands
  if (strcmp("quit", cmd[0]) == 0)
  {
    exitShell();
  }
  else if (strcmp("clear", cmd[0]) == 0)
  {
    printf("clearing\n");
    execSubCommand(l);
  }
  // Other commands
  else
  {
    execSubCommand(l);
  }
}

int main()
{
  signal(SIGCHLD, childHandler);

  while (1)
  {
    struct cmdline* l;
    int             i, j;

    // block ctrl+c signal
    signal(SIGINT, ctrlCHandler);

    prompt();
    l = readcmd();

    /* If input stream closed, normal termination */
    if (!l)
    {
      exitShell();
    }

    if (l->seq[0] == NULL)
    {
      continue;
    }

    if (l->err)
    {
      /* Syntax error, read another command */
      printf("error: %s\n", l->err);
      continue;
    }

    if (l->bg)
    {
      printf("bg\n");
    }

    for (i = 0; l->seq[i] != NULL; i++)
    {
      char** cmd = l->seq[i];
      printf("seq[%d]: ", i);
      for (j = 0; cmd[j] != 0; j++)
      {
        printf("%s ", cmd[j]);
      }
      printf("\n");
    }

    processCommands(l);
  }
}
