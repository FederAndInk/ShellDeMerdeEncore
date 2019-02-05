#include "SignalHandler.h"
#include "shell.h"
#include <stdio.h>

void ctrlCNothing(int i)
{
  printf("\n");
  prompt();
}
