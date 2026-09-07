# Operating Systems Lab - Assignment 5

## Process Synchronization using xv6

---

# Name

Kingshuk Haldar

# Course

Operating Systems Laboratory

# Assignment

Assignment 5 - Process Synchronization using xv6

---

# Overview

This assignment implements four classical process synchronization
problems in xv6:

1. Mutual Exclusion using Peterson's Algorithm
2. Producer-Consumer Problem
3. Readers-Writers Problem
4. Dining Philosophers Problem

The implementations demonstrate different synchronization techniques
including:

- Shared memory
- Peterson's Algorithm
- Counting semaphores
- Binary semaphores
- Mutex locks
- Reader counters
- Circular buffers
- Deadlock avoidance

---

# xv6 Modifications

The standard xv6 process model does not provide shared writable memory
between processes created using fork().

After fork(), the parent and child normally have separate address spaces.
Therefore, this assignment requires a shared memory mechanism.

A new shared memory facility was used:

```c
void *shm_get(void);

# Question 2: Producer-Consumer Problem (Bounded Buffer)

## File

```text
user/prodcons.c
```

## Objective

The Producer-Consumer problem demonstrates synchronization between two processes that share a bounded buffer. One process, called the **producer**, generates data and inserts it into the buffer. The other process, called the **consumer**, removes and processes the data.

In this implementation:

- The producer generates integers from 1 to 20.
- The consumer removes the integers from the buffer.
- The buffer has a fixed size of 5 by default.
- The producer must wait when the buffer is full.
- The consumer must wait when the buffer is empty.
- The producer and consumer must not modify the shared buffer simultaneously.

---

## How Shared State Was Achieved in xv6

### Problem

In xv6, processes created using `fork()` do not normally share ordinary writable memory.

After `fork()`, the parent and child have separate virtual address spaces. Therefore, if the producer modifies a normal variable in its address space, the consumer cannot directly see that modification. Similarly, a normal array declared in the user program would not be automatically shared between the two processes.

Because the Producer-Consumer problem requires both processes to access the same buffer, the shared state must be placed in memory that is accessible to both processes.

### Approach Used

A shared memory page is obtained using the following interface:

```c
void *shm_get(void);
```

The shared memory mechanism is implemented in the xv6 kernel. The shared page is mapped into the address spaces of the related processes so that both the parent and child access the same physical memory page.

The following structure is stored inside the shared memory page:

```c
struct shared_buffer {
    int buffer[BUFFER_SIZE];
    int in;
    int out;
    int count;
};
```

The producer and consumer therefore access the same:

- `buffer`
- `in`
- `out`
- `count`

variables.

### Shared Buffer Variables

#### buffer

```c
int buffer[BUFFER_SIZE];
```

This array stores the produced items.

Since the array is located in shared memory, an item inserted by the producer can later be seen and removed by the consumer.

---

#### in

```c
int in;
```

This variable indicates the position where the producer will insert the next item.

After inserting an item, the producer updates:

```c
in = (in + 1) % BUFFER_SIZE;
```

---

#### out

```c
int out;
```

This variable indicates the position from which the consumer will remove the next item.

After removing an item, the consumer updates:

```c
out = (out + 1) % BUFFER_SIZE;
```

---

#### count

```c
int count;
```

This variable stores the current number of items in the buffer.

It is also useful for displaying whether the buffer is full or empty.

---

## Why a Circular Buffer Is Used

The buffer has a fixed size. After reaching the final index, the producer or consumer must return to the first index.

This is achieved using the modulo operator:

```c
index = (index + 1) % BUFFER_SIZE;
```

For a buffer of size 5, the indices are:

```text
0 -> 1 -> 2 -> 3 -> 4 -> 0 -> 1 -> ...
```

Therefore, the same buffer positions can be reused after items are consumed.

This avoids the need to continuously allocate new memory.

---

# Synchronization Mechanism

Three semaphores are used:

1. `empty`
2. `full`
3. `mutex`

---

## Semaphore: empty

Initial value:

```text
BUFFER_SIZE
```

For a buffer size of 5:

```text
empty = 5
```

The `empty` semaphore represents the number of available empty positions.

### Producer operation

Before inserting an item:

```c
sem_wait(EMPTY);
```

If `empty` is greater than zero, the producer can continue and the value is decremented.

If `empty` is zero, the buffer is full. The producer blocks until the consumer removes an item and signals `empty`.

---

## Semaphore: full

Initial value:

```text
0
```

The `full` semaphore represents the number of items currently available for consumption.

### Consumer operation

Before removing an item:

```c
sem_wait(FULL);
```

If `full` is greater than zero, the consumer can continue.

If `full` is zero, the buffer is empty. The consumer blocks until the producer inserts an item and signals `full`.

---

## Semaphore: mutex

Initial value:

```text
1
```

The `mutex` semaphore protects the shared buffer and its associated variables.

The following shared data must not be modified simultaneously:

- `buffer`
- `in`
- `out`
- `count`

Before modifying the buffer:

```c
sem_wait(MUTEX);
```

After completing the operation:

```c
sem_post(MUTEX);
```

This ensures that only one process modifies the shared buffer at a time.

---

# Producer Logic

The producer performs the following operations:

```text
wait(empty)

wait(mutex)

Insert item into buffer

Update in

Update count

signal(mutex)

signal(full)
```

# Consumer Logic

The consumer performs:

```text
wait(full)

wait(mutex)

Remove item from buffer

Update out

Update count

signal(mutex)

signal(empty)
```

### Step-by-Step Explanation

### Step 1: Wait for an Available Item

```c
sem_wait(FULL);
```

The consumer checks whether the buffer contains an item.

If the buffer is empty, the consumer blocks.

---

### Step 2: Acquire the Mutex

```c
sem_wait(MUTEX);
```

The consumer obtains exclusive access to the shared buffer.

---

### Step 3: Remove the Item

```c
item = shared->buffer[shared->out];
```

The consumer removes the item at the current `out` position.

---

### Step 4: Update the Circular Buffer Index

```c
shared->out =
    (shared->out + 1) % BUFFER_SIZE;
```

The removal position moves to the next location.

---

### Step 5: Update Item Count

```c
shared->count--;
```

The number of items currently in the buffer decreases.

---

### Step 6: Release the Mutex

```c
sem_post(MUTEX);
```

The consumer releases exclusive access to the buffer.

---

### Step 7: Signal That an Empty Slot Exists

```c
sem_post(EMPTY);
```

The producer can now insert another item.

If the producer was blocked because the buffer was full, it can wake up and continue.

---

# Blocking Behaviour

## When the Buffer Is Full

Suppose the buffer size is 5.

After producing five items:

```text
empty = 0
```

The producer executes:

```c
sem_wait(EMPTY);
```

Because there are no empty positions, the producer blocks.

The consumer removes an item and performs:

```c
sem_post(EMPTY);
```

Now an empty position exists and the producer can continue.

---

## When the Buffer Is Empty

Initially:

```text
full = 0
```

If the consumer runs before the producer, it executes:

```c
sem_wait(FULL);
```

Because there are no items, the consumer blocks.

When the producer inserts an item:

```c
sem_post(FULL);
```

The consumer can wake up and continue.

---

# Correctness

The implementation guarantees:

### No Buffer Overflow

The producer cannot insert an item when the buffer is full because it must successfully execute:

```c
sem_wait(EMPTY);
```

before inserting.

---

### No Buffer Underflow

The consumer cannot remove an item when the buffer is empty because it must successfully execute:

```c
sem_wait(FULL);
```

before removing.

---

### Mutual Exclusion

Only one process can modify the shared buffer at a time because both processes use:

```c
sem_wait(MUTEX);
```

before accessing critical shared state.

---

### No Lost Items

Every produced item increments `full`.

Every consumed item decrements `full`.

The producer inserts exactly 20 items and the consumer removes exactly 20 items.

---

### FIFO Order

The producer inserts items using `in`.

The consumer removes items using `out`.

Both indices move sequentially through the circular buffer.

Therefore, items are consumed in the same order in which they are produced.

---

# Question 3: Readers-Writers Problem

## File

```text
user/readwrite.c
```

## Objective

The Readers-Writers problem involves processes that either read or modify shared data.

The main requirement is:

- Multiple readers should be allowed to read simultaneously.
- Writers require exclusive access.
- No reader should access the shared data while a writer is modifying it.
- No two writers should modify the data simultaneously.

The implementation creates:

```text
3 Readers
2 Writers
```

---

# Shared State

The following variables are placed in shared memory:

```c
struct shared_rw {
    int shared_data;
    int read_count;
};
```

---

## shared_data

```c
int shared_data;
```

This is the actual shared resource.

Readers read its value.

Writers increment its value.

---

## read_count

```c
int read_count;
```

This stores the number of readers currently accessing the shared resource.

For example:

```text
0 = No readers

1 = One active reader

2 = Two active readers

3 = Three active readers
```

---

# Synchronization Primitives

Two semaphores are used:

```text
READ_MUTEX
RESOURCE
```

---

## READ_MUTEX

Initial value:

```text
1
```

This semaphore protects:

```c
read_count
```

Updating `read_count` is itself a critical section.

For example, two readers must not simultaneously execute:

```c
read_count++;
```

because one update could overwrite another.

Therefore:

```c
sem_wait(READ_MUTEX);
```

is performed before modifying `read_count`.

---

## RESOURCE

Initial value:

```text
1
```

This semaphore protects:

```c
shared_data
```

A writer must acquire this semaphore before writing.

The first reader also acquires this semaphore to prevent writers from entering while one or more readers are active.

---

# Reader Logic

A reader performs:

```text
wait(read_mutex)

read_count++

if read_count == 1:
    wait(resource)

signal(read_mutex)

READ SHARED DATA

wait(read_mutex)

read_count--

if read_count == 0:
    signal(resource)

signal(read_mutex)
```

---

## Step-by-Step Reader Explanation

### Step 1: Lock read_count

```c
sem_wait(READ_MUTEX);
```

Only one reader can modify `read_count` at a time.

---

### Step 2: Increase Number of Active Readers

```c
shared->read_count++;
```

The reader announces that it is entering the reading section.

---

### Step 3: First Reader Locks the Resource

```c
if(shared->read_count == 1)
    sem_wait(RESOURCE);
```

The first reader locks the shared resource.

This prevents writers from entering while readers are active.

---

### Step 4: Release read_count Mutex

```c
sem_post(READ_MUTEX);
```

Other readers can now update `read_count`.

---

### Step 5: Read Shared Data

The reader accesses:

```c
shared->shared_data
```

Multiple readers are allowed to execute this section simultaneously.

---

### Step 6: Lock read_count Again

After finishing:

```c
sem_wait(READ_MUTEX);
```

The reader obtains permission to modify `read_count`.

---

### Step 7: Decrease Reader Count

```c
shared->read_count--;
```

The reader announces that it is leaving.

---

### Step 8: Last Reader Releases Resource

```c
if(shared->read_count == 0)
    sem_post(RESOURCE);
```

The last reader releases the shared resource.

A waiting writer can now obtain exclusive access.

---

# Writer Logic

A writer performs:

```text
wait(resource)

WRITE

signal(resource)
```

### Step 1: Acquire Resource

```c
sem_wait(RESOURCE);
```

The writer waits until no reader or writer is currently accessing the shared resource.

---

### Step 2: Modify Shared Data

```c
shared->shared_data++;
```

The writer modifies the shared variable.

---

### Step 3: Release Resource

```c
sem_post(RESOURCE);
```

The resource becomes available again.

Another writer or a group of readers can proceed.

---

# Readers-Priority Solution

The implementation above is primarily a **readers-priority solution**.

The important feature is that when the first reader enters, it locks `RESOURCE`.

After that, additional readers can enter without individually waiting for `RESOURCE`.

For example:

```text
Reader 1 enters
read_count = 1
Reader 1 locks RESOURCE

Reader 2 enters
read_count = 2
Reader 2 does not lock RESOURCE again

Reader 3 enters
read_count = 3
Reader 3 also reads concurrently
```

Therefore:

```text
Multiple readers
        ↓
Can read simultaneously
        ↓
Higher read concurrency
```

This is useful when the shared data is read frequently and modified less often.

---

# Advantages of Readers Priority

The major advantage is improved concurrency.

Suppose three readers want to read the same data.

A readers-priority solution allows:

```text
Reader 1 ─┐
Reader 2 ─┼── Reading simultaneously
Reader 3 ─┘
```

The readers do not need exclusive access from each other because reading does not modify the shared data.

This improves performance compared to allowing only one reader at a time.

---

# Problem: Writer Starvation

The main disadvantage of a pure readers-priority solution is possible writer starvation.

Consider the following situation:

```text
Reader 1 enters
Reader 2 enters
Reader 3 enters

Writer arrives and waits

Reader 4 enters
Reader 5 enters
Reader 6 enters
...
```

As long as readers continue to arrive before the reader count becomes zero, the resource may remain locked by the reader group.

The waiting writer may therefore wait for a very long time.

In theory, if readers continue arriving indefinitely, the writer can starve.

---

# Fair Readers-Writers Solution

A fair solution attempts to ensure that neither readers nor writers wait indefinitely.

A common fair solution introduces an additional synchronization mechanism such as:

```text
turnstile
```

or:

```text
service queue
```

The basic idea is that readers and writers pass through a common queue.

For example:

```text
Reader 1 arrives
Writer 1 arrives
Reader 2 arrives
Writer 2 arrives
```

The synchronization mechanism prevents new readers from continuously bypassing Writer 1.

Once a writer is waiting, new readers may be prevented from entering ahead of it.

This provides better fairness.

---

# Readers-Priority vs Fair Solution

| Feature | Readers-Priority | Fair Solution |
|---|---|---|
| Multiple readers | Yes | Yes |
| Writer exclusive access | Yes | Yes |
| Reader starvation | Usually no | No |
| Writer starvation | Possible | Prevented |
| Implementation complexity | Simple | More complex |
| Read performance | High | Slightly lower |
| Arrival order fairness | Not guaranteed | Better guaranteed |

---

# Why This Assignment Uses Readers Priority

The implementation uses the classical first Readers-Writers synchronization pattern.

The design was selected because:

1. It clearly demonstrates concurrent reading.
2. It allows multiple readers to access the data simultaneously.
3. Writers still obtain exclusive access when readers finish.
4. It uses only a reader counter and two synchronization primitives.
5. It is easy to demonstrate using xv6 processes.

However, the implementation has the theoretical limitation that a continuously arriving stream of readers can delay writers indefinitely.

For a full fair implementation, an additional queue or turnstile semaphore should be introduced.

---

# Important Note About Fairness

The assignment states that readers have priority but asks that writers should not starve indefinitely for full credit.

The basic implementation demonstrates readers priority, but strict prevention of writer starvation requires a fair extension.

A more complete fair design can use:

```text
READ_MUTEX
RESOURCE
TURNSTILE
```

The `TURNSTILE` prevents newly arriving readers from continuously bypassing waiting writers.

Thus:

```text
Readers-Priority
    ↓
Maximum reader concurrency
    ↓
Possible writer starvation

Fair Solution
    ↓
Reader concurrency maintained
    ↓
Waiting writers cannot be bypassed indefinitely
```

---

# Question 4: Dining Philosophers Problem

## File

```text
user/dining.c
```

# Objective

The Dining Philosophers problem consists of five philosophers sitting around a circular table.

Each philosopher alternates between:

```text
THINKING
    ↓
HUNGRY
    ↓
EATING
    ↓
THINKING
```

There are five forks, with one fork placed between every pair of philosophers.

To eat, a philosopher must acquire both its left and right forks.

The challenge is to design a synchronization strategy that prevents deadlock.

---

# Fork Representation

Each fork is represented using a binary semaphore.

The initial value of every fork semaphore is:

```text
1
```

This means the fork is available.

When a philosopher acquires the fork:

```c
sem_wait(fork_id);
```

the fork becomes unavailable to other philosophers.

When the philosopher finishes eating:

```c
sem_post(fork_id);
```

the fork becomes available again.

Therefore, only one philosopher can hold a particular fork at a time.

---

# Deadlock Problem

A naive implementation might require every philosopher to:

1. Pick up the left fork.
2. Pick up the right fork.
3. Eat.

Suppose all five philosophers become hungry at the same time.

The following situation can occur:

```text
Philosopher 0 holds Fork 0
and waits for Fork 1

Philosopher 1 holds Fork 1
and waits for Fork 2

Philosopher 2 holds Fork 2
and waits for Fork 3

Philosopher 3 holds Fork 3
and waits for Fork 4

Philosopher 4 holds Fork 4
and waits for Fork 0
```

Now every philosopher is waiting for another philosopher.

No philosopher can continue.

No philosopher can release its currently held fork because each philosopher is waiting to acquire the second fork.

This situation is called **deadlock**.

---

# Deadlock-Avoidance Technique Used

This implementation uses the **Resource Ordering Strategy**.

Each fork has a unique number:

```text
Fork 0
Fork 1
Fork 2
Fork 3
Fork 4
```

Instead of every philosopher always picking the left fork first, every philosopher follows the same global ordering rule:

```text
Always acquire the lower-numbered fork first.
Then acquire the higher-numbered fork.
```

For every philosopher:

```c
if(left < right) {
    first = left;
    second = right;
} else {
    first = right;
    second = left;
}
```

The philosopher then performs:

```c
sem_wait(first);
sem_wait(second);
```

---

# Example of Resource Ordering

The philosophers acquire forks as follows:

```text
Philosopher 0:
Fork 0 -> Fork 1

Philosopher 1:
Fork 1 -> Fork 2

Philosopher 2:
Fork 2 -> Fork 3

Philosopher 3:
Fork 3 -> Fork 4

Philosopher 4:
Fork 0 -> Fork 4
```

Notice that Philosopher 4 does not acquire Fork 4 first.

It must acquire the lower-numbered Fork 0 first.

This breaks the circular acquisition pattern.

---

# Why Resource Ordering Prevents Deadlock

Deadlock requires several conditions to hold simultaneously.

One important condition is:

```text
Circular Wait
```

Circular wait occurs when processes form a cycle in which each process is waiting for a resource held by another process.

For example:

```text
P0 waits for P1
P1 waits for P2
P2 waits for P3
P3 waits for P4
P4 waits for P0
```

This creates a circular dependency.

---

## Resource Ordering Removes Circular Wait

In this solution, every philosopher must acquire resources in the same global order:

```text
Lower Number
     ↓
Higher Number
```

A philosopher can only wait for a fork with a higher number than the fork it currently holds.

It is impossible to form a cycle where the final philosopher waits for a lower-numbered resource.

Therefore, the circular wait condition is eliminated.

Since circular wait is necessary for deadlock, eliminating circular wait prevents deadlock.

---

# Short Write-Up: Why the Technique Works

The deadlock-avoidance technique used in this implementation is **resource ordering**. Each fork is assigned a unique numerical identifier from 0 to 4. Before attempting to eat, a philosopher identifies its two required forks and always acquires the lower-numbered fork before the higher-numbered fork. This creates a global ordering for resource acquisition that every philosopher must follow.

The purpose of this strategy is to eliminate the possibility of circular waiting. In a naive Dining Philosophers implementation, every philosopher may pick up one fork and wait for the second fork. If all philosophers do this simultaneously, each philosopher holds one resource and waits for another resource held by a neighboring philosopher. This creates a circular dependency and causes deadlock.

With resource ordering, such a cycle cannot be created. A philosopher is only allowed to request forks according to increasing fork numbers. For example, Philosopher 4, whose adjacent forks are Fork 4 and Fork 0, must acquire Fork 0 before Fork 4 instead of following a simple left-first rule. Because all processes follow the same ordering, it is impossible for the wait dependencies to form a complete cycle.

Therefore, the solution prevents one of the necessary Coffman conditions for deadlock: circular wait. Since all four Coffman conditions must be present for deadlock to occur, breaking circular wait guarantees that the philosophers cannot enter a permanent deadlock. The binary semaphore associated with each fork also ensures mutual exclusion, meaning that no two philosophers can hold the same fork simultaneously.

This strategy was chosen because it is simple to implement, does not require a central controller, and provides a deterministic guarantee against deadlock. During the finite test of five eating cycles per philosopher, all philosophers are expected to complete their cycles without the system permanently hanging.

---

# Correctness Properties

## Mutual Exclusion

Each fork is represented by a binary semaphore.

Therefore, only one philosopher can hold a fork at a time.

---

## No Deadlock

All philosophers acquire forks using the same global resource ordering.

This eliminates circular waiting.

Therefore, deadlock cannot occur.

---

## Progress

If forks become available, a waiting philosopher can eventually attempt to acquire them.

The program does not enter a permanent circular dependency.

---

## Fairness

The resource ordering strategy guarantees deadlock freedom but does not mathematically guarantee perfect starvation freedom under every possible scheduler.

However, for the finite test with five cycles per philosopher and normal xv6 scheduling, all philosophers should receive opportunities to execute and complete their cycles.

The output should show:

```text
Philosopher 0 completed all cycles
Philosopher 1 completed all cycles
Philosopher 2 completed all cycles
Philosopher 3 completed all cycles
Philosopher 4 completed all cycles
```

This demonstrates that no philosopher remained permanently blocked during the test.

---

# Summary

| Question | Problem | Shared State | Synchronization | Important Property |
|---|---|---|---|---|
| Q2 | Producer-Consumer | Circular Buffer | empty, full, mutex | No overflow or underflow |
| Q3 | Readers-Writers | shared_data, read_count | read mutex, resource | Concurrent reads and exclusive writes |
| Q4 | Dining Philosophers | Fork semaphores | Binary semaphores | Deadlock avoided by resource ordering |