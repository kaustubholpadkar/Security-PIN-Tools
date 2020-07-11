all: bbcount_test1 ctcount_test1 maxstack_test1 maxstack_test2 wrapmalloc_test1 

bbcount_test1: bbcount_test1.c
	gcc -o bbcount_test1.out bbcount_test1.c  

ctcount_test1: ctcount_test1.c
	gcc -o ctcount_test1.out ctcount_test1.c  

maxstack_test1: maxstack_test1.c
	gcc -o maxstack_test1.out maxstack_test1.c  

maxstack_test2: maxstack_test2.cpp
	g++ -pthread  -std=c++11 -o maxstack_test2.out maxstack_test2.cpp

wrapmalloc_test1: wrapmalloc_test1.c
	gcc -pthread -o wrapmalloc_test1.out wrapmalloc_test1.c
