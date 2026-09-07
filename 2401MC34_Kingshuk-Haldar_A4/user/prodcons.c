#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define BUFFER_SIZE 5
#define TOTAL_ITEMS 20

extern void *shm_get(void);
extern int sem_init(int id, int value);
extern int sem_wait(int id);
extern int sem_post(int id);

/*
 * Semaphore IDs
 */
#define EMPTY 0
#define FULL  1
#define MUTEX 2

struct shared_buffer {
  int buffer[BUFFER_SIZE];
  int in;
  int out;
  int count;
};

void
delay(void)
{
  volatile int i;
  for(i = 0; i < 500000; i++);
}

void
producer(struct shared_buffer *shared)
{
  int item;
  for(item = 1; item <= TOTAL_ITEMS; item++) {
    /*
     * Wait until at least one empty slot exists.
     */
    if(shared->count == BUFFER_SIZE)
      printf("Producer: BUFFER FULL -> waiting\n");

    sem_wait(EMPTY);
    /*
     * Lock buffer before modifying it.
     */
    sem_wait(MUTEX);
    shared->buffer[shared->in] = item;
    printf("Producer produced: %d -> slot %d\n",item, shared->in);
    shared->in = (shared->in + 1) % BUFFER_SIZE;
    shared->count++;

    /*
     * Release buffer mutex.
     */
    sem_post(MUTEX);
    /*
     * Signal that a new item exists.
     */
    sem_post(FULL);
    delay();
  }
  printf("Producer finished\n");
}

void
consumer(struct shared_buffer *shared)
{
  int item;
  int i;
  for(i = 0; i < TOTAL_ITEMS; i++) {
    /*
     * Wait until at least one item exists.
     */
    if(shared->count == 0)
      printf("Consumer: BUFFER EMPTY -> waiting\n");

    sem_wait(FULL);
    /*
     * Lock shared buffer.
     */
    sem_wait(MUTEX);
    item = shared->buffer[shared->out];
    printf("Consumer consumed: %d <- slot %d\n",item, shared->out);
    shared->out =(shared->out + 1) % BUFFER_SIZE;
    shared->count--;
    /*
     * Release mutex.
     */
    sem_post(MUTEX);

    /*
     * Signal that an empty slot exists.
     */
    sem_post(EMPTY);

    delay();
  }

  printf("Consumer finished\n");
}

int
main(int argc, char *argv[])
{
  struct shared_buffer *shared;
  int pid;
  shared = (struct shared_buffer *)shm_get();
  if(shared == 0) {
    printf("prodcons: shared memory failed\n");
    exit(1);
  }
  /*
   * Initialize circular buffer.
   */
  shared->in = 0;
  shared->out = 0;
  shared->count = 0;
  /*
   * Initialize semaphores.
   *
   * empty = BUFFER_SIZE
   * full  = 0
   * mutex = 1
   */
  sem_init(EMPTY, BUFFER_SIZE);
  sem_init(FULL, 0);
  sem_init(MUTEX, 1);
  pid = fork();
  if(pid < 0) {
    printf("prodcons: fork failed\n");
    exit(1);
  }
  if(pid == 0) {
    /*
     * Child = Consumer
     */
    consumer(shared);
  } else {
    /*
     * Parent = Producer
     */
    producer(shared);
    wait(0);
    printf("\nProducer-Consumer completed successfully\n");
  }
  exit(0);
}