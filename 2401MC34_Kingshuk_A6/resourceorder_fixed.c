// resourceorder_fixed.c — Q3: the FIX. Same workload as
// resourceorder_bad.c, except both processes now acquire locks in the
// same global order (always Lock1 before Lock2), which eliminates the
// circular-wait condition and lets both processes finish normally.
//
// Add "_resourceorder_fixed\" to UPROGS in the Makefile.

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
  write(l->fd[1], "x", 1);
}

void
lock_acquire(lock_t *l)
{
  char buf;
  read(l->fd[0], &buf, 1);
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

  printf(1, "=== resourceorder_fixed: consistent lock ordering ===\n");

  pidA = fork();
  if(pidA == 0){
    // Process A's "natural" need is Lock1 then Lock2 -- already the
    // global order, so nothing changes for A.
    printf(1, "[Process A] step 1: acquiring Lock1 (global order pos 1)\n");
    lock_acquire(&lock1);
    printf(1, "[Process A] step 2: acquired Lock1, now delaying\n");
    busy_delay(2000000);
    printf(1, "[Process A] step 3: acquiring Lock2 (global order pos 2)\n");
    lock_acquire(&lock2);
    printf(1, "[Process A] step 4: acquired both locks, doing work\n");
    lock_release(&lock2);
    lock_release(&lock1);
    printf(1, "[Process A] step 5: released both locks, done\n");
    exit();
  }

  pidB = fork();
  if(pidB == 0){
    // Process B's "natural" need was Lock2 then Lock1 (as in the bad
    // version) -- under the fix, it must still request Lock1 FIRST
    // because that's the agreed global order, even though it doesn't
    // "need" it first for its own work. This is the whole point of
    // resource ordering: every process obeys the same global order,
    // regardless of its own natural acquisition preference.
    printf(1, "[Process B] step 1: acquiring Lock1 (global order pos 1)\n");
    lock_acquire(&lock1);
    printf(1, "[Process B] step 2: acquired Lock1, now delaying\n");
    busy_delay(2000000);
    printf(1, "[Process B] step 3: acquiring Lock2 (global order pos 2)\n");
    lock_acquire(&lock2);
    printf(1, "[Process B] step 4: acquired both locks, doing work\n");
    lock_release(&lock2);
    lock_release(&lock1);
    printf(1, "[Process B] step 5: released both locks, done\n");
    exit();
  }

  wait();
  wait();
  printf(1, "[Parent] Both processes completed -- no deadlock.\n");
  exit();
}

/*
 * Why consistent ordering eliminates circular wait:
 *
 * Deadlock via the Coffman conditions requires all four of: mutual
 * exclusion, hold-and-wait, no preemption, AND circular wait. Resource
 * ordering attacks circular wait directly: assign every resource a fixed
 * global rank (Lock1 = 1, Lock2 = 2, ...) and require every process to
 * request resources in strictly increasing rank order.
 *
 * With that rule, whichever process manages to acquire the LOWEST-ranked
 * resource it needs first can never be blocked by a process that is
 * itself waiting on that same low-ranked resource (that process, by the
 * rule, would have had to acquire it before requesting anything higher-
 * ranked). So the only kind of "wait" that can occur is a process
 * waiting for a strictly higher-ranked resource held by another process --
 * which can never point back around to a lower-ranked resource the first
 * process already holds. There is no way to build a cycle out of edges
 * that all point in the same direction (increasing rank), so a circular
 * wait -- and therefore deadlock -- becomes structurally impossible,
 * regardless of timing or scheduling.
 */
