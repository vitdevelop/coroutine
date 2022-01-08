#include "coroutine.h"
#include "error_util.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

void co_scheduler_insertCoroutine(coroutine_t *coroutine);
void co_scheduler_removeCoroutine(struct co_scheduler_queue_t *queue);
void co_destroy(coroutine_t *coroutine);
void co_scheduler_mainFunction(void);
void co_entry_point(coroutine_t *coroutine);

static __thread co_scheduler_t *co_scheduler;

void co_scheduler_insertCoroutine(coroutine_t *coroutine) {
  struct co_scheduler_queue_t *queue =
      malloc(sizeof(struct co_scheduler_queue_t));
  queue->coroutine = coroutine;
  queue->next = NULL;

  if (co_scheduler->tail != NULL) {
    co_scheduler->tail->next = queue;
    queue->next = co_scheduler->current;
  }
  if (co_scheduler->current == NULL) {
    co_scheduler->current = queue;
  }

  co_scheduler->tail = queue;
}

void co_scheduler_removeCoroutine(struct co_scheduler_queue_t *queue) {
  co_scheduler->current = queue->next;
  co_scheduler->tail->next = co_scheduler->current;
  co_scheduler->tail =
      (co_scheduler->tail == queue ? NULL : co_scheduler->tail);

  if (co_scheduler->current == co_scheduler->tail &&
      co_scheduler->current != NULL) {
    co_scheduler->current->next = NULL;
  }

  co_destroy(queue->coroutine);

  free(queue);
}

void co_destroy(coroutine_t *coroutine) {
  if (coroutine->args)
    free(coroutine->args);
  free(coroutine->stack);
  free(coroutine);
}

void co_spawn(co_func func, void *args) {
  coroutine_t *coroutine = malloc(sizeof(coroutine_t));
  if (coroutine == NULL)
    errExit("coroutine malloc");
  memset(coroutine, 0, sizeof(coroutine_t));

  coroutine->stack_size = STACK_SIZE;
  coroutine->stack = malloc(coroutine->stack_size);
  if (coroutine->stack == NULL)
    errExit("coroutine stack malloc");

  getcontext(&(coroutine->uc));

  coroutine->uc.uc_stack.ss_sp = coroutine->stack;
  coroutine->uc.uc_stack.ss_size = coroutine->stack_size;
  coroutine->uc.uc_link = &(co_scheduler->sched_uc);
  makecontext(&(coroutine->uc), (void (*)(void))co_entry_point, 1, coroutine);

  coroutine->scheduler = co_scheduler;
  coroutine->status = COROUTINE_STATE_SUSPEND;
  coroutine->func = func;
  coroutine->args = args;

  coroutine_t *currentCoroutine =
      (co_scheduler->current != NULL ? co_scheduler->current->coroutine : NULL);

  co_scheduler_insertCoroutine(coroutine);

  if (currentCoroutine == NULL) {
    swapcontext(&(co_scheduler->main_uc), &(co_scheduler->sched_uc));
  } else {
    co_yield();
  }
}

void co_entry_point(coroutine_t *coroutine) {
  coroutine->func(coroutine->args);
  coroutine->status = COROUTINE_STATE_DONE;
  swapcontext(&(coroutine->uc), &(coroutine->scheduler->sched_uc));
}

void co_scheduler_mainFunction() {
  while (co_scheduler->current != NULL) {
    struct co_scheduler_queue_t *currentQueue = co_scheduler->current;
    if (currentQueue->next != NULL) {
      co_scheduler->tail = currentQueue;
      currentQueue = currentQueue->next;
      co_scheduler->current = currentQueue;
    }
    coroutine_t *coroutine = currentQueue->coroutine;
    if (coroutine->status == COROUTINE_STATE_SUSPEND) {
      coroutine->status = COROUTINE_STATE_RUNNING;
      swapcontext(&(co_scheduler->sched_uc), &(coroutine->uc));
    }

    if (coroutine->status == COROUTINE_STATE_DONE) {
      co_scheduler_removeCoroutine(currentQueue);
    }
  }
  setcontext(&(co_scheduler->main_uc));
}

void initScheduler(void) {
  co_scheduler = malloc(sizeof(co_scheduler_t));
  if (co_scheduler == NULL)
    errExit("scheduler malloc");

  memset(co_scheduler, 0, sizeof(co_scheduler_t));

  getcontext(&(co_scheduler->sched_uc));

  co_scheduler->sched_uc.uc_stack.ss_sp = malloc(MINSIGSTKSZ);
  if (co_scheduler->sched_uc.uc_stack.ss_sp == NULL)
    errExit("scheduler mainFunction malloc");

  memset(co_scheduler->sched_uc.uc_stack.ss_sp, 0, MINSIGSTKSZ);
  co_scheduler->sched_uc.uc_stack.ss_size = MINSIGSTKSZ;
  co_scheduler->sched_uc.uc_stack.ss_flags = 0;

  makecontext(&(co_scheduler->sched_uc),
              (void (*)(void))co_scheduler_mainFunction, 1, co_scheduler);
}
void destroyScheduler() {
  free(co_scheduler->sched_uc.uc_stack.ss_sp);
  free(co_scheduler);
}

void co_yield() {
  coroutine_t *currentCoroutine = co_scheduler->current->coroutine;
  currentCoroutine->status = COROUTINE_STATE_SUSPEND;
  swapcontext(&(currentCoroutine->uc), &(co_scheduler->sched_uc));
}
