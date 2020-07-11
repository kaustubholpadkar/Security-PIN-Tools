#include <stdio.h>

static int ans = 0;

void recurse ( int count ) /* Each call gets its own copy of count */
{
    if (count <= 0) {return;}

    ans += count;

//    printf( "%d\n", count );
    /* It is not necessary to increment count since each function's
       variables are separate (so each count will be initialized one greater)
     */
    recurse ( count - 1 );
}
 
int main(int argc, char * argv[])
{

  if (argc <= 1) { return; }

  int n = atoi(argv[1]);

  recurse ( n ); /* First function call, so it starts at one */
  printf("result: %d\n", ans);
  return 0;
}
