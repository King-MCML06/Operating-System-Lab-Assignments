#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define ITERATIONS 10

struct shared_data {
  volatile int flag[2];
  volatile int turn;
  volatile int counter;
};
extern void *shm_get(void);

void
delay(void)
{
  volatile int i;
  for(i = 0; i < 1000000; i++);
}

int
main(int argc, char *argv[])
{
  struct shared_data *shared;
  int pid;
  int id;
  int other;
  int i;

  shared = (struct shared_data *)shm_get();

  if(shared == 0) {
    printf("peterson: shared memory allocation failed\n");
    exit(1);
  }

  /*
   * Initialize shared variables before fork.
   */
  shared->flag[0] = 0;
  shared->flag[1] = 0;
  shared->turn = 0;
  shared->counter = 0;

  pid = fork();

  if(pid < 0) {
    printf("peterson: fork failed\n");
    exit(1);
  }

  if(pid == 0)
    id = 1;
  else
    id = 0;

  other = 1 - id;
  for(i = 0; i < ITERATIONS; i++) {

    /*
     * ENTRY SECTION
     *
     * Process announces that it wants to enter
     * the critical section.
     */
    shared->flag[id] = 1;

    /*
     * Give priority to the other process
     * if both processes want to enter.
     */
    shared->turn = other;
    /*
     * Busy waiting.
     *
     * Wait while:
     * 1. Other process also wants to enter.
     * 2. It is the other process's turn.
     */
    while(shared->flag[other] == 1 && shared->turn == other);
    /*
     * CRITICAL SECTION
     */
    shared->counter++;
    printf("Process %d in CS, counter = %d\n", id, shared->counter);
    /*
     * EXIT SECTION
     */
    shared->flag[id] = 0;
    /*
     * REMAINDER SECTION
     */
    delay();
  }
  if(pid > 0) {
    wait(0);
    printf("\nFinal shared counter = %d\n", shared->counter);
    printf("Expected counter = %d\n", ITERATIONS * 2);
  }
  exit(0);
}