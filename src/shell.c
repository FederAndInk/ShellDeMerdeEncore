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

void execSubCommand(CmdLine cmdL)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        // For the first process in the pipe
        int in = openIn(cmdL->in, READ);
        if (in >= 0)
        {
            dup2(in, STDIN_FILENO);
        }

        // For the last process in the pipe
        int out = openIn(cmdL->out, WRITE);
        if (out >= 0)
        {
            dup2(out, STDOUT_FILENO);
        }
        char** cmd = cmdL->seq[0];
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
        signal(SIGINT, ctrlCHandler);
    }
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

        if (l->err)
        {
            /* Syntax error, read another command */
            printf("error: %s\n", l->err);
            continue;
        }

        processCommands(l);
    }
}
