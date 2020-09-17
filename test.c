#include <signal.h>
#include <stdio.h>
#include <unistd.h>
void myAlrmHandler(int signum);
int main(int argc, char const *argv[]) {
if (signal(SIGALRM, myAlrmHandler) == SIG_ERR) {
perror("signal() error!");
}
alarm(5);
puts("Good bye~ ");
sleep(100);
return 0;
}
void myAlrmHandler(int signum) { puts("Hello! I'm wake!"); }