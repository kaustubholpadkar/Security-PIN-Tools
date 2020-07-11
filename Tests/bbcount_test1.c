#include <stdio.h>


int main (int argc, char ** argv) {

  if (argc == 1) {
    return 0;
  }

  int count = atoi(argv[1]);

  for (int i = 0; i < count; i++) {
    printf("%d", i);
  }

  printf("\n");

}
