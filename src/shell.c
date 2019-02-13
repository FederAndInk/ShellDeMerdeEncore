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

/**
 * @brief open file in mode mod 
 * (WRITE, READ)
 * @param path 
 * @param mod 
 * @return int 
 */
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

void callExec(char** cmd)
{
  signal(SIGINT, SIG_DFL);
  if (execvp(*cmd, cmd) < 0)
  {
    fprintf(stderr, "command not found: %s\n", *cmd);
    exit(1);
  }
}

/**
 * @brief 
 * Execute the subcommand cmd in a fork
 * closing in and out in the parent
 * 
 * @param cmd command
 * @param in file descriptor to use as stdin (NO_FILE can be used to keep stdin)
 * @param out file descriptor to use as stdout (NO_FILE can be used to keep stdout)
 * @param fdToClose file descriptor to close in  the child (NO_FILE can be used to do nothing)
 */
pid_t execSubCommand(char** cmd, int in, int out, int fdToClose)
{
  pid_t pid = fork();
  if (pid == 0)
  {
    if (in >= 0)
    {
      dup2(in, STDIN_FILENO);
      close(in);
    }
    if (out >= 0)
    {
      dup2(out, STDOUT_FILENO);
      close(out);
    }
    if (fdToClose >= 0)
    {
      close(fdToClose);
    }

    callExec(cmd);
  }
  else
  {
    close(in);
    close(out);
  }
  return pid;
}

void execCommands(CmdLine cmdL)
{
  pid_t* pids = malloc(sizeof(pid_t) * cmdL->seqSize);

  // No pipe
  if (cmdL->seq[1] == NULL)
  {
    int in = openIn(cmdL->in, READ);
    int out = openIn(cmdL->out, WRITE);
    pids[0] = execSubCommand(cmdL->seq[0], in, out, NO_FILE);
  }
  // With pipe(s)
  else
  {
    struct PipeArray
    {
      int p[2];
    }* pipeDesc =
        (struct PipeArray*)malloc(sizeof(struct PipeArray) * (cmdL->seqSize - 1));

    pipe(pipeDesc[0].p);

    int in = openIn(cmdL->in, READ);
    pids[0] = execSubCommand(cmdL->seq[0], in, pipeDesc[0].p[PIPE_WRITE],
                             pipeDesc[0].p[PIPE_READ]);
    size_t i;
    for (i = 1; i < cmdL->seqSize - 1; i++)
    {
      pipe(pipeDesc[i].p);
      pids[i] = execSubCommand(cmdL->seq[i], pipeDesc[i - 1].p[PIPE_READ],
                               pipeDesc[i].p[PIPE_WRITE], pipeDesc[i].p[PIPE_READ]);
    }

    if (pids[i - 1] != 0)
    {
      int out = openIn(cmdL->out, WRITE);
      pids[i] = execSubCommand(cmdL->seq[i], pipeDesc[i - 1].p[PIPE_READ], out, NO_FILE);
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
    execCommands(l);
  }
  // Other commands
  else
  {
    execCommands(l);
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
