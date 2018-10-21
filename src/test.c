/*
 * picotm-perf - Picotm Performance Tests
 * Copyright (c) 2017   Thomas Zimmermann <contact@tzimmermann.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test.h"
#include <assert.h>
#include <errno.h>
#include <picotm/picotm.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/* Returns the number of milliseconds since the epoch */
static long long
getmsofday(void* tzp)
{
    struct timeval t;
    int res = gettimeofday(&t, tzp);
    if (res < 0) {
        fprintf(stderr, "gettimeofday() failed: %s\n",
                strerror(res));
        return -1;
    }

    return t.tv_sec * 1000 + t.tv_usec / 1000;
}

struct thread {

    pthread_t          thread;
    pthread_barrier_t* wait;
    unsigned long      nmsecs;

    unsigned long long  res_niters;
    unsigned long long  res_nmsecs;
    unsigned long long  res_nrestarts;

    call_func   call;
    unsigned long tid;
    unsigned long nloads;
    unsigned long nstores;
};

static void
thread_init(struct thread* self, pthread_barrier_t* wait, unsigned long nmsecs,
            call_func call, unsigned long tid, unsigned long nloads,
            unsigned long nstores)
{
    assert(self);

    self->wait = wait;
    self->nmsecs = nmsecs;
    self->res_niters = 0;
    self->res_nmsecs = 0;
    self->res_nrestarts = 0;
    self->call = call;
    self->tid = tid;
    self->nloads = nloads;
    self->nstores = nstores;
}

static void
thread_uninit(struct thread* self)
{
    assert(self);
}

static void
cleanup_picotm_cb(void* data)
{
    picotm_release();
}

static void*
thread_func(void* arg)
{
    struct thread* self = arg;
    assert(self);

    pthread_cleanup_push(cleanup_picotm_cb, NULL);

    if (self->wait) {
        int res = pthread_barrier_wait(self->wait);
        if (res && (res != PTHREAD_BARRIER_SERIAL_THREAD)) {
            fprintf(stderr, "pthread_barrier_wait() failed: %s\n",
                    strerror(res));
            return NULL;
        }
    }

    self->res_niters = 0;
    self->res_nmsecs = 0;
    self->res_nrestarts = 0;

    unsigned long long iters = 0;

    long long res = getmsofday(NULL);
    if (res < 0) {
        return NULL;
    }
    unsigned long long start_time = res;
    unsigned long long current_time = start_time;

    unsigned long long nrestarts = 0;

    while ((current_time - start_time) < self->nmsecs) {

        self->call(self->tid, self->nloads, self->nstores);

        long long res = getmsofday(NULL);
        if (res < 0) {
            return NULL;
        }
        current_time = res;
        ++iters;
        nrestarts += picotm_number_of_restarts();
    }

    self->res_niters = iters;
    self->res_nmsecs = current_time - start_time;
    self->res_nrestarts = nrestarts;

    pthread_cleanup_pop(1);

    return NULL;
}

static int
thread_run(struct thread* self)
{
    int err = pthread_create(&self->thread, NULL, thread_func, self);
    if (err) {
        fprintf(stderr, "pthread_create() failed: %s\n", strerror(err));
        return -1;
    }
    return 0;
}

static int
thread_join(struct thread* self)
{
    void* retval;

    int err = pthread_join(self->thread, &retval);
    if (err) {
        fprintf(stderr, "pthread_join() failed: %s\n", strerror(err));
        return -1;
    }
    return 0;
}

static void
thread_cancel(struct thread* self)
{
    int err = pthread_cancel(self->thread);
    if (err) {
        fprintf(stderr, "pthread_cancel() failed: %s\n", strerror(err));
        return;
    }
}

static struct thread*
new_threads(pthread_barrier_t* wait, unsigned long nthreads,
            unsigned long nmsecs, call_func call, unsigned long nloads,
            unsigned long nstores)
{
    size_t siz = sizeof(struct thread) * nthreads;

    struct thread* th = malloc(siz);
    if (siz && !th) {
        fprintf(stderr, "malloc() failed: %s\n", strerror(errno));
        return NULL;
    }

    for (unsigned int i = 0; i < nthreads; ++i) {
        thread_init(th + i, wait, nmsecs, call, i, nloads, nstores);
    }

    return th;
}

static void
destroy_threads(struct thread* beg, const struct thread* end)
{
    while (beg < end) {
        thread_uninit(beg);
        ++beg;
    }
}

static void
delete_threads(struct thread* th, unsigned long nthreads)
{
    destroy_threads(th, th + nthreads);
    free(th);
}

static int
run_threads(struct thread* beg, const struct thread* end)
{
    struct thread* pos = beg;

    while (pos < end) {
        int res = thread_run(pos);
        if (res < 0) {
            goto err_thread_run;
        }
        ++pos;
    }

    return 0;

err_thread_run:
    while (pos > beg) {
        --pos;
        thread_cancel(pos);
    }
    return -1;
}

static int
join_threads(struct thread* beg, const struct thread* end)
{
    struct thread* pos = beg;

    while (pos < end) {
        int res = thread_join(pos);
        if (res < 0) {
            goto err_thread_join;
        }
        ++pos;
    }

    return 0;

err_thread_join:
    while (pos < end) {
        thread_cancel(pos);
        ++pos;
    }
    return -1;
}

static void
print_results(const struct thread* beg, const struct thread* end)
{
    const struct thread* pos = beg;

    while (pos < end) {
        printf("%tu %llu %llu %llu\n", pos - beg + 1, pos->res_nmsecs,
               pos->res_niters, pos->res_nrestarts);
        ++pos;
    }
}

int
run_test(unsigned long nthreads, unsigned long nmsecs,
         const struct test_func* test, unsigned long nloads,
         unsigned long nstores)
{
    pthread_barrier_t wait;
    int err = pthread_barrier_init(&wait, NULL, nthreads);
    if (err) {
        fprintf(stderr, "pthread_barrier_init() failed: %s\n", strerror(err));
        return -1;
    }

    struct thread* th = new_threads(&wait, nthreads, nmsecs, test->call,
                                    nloads, nstores);
    if (!th) {
        goto err_new_threads;
    }

    int res = run_threads(th, th + nthreads);
    if (res < 0) {
        goto err_run_threads;
    }

    res = join_threads(th, th + nthreads);
    if (res < 0) {
        goto err_join_threads;
    }

    print_results(th, th + nthreads);

    delete_threads(th, nthreads);

    err = pthread_barrier_destroy(&wait);
    if (err) {
        fprintf(stderr, "pthread_barrier_destroy() failed: %s\n",
                strerror(err));
        return -1;
    }

    return 0;

err_join_threads:
    /* fall through */
err_run_threads:
    delete_threads(th, nthreads);
err_new_threads:
    pthread_barrier_destroy(&wait);
    return -1;
}
