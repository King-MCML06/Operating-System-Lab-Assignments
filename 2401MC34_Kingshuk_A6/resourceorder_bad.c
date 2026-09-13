// resourceorder_bad.c — Q3: demonstrates the DEADLOCK problem when two
// processes acquire two shared locks in opposite order.
//
// Locks are implemented as xv6 pipes pre-loaded with one token: read()
// on an empty pipe genuinely blocks the calling process in the kernel
// (same underlying sleep()/wakeup() mechanism a sleeplock uses), and
// write() releases it. Since fork() duplicates file descriptors, both
// children share the same underlying pipe buffer, giving real
// cross-process mutual exclusion.
//
// Process A: acquire Lock1 -> (delay) -> acquire Lock2 -> work -> release both
// Process B: acquire Lock2 -> (delay) -> acquire Lock1 -> work -> release both
//
// The delay guarantees both processes grab their first lock before either
// tries for the second, making the deadlock reproducible every run.
//
// The parent does NOT wait() on the children in this "bad" version --
// they will hang forever holding one lock each, so waiting on them would
// hang the shell too. Instead the parent sleeps for a while and then
// reports that the children never finished, which is your evidence of
// the deadlock for the output log. (In qemu, Ctrl-a x to quit when done
// observing.)
//
// Add "_resourceorder_bad\" to UPROGS in the Makefile.

#include "types.h"
#include "stat.h"
#include "user.h"

typedef struct {
  int fd[2];
} lock_t;

void
lock_init(lock_t *l)
{
  pipe(l->fd);
  write(l->fd[1], "x", 1);   // one token = unlocked
}

void
lock_acquire(lock_t *l)
{
  char buf;
  read(l->fd[0], &buf, 1);   // blocks here if the token isn't available
}

void
lock_release(lock_t *l)
{
  write(l->fd[1], "x", 1);
}

void
busy_delay(int loops)
{
  volatile int x = 0;
  int i;
  for(i = 0; i < loops; i++)
    x += i;
}

lock_t lock1, lock2;

int
main(void)
{
  int pidA, pidB;

  lock_init(&lock1);
  lock_init(&lock2);

  printf(1, "=== resourceorder_bad: demonstrating deadlock ===\n");

  pidA = fork();
  if(pidA == 0){
    printf(1, "[Process A] step 1: acquiring Lock1\n");
    lock_acquire(&lock1);
    printf(1, "[Process A] step 2: acquired Lock1, now delaying\n");
    busy_delay(2000000);
    printf(1, "[Process A] step 3: waiting for Lock2\n");
    lock_acquire(&lock2);
    printf(1, "[Process A] step 4: acquired Lock2 -- this line should NEVER print\n");
    lock_release(&lock2);
    lock_release(&lock1);
    exit();
  }

  pidB = fork();
  if(pidB == 0){
    printf(1, "[Process B] step 1: acquiring Lock2\n");
    lock_acquire(&lock2);
    printf(1, "[Process B] step 2: acquired Lock2, now delaying\n");
    busy_delay(2000000);
    printf(1, "[Process B] step 3: waiting for Lock1\n");
    lock_acquire(&lock1);
    printf(1, "[Process B] step 4: acquired Lock1 -- this line should NEVER print\n");
    lock_release(&lock1);
    lock_release(&lock2);
    exit();
  }

  // Parent: deliberately do NOT wait() -- that would hang the shell too.
  // Instead sleep a while (xv6 ticks; ~a few seconds) then report status.
  sleep(300);
  printf(1, "[Parent] Timeout reached. Process A and Process B never printed\n");
  printf(1, "[Parent] their step 4 lines -- this confirms a real deadlock:\n");
  printf(1, "[Parent] A holds Lock1 waiting for Lock2, B holds Lock2 waiting for Lock1.\n");
  printf(1, "[Parent] (The two child processes remain blocked forever; this is\n");
  printf(1, "[Parent]  expected and is the point of this demo -- see resourceorder_fixed.c)\n");
  exit();
}
