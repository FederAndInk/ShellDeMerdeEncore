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

void prompt()
{
  printf("shell> ");
  fflush(stdout);
}

void exitShell()
{
  printf("exit\n");
  exit(0);
}

void handleStatus(int status) {}

void execSubCommand(char** cmd, char const* in, char const* out)
{
  pid_t pid = fork();
  if (pid == 0)
  {
    if (out)
    {
      int outDesc = open(out, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      if (outDesc >= 0)
      {
        dup2(outDesc, STDOUT_FILENO);
      }
      else
      {
        perror(out);
        exit(1);
      }
    }

    if (in)
    {
      int inDesc = open(in, O_RDONLY);
      if (inDesc >= 0)
      {
        dup2(inDesc, STDIN_FILENO);
      }
      else
      {
        perror(in);
        exit(1);
      }
    }

    signal(SIGINT, SIG_DFL);
    if (execvp(*cmd, cmd) < 0)
    {
      fprintf(stderr, "command not found: %s\n", *cmd);
      exit(1);
    }
  }
  else
  {
    signal(SIGINT, SIG_IGN);
    int status;
    wait(&status);
    handleStatus(status);
    signal(SIGINT, ctrlCNothing);
  }
}

/**
 * @brief process internal commands
 * 
 * @param cmd 
 */
void processCommands(CmdLine l, size_t no)
{
  char** cmd = l->seq[no];

  // Internal commands
  if (strcmp("quit", cmd[0]) == 0)
  {
    exitShell();
  }
  else if (strcmp("clear", cmd[0]) == 0)
  {
    printf("clearing\n");
    execSubCommand(cmd, NULL, NULL);
  }
  // Other commands
  else
  {
    execSubCommand(cmd, l->in, l->out);
  }
}

int main()
{
  while (1)
  {
    struct cmdline* l;
    int             i, j;

    // block ctrl+c signal
    signal(SIGINT, ctrlCNothing);

    prompt();
    l = readcmd();

    /* If input stream closed, normal termination */
    if (!l)
    {
      exitShell();
    }

    if (l->err)
    {
      /* Syntax error, read another command */
      printf("error: %s\n", l->err);
      continue;
    }

    if (l->in)
    {
      printf("in: %s\n", l->in);
    }
    if (l->out)
    {
      printf("out: %s\n", l->out);
    }

    /* Display each command of the pipe */
    for (i = 0; l->seq[i] != 0; i++)
    {
      char** cmd = l->seq[i];
      printf("seq[%d]: ", i);
      for (j = 0; cmd[j] != 0; j++)
      {
        printf("%s ", cmd[j]);
      }
      printf("\n");
      processCommands(l, i);
    }
  }
}
