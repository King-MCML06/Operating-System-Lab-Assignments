#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_PHILOSOPHERS 5
#define NUM_CYCLES 5

extern void *shm_get(void);
extern int sem_init(int id, int value);
extern int sem_wait(int id);
extern int sem_post(int id);

/*
 * Fork semaphore IDs:
 *
 * Fork 0 -> Semaphore 0
 * Fork 1 -> Semaphore 1
 * ...
 * Fork 4 -> Semaphore 4
 */

void
delay(void)
{
  volatile int i;
  for(i = 0; i < 700000; i++);
}

void
philosopher(int id)
{
  int left;
  int right;
  int first;
  int second;
  int cycle;
  left = id;
  right = (id + 1) % NUM_PHILOSOPHERS;

  /*
   * DEADLOCK AVOIDANCE:
   *
   * Always acquire lower-numbered fork first.
   *
   * This creates a global ordering and prevents
   * circular wait.
   */
  if(left < right) {
    first = left;
    second = right;
  } else {
    first = right;
    second = left;
  }

  for(cycle = 0;
      cycle < NUM_CYCLES;
      cycle++) {

    /*
     * THINKING
     */
    printf("Philosopher %d: THINKING\n", id);
    delay();

    /*
     * HUNGRY
     */
    printf("Philosopher %d: THINKING -> HUNGRY\n",id);

    /*
     * Pick first fork.
     */
    sem_wait(first);
    printf("Philosopher %d picked fork %d\n",id, first);

    /*
     * Pick second fork.
     */
    sem_wait(second);
    printf("Philosopher %d picked fork %d\n",id, second);

    /*
     * EATING
     */
    printf("Philosopher %d: HUNGRY -> EATING (cycle %d)\n",id, cycle + 1);
    delay();

    /*
     * Put down forks.
     */
    sem_post(second);
    sem_post(first);
    printf("Philosopher %d: EATING -> THINKING\n",id);
    delay();
  }
  printf("Philosopher %d completed all cycles\n",id);
}

int
main(int argc, char *argv[])
{
  int i;
  int pid;

  /*
   * Initialize 5 binary semaphores.
   *
   * Each semaphore represents one fork.
   */
  for(i = 0;
      i < NUM_PHILOSOPHERS;
      i++) {

    sem_init(i, 1);
  }

  /*
   * Create philosopher processes.
   */
  for(i = 0;i < NUM_PHILOSOPHERS;i++) {
    pid = fork();
    if(pid < 0) {
      printf("dining: fork failed\n");
      exit(1);
    }
    if(pid == 0) {
      philosopher(i);
      exit(0);
    }
  }

  /*
   * Parent waits for all philosophers.
   */
  for(i = 0;i < NUM_PHILOSOPHERS;i++) {
    wait(0);
  }
  printf("\nDining Philosophers completed successfully.\n");
  printf("No deadlock occurred.\n");
  exit(0);
}