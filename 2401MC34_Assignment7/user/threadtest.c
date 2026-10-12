#include "types.h"
#include "stat.h"
#include "user.h"

#define NTHREADS 3
#define LOOPS 100000

volatile int counter = 0;
void worker(void *arg) {
  int i;
  (void)arg;
  for(i = 0; i < LOOPS; i++) counter++;
  exit();
}

int main(void) {
  int i;
  void *stacks[NTHREADS];
  for(i = 0; i < NTHREADS; i++) {
    stacks[i] = malloc(4096);
    if(stacks[i] == 0 || clone(worker, 0, stacks[i]) < 0) {
      printf(1, "threadtest: clone/stack allocation failed\n"); exit();
    }
  }
  for(i = 0; i < NTHREADS; i++) { if(join() < 0) printf(1,"threadtest: join failed\n"); free(stacks[i]); }
  printf(1, "threadtest: final counter = %d (expected 300000 without lost updates)\n", counter);
  exit();
}
