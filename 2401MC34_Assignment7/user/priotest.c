#include "types.h"
#include "stat.h"
#include "user.h"
#define N 4
#define SPINS 5000000
int main(void){int i,pids[N]={0}; int priorities[N]={1,5,10,20};
  for(i=0;i<N;i++){int pid=fork();if(pid<0){printf(1,"fork failed\n");exit();}if(pid==0){int k;set_priority(getpid(),priorities[i]);for(k=0;k<SPINS;k++){}printf(1,"completed pid=%d priority=%d final_priority=%d\n",getpid(),priorities[i],get_priority(getpid()));exit();}pids[i]=pid;}
  for(i=0;i<N;i++)wait(); printf(1,"priotest: all children completed; inspect scheduler wait/aging trace\n");exit();}
