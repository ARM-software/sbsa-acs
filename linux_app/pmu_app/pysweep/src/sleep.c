/** @file
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

/*
 * High-resolution sleep. The aim is to wait for the given duration,
 * even if some kind of userspace-interrupt-driven profiling is active.
 *
 * We use a timed select() call. This may return early with EINTR,
 * e.g. after a SIGPROF interrupt. We can calculate the remaining time
 * and re-execute a select() for the remaining time.
 *
 * However, we do want to be able to break out of a long wait with a SIGINT.
 *
 * TBD: this is not thread-safe!
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */

#include "sleep.h"

#include <time.h>
#include <stddef.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>

#define SECONDS 1000000000
#define MAX_BLOCKING (1ULL*SECONDS)


static unsigned long long nowns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000000000ULL) + ts.tv_nsec;
}


static int volatile sigint_received = 0;


static void sigint_handler(int sig)
{
    sigint_received = 1;
}


int microsleep_ns(long long sleep_nano)
{
    int n_iters = 0;
    int rc;
    unsigned long long const then = nowns() + sleep_nano;
    int const handle_sigint = 1 && (sleep_nano >= MAX_BLOCKING);
    struct sigaction oldact;
    if (handle_sigint) {
        int rc;
        struct sigaction act;
        memset(&act, 0, sizeof act);
        act.sa_handler = sigint_handler;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;
        act.sa_restorer = NULL;
        sigint_received = 0;
        rc = sigaction(SIGINT, &act, &oldact);
        if (rc) {
            perror("sigaction");
        }
    }
    while (sleep_nano > 0) {
        struct timespec ts_wait;
        ts_wait.tv_sec  = (sleep_nano / 1000000000);
        ts_wait.tv_nsec = (sleep_nano % 1000000000);
        rc = pselect(0, NULL, NULL, NULL, &ts_wait, NULL);
        if (rc == -1 && errno == EINTR) {
            /* woken up by interrupt - perhaps SIGPROF or SIGINT? */
            /* Ideally, we should be able to find out what kind of interrupt
               caused the EINTR, but I don't know how to do that.
               sigpending() doesn't help. */
            unsigned long long now = nowns();
            if (now >= then) {
                /* but rolled off the end anyway */
                rc = n_iters;
                break;
            }
            if (sigint_received) {
                break;
            }
            /* resume the wait, but with a shorter time */
            sleep_nano = then - now;
        } else if (rc < 0) {
            break;
        } else {
            rc = n_iters;
            break;
        }
        ++n_iters;
    }
    if (handle_sigint) {
        sigaction(SIGINT, &oldact, NULL);
        if (sigint_received) {
            raise(SIGINT);
            return -1;
        }
    }
    return rc;
}


int microsleep(double t)
{
    return microsleep_ns(t * 1e9);
}
