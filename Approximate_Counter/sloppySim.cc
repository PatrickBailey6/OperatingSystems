/* Patrick Bailey */

#include<semaphore.h>
#include<iostream>
#include<vector>
#include<pthread.h>
#include<unistd.h>


using std::cout;
using std::endl;


struct shared {
  int n_threads;
  int work_time;
  int work_iterations;
  bool cpu_bound;
  bool do_logging;
};

struct targs {
  counter_t* c;
  shared* sh;
  int local;
  pthread_mutex_t llock;  // local lock
};

struct counter_t {
  int global;
  pthread_mutex_t glock;  // global lock
  int threshold;
};

/*  Args: 1. Global Counter 2. Thread Reference 3. Count to add*/
void update(targs *ta, int amount) {
  pthread_mutex_lock(&ta->llock);  // Lock Local
  ta->local += amount;
  if (ta->local >= ta->c->threshold) {
    pthread_mutex_lock(&ta->c->glock);  // Lock Global
    ta->c->global += ta->local; 
    pthread_mutex_unlock(&ta->c->glock);  // Unlock Global
    ta->local = 0;
  }
  pthread_mutex_unlock(&ta->llock);  // Unlock Local
}

void* my_thread(void* args) {
  targs* ta = (targs*) args;
  if (ta->sh->cpu_bound) {  // CPU work being done
    long increments = 1000 * ta->sh->work_time * 1000000;
    for (long j = 0; j < increments; j++);
    update(ta, 1);
  } else {  // I/O work
    usleep(1000000);
  }
}

int get(targs *ta) {
  pthread_mutex_lock(&ta->c->glock);
  int val = ta->c->global;
  pthread_mutex_unlock(&ta->c->glock);
  return val;
}

int main(int argc, char* argv[]) {

  /* Defaulting */
  int n_threads = 2;          // 2 threads
  int sloppiness = 10;        // 10 events before update
  int work_time = 10;         // 10ms of work (busy loop)
  int work_iterations = 100;  // 100 iterations per thread
  bool cpu_bound = false;     // IO-bound work (1ms loop)
  bool do_logging = false;    // No logging

  if (argc > 1) n_threads = atoi(argv[1]);
  if (argc > 2) sloppiness = atoi(argv[2]);
  if (argc > 3) work_time = atoi(argv[3]);
  if (argc > 4) work_iterations = atoi(argv[4]);
  if (argc > 5) cpu_bound = atoi(argv[5]);
  if (argc > 6) do_logging = atoi(argv[6]);

  /* Load shared */
  shared sh; 
  sh.n_threads = n_threads;
  sh.work_time = work_time;
  sh.work_iterations = work_iterations;
  sh.cpu_bound = cpu_bound;
  sh.do_logging = do_logging;
  
  /* Instantiate Global Counter */
  counter_t gcount;
  gcount.threshold = sloppiness;

  /* Start Threadding */
  pthread_t threads[n_threads];
  targs all_targs[n_threads];

  for(int i =0;i<n_threads;++i) {
    all_targs[i].sh = &sh;
    all_targs[i].local = 0;
    pthread_create(&threads[i], NULL, my_thread, &all_targs[i]);
  }

  for(int i=0; i<n_threads; ++i) {
    pthread_join(threads[i], NULL);
  }
}
