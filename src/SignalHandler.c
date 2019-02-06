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
    // TODO: to complete
    int status;
    while (waitpid(-1, &status, WNOHANG | WUNTRACED) >= 0)
    {
        handleStatus(status);
    }
}
