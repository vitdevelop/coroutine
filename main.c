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
  coroutine_t *coroutine = (coroutine_t *)args;
  puts("Enter func 1");

  co_spawn(coroutine->scheduler, func2, NULL);

  puts("Resume func 1");
}

void func2(void *args) {
  puts("Enter func 2");
  coroutine_t *coroutine = (coroutine_t *)args;

  co_spawn(coroutine->scheduler, func3, NULL);

  puts("Resume func 2");
}

void func3(void *args) {
  puts("Enter func 3");
  coroutine_t *coroutine = (coroutine_t *)args;

  co_spawn(coroutine->scheduler, func4, NULL);

  puts("Resume func 3");
}

void func4(void *args) {
  printf("Enter func 4\n");
  coroutine_t *coroutine = (coroutine_t *)args;

  co_yield(coroutine);

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
  co_scheduler_t *scheduler = initScheduler();

  for (int i = 0; i < 1; i++) {
    co_spawn(scheduler, func1, NULL);
  }

  co_spawn(scheduler, func1, NULL);
  co_spawn(scheduler, func2, NULL);

  puts("End of coroutines");

  destroyScheduler(scheduler);

  exit(EXIT_SUCCESS);
}
