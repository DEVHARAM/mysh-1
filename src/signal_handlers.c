#include "signal_handlers.h"
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


char cmp[4]="exit";

void catch_sigint(int signalNo)
{
  signal(SIGINT,SIG_IGN);
}
void catch_sigtstp(int signalNo)
{
  signal(SIGTSTP,SIG_IGN); 
  
}
