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
  // printf("Enter func 1");

  co_spawn(func2, NULL);

  // printf("Resume func 1");
}

void func2(void *args) {
  // printf("Enter func 2");

  co_spawn(func3, NULL);

  // printf("Resume func 2");
}

void func3(void *args) {
  // printf("Enter func 3");

  co_spawn(func4, NULL);

  // printf("Resume func 3");
}

void func4(void *args) {
  // printf("Enter func 4\n");

  co_yield();

  // printf("Resume func 4\n");
}

void sig_handler(int signal) {
  // printf("Handled %d\n", signal);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  /* Disable user-space buffer, memory leaks detected when enabled */
  setbuf(stdout, NULL);

  signal(SIGSEGV, sig_handler);
  initScheduler();

  for (int i = 0; i < 1000000; i++) {
    co_spawn(func1, NULL);
  }

  co_spawn(func1, NULL);
  co_spawn(func2, NULL);

  // printf("End of coroutines");

  destroyScheduler();

  exit(EXIT_SUCCESS);
}
