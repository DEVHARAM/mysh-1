#include "signal.h"
#include <siginto.h>
#include <unistd.h>

void catch_sigint(int)
{
sigset_t blockset;
sigemptyset(&blockset);
sigaddset(&blockset,SIGINT);
sigprocmask(SIG_BLOCK,&blockset,NULL);

//signal(SIGINT,SIG_IGN);
sigaction(SIGINT,SIG_IGN,NULL);
pause();
Stopped(SIGINT);
exit(1);
}
void catch_sigtstp(int)
{
signal(SIGTSTP,SIG_IGN);
sigaction(SIGTSTP,SIGSTOP,NULL);
Stoppend(SIGTSTP);
pause();
exit(0);


}
