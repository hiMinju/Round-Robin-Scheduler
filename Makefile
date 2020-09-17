a.out: main.o Thread.o Init.o Scheduler.o TestCase1.o TestCase2.o TestCase3.o TestCase4.o
	gcc main.o Thread.o Init.o Scheduler.o TestCase1.o TestCase2.o TestCase3.o TestCase4.o -o a.out
main.o: Init.h Scheduler.h Thread.h TestCase1.h TestCase2.h TestCase3.h TestCase4.h main.c
	gcc -c main.c
Init.o: Init.h Thread.h Scheduler.h Queue.h Init.c
	gcc -c Init.c
Thread.o: Init.h Thread.h Scheduler.h Queue.h Thread.c
	gcc -c Thread.c
Scheduler.o: Init.h Thread.h Scheduler.h Queue.h Scheduler.c
	gcc -c Scheduler.c
TestCase1.o: TestCase1.h TestCase1.c
	gcc -c TestCase1.c
TestCase2.o: TestCase2.h TestCase2.c
	gcc -c TestCase2.c
TestCase3.o: TestCase3.h TestCase3.c
	gcc -c TestCase3.c
TestCase4.o: TestCase4.h TestCase4.c
	gcc -c TestCase4.c

clean:
	rm -f *.o *.out
