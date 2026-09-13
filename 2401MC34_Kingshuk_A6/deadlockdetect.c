// deadlockdetect.c — Q2: Deadlock detection using a Resource Allocation
// Graph reduced to a wait-for graph, with DFS-based cycle detection.
//
// Model: alloc[i][r] = 1 if process i currently holds resource r.
//        request[i][r] = 1 if process i is currently waiting for resource r.
// Wait-for edge i -> j exists if process i is waiting for some resource
// that process j currently holds.
//
// Two hardcoded scenarios are run: one acyclic (no deadlock) and one with
// a 3-process circular wait (deadlock).
//
// Add "_deadlockdetect\" to UPROGS in the Makefile.

#include "types.h"
#include "stat.h"
#include "user.h"

#define MAXP 10
#define MAXR 10

int wait_for[MAXP][MAXP];
int n_proc;

// DFS coloring: 0 = unvisited (white), 1 = in current path (gray), 2 = done (black)
int color[MAXP];
int path[MAXP];
int path_len;

void
print_cycle(int start_idx)
{
  int i;
  printf(1, "Deadlock detected! Cycle: ");
  for(i = start_idx; i < path_len; i++){
    printf(1, "P%d -> ", path[i]);
  }
  printf(1, "P%d\n", path[start_idx]);
}

// Returns 1 if a cycle was found (and prints it), else 0.
int
dfs(int u)
{
  int v;
  color[u] = 1;
  path[path_len++] = u;

  for(v = 0; v < n_proc; v++){
    if(!wait_for[u][v])
      continue;
    if(color[v] == 1){
      // found a back-edge into the current path -> cycle.
      // find where v first appears in path[] to print the exact cycle.
      int i, start = 0;
      for(i = 0; i < path_len; i++){
        if(path[i] == v){
          start = i;
          break;
        }
      }
      print_cycle(start);
      return 1;
    }
    if(color[v] == 0){
      if(dfs(v))
        return 1;
    }
  }

  path_len--;
  color[u] = 2;
  return 0;
}

int
detect_deadlock(void)
{
  int i;
  for(i = 0; i < n_proc; i++)
    color[i] = 0;
  path_len = 0;

  for(i = 0; i < n_proc; i++){
    if(color[i] == 0){
      path_len = 0;
      if(dfs(i))
        return 1;
    }
  }
  return 0;
}

void
build_wait_for(int n_procs, int n_res, int alloc[MAXP][MAXR], int request[MAXP][MAXR])
{
  int i, j, r;
  n_proc = n_procs;

  for(i = 0; i < n_proc; i++)
    for(j = 0; j < n_proc; j++)
      wait_for[i][j] = 0;

  for(i = 0; i < n_proc; i++){
    for(r = 0; r < n_res; r++){
      if(!request[i][r])
        continue;
      for(j = 0; j < n_proc; j++){
        if(i == j)
          continue;
        if(alloc[j][r]){
          wait_for[i][j] = 1;
        }
      }
    }
  }
}

void
print_wait_for(void)
{
  int i, j;
  printf(1, "Wait-for graph edges:\n");
  for(i = 0; i < n_proc; i++)
    for(j = 0; j < n_proc; j++)
      if(wait_for[i][j])
        printf(1, "  P%d -> P%d\n", i, j);
}

int
main(void)
{
  printf(1, "=== Scenario 1: no deadlock (acyclic wait-for graph) ===\n");
  {
    // 3 processes, 3 resources
    // P0 holds R0, waits for R1 (held by P1)
    // P1 holds R1, waits for nothing
    // P2 holds R2, waits for R1 (held by P1)
    int alloc[MAXP][MAXR]   = {0};
    int request[MAXP][MAXR] = {0};

    alloc[0][0] = 1; request[0][1] = 1;
    alloc[1][1] = 1;                       // P1 wants nothing
    alloc[2][2] = 1; request[2][1] = 1;

    build_wait_for(3, 3, alloc, request);
    print_wait_for();

    if(detect_deadlock())
      printf(1, "Result: DEADLOCK\n");
    else
      printf(1, "Result: NO DEADLOCK\n");
  }

  printf(1, "\n=== Scenario 2: deadlock (3-process circular wait) ===\n");
  {
    // 4 processes, 3 resources
    // P0 holds R0, waits for R1 (held by P1)
    // P1 holds R1, waits for R2 (held by P2)
    // P2 holds R2, waits for R0 (held by P0)   <-- closes the cycle
    // P3 holds nothing, waits for nothing -- present just to show it
    //     doesn't interfere with detecting the cycle among the others.
    int alloc[MAXP][MAXR]   = {0};
    int request[MAXP][MAXR] = {0};

    alloc[0][0] = 1; request[0][1] = 1;
    alloc[1][1] = 1; request[1][2] = 1;
    alloc[2][2] = 1; request[2][0] = 1;
    // P3: no allocation, no request

    build_wait_for(4, 3, alloc, request);
    print_wait_for();

    if(detect_deadlock())
      printf(1, "Result: DEADLOCK\n");
    else
      printf(1, "Result: NO DEADLOCK\n");
  }

  exit();
}
