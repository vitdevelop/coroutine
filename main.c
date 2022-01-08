#include "coroutine.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void func1(void *args);
void func2(void *args);
void func3(void *args);
void func4(void *args);
void sig_handler(int signal);

void func1(void *args) {
  printf("Enter func 1\n");

  co_spawn(func2, NULL);

  printf("Resume func 1\n");
}

void func2(void *args) {
  printf("Enter func 2\n");

  co_spawn(func3, NULL);

  printf("Resume func 2\n");
}

void func3(void *args) {
  printf("Enter func 3\n");

  co_spawn(func4, NULL);

  printf("Resume func 3\n");
}

void func4(void *args) {
  printf("Enter func 4\n");

  co_yield();

  printf("Resume func 4\n");
}

void sig_handler(int signal) {
  printf("Handled %d\n", signal);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  /* Disable user-space buffer, memory leaks detected when enabled */
  setbuf(stdout, NULL);

  signal(SIGSEGV, sig_handler);
  initScheduler();

  co_spawn(func1, NULL);

  co_spawn(func1, NULL);
  co_spawn(func2, NULL);

  printf("End of coroutines\n");

  destroyScheduler();

  exit(EXIT_SUCCESS);
}
