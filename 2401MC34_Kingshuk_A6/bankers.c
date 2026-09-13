// bankers.c — Q1: Banker's Algorithm for deadlock avoidance.
//
// Pure user-space simulation: 5 processes, 3 resource types, using the
// classic textbook scenario (Silberschatz et al.) so the numbers are easy
// to sanity-check against a reference if needed.
//
// Usage:
//   bankers                     -> runs the built-in demo (safe sequence,
//                                  one granted request, one denied request)
//   bankers <pid> <r0> <r1> <r2> -> treats this as a single custom
//                                  Resource-Request-Algorithm call against
//                                  the same hardcoded initial state.
//
// Add "_bankers\" to UPROGS in the Makefile.

#include "types.h"
#include "stat.h"
#include "user.h"

#define N 5   // number of processes
#define M 3   // number of resource types

int allocation[N][M] = {
  {0, 1, 0},
  {2, 0, 0},
  {3, 0, 2},
  {2, 1, 1},
  {0, 0, 2},
};

int maxdemand[N][M] = {
  {7, 5, 3},
  {3, 2, 2},
  {9, 0, 2},
  {2, 2, 2},
  {4, 3, 3},
};

int available[M] = {3, 3, 2};   // total (10,5,7) minus what's allocated above

int need[N][M];

void
compute_need(void)
{
  int i, j;
  for(i = 0; i < N; i++)
    for(j = 0; j < M; j++)
      need[i][j] = maxdemand[i][j] - allocation[i][j];
}

void
print_state(void)
{
  int i, j;
  printf(1, "\n--- Current state ---\n");
  printf(1, "Available: ");
  for(j = 0; j < M; j++)
    printf(1, "%d ", available[j]);
  printf(1, "\n\n%s", "Proc  Allocation      Max             Need\n");
  for(i = 0; i < N; i++){
    printf(1, "P%d    ", i);
    for(j = 0; j < M; j++) printf(1, "%d ", allocation[i][j]);
    printf(1, "     ");
    for(j = 0; j < M; j++) printf(1, "%d ", maxdemand[i][j]);
    printf(1, "     ");
    for(j = 0; j < M; j++) printf(1, "%d ", need[i][j]);
    printf(1, "\n");
  }
}

// Safety algorithm. Fills safe_seq[] with a valid finishing order if one
// exists. Returns 1 if safe, 0 if not. Operates on a *copy* of available
// (work[]) passed in, and a local finished[] tracker, so it never mutates
// global state itself — the caller decides whether to commit.
int
is_safe(int work[M], int alloc_copy[N][M], int need_copy[N][M], int safe_seq[N])
{
  int finished[N];
  int i, j, k, count = 0;

  for(i = 0; i < N; i++)
    finished[i] = 0;

  while(count < N){
    int found = 0;
    for(i = 0; i < N; i++){
      if(finished[i])
        continue;
      int can_run = 1;
      for(j = 0; j < M; j++){
        if(need_copy[i][j] > work[j]){
          can_run = 0;
          break;
        }
      }
      if(can_run){
        for(k = 0; k < M; k++)
          work[k] += alloc_copy[i][k];
        finished[i] = 1;
        safe_seq[count++] = i;
        found = 1;
      }
    }
    if(!found)
      break;   // no process could proceed this pass -> unsafe
  }

  return (count == N);
}

void
print_safe_sequence(int safe_seq[N])
{
  int i;
  printf(1, "Safe sequence: ");
  for(i = 0; i < N; i++){
    printf(1, "P%d", safe_seq[i]);
    if(i < N - 1) printf(1, " -> ");
  }
  printf(1, "\n");
}

// Resource-Request Algorithm for process p. Returns 1 if granted (and
// commits the change to global state), 0 if denied (state left untouched).
int
request_resources(int p, int req[M])
{
  int i;
  int work[M];
  int alloc_copy[N][M];
  int need_copy[N][M];
  int safe_seq[N];

  printf(1, "\nProcess P%d requests: ", p);
  for(i = 0; i < M; i++) printf(1, "%d ", req[i]);
  printf(1, "\n");

  // Step 1: request must not exceed the process's declared need
  for(i = 0; i < M; i++){
    if(req[i] > need[p][i]){
      printf(1, "ERROR: request exceeds Need[%d] -- denied\n", p);
      return 0;
    }
  }

  // Step 2: request must not exceed what's currently available
  for(i = 0; i < M; i++){
    if(req[i] > available[i]){
      printf(1, "Request denied -- exceeds Available, process must wait\n");
      return 0;
    }
  }

  // Step 3: pretend to allocate, then re-run the safety algorithm
  for(i = 0; i < M; i++) work[i] = available[i];
  memmove(alloc_copy, allocation, sizeof(allocation));
  memmove(need_copy, need, sizeof(need));

  for(i = 0; i < M; i++){
    work[i]         -= req[i];
    alloc_copy[p][i] += req[i];
    need_copy[p][i]  -= req[i];
  }

  if(is_safe(work, alloc_copy, need_copy, safe_seq)){
    // commit -- the tentative state becomes real
    for(i = 0; i < M; i++){
      available[i]      -= req[i];
      allocation[p][i]  += req[i];
      need[p][i]        -= req[i];
    }
    printf(1, "Request GRANTED -- resulting state is safe\n");
    print_safe_sequence(safe_seq);
    return 1;
  } else {
    printf(1, "Request denied -- would lead to unsafe state\n");
    return 0;
  }
}

int
main(int argc, char *argv[])
{
  compute_need();

  if(argc == 2 + M){
    // custom request via argv: bankers <pid> <r0> <r1> <r2>
    int p = atoi(argv[1]);
    int req[M];
    int i;
    for(i = 0; i < M; i++)
      req[i] = atoi(argv[2 + i]);

    if(p < 0 || p >= N){
      printf(1, "invalid process id\n");
      exit();
    }

    print_state();
    request_resources(p, req);
    print_state();
    exit();
  }

  // ---- Default demo ----
  printf(1, "=== Banker's Algorithm demo ===\n");
  print_state();

  {
    int work[M];
    int safe_seq[N];
    int i;
    for(i = 0; i < M; i++) work[i] = available[i];
    printf(1, "\nChecking initial safety...\n");
    if(is_safe(work, allocation, need, safe_seq)){
      printf(1, "System is in a SAFE state.\n");
      print_safe_sequence(safe_seq);
    } else {
      printf(1, "System is UNSAFE.\n");
    }
  }

  printf(1, "\n=== Scenario 1: a request that should be GRANTED ===\n");
  {
    int req1[M] = {1, 0, 2};   // P1 requests (1,0,2)
    request_resources(1, req1);
  }

  printf(1, "\n=== Scenario 2: a request that should be DENIED ===\n");
  {
    int req2[M] = {0, 2, 0};   // P0 requests (0,2,0) -- exceeds safety margin
    request_resources(0, req2);
  }

  print_state();
  printf(1, "\nDone.\n");
  exit();
}
