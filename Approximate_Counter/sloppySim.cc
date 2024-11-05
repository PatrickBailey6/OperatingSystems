/* Patrick Bailey */

#include<semaphore.h>
#include<iostream>
#include<vector>
#include<pthread.h>
#include<unistd.h>


using std::cout;
using std::endl;


struct shared {
  int threshold;
  int n_threads;
  int work_time;
  int work_iterations;
  bool cpu_bound;
  bool do_logging;
  int gcount;
  pthread_mutex_t glock;
};

struct targs {
  shared* sh;
  int thread_number;
};

/*  Args: 1. Global Counter 2. Thread Reference 3. Count to add*/
void update(targs *ta, int count) {
  pthread_mutex_lock(&ta->sh->glock);  // Lock Global
  ta->sh->gcount += count;
  pthread_mutex_unlock(&ta->sh->glock);  // Unlock Global
}

void* my_thread(void* args) {
  targs* ta = (targs*) args;
  int count = 0;
  for (int i = 0; i < ta->sh->work_iterations; ++i) {
    if (ta->sh->cpu_bound) {  // CPU work being done
      for (long j = 0; j < 1000000; j++);
    }
    else {  // I/O work
      usleep((rand() % (3 * ta->sh->work_time) + ta->sh->work_time / 2) * 1000);
    }
    count++;
    if(count >= ta->sh->threshold) {
      update(ta, count);
      count = 0;
    }
  }
  return NULL;
}

int get(targs *ta) {
  pthread_mutex_lock(&ta->sh->glock);
  int val = 0;
  val = ta->sh->gcount;
  pthread_mutex_unlock(&ta->sh->glock);
  return val;
}

void* log_thread(void* args) {
  targs* ta = (targs*) args;
  for(int i=0; i<10; i++) {
    usleep(100000);
    cout << "Global count = " << get(ta) << endl;
  }
  return NULL;
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

  cout << "Running with:" << endl;
  cout << n_threads << endl;
  cout << sloppiness << endl;
  cout << work_time << endl;
  cout << work_iterations << endl;
  cout << cpu_bound << endl;
  cout << do_logging << endl;

  /* Load shared */
  shared sh; 
  sh.gcount = 0;
  sh.threshold = sloppiness;
  sh.n_threads = n_threads;
  sh.work_time = work_time;
  sh.work_iterations = work_iterations;
  sh.cpu_bound = cpu_bound;
  sh.do_logging = do_logging;

  /* Start Threadding */
  pthread_t threads[n_threads+1];
  targs all_targs[n_threads+1];

  for(int i =0;i<n_threads;++i) {
    all_targs[i].sh = &sh;
    all_targs[i].thread_number = i;
    pthread_create(&threads[i], NULL, my_thread, &all_targs[i]);
  }

  pthread_create(&threads[n_threads], NULL, log_thread, &all_targs[n_threads]);

  for(int i=0; i<n_threads; ++i) {
    pthread_join(threads[i], NULL);
  }
  if(do_logging)
    pthread_join(threads[n_threads], NULL);

  cout << "Final global count: " << sh.gcount << endl;
  return 0;
}
