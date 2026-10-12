#include "types.h"
#include "stat.h"
#include "user.h"

#define NTHREADS 3
#define LOOPS 100000
#define BSIZE 5
#define ITEMS 10
volatile int counter = 0;
void counter_worker(void *arg) {
  int i; (void)arg;
  for(i=0;i<LOOPS;i++){ sem_wait(0); counter++; sem_signal(0); }
  exit();
}
/* Producer/consumer is run sequentially after the counter test so this small
   test does not depend on xv6 wait() treating threads as children. */
int main(void) {
  int i, buf[BSIZE], in=0, out=0;
  sem_init(0,1);
  for(i=0;i<NTHREADS;i++) { void *s=malloc(4096); if(!s || clone(counter_worker,0,s)<0){printf(1,"synctest: clone failed\n");exit();} }
  for(i=0;i<NTHREADS;i++) join();
  printf(1,"synctest: protected counter = %d (expected 300000)\n",counter);
  /* Counting semaphore IDs: 1=empty slots, 2=full slots, 3=buffer mutex. */
  sem_init(1,BSIZE); sem_init(2,0); sem_init(3,1);
  for(i=0;i<ITEMS;i++){ sem_wait(1); sem_wait(3); buf[in]=i; in=(in+1)%BSIZE; printf(1,"produced %d\n",i); sem_signal(3); sem_signal(2); }
  for(i=0;i<ITEMS;i++){ int item; sem_wait(2); sem_wait(3); item=buf[out]; out=(out+1)%BSIZE; printf(1,"consumed %d\n",item); sem_signal(3); sem_signal(1); }
  exit();
}
