#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

// [A9-Q2] Set to 0 to get the original eager sbrk() (useful for the Q1
// demo, where sbrk(4*4096) must immediately add four mapped pages).
#define LAZY_SBRK 1

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
#if LAZY_SBRK
  if(n > 0){
    // [A9-Q2] Lazy allocation: only grow the logical size. No kalloc()
    // here; the page is allocated by the T_PGFLT handler in trap.c on
    // first access.
    if(addr + n >= KERNBASE || addr + n < addr)
      return -1;
    myproc()->sz += n;
    return addr;
  }
#endif
  // n <= 0 (or eager mode): shrinking still goes through growproc() ->
  // deallocuvm(), which skips pages that were never allocated.
  if(growproc(n) < 0)
    return -1;
  return addr;
}

// [A9-Q1] int vmprint(void): dump the calling process's user page table.
int
sys_vmprint(void)
{
  vmprint(myproc()->pgdir);
  return 0;
}

// [A9-Q2] int getfaults(void): page faults handled for this process.
int
sys_getfaults(void)
{
  return myproc()->faults;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
