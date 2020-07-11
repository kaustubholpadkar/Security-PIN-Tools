#include <stdio.h>
#include <pthread.h>

void allocate(int count) {
  for (int i = 0; i < count; i++) {
    malloc(100);
  }
}


int main (int argc, char ** argv) {

  if (argc == 1) {
    return 0;
  }

  int nthreads = atoi(argv[2]);
  pthread_t *t;

  t = (pthread_t *)malloc(nthreads * sizeof(pthread_t ));

  int count = atoi(argv[1]);

  for (int i=0; i<nthreads; i++) {
    pthread_create(&t[i], NULL, allocate, count);
  }

  for (int i=0; i<nthreads; i++) {
    pthread_join(t[i],NULL);  
  }  
  return 0;
}
