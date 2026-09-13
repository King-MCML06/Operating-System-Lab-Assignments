// syncdeadlock.c — Q4: 5 processes, 3 shared resource types with limited
// instances (2 printers, 1 scanner, 2 disks). Each process needs 2 of the
// 3 resource types to complete a task, runs multiple cycles, and must
// never deadlock or over-allocate a resource pool.
//
// Deadlock-avoidance strategy used: RESOURCE ORDERING (same technique as
// Q3), reused here rather than a Banker's-Algorithm admission check.
// Justification: resource ordering is simpler to reason about and to
// verify correct (no runtime safety-algorithm bookkeeping needed), and
// it structurally eliminates circular wait regardless of scheduling or
// timing, which is exactly the failure mode this setup could otherwise
// hit (P0 wants Printer+Scanner, P1 wants Scanner+Disk, P2 wants
// Disk+Printer -- a classic 3-way circular dependency if each process
// grabbed its two resources in its own preferred order). Forcing a
// single global order (Printer < Scanner < Disk) for every process,
// regardless of which two resources it personally needs, removes that
// risk entirely.
//
// Resource pools with >1 instance are implemented as counting semaphores
// (a pipe pre-loaded with N tokens): each acquire() reads one token
// (blocking if none are available), each release() writes one back. This
// guarantees the pool is never over-allocated, enforced by the kernel's
// pipe buffer itself.
//
// Add "_syncdeadlock\" to UPROGS in the Makefile.

#include "types.h"
#include "stat.h"
#include "user.h"

#define NPROCS   5
#define NRES     3
#define CYCLES   3

#define RES_PRINTER 0
#define RES_SCANNER 1
#define RES_DISK    2

int capacity[NRES] = {2, 1, 2};
char *resname[NRES] = {"Printer", "Scanner", "Disk"};

// each process needs exactly these two resource types
int proc_needs[NPROCS][2] = {
  {RES_PRINTER, RES_SCANNER},
  {RES_SCANNER, RES_DISK},
  {RES_DISK,    RES_PRINTER},
  {RES_PRINTER, RES_SCANNER},
  {RES_SCANNER, RES_DISK},
};

typedef struct {
  int fd[2];
} sem_t;

sem_t sem[NRES];

void
sem_init(sem_t *s, int count)
{
  int i;
  pipe(s->fd);
  for(i = 0; i < count; i++)
    write(s->fd[1], "x", 1);
}

void
sem_wait(sem_t *s)
{
  char buf;
  read(s->fd[0], &buf, 1);
}

void
sem_post(sem_t *s)
{
  write(s->fd[1], "x", 1);
}

void
busy_work(int loops)
{
  volatile int x = 0;
  int i;
  for(i = 0; i < loops; i++)
    x += i;
}

void
run_process(int id)
{
  int a = proc_needs[id][0];
  int b = proc_needs[id][1];
  int lo = (a < b) ? a : b;   // enforce global resource ordering:
  int hi = (a < b) ? b : a;   // always acquire the lower-ranked one first
  int cycle;

  for(cycle = 0; cycle < CYCLES; cycle++){
    printf(1, "[P%d] cycle %d: requesting %s\n", id, cycle, resname[lo]);
    sem_wait(&sem[lo]);
    printf(1, "[P%d] cycle %d: granted %s\n", id, cycle, resname[lo]);

    printf(1, "[P%d] cycle %d: requesting %s\n", id, cycle, resname[hi]);
    sem_wait(&sem[hi]);
    printf(1, "[P%d] cycle %d: granted %s\n", id, cycle, resname[hi]);

    printf(1, "[P%d] cycle %d: working with %s + %s\n",
           id, cycle, resname[lo], resname[hi]);
    busy_work(500000 + id * 100000);   // stagger workload a bit per process

    sem_post(&sem[hi]);
    sem_post(&sem[lo]);
    printf(1, "[P%d] cycle %d: released %s and %s\n",
           id, cycle, resname[hi], resname[lo]);
  }

  printf(1, "[P%d] finished all %d cycles\n", id, CYCLES);
}

int
main(void)
{
  int i;
  int pids[NPROCS];

  sem_init(&sem[RES_PRINTER], capacity[RES_PRINTER]);
  sem_init(&sem[RES_SCANNER], capacity[RES_SCANNER]);
  sem_init(&sem[RES_DISK],    capacity[RES_DISK]);

  printf(1, "=== syncdeadlock: 5 processes, 3 resource pools ===\n");
  printf(1, "Capacities -- Printer:%d Scanner:%d Disk:%d\n",
         capacity[RES_PRINTER], capacity[RES_SCANNER], capacity[RES_DISK]);
  printf(1, "Avoidance strategy: global resource ordering (Printer < Scanner < Disk)\n\n");

  for(i = 0; i < NPROCS; i++){
    pids[i] = fork();
    if(pids[i] == 0){
      run_process(i);
      exit();
    }
  }

  for(i = 0; i < NPROCS; i++)
    wait();

  printf(1, "\nAll %d processes completed %d cycles each with no deadlock.\n",
         NPROCS, CYCLES);
  exit();
}
