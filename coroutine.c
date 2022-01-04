#include "coroutine.h"
#include "error_util.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

void co_scheduler_insertCoroutine(co_scheduler_t *scheduler,
                                  coroutine_t *coroutine);
void co_scheduler_removeCoroutine(co_scheduler_t *scheduler,
                                  struct co_scheduler_queue_t *queue);
void co_destroy(coroutine_t *coroutine);
void co_scheduler_mainFunction(co_scheduler_t *scheduler);
void co_entry_point(coroutine_t *coroutine);

void co_scheduler_insertCoroutine(co_scheduler_t *scheduler,
                                  coroutine_t *coroutine) {
  struct co_scheduler_queue_t *queue =
      malloc(sizeof(struct co_scheduler_queue_t));
  queue->coroutine = coroutine;
  queue->next = NULL;

  if (scheduler->tail != NULL) {
    scheduler->tail->next = queue;
    queue->next = scheduler->current;
  }
  if (scheduler->current == NULL) {
    scheduler->current = queue;
  }

  scheduler->tail = queue;
}

void co_scheduler_removeCoroutine(co_scheduler_t *scheduler,
                                  struct co_scheduler_queue_t *queue) {
  scheduler->current = queue->next;
  scheduler->tail->next = scheduler->current;
  scheduler->tail = (scheduler->tail == queue ? NULL : scheduler->tail);

  if (scheduler->current == scheduler->tail && scheduler->current != NULL) {
    scheduler->current->next = NULL;
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

void co_spawn(co_scheduler_t *scheduler, co_func func, void *args) {
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
  coroutine->uc.uc_link = &(scheduler->sched_uc);
  makecontext(&(coroutine->uc), (void (*)(void))co_entry_point, 1, coroutine);

  coroutine->scheduler = scheduler;
  coroutine->status = COROUTINE_STATE_SUSPEND;
  coroutine->func = func;
  coroutine->args = args;

  coroutine_t *currentCoroutine =
      (scheduler->current != NULL ? scheduler->current->coroutine : NULL);

  co_scheduler_insertCoroutine(scheduler, coroutine);

  if (currentCoroutine == NULL) {
    swapcontext(&(scheduler->main_uc), &(scheduler->sched_uc));
  } else {
    co_yield(currentCoroutine);
  }
}

void co_entry_point(coroutine_t *coroutine) {
  coroutine->func(coroutine);
  coroutine->status = COROUTINE_STATE_DONE;
  swapcontext(&(coroutine->uc), &(coroutine->scheduler->sched_uc));
}

void co_scheduler_mainFunction(co_scheduler_t *scheduler) {
  while (scheduler->current != NULL) {
    struct co_scheduler_queue_t *currentQueue = scheduler->current;
    if (currentQueue->next != NULL) {
      scheduler->tail = currentQueue;
      currentQueue = currentQueue->next;
      scheduler->current = currentQueue;
    }
    coroutine_t *coroutine = currentQueue->coroutine;
    if (coroutine->status == COROUTINE_STATE_SUSPEND) {
      coroutine->status = COROUTINE_STATE_RUNNING;
      swapcontext(&(scheduler->sched_uc), &(coroutine->uc));
    }

    if (coroutine->status == COROUTINE_STATE_DONE) {
      co_scheduler_removeCoroutine(scheduler, currentQueue);
    }
  }
  setcontext(&(scheduler->main_uc));
}

co_scheduler_t *initScheduler(void) {
  co_scheduler_t *scheduler = malloc(sizeof(co_scheduler_t));
  if (scheduler == NULL)
    errExit("scheduler malloc");

  memset(scheduler, 0, sizeof(co_scheduler_t));

  getcontext(&(scheduler->sched_uc));

  scheduler->sched_uc.uc_stack.ss_sp = malloc(MINSIGSTKSZ);
  if (scheduler->sched_uc.uc_stack.ss_sp == NULL)
    errExit("scheduler mainFunction malloc");

  memset(scheduler->sched_uc.uc_stack.ss_sp, 0, MINSIGSTKSZ);
  scheduler->sched_uc.uc_stack.ss_size = MINSIGSTKSZ;
  scheduler->sched_uc.uc_stack.ss_flags = 0;

  makecontext(&(scheduler->sched_uc), (void (*)(void))co_scheduler_mainFunction,
              1, scheduler);

  return scheduler;
}
void destroyScheduler(co_scheduler_t *scheduler) {
  free(scheduler->sched_uc.uc_stack.ss_sp);
  free(scheduler);
}

void co_yield(coroutine_t *coroutine) {
  coroutine->status = COROUTINE_STATE_SUSPEND;
  swapcontext(&(coroutine->uc), &(coroutine->scheduler->sched_uc));
}
