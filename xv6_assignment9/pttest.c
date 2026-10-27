// pttest.c - Q1(c): print the page table at start-up and after sbrk(4*4096).
// Expect exactly four more valid user pages in the second dump.
// NOTE: run this with the ORIGINAL (eager) sbrk, i.e. before the Q2 lazy-
// allocation change. With lazy sbrk the new pages would only show up
// after they are touched (see the optional touch loop below).
#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  printf(1, "===== vmprint at start-up =====\n");
  vmprint();

  char *p = sbrk(4 * 4096);          // grow heap by exactly 4 pages
  if(p == (char*)-1){
    printf(1, "pttest: sbrk failed\n");
    exit();
  }

  // Uncomment if sbrk() is lazy (Q2 applied): touching each page forces
  // the kernel to allocate it so it appears in the page table.
  // for(int i = 0; i < 4; i++) p[i * 4096] = 1;

  printf(1, "\n===== vmprint after sbrk(4 * 4096) =====\n");
  vmprint();

  exit();
}
