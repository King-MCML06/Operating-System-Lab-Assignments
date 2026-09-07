#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define NUM_READERS 3
#define NUM_WRITERS 2

#define READ_MUTEX 0
#define RESOURCE   1

extern void *shm_get(void);
extern int sem_init(int id, int value);
extern int sem_wait(int id);
extern int sem_post(int id);

struct shared_rw {
  int shared_data;
  int read_count;
};

void
delay(void)
{
  volatile int i;
  for(i = 0; i < 800000; i++);
}

void
reader(struct shared_rw *shared, int id)
{
  int value;
  /*
   * Protect read_count.
   */
  sem_wait(READ_MUTEX);
  shared->read_count++;
  /*
   * First reader locks the resource.
   *
   * This prevents writers from entering.
   */
  if(shared->read_count == 1)
    sem_wait(RESOURCE);

  sem_post(READ_MUTEX);

  /*
   * READING SECTION
   *
   * Multiple readers may execute here
   * simultaneously.
   */
  value = shared->shared_data;
  printf("READER %d [PID %d]: reading value = %d, active readers = %d\n",id, getpid(), value, shared->read_count);
  delay();

  /*
   * EXIT READING SECTION
   */
  sem_wait(READ_MUTEX);
  shared->read_count--;
  /*
   * Last reader releases the resource,
   * allowing a writer to enter.
   */
  if(shared->read_count == 0)
    sem_post(RESOURCE);

  sem_post(READ_MUTEX);
  printf("READER %d [PID %d]: finished reading\n",id, getpid());
}

void
writer(struct shared_rw *shared, int id)
{
  int value;
  /*
   * Writer requires exclusive access.
   */
  sem_wait(RESOURCE);
  shared->shared_data++;
  value = shared->shared_data;
  printf("WRITER %d [PID %d]: writing new value = %d\n",id, getpid(), value);
  delay();
  sem_post(RESOURCE);
  printf("WRITER %d [PID %d]: finished writing\n",id, getpid());
}

int
main(int argc, char *argv[])
{
  struct shared_rw *shared;
  int i;
  int pid;
  shared = (struct shared_rw *)shm_get();
  if(shared == 0) {
    printf("readwrite: shared memory failed\n");
    exit(1);
  }
  shared->shared_data = 0;
  shared->read_count = 0;

  /*
   * read_count mutex
   */
  sem_init(READ_MUTEX, 1);

  /*
   * Protect shared_data
   */
  sem_init(RESOURCE, 1);

  /*
   * Create readers.
   */
  for(i = 0; i < NUM_READERS; i++) {
    pid = fork();
    if(pid == 0) {
      reader(shared, i);
      exit(0);
    }
  }

  /*
   * Create writers.
   */
  for(i = 0; i < NUM_WRITERS; i++) {
    pid = fork();
    if(pid == 0) {
      writer(shared, i);
      exit(0);
    }
  }

  /*
   * Parent waits for all children.
   */
  for(i = 0;i < NUM_READERS + NUM_WRITERS;i++) {
    wait(0);
  }
  printf("\nFinal shared_data = %d\n",shared->shared_data);
  exit(0);
}