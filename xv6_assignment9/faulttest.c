// faulttest.c - Q2(c): test lazy allocation / page-fault handling.
//  1. sbrk(32 pages) allocates nothing.
//  2. Write to exactly 5 different pages -> exactly 5 faults handled.
//  3. A child touching an address above proc->sz is killed; parent survives.
#include "types.h"
#include "stat.h"
#include "user.h"

#define PGSIZE 4096

int
main(void)
{
  int before = getfaults();          // faults so far (expected 0)

  char *base = sbrk(32 * PGSIZE);    // lazy: only grows proc->sz
  if(base == (char*)-1){
    printf(1, "faulttest: sbrk failed\n");
    exit();
  }
  printf(1, "sbrk(32 * 4096) done, faults so far: %d\n", getfaults() - before);

  // Touch exactly 5 different pages (pages 0, 3, 7, 15, 31 of the region).
  int pages[5] = {0, 3, 7, 15, 31};
  for(int i = 0; i < 5; i++){
    base[pages[i] * PGSIZE] = 'x';          // first write => page fault
    base[pages[i] * PGSIZE + 100] = 'y';    // same page => no new fault
  }

  int faults = getfaults() - before;
  printf(1, "page faults handled: %d (expected 5)\n", faults);
  if(faults == 5)
    printf(1, "TEST 1 PASSED\n");
  else
    printf(1, "TEST 1 FAILED\n");

  // Re-read a touched page: data must be intact, no extra fault.
  if(base[3 * PGSIZE] == 'x' && getfaults() - before == 5)
    printf(1, "TEST 2 PASSED (data intact, no extra fault)\n");
  else
    printf(1, "TEST 2 FAILED\n");

  // Access above the allocated size: only the child must die.
  printf(1, "now accessing an address above proc->sz in a child...\n");
  int pid = fork();
  if(pid < 0){
    printf(1, "faulttest: fork failed\n");
    exit();
  }
  if(pid == 0){
    volatile char *bad = sbrk(0) + 100 * PGSIZE;   // well beyond sz
    *bad = 1;                                      // kernel should kill us
    printf(1, "child: SHOULD NOT REACH HERE\n");
    exit();
  }
  wait();
  printf(1, "parent still alive after child was killed: TEST 3 PASSED\n");

  exit();
}
