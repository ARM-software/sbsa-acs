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
 * Python module to generate artificial loads.
 */

/*
 * The main object created by this module is an object of type Load.
 *
 *   load = pyperf.create(spec)
 *   load.start()
 *   load.update(new_spec)
 *   load.stop()
 *
 * Internally, some worker threads are created, which all run the workload.
 * (If you want to have threads running different workloads, then just create
 * multiple Load objects.)
 *
 * As seen above, the characteristics of a workload can be dynamically updated
 * while the workload is running. How this is achieved is described in a
 * comment below.
 */


#ifndef MODULE_NAME
#define MODULE_NAME pysweep
#endif /* !MODULE_NAME */

#include <Python.h>
#if PY_MAJOR_VERSION < 3
#define PyBytes_FromStringAndSize PyString_FromStringAndSize
#else
#define PyString_FromFormat PyUnicode_FromFormat
#define PyInt_FromLong PyLong_FromLong
#define PyInt_AsLong PyLong_AsLong
#define PyInt_Check PyLong_Check
#define PyInt_AsUnsignedLongLongMask PyLong_AsUnsignedLongLongMask
#endif


#include "loadgen.h"
#include "loadgenp.h"     /* for diagnostics and benchmarking */
#include "prepcode.h"
#include "sleep.h"
#include "branch_prediction.h"
#include "arch.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sched.h>

#include <assert.h>
#include <stdlib.h>
#include <signal.h>

#define MODULE_NAME_STRING3(m) #m
#define MODULE_NAME_STRING2(m) MODULE_NAME_STRING3(m)
#define MODULE_NAME_STRING MODULE_NAME_STRING2(MODULE_NAME)

extern int workload_verbose;


/*
Dynamically updating workloads.

Internally, workloads use the workload object managed by "loadgen.h".
A workload consists of a code sequence which can be executed repeatedly.
After each repetition we might take some other action.

We want to be able to dynamically change the characteristics of a workload
while it's running - not just changing scheduling/pinning of the worker threads,
but changing the code they are executing. We want to free up any resources
used by the old workload, but not while it's still running.

We could do this in different ways:

  - keep the worker threads running, but have them switch to a new load object
    if available at the end of an iteration. Ensure that an old load object
    is cleaned up when no longer needed.

  - cancel the worker threads (pthread_cancel). Create new threads.
    Disadvantage: any external monitoring, such as perf events, that is tied
    to the worker threads, would have to be set up for the new threads.
*/


typedef struct load_thread load_thread_t;
typedef struct load_thread_local load_thread_local_t;


/*
 * pysweep.Load: a Python object representing a workload that we can
 * dynamically vary and which has one or more workload execution
 * threads associated with it.
 */
typedef struct {
    PyObject_HEAD
    unsigned int n_threads;        /* Number of threads requested */
    /* The workload itself - executable code and its characteristics.
       This is a central copy of the pointer. The threads each have
       their own pointer to the workload, which they will monitor
       for changes. */
    Workload *work;                /* Workload as created by loadgen.c */
    load_thread_t *first_thread;   /* List of execution threads */
    unsigned int suspend_reasons;  /* Supension reason(s) */
#define SUSPEND_REQUEST 0x01       /* Suspended because requested to be suspended */
#define SUSPEND_ZEROAFF 0x02       /* Suspended because pinned to the empty set of threads */
#define SUSPEND_BADWORK 0x04       /* Suspended because couldn't create workload */
    pthread_attr_t thread_attr;    /* Default thread attributes (including affinity) */
} LoadObject;


/*
 * Description for a workload executor thread.
 * Always on the thread list of its owning LoadObject.
 * The expectation is that this is mostly unchanging and can live
 * as a shared copy in the worker's and master's caches.
 */
struct load_thread {
    PyObject_HEAD
    struct load_thread *next_thread;
    LoadObject *load;             /* Point back to the load */
    pthread_t pthread_id;         /* The pthread thread id, not the OS thread id */
    pid_t os_tid;                 /* OS tid, as used for e.g. perf_event_open */
    sem_t sem_started;            /* Thread has started and OS tid is available */
    sem_t sem_worktodo;           /* Contoller signals thread that there is work to do */
    load_thread_local_t volatile *loc;     /* Local, rapidly changing data */
};

typedef load_thread_t ThreadObject;
static PyTypeObject ThreadType;


/*
 * Local data for a thread. The intention is that this has
 * rapidly changing data and will live as an exclusive copy in the
 * worker's cache.
 */
struct load_thread_local {
    struct load_thread *thread;   /* Point back to the thread */
    Workload *volatile vol_work;  /* Copy of the workload - NULL if nothing to run */
    unsigned int volatile n_iters;/* Number of times through this workload */
};


/*
 * Some distributions provide a gettid(), others don't.
 */
static pid_t _private_gettid(void)
{
    return (pid_t)syscall(SYS_gettid);
}

#undef gettid
#define gettid() _private_gettid()


static PyObject *load_new(PyTypeObject *t, PyObject *args, PyObject *kwds)
{
    LoadObject *p = (LoadObject *)t->tp_alloc(t, 0);
    assert(p != NULL);
    p->n_threads = 1;
    p->first_thread = NULL;
    p->suspend_reasons = 0;
    p->work = NULL;
    pthread_attr_init(&p->thread_attr);
    return (PyObject *)p;
}


/*
Given a Python dictionary object of characteristics, update a loadgen
characteristics structure.
Return -1 if unsuccessful.
*/
static int update_field_long(unsigned long *field, PyObject *spec, char *name)
{
    PyObject *oval = PyDict_GetItemString(spec, name);
    if (oval == Py_None) {
        /* The value might be 'None'. Should we treat this as
           the field not being present? (TBD) */
        /* ignore */
    } else if (oval) {
        /* To avoid a DeprecationWarning, check for float and use a different converter */
        if (PyFloat_Check(oval)) {
            double d = PyFloat_AsDouble(oval);
            if (d < 0) {
                PyErr_SetString(PyExc_ValueError, "parameter must be non-negative");
                return -1;
            }
            *field = (unsigned long)d;
        } else {
            *field = PyInt_AsLong(oval);
        }
        if (PyErr_Occurred()) {
            return -1;
        }
    } else {
        /* ignore if value not supplied - not an error */
    }
    return 0;
}


static int update_field_int(unsigned int *field, PyObject *spec, char *name)
{
    unsigned long lfield = *field;
    int rc = update_field_long(&lfield, spec, name);
    *field = lfield;
    return rc;
}


static int update_field_float(double *field, PyObject *spec, char *name)
{
    PyObject *oval = PyDict_GetItemString(spec, name);
    if (oval) {
        *field = PyFloat_AsDouble(oval);
        if (PyErr_Occurred()) {
            return -1;
        }
    }
    return 0;
}


/*
Given a Python map containing workload attributes,
populate fields in a Character object. Fields that aren't
mentioned in the input map are not affected.
*/
static int setup_char(PyObject *spec, Character *c)
{
    int rc;
    /* The working set sizes are 64-bit */
    rc = update_field_long(&c->inst_working_set, spec, "inst");
    if (rc) return rc;
    rc = update_field_long(&c->data_working_set, spec, "data");
    if (rc) return rc;
    /* Remaining fields control aspects of the workload */
    rc = update_field_int(&c->workload_flags, spec, "flags");
    if (rc) return rc;
    rc = update_field_int(&c->debug_flags, spec, "debug_flags");
    if (rc) return rc;
    rc = update_field_long(&c->inst_target, spec, "inst_target");
    if (rc) return rc;
    rc = update_field_int(&c->data_pointer_offset, spec, "data_pointer_offset");
    if (rc) return rc;
    rc = update_field_int(&c->data_dispersion, spec, "data_dispersion");
    if (rc) return rc;
    rc = update_field_int(&c->data_alignment, spec, "data_alignment");
    if (rc) return rc;
    rc = update_field_int(&c->fp_intensity, spec, "fp_intensity");
    if (rc) return rc;    
    rc = update_field_int(&c->fp_operation, spec, "fp_operation");
    if (rc) return rc;    
    rc = update_field_int(&c->fp_precision, spec, "fp_precision");
    if (rc) return rc;    
    rc = update_field_int(&c->fp_concurrency, spec, "fp_concurrency");
    if (rc) return rc;
    rc = update_field_int(&c->fp_simd, spec, "fp_simd");
    if (rc) return rc;
    rc = update_field_int(&c->fp_flags, spec, "fp_flags");
    if (rc) return rc;
    rc = update_field_float(&c->fp_value, spec, "fp_value1");
    if (rc) return rc;
    rc = update_field_float(&c->fp_value2, spec, "fp_value2");
    if (rc) return rc;
    /* Force the instruction working set to a suitable minimum? */
#define MINIMUM_INST_WORKING_SET 64
    if (c->inst_working_set < MINIMUM_INST_WORKING_SET) {
        c->inst_working_set = MINIMUM_INST_WORKING_SET;
    }
    return 0;
}


/*
Instance initialization function. Called when a Load object is created:

   w = Load(...)

Should return 0 on success and -1 on error (see Python issue 17380).
*/
static int load_init(PyObject *x, PyObject *args, PyObject *kwds)
{
    LoadObject *p = (LoadObject *)x;
    PyObject *spec = NULL;
    static char *keys[] = { "spec", "threads", "verbose", NULL };
    int verbose = 0;
    int n_threads = p->n_threads;    /* load_new will have defaulted this to 1 */
    Character c;
    /* The default workload characteristics have no data and no FP operations.
       setup_char() will default the code working set to at least 1024 bytes. */
    workload_init(&c);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|ii", keys, &spec, &n_threads, &verbose)) {
        return -1;
    }
    assert(spec != NULL);

    if (setup_char(spec, &c) < 0) {
        return -1;
    }

    if (n_threads <= 0) {
        return -1;
    }
    p->n_threads = n_threads;

    if (verbose) {
        workload_verbose = verbose;
        fprintf(stderr, "pysweep: setting verbosity level to %d\n", verbose);
    }
    assert(p->work == NULL);
    p->work = workload_create(&c);
    if (p->work == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "load could not be created");
        return -1;
    }
    if (workload_verbose) {
        fprintf(stderr, "pysweep: %p: workload created\n", p->work);
    }
    assert(p->work != NULL);
    if (0) {
        /* Run the workload once on the current thread, as a check */
        /* This might trap with SIGILL if we've generated an invalid instruction */
        if (workload_verbose) {
            fprintf(stderr, "pysweep: %p: run once...\n", p->work);
        }
        workload_run_once(p->work);
        if (workload_verbose) {
            fprintf(stderr, "pysweep: finished running workload once.\n");
        }
    }
    return 0;
}


#define BENCH_NO_TRIAL    0x8000
#define BENCH_MMAP        0x4000
#define BENCH_CODE        0x2000


static PyObject *gfn_bench(PyObject *x, PyObject *args)
{
    int n_iters, flags;
    PyObject *spec = NULL;
    Character c;
    int i;
    if (!PyArg_ParseTuple(args, "Oii", &spec, &n_iters, &flags)) {
        return NULL;
    }
    workload_init(&c);
    if (setup_char(spec, &c) < 0) {
        return NULL;
    }
    flags |= c.debug_flags;
    if (flags & BENCH_MMAP) {
        /* Just mmap/munmap */
        unsigned long map_size = sysconf(_SC_PAGESIZE);
        unsigned int prot = PROT_READ|PROT_WRITE;
        if (flags & BENCH_CODE) {
            prot |= PROT_EXEC;
        }
        if (workload_verbose) {
            fprintf(stderr, "pysweep: benchmark mmap size %lu prot %#x\n", map_size, prot);
        }
        for (i = 0; i < n_iters; ++i) {
            void *p = mmap(NULL, map_size, prot, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
            assert(p != MAP_FAILED);
            munmap(p, map_size);
        }
    } else if (flags & BENCH_CODE) {
        /* Just coherency */ 
        unsigned int code_size = c.inst_working_set;
        unsigned long map_size = round_size(code_size, sysconf(_SC_PAGESIZE));
        void *p;
        if (workload_verbose) {
            fprintf(stderr, "pysweep: benchmark coherence size %u\n", code_size);
        }
        p = mmap(NULL, map_size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        for (i = 0; i < n_iters; ++i) {
            memset(p, (unsigned char)i, code_size);
            prepare_code(p, code_size, (PREPCODE_ALL & ~PREPCODE_PROTECT));
        }
        munmap(p, map_size);
    } else for (i = 0; i < n_iters; ++i) {
        Workload *w = workload_create(&c);
        if (w == NULL) {
            PyErr_SetString(PyExc_RuntimeError, "load could not be created");
            return NULL;
        }        
        if (!(flags & BENCH_NO_TRIAL)) {
            workload_run_once(w);
        }
        workload_free(w);
    }
    Py_RETURN_NONE;
} 


static PyObject *gfn_debug(PyObject *x, PyObject *args)
{
    int flags;
    if (!PyArg_ParseTuple(args, "i", &flags)) {
        return NULL;
    }
    workload_verbose = flags;
    fprintf(stderr, "pysweep: set diagnostic level to %u\n", workload_verbose);
    Py_RETURN_NONE;
}


#ifdef ARCH_AARCH64
static unsigned long long get_ctr(void)
{
    unsigned long ctr;
    __asm__ __volatile__("mrs %0,ctr_el0":"=r"(ctr)::);
    return ctr;
}

static PyObject *gfn_ctr(PyObject *x)
{
    return PyInt_FromLong(get_ctr());
}
#endif /* ARCH_AARCH64 */


/*
 * Sleep for a given amount of time, handling EINTR.
 */
static PyObject *gfn_sleep(PyObject *x, PyObject *args)
{
    int n_wait;
    double t;
    if (!PyArg_ParseTuple(args, "d", &t)) {
        return NULL;
    }
    n_wait = microsleep(t);
    return PyInt_FromLong(n_wait);
}

/*
 * Branch prediction code gen
 */
static PyObject *gfn_br_pred(PyObject *x, PyObject *args)
{
    int s;
    int ret;

    if (!PyArg_ParseTuple(args, "i", &s)) {
        return NULL;
    }
    ret = branch_load_gen(s);
    return PyInt_FromLong(ret);
}


/*
Thread 'main' function for the worker threads.
*/
static void *thread_start(void *ltv)
{
    load_thread_t *const lt = (load_thread_t *)ltv;
    load_thread_local_t volatile *const loc = lt->loc;
    LoadObject const *const lob = lt->load;
    Workload *last_work = NULL;
    void *work_data = NULL;
    int otype;
    /* The tid of this worker thread can be used to control it and also appears
       in diagnostic messages. */
    lt->os_tid = gettid();
    /* Allow the thread to be cancelled immediately without waiting until it
       encounters a system call. */
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &otype);
    /* We are good to go and can signal the parent to return to its caller. */
    sem_post(&lt->sem_started);
    /* Wait for the controller thread to release us */
    sem_wait(&lt->sem_worktodo);
    /* This loop runs continually, even while the workload is being updated. */
    for (;;) {
        /* The workload code doesn't take long, so we iterate it several times
           in order to get a suitable chunk of work, after which we can check
           for new work. To avoid resonance with working set sizes etc.,
           pick a prime number. */           
        const unsigned int N_ITERS = 1;  /* or 3, 117 etc. */
        /* Each iteration, we load whatever the workload is, and then run it.
           The workload might have changed since last time! */
        Workload *work = loc->vol_work;
        if (__builtin_expect(work != last_work, 0) || work == NULL) {
            /* Workload has been changed! */
            if (last_work != NULL) {
                if (workload_verbose) {
                    fprintf(stderr, "pysweep: [W %u] workload changed from %p to %p!\n", (unsigned int)lt->os_tid, last_work, work);
                }
                workload_remove_reference(last_work);
            }
            /* If we try to update the workload to a new specification
               and fail, workload_create() will be null. There's no
               point asking the executor threads to keep executing
               so we hope the caller will have suspended us. */
            while (work == NULL) {
                /* Wait for master to give us some work again */
                if (workload_verbose) {
                    fprintf(stderr, "pysweep: [W %u] waiting for work...\n", (unsigned int)lt->os_tid);
                }
                sem_wait(&lt->sem_worktodo);
                if (workload_verbose) {
                    fprintf(stderr, "pysweep: [W %u] resumed (suspend=%#x)\n", (unsigned int)lt->os_tid, lob->suspend_reasons);
                }
                work = loc->vol_work;
            }           
            work_data = work->entry_args[0];  /* Reset - including first time round */
            if (workload_verbose) {
                /* Report that the workload for the worker threads changed. */
                fprintf(stderr, "pysweep: [W %u] workload updated to code=%p with argument data=%p\n",
                    (unsigned int)lt->os_tid, work, work_data);
            }
            last_work = work;
        }
        if (0 && workload_verbose) {
            unsigned int const n_steps = N_ITERS * work->n_chain_steps;
            printf("  run %p for %u iters, %u steps, touched %#x\n",
                work_data, N_ITERS, n_steps, n_steps*64);
        }
        assert(work != NULL);
        work_data = workload_run(work, work_data, N_ITERS);
        /* Update iteration count for this thread */
        loc->n_iters += N_ITERS;
        /* TBD: should we put a memory fence here to flush the store? */
    }
    /* Don't expect to get here? */
    return NULL;
}


static PyObject *thread_new(PyTypeObject *ot, PyObject *args, PyObject *kwds)
{
    ThreadObject *t = (ThreadObject *)ot->tp_alloc(ot, 0);
    return (PyObject *)t;
}


/*
 * Update all threads' local copy of the workload.
 * This might be called with NULL, to temporarily stop a thread
 * working on anything.
 */
static void load_update_thread_work(LoadObject *p, Workload *w)
{
    load_thread_t *t;
    for (t = p->first_thread; t != NULL; t = t->next_thread) {
        int was_null = (t->loc->vol_work == NULL);
        if (w != NULL) {
            workload_add_reference(w);
        }
        t->loc->vol_work = w;
        if (w != NULL && was_null) {
            sem_post(&t->sem_worktodo);
        }
    }
}


/*
 * Create the worker threads for the load, using pthread_create().
 */
static PyObject *load_start(PyObject *x)
{
    LoadObject *p = (LoadObject *)x;
    unsigned int i;
    assert(p->n_threads > 0);
    if (p->first_thread != NULL) {
        /* Load is already started */
        PyErr_SetString(PyExc_RuntimeError, "load is already started");
        return NULL;
    }
    if (workload_verbose) {
        fprintf(stderr, "pysweep: starting workload %p...\n", p->work);
    }
    for (i = 0; i < p->n_threads; ++i) {
        int rc;
        char name[32];
        //load_thread_t *lt = (load_thread_t *)malloc(sizeof(load_thread_t));
        load_thread_local_t *loc = NULL;
        load_thread_t *lt = (ThreadObject *)PyObject_CallObject((PyObject *)&ThreadType, NULL);
        if (posix_memalign((void **)&loc, 64, sizeof(load_thread_local_t)) != 0) {
            PyErr_SetString(PyExc_RuntimeError, "could not allocate aligned memory");
            return NULL;
        }
        lt->loc = loc;
        loc->thread = lt;
        lt->load = p;
        sem_init(&lt->sem_started, 0, 0);
        sem_init(&lt->sem_worktodo, 0, 0);
        lt->os_tid = 0;    /* don't know it yet, will be found in-thread */
        loc->vol_work = NULL;
        loc->n_iters = 0;
        lt->next_thread = p->first_thread;
        p->first_thread = lt;        
        rc = pthread_create(&lt->pthread_id, &p->thread_attr, &thread_start, lt);
        if (rc < 0) {
            perror("pthread_create");
            break;
        }
        sprintf(name, "sweep-%u", i);
        rc = pthread_setname_np(lt->pthread_id, name);
        if (rc != 0) {
            perror("pthread_setname_np");
            /* Failure to set thread name isn't fatal */
        }
        if (workload_verbose) {
            fprintf(stderr, "pysweep: [* %u] created thread \"%s\"\n",
                (unsigned int)gettid(), name);
        }
    }
    /* At this point we should wait for all the threads to start and
       progress to the point where we know their thread identifiers. */
    {
        load_thread_t *lt;
        for (lt = p->first_thread; lt != NULL; lt = lt->next_thread) {
            sem_wait(&lt->sem_started);
            /* The thread should have started and recorded its tid. */
            assert(lt->os_tid > 0);
            if (workload_verbose) {
                fprintf(stderr, "pysweep: [* %u] has noted start of worker thread [W %u]\n",
                    (unsigned int)gettid(), (unsigned int)lt->os_tid);
            }
        }
    }
    /* All the threads have recorded their identifiers. We can now return
       to the caller and they can call the tids() method to get the tids. */
    /* First we release the threads, by setting the workload. */
    (void)sched_yield();
    load_update_thread_work(p, p->work);
    assert(p->first_thread != NULL);
    if (workload_verbose) {
        fprintf(stderr, "pysweep: workload threads started\n");
    }
    Py_RETURN_NONE;
}


static char const *signame(int sig)
{
    switch (sig) {
    case SIGSTOP:
        return "SIGSTOP";
    case SIGCONT:
        return "SIGCONT";
    case SIGKILL:
        return "SIGKILL";
    default:
        return "?";
    }
}


/*
 * Send a signal to all threads in the workload.
 */
static PyObject *load_signal_internal(PyObject *x, int sig)
{
    LoadObject *p = (LoadObject *)x;
    load_thread_t *t;
    for (t = p->first_thread; t != NULL; t = t->next_thread) {
        if (workload_verbose) {
            fprintf(stderr, "pysweep: [* %u] sending %s to thread [W %u]\n",
                (unsigned int)gettid(),
                signame(sig),
                (unsigned int)t->os_tid);
        } 
        pthread_kill(t->pthread_id, sig);
    }
    Py_RETURN_NONE;
}


static PyObject *load_signal(PyObject *x, PyObject *arg)
{
    return load_signal_internal(x, PyInt_AsLong(arg));
}


/*
 * We've got a reason to suspend the workers. If we hadn't already
 * suspended them for any other reason, suspend them now.
 */
static PyObject *load_suspend_internal(LoadObject *p, unsigned int reason)
{
    int was_suspended = (p->suspend_reasons != 0);
    p->suspend_reasons |= reason;
    if (!was_suspended) {
        if (workload_verbose) {
            fprintf(stderr, "pysweep: [* %u] suspending workers because %#x\n",
                (unsigned int)gettid(),
                reason);
        }
        /* Remove the threads' workload */
        load_update_thread_work(p, NULL);
        Py_RETURN_NONE;
    } else {
        /* Workload was already suspended - but this is another reason
           to suspend, which we musn't forget about. */
        if (workload_verbose) {
            fprintf(stderr, "pysweep: [* %u] re-suspending workers because %#x: now %#x\n",
                (unsigned int)gettid(),
                reason,
                p->suspend_reasons);
        }
        Py_RETURN_NONE;
    }
}


/*
 * We've removed a reason to suspend the workers. If there's no
 * longer any reason to suspend the workers, resume them.
 */
static PyObject *load_release_internal(LoadObject *p, unsigned int reason)
{
    if ((p->suspend_reasons & reason) != 0) {
        /* Was previously suspended because of request for zero affinity -
           but should not be suspended (for that reason) any more. */
        p->suspend_reasons &= ~reason;
        if (!p->suspend_reasons) {
            load_update_thread_work(p, p->work);
        }
    }
    Py_RETURN_NONE;
}


/*
 * Update the characteristics of an active workload object.
 * This will generally involve creating a new workload,
 * and deleting the old workload.
 * Active threads may be running the old workload,
 * so its destruction may be deferred.
 */
static PyObject *load_update(PyObject *x, PyObject *args)
{
    Workload *w, *w_old;
    Character c;
    LoadObject *p = (LoadObject *)x;
    PyObject *spec;
    if (!PyArg_ParseTuple(args, "O", &spec)) {
        return NULL;
    }
    if (workload_verbose) {
        fprintf(stderr, "pysweep: creating new workload for spec update\n");
    }
    workload_init(&c);
    if (setup_char(spec, &c)) {
        return NULL;
    }
    /* Try to create a new workload with these characteristics. */
    w = workload_create(&c);
    /* Update the workload. At some point the worker threads will pick up this
       new workload and start running it. It's possible that we failed
       to create the workload and that w is NULL. */
    w_old = p->work;
    p->work = w;
    if (w_old != NULL && w == NULL) {
        /* Now have no workload, so suspend the executors. */
        load_suspend_internal(p, SUSPEND_BADWORK);
        /* TBD: perhaps we should wait until the threads have suspended */
    } else if (w_old == NULL && w != NULL) {
        load_release_internal(p, SUSPEND_BADWORK);
    } else if (!p->suspend_reasons) {
        load_update_thread_work(p, w);
    }
    if (workload_verbose) {
        fprintf(stderr, "pysweep: destroying old workload %p\n", w_old);
    }
    workload_free(w_old);     /* Will be deferred until no longer in use */    
    if (workload_verbose) {
        fprintf(stderr, "pysweep: workload updated\n");
    }
    Py_RETURN_NONE;
}


/*
Stop (cancel and destroy) the object's threads.
If there are no threads this is a no-op.
This also means that any tids we have got for the load, or any events on
the load, become invalid.
*/
static PyObject *load_stop(PyObject *x)
{
    LoadObject *p = (LoadObject *)x;
    load_thread_t *t, *t_next;    
    void *retval;
    if (workload_verbose) {
        fprintf(stderr, "pysweep: stop workload\n");
    }
    /* Send cancellation requests to all threads */
    for (t = p->first_thread; t != NULL; t = t->next_thread) {
        int rc = pthread_cancel(t->pthread_id);
        if (rc) {
            perror("pthread_cancel");
        }
    }
    /* We should arrange to clean up thread resources once the thread has
       been cancelled. We can do that two ways:         
         - use pthread_join waiting to see PTHREAD_CANCELED as exit status
         - use pthread_cleanup_push to push a cleanup handler
     */
    for (t = p->first_thread; t != NULL; t = t_next) {
        int rc;
        t_next = t->next_thread;
        rc = pthread_join(t->pthread_id, &retval);
        if (rc) {
            perror("pthread_join");
        }
        if (t->loc->vol_work) {
            workload_remove_reference(t->loc->vol_work);
        }
        assert(retval == PTHREAD_CANCELED);
        sem_destroy(&t->sem_started);
        free((void *)t->loc);
        Py_DECREF(t);
        //free(t);
    }
    p->first_thread = NULL;
    Py_RETURN_NONE;
}


/*
 * Return the list of OS thread identifiers for the workload.
 * This list is non-empty if the workload has been started.
 */
static PyObject *load_tids(PyObject *x)
{
    LoadObject *p = (LoadObject *)x;
    load_thread_t *t;
    PyObject *list;
    /* If load is not started, it will return an empty list */
    list = PyList_New(0);
    for (t = p->first_thread; t != NULL; t = t->next_thread) {       
        PyList_Append(list, PyInt_FromLong(t->os_tid));
    }
    return list;
}


static PyObject *thread_tid(PyObject *x)
{
    ThreadObject *t = (ThreadObject *)x;
    if (!t->os_tid) {
        Py_RETURN_NONE;
    }
    return PyInt_FromLong(t->os_tid);
}


/*
 * Provide our own definition so we can build with Python 2.6
 */
static int MyPyLong_AsLongAndOverflow(PyObject *x, int *overflow)
{
    int n = PyLong_AsLong(x);
    if (PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_OverflowError)) {
        *overflow = 1;
        PyErr_Clear();
    } else {
        *overflow = 0;
    }
    return n;
}


static int affinity_object_to_set(PyObject *x, cpu_set_t *cpus)
{
    CPU_ZERO(cpus);
    if (PyInt_Check(x)) {
        unsigned long long cpumask = PyInt_AsUnsignedLongLongMask(x);
        unsigned int i;
        //printf("cpuset: int 0x%lx\n", cpumask);
        for (i = 0; i < 64; ++i) {
            if ((cpumask & (1ULL<<i)) != 0) {
                CPU_SET(i, cpus);
            }
        }
    } else if (PyLong_Check(x)) {
        int done;
        unsigned int pos = 0;
        do {
            unsigned int i;
            int overflow;
            unsigned long long bits = PyLong_AsUnsignedLongLongMask(x);
            for (i = 0; i < 64; ++i) {
                //printf("cpuset: long@%u: 0x%lx\n", pos, bits);
                if ((bits & (1ULL<<i)) != 0) {
                    if (i+pos >= CPU_SETSIZE) {
                        PyErr_SetString(PyExc_ValueError, "CPU number out of range");
                        return 0;
                    }
                    CPU_SET(i+pos, cpus);
                }
            }
            x = PyNumber_Rshift(x, PyInt_FromLong(64));
            pos += 64;
            done = (MyPyLong_AsLongAndOverflow(x, &overflow) == 0) && !overflow; 
        } while (!done);
    } else if (PyList_Check(x)) {
        unsigned int const size = PyList_Size(x);
        unsigned int i;
        CPU_ZERO(cpus);
        for (i = 0; i < size; ++i) {
            unsigned int n = PyInt_AsLong(PyList_GET_ITEM(x, i));
            if (n >= CPU_SETSIZE) {
                PyErr_SetString(PyExc_ValueError, "CPU number out of range");
                return 0;
            }
            CPU_SET(n, cpus);
        }
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected list or mask");
        return 0;
    }
    //printf("cpuset done: %u cpus\n", CPU_COUNT(cpus));
    return 1;
}

static PyObject *cpu_set_to_list(cpu_set_t const *cpus)
{
    unsigned int i;
    PyObject *r = PyList_New(0);
    for (i = 0; i < CPU_SETSIZE; ++i) {
        if (CPU_ISSET(i, cpus)) {
            PyList_Append(r, PyInt_FromLong(i));
        }
    }
    return r;
}


/*
 * Report total iterations across all threads
 */
static PyObject *load_iterations(PyObject *x)
{   
    LoadObject *p = (LoadObject *)x;
    unsigned long long n_iters = 0;
    load_thread_t *t;    
    for (t = p->first_thread; t != NULL; t = t->next_thread) {
         n_iters += t->loc->n_iters;
    }
    return PyInt_FromLong(n_iters);
}


static PyObject *load_thread_iterations(PyObject *x, PyObject *args)
{
    LoadObject *p = (LoadObject *)x;
    unsigned int tid;
    load_thread_t *t;
    if (!PyArg_ParseTuple(args, "I", &tid)) {
        return NULL;
    }
    for (t = p->first_thread; t != NULL; t = t->next_thread) {
        if (t->os_tid == (pid_t)tid) {
            return PyInt_FromLong(t->loc->n_iters);
        }
    }
    /* Either None or an exception will hopefully fault the caller. */
    return NULL;
}


static PyObject *thread_iterations(PyObject *x)
{
    ThreadObject *t = (ThreadObject *)x;
    return PyInt_FromLong(t->loc->n_iters);
}


/*
 * Set CPU affinity for the current workload. Affinity is supplied as a bitmask.
 * The threads of the workload are each free to use any of the given CPUs.
 * The set may be zero in which case the workload is suspended.
 * TBD: allow fine-grained pinning of individual threads.
 */
static PyObject *load_setaffinity(PyObject *x, PyObject *mask)
{
    load_thread_t *t;
    LoadObject *p = (LoadObject *)x;
    cpu_set_t affinity;
    if (!affinity_object_to_set(mask, &affinity)) {
        return NULL;
    }

    /* Set the affinity in the thread attributes. If we haven't created
       the threads yet, they will pick it up from here. */
    pthread_attr_setaffinity_np(&p->thread_attr, sizeof affinity, &affinity);

    /* Update running threads. We might not have started any threads yet. */

    /* If the mask is zero, sched_setaffinity will fail. So instead,
       suspend the thread. */
    if (CPU_COUNT(&affinity) == 0) {
        return load_suspend_internal(p, SUSPEND_ZEROAFF);
    }

    for (t = p->first_thread; t != NULL; t = t->next_thread) {
        int rc = sched_setaffinity(t->os_tid, sizeof affinity, &affinity);
        if (rc != 0) {
            PyErr_SetString(PyExc_RuntimeError, "sched_setaffinity failed");
            return NULL;
        }
    }

    return load_release_internal(p, SUSPEND_ZEROAFF);
}

static PyObject *load_getaffinity(PyObject *x)
{
    LoadObject *p = (LoadObject *)x;
    cpu_set_t affinity;
    pthread_attr_getaffinity_np(&p->thread_attr, sizeof affinity, &affinity);
    return cpu_set_to_list(&affinity);
}


/*
 * Set affinity for a single thread.
 */
static PyObject *thread_setaffinity(PyObject *x, PyObject *mask)
{
    int rc;
    ThreadObject *t = (ThreadObject *)x;
    cpu_set_t affinity;
    if (!affinity_object_to_set(mask, &affinity)) {
        return NULL;
    }
    rc = sched_setaffinity(t->os_tid, sizeof affinity, &affinity);
    if (rc != 0) {
        PyErr_SetString(PyExc_RuntimeError, "sched_setaffinity failed");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *thread_getaffinity(PyObject *x)
{
    ThreadObject *t = (ThreadObject *)x;
    cpu_set_t affinity;
    int rc = sched_getaffinity(t->os_tid, sizeof affinity, &affinity);
    if (rc) {
        PyErr_SetString(PyExc_RuntimeError, "sched_getaffinity failed");
        return NULL;
    }
    return cpu_set_to_list(&affinity);
}


/*
Set affinity for the current (monitoring) thread.
Input is an integer (up to 64 bits) representing a CPU mask.
TBD: handle larger integers.
*/
static PyObject *gfn_setaffinity(PyObject *x, PyObject *mask)
{
    cpu_set_t cpus;
    int rc;
    if (!affinity_object_to_set(mask, &cpus)) {
        return NULL;
    }
    rc = sched_setaffinity(0, sizeof cpus, &cpus);
    if (rc != 0) {
        PyErr_SetString(PyExc_RuntimeError, "sched_setaffinity failed");
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject *gfn_sched_yield(PyObject *x)
{
    (void)sched_yield();
    Py_RETURN_NONE;
}


static PyObject *load_suspend(PyObject *x)
{
    if (workload_verbose) {
        fprintf(stderr, "pysweep: suspending workload\n");
    }
    load_suspend_internal((LoadObject *)x, SUSPEND_REQUEST);
    Py_RETURN_NONE;
}


static PyObject *load_resume(PyObject *x)
{
    return load_release_internal((LoadObject *)x, SUSPEND_REQUEST);
}


static PyObject *load_suspense(PyObject *x)
{
    LoadObject *p = (LoadObject *)x;
    return PyInt_FromLong(p->suspend_reasons);
}


static PyObject *load_expected(PyObject *x)
{
    LoadObject *p = (LoadObject *)x;
    Workload *w = p->work;
    struct inst_counters const *e = &w->expected;

    if (e->n[COUNT_INST] == 0) {
        /* Either we haven't built the workload yet or something went wrong
           when we generated the metrics? */
        Py_RETURN_NONE;
    }
    /* Build a data structure with the expected workload metrics */
    PyObject *data = PyDict_New();
    PyDict_SetItemString(data, "n_inst", PyInt_FromLong(e->n[COUNT_INST]));
#define SETITEM(x, K) \
    PyDict_SetItemString(data, #x, PyFloat_FromDouble((float)e->n[COUNT_##K] / e->n[COUNT_INST]))
    SETITEM(branch, BRANCH);
    SETITEM(mem_read, INST_RD);
    SETITEM(bytes_read, BYTES_RD);
    SETITEM(mem_write, INST_WR);
    SETITEM(bytes_write, BYTES_WR);
    SETITEM(flop_sp, FLOP_SP);
    SETITEM(flop_dp, FLOP_DP);
    SETITEM(fence, FENCE);
    SETITEM(unit, UNIT);
#undef SETITEM
    return data;
}


static PyObject *load_threads(PyObject *x)
{
    LoadObject *p = (LoadObject *)x;
    ThreadObject *t;
    PyObject *tmap = PyDict_New();
    for (t = p->first_thread; t != NULL; t = t->next_thread) {
        PyDict_SetItem(tmap, PyInt_FromLong(t->os_tid), (PyObject *)t);
    }
    return tmap;
}


static PyObject *load_dump(PyObject *x, PyObject *args)
{
    LoadObject *p = (LoadObject *)x;
    int rc;
    char *fn;
    Workload *w = p->work;
    if (!PyArg_ParseTuple(args, "s", &fn)) {
        PyErr_SetString(PyExc_TypeError, "expected file name");
        return 0;
    }
    if (!w) {
        PyErr_SetString(PyExc_RuntimeError, "no workload to dump");
        return 0;
    }
    rc = workload_dump(w, fn, /*flags=*/0);
    /* TBD: bad file name etc. ought to be a Python exception */
    return PyInt_FromLong(rc);
}


static void load_dealloc(PyObject *x)
{
    LoadObject *p = (LoadObject *)x;
    if (workload_verbose) {
        fprintf(stderr, "pysweep: dealloc\n");
    }
    (void)load_stop(x);
    /* Any worker threads have now been cancelled and joined,
       so it's safe to free the workload. */
    workload_free(p->work);
    pthread_attr_destroy(&p->thread_attr);
    /* "finally (as its last action) call the type's tp_free function." */
    x->ob_type->tp_free(x);
}


static PyObject *thread_str(PyObject *x)
{
    ThreadObject *t = (ThreadObject *)x;
    return PyString_FromFormat("[%u]", (unsigned int)t->os_tid);
}


static PyMethodDef Load_methods[] = {
    {"start", (PyCFunction)&load_start, METH_VARARGS|METH_KEYWORDS, "None: start running a load"},
    {"update", (PyCFunction)&load_update, METH_VARARGS, "spec -> None: update load specification"},
    {"setaffinity", (PyCFunction)&load_setaffinity, METH_O, "list or mask -> None: set CPU affinity mask for workload"},
    {"getaffinity", (PyCFunction)&load_getaffinity, METH_NOARGS, "list: get CPU affinity"},
    {"stop", (PyCFunction)&load_stop, METH_NOARGS, "None: stop (cancel) load threads"},
    {"suspend", (PyCFunction)&load_suspend, METH_NOARGS, "None: suspend load threads"},
    {"resume", (PyCFunction)&load_resume, METH_NOARGS, "None: resume load threads"},
    {"signal", (PyCFunction)&load_signal, METH_O, "int: send signal to load threads"},
    {"suspense", (PyCFunction)&load_suspense, METH_NOARGS, "int: suspension status"},
    {"iterations", (PyCFunction)&load_iterations, METH_NOARGS, "int: total iterations so far"},
    {"thread_iterations", (PyCFunction)&load_thread_iterations, METH_VARARGS, "int -> int: iterations of a thread"},
    {"threads", (PyCFunction)&load_threads, METH_NOARGS, "{}: get set of threads"},
    {"tids", (PyCFunction)&load_tids, METH_NOARGS, "[tids]: get OS thread ids"},
    {"expected", (PyCFunction)&load_expected, METH_NOARGS, "{}: get expected instruction counts"},
    {"dump", (PyCFunction)&load_dump, METH_VARARGS, "str -> int: generate program image file"},
    {NULL}
};


static PyTypeObject LoadType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    tp_basicsize: sizeof(LoadObject),
    tp_name: "pysweep.Load",
    tp_doc: "system load",
    tp_flags: Py_TPFLAGS_DEFAULT,
    tp_methods: Load_methods,
    //tp_members: Load_members,
    tp_new: load_new,
    tp_init: load_init,
    tp_dealloc: load_dealloc
};


static PyMethodDef Thread_methods[] = {
    {"tid", (PyCFunction)&thread_tid, METH_NOARGS, "int: OS thread id"},
    {"setaffinity", (PyCFunction)&thread_setaffinity, METH_O, "list or mask -> None: set CPU affinity mask for thread"},
    {"getaffinity", (PyCFunction)&thread_getaffinity, METH_NOARGS, "list: get CPU affinity"},
    {"iterations", (PyCFunction)&thread_iterations, METH_NOARGS, "int: iterations so far"},
    {NULL}
};


static PyTypeObject ThreadType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    tp_basicsize: sizeof(ThreadObject),
    tp_name: "pysweep.Thread",
    tp_doc: "system load thread",
    tp_flags: Py_TPFLAGS_DEFAULT,
    tp_methods: Thread_methods,
    tp_new: thread_new,
    tp_str: thread_str
};


/* Static functions of this module */
static PyMethodDef funcs[] = {
    {"setaffinity", (PyCFunction)&gfn_setaffinity, METH_O, "list or mask -> None: set CPU affinity mask for future workloads"},
    {"sleep", (PyCFunction)&gfn_sleep, METH_VARARGS, "float -> int: sleep; like time.sleep() but correctly handling interrupts"},
    {"sched_yield", (PyCFunction)&gfn_sched_yield, METH_NOARGS, "None: yield to scheduler"},
    {"bench", (PyCFunction)&gfn_bench, METH_VARARGS, "(spec, int, int) -> None: measure workload creation time"},
    {"debug", (PyCFunction)&gfn_debug, METH_VARARGS, "int -> None: set diagnostic options"},
    {"br_pred", (PyCFunction)&gfn_br_pred, METH_VARARGS, "int -> scaling factor: Run Branch Prediction workload"},
#ifdef ARCH_AARCH64
    {"ctr", (PyCFunction)&gfn_ctr, METH_NOARGS, "-> int: get value of Cache Type Register"},
#endif /* ARCH_AARCH64 */
    {NULL}
};

static struct {
    char const *name;
    unsigned int value;
} const constants[] = {
    { "MEM_PREFETCH", WL_MEM_PREFETCH },
    { "MEM_NONTEMPORAL", WL_MEM_NONTEMPORAL },
    { "MEM_STREAM", WL_MEM_STREAM },
    { "MEM_HUGEPAGE", WL_MEM_HUGEPAGE },
    { "MEM_NO_HUGEPAGE", WL_MEM_NO_HUGEPAGE },
    { "MEM_FORCE_HUGEPAGE", WL_MEM_FORCE_HUGEPAGE },
    { "MEM_ACQUIRE", WL_MEM_ACQUIRE },
    { "MEM_BARRIER", WL_MEM_BARRIER },
    { "DEBUG_NO_CODE", WORKLOAD_DEBUG_DUMMY_CODE },
    { "DEBUG_NO_COHERENCE", WORKLOAD_DEBUG_NO_UNIFICATION },
    { "DEBUG_NO_MPROTECT", WORKLOAD_DEBUG_NO_MPROTECT },
    { "DEBUG_NO_WX", WORKLOAD_DEBUG_NO_WX },
    { "DEBUG_MMAP", BENCH_MMAP },
    { "DEBUG_CODE", BENCH_CODE },
    { "DEBUG_NO_TRIAL", BENCH_NO_TRIAL }
};

#if PY_MAJOR_VERSION < 3
#define INIT_NAME2(m) init##m
#else
#define INIT_NAME2(m) PyInit_##m
#endif
#define INIT_NAME(m) INIT_NAME2(m)
PyMODINIT_FUNC INIT_NAME(MODULE_NAME)(void)
{
    unsigned int i;
    PyObject *pmod;
#if PY_MAJOR_VERSION < 3
    pmod = Py_InitModule3(MODULE_NAME_STRING, funcs, "load generator");
#else
    static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        .m_name = MODULE_NAME_STRING,
        .m_doc = "load generator",
        .m_size = -1,
        .m_methods = funcs
    };
    pmod = PyModule_Create(&moduledef);
#endif
    PyType_Ready(&LoadType);
    PyObject_SetAttrString(pmod, "Load", (PyObject *)&LoadType);
    PyType_Ready(&ThreadType);
    for (i = 0; i < (sizeof constants / sizeof constants[0]); ++i) {
        PyModule_AddIntConstant(pmod, constants[i].name, constants[i].value);
    }
#if PY_MAJOR_VERSION >= 3
    return pmod;
#endif
}

/* end of pysweep.c */
