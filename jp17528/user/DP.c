/*
  Strategy in Dining Philosopher (DP)

  This problem implemented Chandy-Misra solution for DP from following URL:
  https://en.wikipedia.org/wiki/Dining_philosophers_problem

  * Create forks and give them to the philosophers. Each for can either be 'dirty' or 'clean'.
    dirty == 1, clean == 0

  * Philosopher always get LHS fork first and RHS fork afterwards.

  * Semaphore for each fork

  * Ensures mutual exclusion and preventing race conditions:
    Mutexes guarantee the philosopher not to be interrupted when they are changing their states such as:
      1. picking up the forks
      2. putting down the forks

  * Solving starvation problem:
    Clean/dirty labels prevents starvation problem by giving
      1. advantage to the most starved processes
      2. disadvantage to processes that have just eaten

  main_DP()

    1. Create philosophers. i is equal to id for philosophers.
        ex) i == 0 -> 0-th philosopher

    2. Lock 'picking up forks' process by sem_wait()          2 ~ 3 is critical section

    3. Pick up forks and release lock by sem_post()

    4. Eat

    5. Put down forks                                         putting down forks is critical section
*/
#include "DP.h"

// make 16 Fork for 16 philosopher child processes.
int forks[16] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
int lock = 1;
int choose_fork(int index, char c);

void main_DP() {
  write( STDOUT_FILENO, "main_DP() is now running", 24 );

  for (int i = 0; i < 16; i++) {
    if (fork() == 0) {
      char philosopher[2] = { i / 10 + '0', i % 10 + '0' };
      while (1) {

        sleep();

        write( STDOUT_FILENO, philosopher, 2 );
        write( STDOUT_FILENO, " is thinking\n", 13 );

        sem_wait(&lock);

        int left = choose_fork(i, 'l');
        int right = choose_fork(i, 'r');
        if (forks[left] == 1 && forks[right] == 1) {
          sem_wait(&forks[left]);
          sem_wait(&forks[right]);
          sem_post(&lock);

          write( STDOUT_FILENO, philosopher, 2 );
          write( STDOUT_FILENO, " PICKS UP both forks\n", 21 );

        } else {
          sem_post(&lock);

          write( STDOUT_FILENO, philosopher, 2 );
          write( STDOUT_FILENO, " FAILS PICKING UP both forks\n", 29 );

          continue;
        }

        write( STDOUT_FILENO, philosopher, 2);
        write( STDOUT_FILENO, " is eating\n", 11 );

        sleep();

        write( STDOUT_FILENO, philosopher, 2 );
        write( STDOUT_FILENO, " finishes eating\n", 17 );

        sem_wait(&lock);
        sem_post(&forks[left]);
        sem_post(&forks[right]);
        sem_post(&lock);

        write( STDOUT_FILENO, philosopher, 2 );
        write( STDOUT_FILENO, " puts down forks\n", 17 );
      }
    }
  }
  exit( EXIT_SUCCESS );
}

/******************************************************************************/

int choose_fork(int index, char c) {
  if (c == 'l') {
    return index;
  } else {
    return (index + 1) % 16;
  }
}
