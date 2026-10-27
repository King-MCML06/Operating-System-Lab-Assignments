// pagerepl.c - Q3: compare FIFO and LRU page replacement.
//
// Usage:
//   pagerepl                          -> default string, 3 frames, then table
//   pagerepl <frames> <r1> <r2> ...   -> your own frames/reference string
//
// Prints every reference with the frame contents and HIT/FAULT, then a
// table for 3, 4 and 5 frames. (xv6 has no floating point, so the hit
// ratio is printed with integer arithmetic to 2 decimals.)
#include "types.h"
#include "stat.h"
#include "user.h"

#define MAXREF   128
#define MAXFRAME 16

static int refs[MAXREF];
static int nrefs;
static int frames[MAXFRAME];
static int stamp[MAXFRAME];   // LRU: time of last use of each frame
static int nframes;

static void
printframes(int ref, int hit)
{
  printf(1, "ref %d: [", ref);
  for(int i = 0; i < nframes; i++){
    if(frames[i] < 0)
      printf(1, " -");
    else
      printf(1, " %d", frames[i]);
  }
  printf(1, " ]  %s\n", hit ? "HIT" : "FAULT");
}

static int
find(int page)
{
  for(int i = 0; i < nframes; i++)
    if(frames[i] == page)
      return i;
  return -1;
}

// FIFO: replace the page that has been in memory the longest.
static int
fifo(int verbose)
{
  int faults = 0, next = 0;   // 'next' = oldest frame (round-robin pointer)
  for(int i = 0; i < nframes; i++)
    frames[i] = -1;

  for(int t = 0; t < nrefs; t++){
    int hit = (find(refs[t]) >= 0);
    if(!hit){
      faults++;
      frames[next] = refs[t];          // fills empty slots first, then evicts oldest
      next = (next + 1) % nframes;
    }
    if(verbose)
      printframes(refs[t], hit);
  }
  return faults;
}

// LRU: replace the page whose last use is the oldest.
static int
lru(int verbose)
{
  int faults = 0;
  for(int i = 0; i < nframes; i++){
    frames[i] = -1;
    stamp[i] = -1;
  }

  for(int t = 0; t < nrefs; t++){
    int idx = find(refs[t]);
    int hit = (idx >= 0);
    if(hit){
      stamp[idx] = t;                  // refresh recency on a hit
    } else {
      faults++;
      int victim = 0;                  // empty frame (stamp -1) or least recently used
      for(int i = 1; i < nframes; i++)
        if(stamp[i] < stamp[victim])
          victim = i;
      frames[victim] = refs[t];
      stamp[victim] = t;
    }
    if(verbose)
      printframes(refs[t], hit);
  }
  return faults;
}

// Print hits*100/refs as "xx.yy" using integers only.
static void
printratio(int hits, int total)
{
  int v = hits * 10000 / total;        // ratio in hundredths of a percent
  int whole = v / 100, frac = v % 100;
  printf(1, "%d.%s%d%%", whole, frac < 10 ? "0" : "", frac);
}

static void
table(void)
{
  printf(1, "\nFrames | FIFO faults  FIFO hit%%  | LRU faults  LRU hit%%\n");
  printf(1, "-------+-------------------------+----------------------\n");
  for(int f = 3; f <= 5; f++){
    nframes = f;
    int ff = fifo(0);
    int lf = lru(0);
    printf(1, "  %d    |     %d         ", f, ff);
    printratio(nrefs - ff, nrefs);
    printf(1, "    |    %d        ", lf);
    printratio(nrefs - lf, nrefs);
    printf(1, "\n");
  }
}

int
main(int argc, char *argv[])
{
  int def[] = {7,0,1,2,0,3,0,4,2,3,0,3,2,1,2,0,1,7,0,1};

  if(argc >= 3){
    nframes = atoi(argv[1]);
    nrefs = argc - 2;
    if(nframes < 1 || nframes > MAXFRAME || nrefs > MAXREF){
      printf(1, "usage: pagerepl <frames 1-%d> <ref> <ref> ... (max %d refs)\n",
             MAXFRAME, MAXREF);
      exit();
    }
    for(int i = 0; i < nrefs; i++)
      refs[i] = atoi(argv[i + 2]);
  } else {
    nframes = 3;
    nrefs = sizeof(def) / sizeof(def[0]);
    for(int i = 0; i < nrefs; i++)
      refs[i] = def[i];
  }

  int f;
  printf(1, "=== FIFO, %d frames ===\n", nframes);
  f = fifo(1);
  printf(1, "FIFO page faults: %d\n\n", f);

  printf(1, "=== LRU, %d frames ===\n", nframes);
  f = lru(1);
  printf(1, "LRU page faults: %d\n", f);

  table();
  exit();
}
