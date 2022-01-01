/*
 * Copyright (c) 2016, Mathias Brossard <mathias@brossard.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file threadpool.c
 * @brief Threadpool implementation file
 */

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "threadpool.h"

typedef enum {
  immediate_shutdown = 1,
  graceful_shutdown = 2
} threadpool_shutdown_t;

/**
 *  @struct threadpool_task
 *  @brief the work struct
 *
 *  @var function Pointer to the function that will perform the task.
 *  @var argument Argument to be passed to the function.
 */

typedef struct {
  void (*function)(uint32_t, void *);
  void *argument;
} threadpool_task_t;

/**
 *  @struct threadpool
 *  @brief The threadpool struct
 *
 *  @var notify       Condition variable to notify worker threads.
 *  @var threads      Array containing worker threads ID.
 *  @var thread_count Number of threads
 *  @var queue        Array containing the task queue.
 *  @var queue_size   Size of the task queue.
 *  @var head         Index of the first element.
 *  @var tail         Index of the next element.
 *  @var count        Number of pending tasks
 *  @var shutdown     Flag indicating if the pool is shutting down
 *  @var started      Number of started threads
 */
struct threadpool_t {
  pthread_mutex_t lock;
  pthread_cond_t notify;
  pthread_t *threads;
  threadpool_task_t *queue;
  uint32_t thread_count;
  uint32_t queue_size;
  uint32_t head;
  uint32_t tail;
  uint32_t count;
  uint32_t shutdown;
  uint32_t started;
};

typedef struct {
  threadpool_t *pool;
  uint32_t thread_id;
} threadpool_thread_arg_t;

/**
 * @function void *threadpool_thread(void *threadpool)
 * @brief the worker thread
 * @param threadpool_thread_arg the id of the thread, and a reference to the
 * pool which owns the thread
 */
static void *threadpool_thread(void *threadpool_thread_arg);

int threadpool_free(threadpool_t *pool);

threadpool_t *threadpool_create(uint32_t thread_count, uint32_t queue_size,
                                uint32_t flags) {
  threadpool_t *pool;
  (void)flags;

  if (thread_count <= 0 || thread_count > MAX_THREADS || queue_size <= 0 ||
      queue_size > MAX_QUEUE) {
    return NULL;
  }

  if ((pool = malloc(sizeof(threadpool_t))) == NULL) {
    goto err;
  }

  /* Initialize */
  pool->thread_count = 0;
  pool->queue_size = queue_size;
  pool->head = pool->tail = pool->count = 0;
  pool->shutdown = pool->started = 0;

  /* Allocate thread and task queue */
  pool->threads = malloc(sizeof(pthread_t) * thread_count);
  pool->queue =
      (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_size);

  /* Initialize mutex and conditional variable first */
  if ((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
      (pthread_cond_init(&(pool->notify), NULL) != 0) ||
      (pool->threads == NULL) || (pool->queue == NULL)) {
    goto err;
  }

  /* Start worker threads */
  for (uint32_t i = 0; i < thread_count; i++) {
    // create thread arg (owned by thread)
    threadpool_thread_arg_t *thread_arg =
        malloc(sizeof(threadpool_thread_arg_t));
    thread_arg->thread_id = i;
    thread_arg->pool = pool;
    // create thread with argumetn
    if (pthread_create(&(pool->threads[i]), NULL, threadpool_thread,
                       thread_arg) != 0) {
      threadpool_destroy(pool, 0);
      return NULL;
    }
    pool->thread_count++;
    pool->started++;
  }

  return pool;

err:
  if (pool) {
    threadpool_free(pool);
  }
  return NULL;
}

int threadpool_add(threadpool_t *pool, void (*function)(uint32_t, void *),
                   void *argument, uint32_t flags) {
  int err = 0;
  (void)flags;

  if (pool == NULL || function == NULL) {
    return threadpool_invalid;
  }

  if (pthread_mutex_lock(&(pool->lock)) != 0) {
    return threadpool_lock_failure;
  }

  uint32_t next = (pool->tail + 1) % pool->queue_size;

  do {
    /* Are we full ? */
    if (pool->count == pool->queue_size) {
      err = threadpool_queue_full;
      break;
    }

    /* Are we shutting down ? */
    if (pool->shutdown) {
      err = threadpool_shutdown;
      break;
    }

    /* Add task to queue */
    pool->queue[pool->tail].function = function;
    pool->queue[pool->tail].argument = argument;
    pool->tail = next;
    pool->count += 1;

    /* pthread_cond_broadcast */
    if (pthread_cond_signal(&(pool->notify)) != 0) {
      err = threadpool_lock_failure;
      break;
    }
  } while (0);

  if (pthread_mutex_unlock(&pool->lock) != 0) {
    err = threadpool_lock_failure;
  }

  return err;
}

int threadpool_destroy(threadpool_t *pool, uint32_t flags) {
  int err = 0;

  if (pool == NULL) {
    return threadpool_invalid;
  }

  if (pthread_mutex_lock(&(pool->lock)) != 0) {
    return threadpool_lock_failure;
  }

  do {
    /* Already shutting down */
    if (pool->shutdown) {
      err = threadpool_shutdown;
      break;
    }

    pool->shutdown =
        (flags & threadpool_graceful) ? graceful_shutdown : immediate_shutdown;

    /* Wake up all worker threads */
    if ((pthread_cond_broadcast(&(pool->notify)) != 0) ||
        (pthread_mutex_unlock(&(pool->lock)) != 0)) {
      err = threadpool_lock_failure;
      break;
    }

    /* Join all worker thread */
    for (uint32_t i = 0; i < pool->thread_count; i++) {
      if (pthread_join(pool->threads[i], NULL) != 0) {
        err = threadpool_thread_failure;
      }
    }
  } while (0);

  /* Only if everything went well do we deallocate the pool */
  if (!err) {
    threadpool_free(pool);
  }
  return err;
}

int threadpool_free(threadpool_t *pool) {
  if (pool == NULL || pool->started > 0) {
    return -1;
  }

  /* Did we manage to allocate ? */
  if (pool->threads) {
    free(pool->threads);
    free(pool->queue);

    /* Because we allocate pool->threads after initializing the
       mutex and condition variable, we're sure they're
       initialized. Let's lock the mutex just in case. */
    pthread_mutex_lock(&(pool->lock));
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));
  }
  free(pool);
  return 0;
}

static void *threadpool_thread(void *threadpool_thread_arg) {
  threadpool_thread_arg_t *arg =
      (threadpool_thread_arg_t *)threadpool_thread_arg;

  // get data out of arg
  uint32_t thread_id = arg->thread_id;
  threadpool_t *pool = arg->pool;

  // free arg
  free(arg);

  for (;;) {
    /* Lock must be taken to wait on conditional variable */
    pthread_mutex_lock(&(pool->lock));

    /* Wait on condition variable, check for spurious wakeups.
       When returning from pthread_cond_wait(), we own the lock. */
    while ((pool->count == 0) && (!pool->shutdown)) {
      pthread_cond_wait(&(pool->notify), &(pool->lock));
    }

    if ((pool->shutdown == immediate_shutdown) ||
        ((pool->shutdown == graceful_shutdown) && (pool->count == 0))) {
      break;
    }

    /* Grab our task */
    threadpool_task_t task = pool->queue[pool->head];
    pool->head = (pool->head + 1) % pool->queue_size;
    pool->count -= 1;

    /* Unlock */
    pthread_mutex_unlock(&(pool->lock));

    /* Get to work */
    (*(task.function))(thread_id, task.argument);
  }

  pool->started--;

  pthread_mutex_unlock(&(pool->lock));
  pthread_exit(NULL);
  return (NULL);
}
