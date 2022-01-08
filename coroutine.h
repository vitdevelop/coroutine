#ifndef __COROUTINE_H__
#define __COROUTINE_H__

#if __APPLE__ && __MACH__
#include <sys/ucontext.h>
#else
#include <ucontext.h>
#endif

#define STACK_SIZE 4 * 1024

typedef void (*co_func)(void *);

enum CoroutineStatus {
  COROUTINE_STATE_READY = 0,
  COROUTINE_STATE_RUNNING = 1,
  COROUTINE_STATE_SUSPEND = 2,
  COROUTINE_STATE_DONE = 3,
};

struct co_scheduler_t;

typedef struct coroutine_t {
  void *stack;
  size_t stack_size;
  ucontext_t uc;

  co_func func;
  void *args;

  struct co_scheduler_t *scheduler;
  enum CoroutineStatus status;

} coroutine_t;

struct co_scheduler_queue_t {
  coroutine_t *coroutine;
  struct co_scheduler_queue_t *next;
};

typedef struct co_scheduler_t {
  ucontext_t sched_uc;
  ucontext_t main_uc;

  struct co_scheduler_queue_t *current;
  struct co_scheduler_queue_t *tail;
} co_scheduler_t;

void initScheduler(void);
void destroyScheduler(void);
void co_spawn(co_func func, void *args);
void co_yield(void);

#endif /* __COROUTINE_H__ */
