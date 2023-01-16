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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef MODULE_NAME
#define MODULE_NAME perf_events
#endif /* !MODULE_NAME */

#define MODULE_NAME_STRING3(m) #m
#define MODULE_NAME_STRING2(m) MODULE_NAME_STRING3(m)
#define MODULE_NAME_STRING MODULE_NAME_STRING2(MODULE_NAME)

#include <Python.h>

#if PY_MAJOR_VERSION < 3
/* Provide a function that can create a byte array (whose elements are 'int')
 * Some recent Python2 defines PyBytes_FromStringAndSize as an alias of
 * PyString_FromStringAndSize but this isn't suitable. */
#define MyBytes_FromStringAndSize PyByteArray_FromStringAndSize
#define MyBytes_AsString PyByteArray_AsString
#else
/* Python3. Emulate some Python2 API. */
#define MyBytes_FromStringAndSize PyBytes_FromStringAndSize
#define MyBytes_AsString PyBytes_AsString
#define PyString_FromFormat PyUnicode_FromFormat
#define PyString_AsString PyUnicode_AsUTF8
#define PyInt_FromLong PyLong_FromLong
#endif
#include <structmember.h>

#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/personality.h>
#include <poll.h>

/*
Include the system header that defines the layout of the structure passed
to perf_event_open.  This must match the kernel version!
*/
#include <linux/perf_event.h>
#ifndef PERF_NO_BREAKPOINTS
#include <linux/hw_breakpoint.h>
#endif /* !PERF_NO_BREAKPOINTS */
#ifndef PERF_AUX_FLAG_COLLISION
#define PERF_AUX_FLAG_COLLISION 0x08
#endif /* !PERF_AUX_FLAG_COLLISION */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#define PRINTF_DIAGNOSTICS


/*
 * Wrap the magic syscall.
 */
static int perf_event_open(struct perf_event_attr *attr,
                           pid_t pid, int cpu, int group_fd,
                           unsigned long flags)
{
    return syscall(__NR_perf_event_open, attr, (int)pid, (int)cpu, (int)group_fd, (int)flags);
}


#define PERF_FLAG_READ_USERSPACE     0x80000000    /* Read counters from userspace when possible */
#define PERF_FLAG_NO_READ_USERSPACE  0x40000000    /* Never read counters from userspace */
#define PERF_FLAG_WEAK_GROUP         0x20000000    /* Fall back to non-membership */


/*
 * Format of the data read using the read() call.
 * This is determined by the read_format field when the event is opened.
 * Now that we handle different values of read_format, this is not
 * directly read by the read call.
 */
typedef struct {
    unsigned long long value;           /* This is reset when we call reset() */
    unsigned long long time_enabled;    /* These aren't */
    unsigned long long time_running;
    unsigned long long id;
} event_sample_t;


/*
 * Capability flags, in case not defined individually
 */
const unsigned int _cap_user_rdpmc      = 0x04;
const unsigned int _cap_user_time       = 0x08;
const unsigned int _cap_user_time_zero  = 0x10;


/*
 * Python object corresponding to a perf event source - i.e. something
 * that we've got as a result of a perf_event_open call.
 */
typedef struct event_object_s EventObject;
struct event_object_s {
    PyObject_HEAD
    struct perf_event_attr attr;   /* kernel perf's info about the event */
    int cpu;                       /* cpu that this event is bound to, or -1 if all-cpu */
    //int pid;                       /* PID that this event is bound to, or -1 if system-wide */
    int fd;                        /* file handle from perf_event_open: unique to this event */
    unsigned long long id;         /* event unique identifier */
    int verbose;                   /* -vv or similar was used */
    int try_userspace_read:1;      /* try reading from userspace rather than read() */
    EventObject *group_leader;     /* group leader, or NULL */
    EventObject *buffer_owner;     /* buffer owner (even if we're not in a group) */
    EventObject *next_sub;         /* subordinate event, or NULL */
    unsigned short sample_id_bytes; /* in sample records, no. of trailing bytes for the sample_id */
    /* Data to support collecting individual perf events */
    struct perf_event_mmap_page *mmap_page;
    unsigned long mmap_size;          /* total size of the area, including header page */
    unsigned long mmap_data_size;     /* data size, exclusive of header page */
#define MMAP_DATA_SIZE_DEFAULT ((unsigned long)(-1))
    unsigned char *mmap_data_start;   /* start of data in the allocated area */
    unsigned char *mmap_data_end;     /* end of the allocated area */
    unsigned char *mmap_cursor;       /* current position in the area */
    int need_aux;                     /* set if event type needs AUX area */
    void *aux_area;                   /* AUX area e.g. for h/w trace */
    unsigned long aux_size;           /* total size of aux area (or 0) */
    /* We want reset() to start a new measurement window, after which we
       can read the event's value and also find out what percentage of
       time it was running for. But PERF_EVENT_IOC_RESET doesn't reset
       the enabled and running measurements. So instead we have to save
       the values at the start of the window. */
    PyObject *datasnap;
};


/*
 * Base class for either a reading or a group reading.
 */
typedef struct {
    PyObject_HEAD
    EventObject *event;
    event_sample_t sample;
    double fraction_running;
} BaseReadingObject;

static PyTypeObject BaseReadingType;


/*
 * A single counter reading. This is either from a non-group event, or as a
 * member of a group event reading.
 */
typedef struct {
    BaseReadingObject base;
    PyObject *adjusted_value;
} ReadingObject;

static PyTypeObject ReadingType;


/*
 * Reading from a group leader, with data from multiple events.
 */
typedef struct {
    BaseReadingObject base;
    unsigned int n_values;
    event_sample_t *samples;
} GroupReadingObject;

static PyTypeObject GroupReadingType;


/*
 * Time conversion parameters
 */
typedef struct {
    PyObject_HEAD
    unsigned long time_zero;
    unsigned long time_mult;
    unsigned long time_shift;
} TimeConvObject;

static PyTypeObject TimeConvType; 


/*
 * Map from fileno to Event object - useful when using select, poll etc.
 * We don't use a Python dict as we don't want this to act as a reference.
 */
static PyObject **fileno_events = NULL;
static unsigned int n_fileno_events = 0;

/*
 * Insert a file handle into the array.
 * We assume file handles are small integers and we can directly index into an array.
 */
static void fileno_event_insert(int fd, EventObject *e)
{
    assert(e != NULL);
    assert(fd >= 0);
    if ((unsigned int)fd >= n_fileno_events) {
        if (n_fileno_events == 0) {
            n_fileno_events = 32;
            fileno_events = (PyObject **)calloc(n_fileno_events, sizeof(PyObject *));
        } else {
            unsigned int nn = n_fileno_events * 4;
            fileno_events = (PyObject **)realloc(fileno_events, nn*sizeof(PyObject *));
            memset(fileno_events + n_fileno_events, 0, (nn-n_fileno_events)*sizeof(PyObject *));
            n_fileno_events = nn;
        }
    }
    assert(fileno_events[fd] == NULL);
    fileno_events[fd] = (PyObject *)e;
}


static PyObject *event_new(PyTypeObject *t, PyObject *args, PyObject *kwds)
{
    EventObject *e = (EventObject *)t->tp_alloc(t, 0);
    assert(e != NULL);
    //memset(&e->attr, 0, sizeof(EventObject) - offsetof(EventObject, attr));
    /* All fields that might be referred to in dealloc must be initialized early,
       but we are guaranteed that the block returned by tp_alloc is zero-initialized. */
    e->fd = -1;
    e->mmap_data_size = MMAP_DATA_SIZE_DEFAULT;
    e->mmap_page = NULL;
    e->group_leader = NULL;
    e->buffer_owner = NULL;
    e->next_sub = NULL;
    e->aux_size = MMAP_DATA_SIZE_DEFAULT;
    e->datasnap = NULL;
    return (PyObject *)e;
}


/*
 * Calculate the size of the sample_id structure that follows the record-specific
 * data when sample_id_all is set.
 * This is similar in principle to the initial part of a PERF_RECORD_SAMPLE payload,
 * but differs in detail as regards the ordering and selection of fields.
 * In particular PERF_SAMPLE_IDENTIFIER puts the identifier first in the
 * PERF_RECORD_SAMPLE payload, but last in the sample_id structure.
 */
static unsigned int perf_event_attr_sample_id_size(struct perf_event_attr const *a)
{
    unsigned int size = 0;
    if (!a->sample_id_all) {
        return 0;
    }
    if (a->sample_type & PERF_SAMPLE_TID) {
        size += 4+4;
    }
    if (a->sample_type & PERF_SAMPLE_TIME) {
        size += 8;
    }
    if (a->sample_type & PERF_SAMPLE_ID) {
        size += 8;
    }
    if (a->sample_type & PERF_SAMPLE_STREAM_ID) {
        size += 8;
    }
    if (a->sample_type & PERF_SAMPLE_CPU) {
        size += 8;
    }
    if (a->sample_type & PERF_SAMPLE_IDENTIFIER) {
        size += 8;
    }
    return size;
}


/*
 * Find the offset, in a sample record, of the PERF_SAMPLE_READ data.
 * We rely on there being no variable-length (data-dependent)
 * items before this data - all the preceding items have sizes
 * fixed at event configuration time.
 */
static unsigned int sample_offset_to_read(struct perf_event_attr const *e)
{
    unsigned int rdata_calc = 0;
    if (e->sample_type & PERF_SAMPLE_IDENTIFIER) {
        rdata_calc += 8;
    }
    if (e->sample_type & PERF_SAMPLE_IP) {
        rdata_calc += 8;
    }
    if (e->sample_type & PERF_SAMPLE_TID) {
        rdata_calc += 4+4;
    }
    if (e->sample_type & PERF_SAMPLE_TIME) {
        rdata_calc += 8;
    }
    if (e->sample_type & PERF_SAMPLE_ADDR) {
        rdata_calc += 8;
    }
    if (e->sample_type & PERF_SAMPLE_ID) {
        rdata_calc += 8;
    }
    if (e->sample_type & PERF_SAMPLE_STREAM_ID) {
        rdata_calc += 8;
    }
    if (e->sample_type & PERF_SAMPLE_CPU) {
        rdata_calc += 4+4;
    }
    if (e->sample_type & PERF_SAMPLE_PERIOD) {
        rdata_calc += 8;
    }
#if defined(PERF_SAMPLE_DATA_SRC) || !defined(PERF_RECORD_MMAP)
    if (e->sample_type & PERF_SAMPLE_DATA_SRC) {
        rdata_calc += 8;
    }
#endif
#if defined(PERF_SAMPLE_PHYS_ADDR) || !defined(PERF_RECORD_MMAP)
    if (e->sample_type & PERF_SAMPLE_PHYS_ADDR) {
        rdata_calc += 8;
    }
#endif
    /* PERF_DATA_READ data follows. Other data types (CALLCHAIN, TRANSACTION etc.) may follow that. */
    return rdata_calc;
}


static int perf_reading_size(EventObject const *e)
{
    int size;
    if (e->attr.read_format & PERF_FORMAT_GROUP) {
        /* Total size is unknown. */
        size = 8;  /* Number of values */
        /* Plus 8 or 16 bytes per value, depending on PERF_FORMAT_ID */
    } else {
        size = 8;  /* value */
        if (e->attr.read_format & PERF_FORMAT_ID) {
            size += 8;
        }
    }
    if (e->attr.read_format & PERF_FORMAT_TOTAL_TIME_ENABLED) {
        size += 8;
    }
    if (e->attr.read_format & PERF_FORMAT_TOTAL_TIME_RUNNING) {
        size += 8;
    }
    return size;
}


static int perf_reading_size_group(EventObject const *e, unsigned int n)
{
    int size = perf_reading_size(e);
    size += (n * 8);
    if (e->attr.read_format & PERF_FORMAT_ID) {
        size += (n * 8);
    }
    return size;
}


/*
 * Return the standard per-CPU buffer allocation, from /proc/sys/kernel/perf_event_mlock_kb
 */
static unsigned int perf_event_mlock_size(void);


#ifdef PRINTF_DIAGNOSTICS
typedef struct {
    char const *name;
    unsigned int value;
} flagnames_t;

static void fprint_flags(FILE *fd, unsigned long flags, flagnames_t const *flagnames, unsigned int n)
{
    unsigned int i;
    for (i = 0; i < n; ++i) {
        if (flags & flagnames[i].value) {
            fprintf(fd, "%s", flagnames[i].name);
            flags &= ~flagnames[i].value;
            if (flags) fprintf(fd, "|");
        }
    }
    if (flags) {
        fprintf(fd, "%#lx", flags);
    }
}

/* flag names, see /usr/include/linux/perf_event.h */
static flagnames_t const sample_flagnames[] = {
#define FLAG(x) { #x, PERF_SAMPLE_##x }
    FLAG(IDENTIFIER),
    FLAG(IP),
    FLAG(TID),
    FLAG(TIME),
    FLAG(ADDR),
    FLAG(READ),
    FLAG(CALLCHAIN),
    FLAG(ID),
    FLAG(CPU),
    FLAG(PERIOD),
    FLAG(STREAM_ID),
    FLAG(RAW),
    FLAG(BRANCH_STACK),
    FLAG(REGS_USER),
    FLAG(STACK_USER),
#if defined(PERF_SAMPLE_DATA_SRC) || !defined(PERF_RECORD_MMAP)
    FLAG(DATA_SRC),
#endif
#if defined(PERF_SAMPLE_PHYS_ADDR) || !defined(PERF_RECORD_MMAP)
    FLAG(PHYS_ADDR),
#endif
#undef FLAG
};

#define FLAG_NAMES(x) x, sizeof(x)/sizeof(x[0])

static void fprint_perf_event_attr(FILE *fd, struct perf_event_attr const *a)
{
    fprintf(fd, "perf_event_attr:\n");
    fprintf(fd, "  type                %d\n", a->type);
    fprintf(fd, "  size                %d\n", a->size);
    fprintf(fd, "  config              %#llx\n", (unsigned long long)a->config);
    if (a->sample_period != 0) {
        fprintf(fd, "  { sample_period, sample_freq }  %lu\n", (unsigned long)a->sample_period);
    }
    if (a->sample_type != 0) {
        /* Show combination of PERF_SAMPLE_XXX flags */
        fprintf(fd, "  sample_type:        ");
        fprint_flags(fd, a->sample_type, FLAG_NAMES(sample_flagnames));
        fprintf(fd, "\n");
    }   
    if (a->read_format != 0) {    
        /* Show combination of PERF_FORMAT_XXX flags */
        /* flag names, see /usr/include/linux/perf_event.h */
        static flagnames_t const flagnames[] = {
#define FLAG(x) { #x, PERF_FORMAT_##x }
            FLAG(TOTAL_TIME_ENABLED),
            FLAG(TOTAL_TIME_RUNNING),
            FLAG(ID),
            FLAG(GROUP)
#undef FLAG
        };
        fprintf(fd, "  read_format         ");
        fprint_flags(fd, a->read_format, FLAG_NAMES(flagnames));
        fprintf(fd, "\n");
    }
    fprintf(fd, "  disabled            %d    inherit            %d\n", a->disabled, a->inherit);
    fprintf(fd, "  pinned              %d    exclusive          %d\n", a->pinned, a->exclusive);  
    fprintf(fd, "  exclude_user        %d    exclude_kernel     %d\n", a->exclude_user, a->exclude_kernel);
    fprintf(fd, "  exclude_hv          %d    exclude_idle       %d\n", a->exclude_hv, a->exclude_idle);
    fprintf(fd, "  enable_on_exec      %d    task               %d\n", a->enable_on_exec, a->task);
    fprintf(fd, "  exclude_host        %d    exclude_guest      %d\n", a->exclude_host, a->exclude_guest);
#ifdef PERF_RECORD_MISC_COMM_EXEC
    if (a->comm || a->comm_exec) {
        fprintf(fd, "  comm                %d    comm_exec          %d\n", a->comm, a->comm_exec);
    }
#else
    if (a->comm) {
        fprintf(fd, "  comm                %d\n", a->comm);
    }
#endif
    if (a->precise_ip) {
        fprintf(fd, "  precise_ip          %d\n", a->precise_ip);
    }
    if (a->freq) {
        fprintf(fd, "  freq                %d\n", a->freq);
    }
    if (a->sample_id_all) {
        fprintf(fd, "  sample_id_all       %d\n", a->sample_id_all);
    }
    if (a->inherit_stat) {
        fprintf(fd, "  inherit_stat        %d\n", a->inherit_stat);
    }
    fprintf(fd, "  bp_type             %#x\n", a->bp_type);
    if (a->type != PERF_TYPE_BREAKPOINT) {
        if (a->config1) {
            fprintf(fd, "  config1             %#llx\n", a->config1);
        }
        if (a->config2) {
            fprintf(fd, "  config2             %#llx\n", a->config2);
        }
    } else {
        fprintf(fd, "  bp_addr             %#llx\n", a->bp_addr);
        fprintf(fd, "  bp_len              %#llx\n", a->bp_len);
    }
    fprintf(fd, "  mmap                %d\n", a->mmap);
    fprintf(fd, "  mmap2               %d\n", a->mmap2);
#if defined(PERF_RECORD_SWITCH) || !defined(PERF_RECORD_MMAP)
    fprintf(fd, "  context_switch      %d\n", a->context_switch);   /* since 4.3 */
#endif /* PERF_RECORD_SWITCH */
    if (a->sample_type & PERF_SAMPLE_BRANCH_STACK) {
        /* Show branch sample properties - field only valid when this flag set */
        static flagnames_t const flagnames[] = {
#define FLAG(x) { #x, PERF_SAMPLE_BRANCH_##x }
            FLAG(USER),
            FLAG(KERNEL),
            FLAG(HV),
            FLAG(ANY),
            FLAG(ANY_CALL),
            FLAG(ANY_RETURN),
            FLAG(IND_CALL),
            FLAG(ABORT_TX),
            FLAG(IN_TX),
            FLAG(NO_TX),
            FLAG(COND),
            FLAG(CALL_STACK),
            FLAG(IND_JUMP),
            FLAG(CALL)
#undef FLAG
        };
        fprintf(fd, "  branch_sample_type  ");
        fprint_flags(fd, a->branch_sample_type, FLAG_NAMES(flagnames));
        fprintf(fd, "\n");
    }
}
#endif /* PRINTF_DIAGNOSTICS */

/* Forward declarations */
static PyTypeObject EventType;
static PyTypeObject RecordType;

static int event_setup_buffer(EventObject *);
static int event_setup_buffer_aux(EventObject *, int);
static int event_get_id(EventObject *);
static PyObject *event_close(PyObject *x);

static int event_is_subordinate(EventObject const *e)
{
    return e->group_leader != NULL && e->group_leader != e;
}


/*
 * Instance initialization function for Event.
 *
 * Compared to the perf_event_open syscall (perf_event_attr, pid, cpu, group, flags),
 * the constructor for Event has some additional arguments:
 *  - sizes for the mmap buffer and AUX buffer, for when we allocate them
 *  - verbosity
 *
 * Return 0 on success.
 * Return -1 on failure to create event.
 */
static int event_init(PyObject *x, PyObject *args, PyObject *kwds)
{
    EventObject *e = (EventObject *)x;
    PyObject *e_attr = NULL;
    unsigned int n_tries = 0;
    int fd;
    /* perf event description parameters */
    int is_sampling;
    int e_pid = 0, e_tid = 0;
    int e_cpu = -1;      /* all CPUs */    
    int e_verbose = 0;
    int e_retry = 1;
    unsigned long e_mmap_data_size = MMAP_DATA_SIZE_DEFAULT;
    unsigned long e_mmap_aux_size = MMAP_DATA_SIZE_DEFAULT;
#ifdef PERF_FLAG_FD_CLOEXEC
    unsigned long e_flags = PERF_FLAG_FD_CLOEXEC;
#else
    unsigned long e_flags = 0;
#endif
    unsigned long e_custom_flags = 0;
    int e_group_fd = -1;
    PyObject *e_enabled_obj = NULL;

    PyObject *e_group_obj = NULL;
    PyObject *e_buffer_obj = NULL;
    EventObject *e_buffer_owner = NULL;

    static char *kwlist[] = {"attr",
                             "verbose",
                             "retry",
                             "pid",
                             "tid",
                             "cpu",
                             "mmap_size",
                             "aux_size",
                             "flags",
                             "enabled",
                             "group",
                             "buffer",
                             NULL};

    assert(e->fd == -1);
    memset(&e->attr, 0, sizeof e->attr);
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|iiiiikkkOOO", kwlist,
                                     &e_attr,
                                     &e_verbose,
                                     &e_retry,
                                     &e_pid,
                                     &e_tid,
                                     &e_cpu,
                                     &e_mmap_data_size,
                                     &e_mmap_aux_size,
                                     &e_flags,
                                     &e_enabled_obj,
                                     &e_group_obj,
                                     &e_buffer_obj)) {
        /* On failure this will already have raised the appropriate exception. */
#ifdef PRINTF_DIAGNOSTICS
        if (e_verbose) {
            fprintf(stderr, "perf event: Python API argument parsing failed\n");
        }
#endif /* PRINTF_DIAGNOSTICS */
        return -1;
    }
    if (e_mmap_data_size != MMAP_DATA_SIZE_DEFAULT) {
        e->mmap_data_size = e_mmap_data_size;
    }
    if (e_mmap_aux_size != MMAP_DATA_SIZE_DEFAULT) {
        e->aux_size = e_mmap_aux_size;
    }
    e->try_userspace_read = 1;

    /* Process non-canonical flags encoded in the same parameter as PERF_FLAG_... */
    e_custom_flags = e_flags & 0xff000000;
    e_flags &= ~e_custom_flags;
    if (e_custom_flags & PERF_FLAG_READ_USERSPACE) {
        e->try_userspace_read = 1;
    }
    if (e_custom_flags & PERF_FLAG_NO_READ_USERSPACE) {
        e->try_userspace_read = 0;
    }
    if (e->attr.read_format & PERF_FORMAT_GROUP) {
        /* If we want all the counters read at the same time, then it doesn't
           make sense to read 'live' values from userspace - unless those
           counters have been simultaneously frozen */
        e->try_userspace_read = 0;
    }

    {
        /* Get the perf_event_attr buffer. The buffer argument may be any of:
             bytes()
             bytearray()
             an object implementing the __bytes__ method, such as PerfEventAttr
        */
        char *data = NULL;
        unsigned int attr_size;
        if (PyBytes_Check(e_attr)) {
            attr_size = PyBytes_Size(e_attr);
            data = PyBytes_AsString(e_attr);
        } else if (PyByteArray_Check(e_attr)) {
            attr_size = PyByteArray_Size(e_attr);
            data = PyByteArray_AsString(e_attr);
        } else {
            PyObject *bytes_callable = PyObject_GetAttrString(e_attr, "__bytes__");
            if (bytes_callable) {
                PyObject *b_attr = PyObject_CallObject(bytes_callable, NULL);
                if (!b_attr) {
                    /* Call failed and an error has already been set */
                    return -1;
                }
                attr_size = PyBytes_Size(b_attr);
                data = PyBytes_AsString(b_attr);
            }
        }
        if (!data) {
#ifdef PRINTF_DIAGNOSTICS
            fprintf(stderr, "perf event: expected bytearray or bytes\n");
#endif /* PRINTF_DIAGNOSTICS */
            PyErr_SetString(PyExc_TypeError, "invalid event object");
            return -1;
        }
        if (attr_size > sizeof e->attr) {
#ifdef PRINTF_DIAGNOSTICS
            fprintf(stderr, "perf event: attribute size %x, only expecting %u\n",
                (unsigned int)attr_size, (unsigned int)(sizeof e->attr));
#endif /* PRINTF_DIAGNOSTICS */
            PyErr_SetString(PyExc_ValueError, "perf_event_open: attribute data too large");
            return -1;
        }
        memcpy(&e->attr, data, attr_size);
        if (e->attr.size != attr_size) {
#ifdef PRINTF_DIAGNOSTICS
            fprintf(stderr, "perf event: attribute length %u doesn't match structure size field %u\n",
                (unsigned int)attr_size, (unsigned int)(e->attr.size));
#endif /* PRINTF_DIAGNOSTICS */
            return -1;
        }
    }

    if (e_enabled_obj != NULL) {
        e->attr.disabled = !PyObject_IsTrue(e_enabled_obj);
    }

    /* This is a sampling event, which will need a buffer allocated.
       Strictly, we don't need to know that when we create the event.
       But it helps to create the buffer early so that
         (a) we don't lose any events if the target starts running
             before we create the buffer
         (b) when grouping events we can direct any subordinate
             events to the leader's buffer. */             
    is_sampling = e->attr.sample_freq != 0;
    if (e_tid != 0 && e_pid != 0) {
        /* mutually exclusive */
        return -1;
    }

    /* Verbosity setting sticks to the event */
    e->verbose = e_verbose;

    /* "This field specifies the format of the data returned by read(2) on a
        perf_event_open() file descriptor." Our event_sample_t must be in sync. */
    if (e->attr.read_format == 0) {
        if (!is_sampling) {
            /* This is a counting event, e.g. a hardware event count. */
            e->attr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED|PERF_FORMAT_TOTAL_TIME_RUNNING|PERF_FORMAT_ID;
        } else {
            /* This is a sampling event, returning records in the mmap buffer. */
            e->attr.read_format = PERF_FORMAT_ID;
        }
    }

    /* A "sampling" event is one that generates an overflow notification
       every N events, where N is given by some sample_period.
       A sampling event has sample_period > 0. When an overflow occurs,
       requested data is recorded in the mmap buffer...
       sample_freq can be used if you wish to use frequency rather than period...
       The kernel will adjust the sampling period to try and achieve the
       desired rate. The rate of adjustment is a timer tick. */
    if (e->attr.inherit && (e->attr.sample_type & PERF_SAMPLE_READ)) {
        /* Currently the kernel doesn't support this combination and will return EINVAL. */
        PyErr_SetString(PyExc_ValueError, "perf_event_open: PERF_SAMPLE_READ not supported with inherited events");
#ifdef PRINTF_DIAGNOSTICS
        if (e_verbose) {
            fprintf(stderr, "perf_event_open: PERF_SAMPLE_READ not supported with inherited events\n");
        }
#endif /* PRINTF_DIAGNOSTICS */
        return -1;        
    }

    if (e->attr.sample_type & PERF_SAMPLE_BRANCH_STACK) {
        /* Default to a usable value if the caller has failed to specify. TBD review */
        if (e->attr.branch_sample_type == 0) {
            e->attr.branch_sample_type = PERF_SAMPLE_BRANCH_ANY;
        }
    }

    if (e->attr.sample_type != 0) {
        /* TBD - set some flags that perf seems to set for perf record */
        e->attr.comm = 1;
#ifdef PERF_RECORD_MISC_COMM_EXEC
        e->attr.comm_exec = 1;
#endif /* PERF_RECORD_MISC_COMM_EXEC */
        e->sample_id_bytes = perf_event_attr_sample_id_size(&e->attr);
    } else {
        e->sample_id_bytes = 0;
    }

    if (e_group_obj == Py_None) {
        e_group_obj = NULL;
    }
    if (e_buffer_obj == Py_None) {
        e_buffer_obj = NULL;
    }
    if (e_buffer_obj != NULL) {
        if (!PyObject_TypeCheck(e_buffer_obj, &EventType)) {
            PyErr_SetString(PyExc_TypeError, "buffer must be another Event object");
            return -1;
        }
        e_buffer_owner = (EventObject *)e_buffer_obj;
    }
    if (e_group_obj != NULL) {
        /* We don't use the "O!" typechecking format as we want None to be
           equivalent to not using the argument - so typecheck here. */
        if (!PyObject_TypeCheck(e_group_obj, &EventType)) {
            PyErr_SetString(PyExc_TypeError, "group must be another Event object");
            return -1;
        } else {
            EventObject *ge = (EventObject *)e_group_obj;
            /* The group object might be a singleton or a group leader,
               but it mustn't be a subordinate. (We could be more flexible
               and use the leader if a subordinate was passed in, but we
               prefer to fault on the grounds that the caller may be confused.) */
            if (event_is_subordinate(ge)) {
                PyErr_SetString(PyExc_ValueError, "perf_event_open: group is not a group leader");
                return -1;
            }
            if (e_buffer_owner == NULL) {
                e_buffer_owner = ge;
            }
            e_group_fd = ge->fd;           
        }
        /* e_flags |= PERF_FLAG_FD_OUTPUT; "broken since Linux 2.6.35 " */
        /* is that equivalent to PERF_SAMPLE_READ? */
    }    
retry:;
    if (n_tries == 1 && !e_retry) {
        PyErr_SetString(PyExc_ValueError, "perf_event_open: invalid descriptor");
        return -1;
    }
    n_tries++;
    /* "pid == -1 and cpu == -1: This setting is invalid and will return an error." */
    /* even though this is the logical setting for offcore PMUs. */
    int const pidtid = (e_pid == 0 && e_tid != 0) ? e_tid : e_pid;
    assert(!(pidtid == -1 && e_cpu == -1));
    /* Userspace PMU read generally doesn't make sense if the live PMC counters are
       on other cores. So we should enable it only if monitoring the current thread. */
    if (e_tid != 0) {
        e->try_userspace_read = 0;
        /* TBD: should we also check attr.inherit=0 ? */
    }
#ifdef PRINTF_DIAGNOSTICS
    if (e_verbose) {
        if (e_verbose >= 2) {
            fprint_perf_event_attr(stderr, &e->attr);
        }
        fprintf(stderr, "perf_event_open: pid %d  cpu %d  group_fd %d  flags %#lx",
            e_pid, e_cpu, e_group_fd, e_flags);
    }
#endif /* PRINTF_DIAGNOSTICS */
    /* Allocate a file descriptor for this event. Note that even when we're
       supplying a fd for the group leader, we still get a new fd. */
    fd = perf_event_open(&e->attr, /*pid=*/pidtid, /*cpu=*/e_cpu, /*group_fd=*/e_group_fd, /*flags=*/e_flags);
    int const perf_event_open_errno = errno;
#ifdef PRINTF_DIAGNOSTICS
    if (e_verbose) {
        fprintf(stderr, " => %d\n", fd);
    }
#endif /* PRINTF_DIAGNOSTICS */
    if (fd < 0) {
        /* The perf event couldn't be opened. Maybe it's an invalid event
           or maybe we tried to open it with attributes that weren't
           valid for this event, in which case we can adjust and retry.
           What kind of error should we raise?
           We take the view that we should
             - raise PermissionError if the operation might have succeeded
               with more user privilege, or more relaxed perf_event_paranoid
             - raise ValueError if the user's input value was invalid
             - raise OSError in other cases
           Python 2 doesn't have PermissionError so we have to raise
           OSError and have the user check errno.
         */
        if (e_verbose) {
            fprintf(stderr, "sys_perf_event_open failed, errno = -%d\n", perf_event_open_errno);
            errno = perf_event_open_errno;
            perror("perf_event_open"); 
        }
        if (perf_event_open_errno == ENOENT ||
            perf_event_open_errno == EOPNOTSUPP) {
            /* Input was ok, we just can't do this event on this system.
               E.g. stalled-cycles-frontend.  perf stat reports "<not supported>". */
            if (e_verbose) {
                fprintf(stderr, "perf event: raising ValueError\n");
            }
            PyErr_SetString(PyExc_ValueError, "perf_event_open: event not supported");
        } else if (perf_event_open_errno == EINVAL) {
            /* E.g. passed in an undefined event code, or flags not appropriate
               for this event. */
            if (e_group_fd != -1) {
                /* Check if it was a bad group (or if we'd reached the group limit),
                   by trying to open the event outside the group. */
                e->attr.read_format &= ~PERF_FORMAT_GROUP;
                int temp_fd = perf_event_open(&e->attr, pidtid, e_cpu, /*group=*/-1, e_flags);
                if (temp_fd >= 0) {
                    /* We could create this event, just not in a group */
                    if (e_custom_flags & PERF_FLAG_WEAK_GROUP) {
                        fd = temp_fd;
                        goto event_created;
                    }
                    close(temp_fd);
                    PyErr_SetString(PyExc_ValueError, "perf_event_open: invalid group");
                    return -1;
                }
            }
            /* Exclusion flags might not be supported for uncore events */
            if (e->attr.exclude_guest) {
                if (e_verbose) {
#ifdef PRINTF_DIAGNOSTICS
                    fprintf(stderr, "perf_event_open: switching off exclude_guest flag\n");
#endif /* PRINTF_DIAGNOSTICS */
                }
                e->attr.exclude_guest = 0;
                goto retry;
            }
            if (e->attr.exclude_hv) {
                if (e_verbose) {
#ifdef PRINTF_DIAGNOSTICS
                    fprintf(stderr, "perf_event_open: switching off exclude_hv flag\n");
#endif /* PRINTF_DIAGNOSTICS */
                }
                e->attr.exclude_hv = 0;
                goto retry;
            }
            PyErr_SetString(PyExc_ValueError, "perf_event_open: invalid value");
        } else {
            /* could be EPERM e.g. trying to do system-wide monitoring */
            /* or EOPNOTSUPP */
            /* EOPNOTSUPP: branch tracing requested on inappropriate event (s/w, tracepoint etc.) */
            /* Unexpected error */
            /* TBD: we find on AArch64, raw events 0..63 not in PMCEIDR return EOPNOTSUPP,
               so we treat that as ENOENT */
            //fprintf(stderr, "perf event attr: type %d  config 0x%llx\n",
            //    (int)e->attr.type, (unsigned long long)e->attr.config);
            if (0 && !e_verbose) {
                /* Not already printed */
                fprintf(stderr, "perf_event_open: pid %d  cpu %d  group_fd %d  flags %#lx\n",
                    e_pid, e_cpu, e_group_fd, e_flags);
                errno = perf_event_open_errno;
                perror("perf_event_open");
            }
            errno = perf_event_open_errno;
            PyErr_SetFromErrno(PyExc_OSError);
        }
        return -1;
    }

event_created:
    /*
     * The event has been successfully created.
     */
    e->cpu = e_cpu;
    e->fd = fd;

    /*
     * Insert the event into our fileno->event map.
     * We don't want this map to act as a retainer for otherwise freed events,
     * so we ensure it doesn't contribute to the reference count.
     */
    fileno_event_insert(fd, e);

    /* Set need_aux to indicate that if we allocate a mmap buffer for events,
       we should also allocate an 'aux' buffer for bulk data. */
    if (e->attr.type == PERF_TYPE_HARDWARE ||
        e->attr.type == PERF_TYPE_HW_CACHE ||
        e->attr.type == PERF_TYPE_RAW ||
        e->attr.type == PERF_TYPE_TRACEPOINT ||
        e->attr.type == PERF_TYPE_BREAKPOINT ||
        e->attr.type == PERF_TYPE_SOFTWARE) {
        e->need_aux = 0;
    } else {
        e->need_aux = 1;   /* TBD should test event type */
    }
    if (e_buffer_owner != NULL) {
        Py_INCREF(e_buffer_owner);
        e->buffer_owner = e_buffer_owner;
        /* Add this event to the group leader's list of subordinates. */
        e->next_sub = e->buffer_owner->next_sub;
        e->buffer_owner->next_sub = e;
        /* TBD: what happens to this list when we delete events
           (in some order?) */
    }
    if (is_sampling) {
        if (e->buffer_owner == NULL) {
            event_setup_buffer(e);
        } else {
            int bfd = e->buffer_owner->fd;
            assert(bfd != -1);
            /* It doesn't seem to be possible for subordinate events to have
               their own buffer even if we wanted to - mmap() returns invalid. */
            int rc;
            rc = ioctl(e->fd, PERF_EVENT_IOC_SET_OUTPUT, bfd);
            if (rc) {
                perror("ioctl");
                fprintf(stderr, "perf event: error redirecting %d's events to %d's buffer\n", e->fd, bfd);
            }
        }
        event_get_id(e);
    } else if (e->attr.read_format & PERF_FORMAT_ID) {
        /* We need our id in order to match up when we read it */
        event_get_id(e);
    }
    if (e->try_userspace_read && !e->mmap_page) {
        if (!event_setup_buffer_aux(e, /*quiet=*/1)) {
            /* Maybe this kernel doesn't support mmap'ing a non-sampling event */
            e->try_userspace_read = 0;
        }
    }
    /* This is the only success return in this function. We must have created an event. */
    assert(e->fd >= 0);
    return 0;
}


#ifdef PRINTF_DIAGNOSTICS
/*
 * Print the current status of an mmap page, from its header.
 * This information will change continuously as the kernel adds more records.
 */
static void fprint_mmap_page_status(FILE *fd, struct perf_event_mmap_page const *mp)
{
    fprintf(fd, "time enabled = %lu, time running = %lu", (unsigned long)mp->time_enabled, (unsigned long)mp->time_running);
    /* "data_head points to the head of the data section. The value continually increases, it does not wrap." */
    fprintf(fd, ", data_tail = %lx, ", (unsigned long)mp->data_tail);    
    /* data_tail should be written by user space to reflect the last read data." */
    fprintf(fd, "data_head = %lx", (unsigned long)mp->data_head);
    if (mp->aux_size) {
        fprintf(fd, ", aux_size = %lx, ", (unsigned long)mp->aux_size);
        fprintf(fd, "aux_tail = %lx, ", (unsigned long)mp->aux_tail);
        fprintf(fd, "aux_head = %lx", (unsigned long)mp->aux_head);
    }
    fprintf(fd, "\n");
    if (mp->capabilities & _cap_user_rdpmc) {
        /* Show if a userspace-readable hardware counter is currently mapped to this event */
        /* mp->index is not necessarily small: see IA32_FIXED_CTR... */
        fprintf(fd, "counter index = 0x%x, offset = 0x%lx\n", (int)mp->index, (unsigned long)mp->offset);
    }
}


static void fprint_mmap_page(FILE *fd, struct perf_event_mmap_page const *mp)
{
    fprintf(fd, "mmap page:\n");
    fprintf(fd, "  version = %u, ", mp->version);
    fprintf(fd, "index = 0x%x, ", mp->index);
    fprintf(fd, "data_size = 0x%lx, data_offset = 0x%lx\n", (unsigned long)mp->data_size, (unsigned long)mp->data_offset);  /* since 4.1 */
#ifndef PERF_NO_CAPABILITIES
    /* usually 0x1e: cap_user_time_zero, cap_user_time, cap_user_rdpmc, cap_bit0_is_deprecated */
    fprintf(fd, "  capabilities = 0x%llx", mp->capabilities);
    if (mp->cap_user_rdpmc) {
        fprintf(fd, ", pmc_width = %u", mp->pmc_width);
    }
    if (mp->cap_user_time_zero) {
        fprintf(fd, ", time_zero = %#llx", mp->time_zero);
        fprintf(fd, ", time_shift = %d", mp->time_shift);
        fprintf(fd, ", time_mult = %d", mp->time_mult);
    }
    fprintf(fd, "\n");
#endif /* !PERF_NO_CAPABILITIES */
    fprint_mmap_page_status(fd, mp);
}
#endif /* PRINTF_DIAGNOSTICS */


/*
 * Map a userspace-accessible ring buffer on to the event's file descriptor so
 * that we can retrieve a series of samples produced by the perf subsystem.
 *
 * Ability to mmap() is subject to the RLIMIT_MEMLOCK resource limit.
 * perf_mmap in events/core.c shows that we get a set amount per CPU
 * (see /proc/sys/kernel/perf_event_mlock_kb), and any allocation above
 * that is counted against RLIMIT_MEMLOCK. Exceeding the limit results
 * in EPERM.
 *
 * So when sampling multiple events, we would generally want to redirect
 * all the events to one sampling buffer per CPU even if the events aren't
 * being scheduled on to PMU as a group.
 */
static int event_setup_buffer_aux(EventObject *e, int quiet)
{
    void *pmap;
    unsigned int const page_size = sysconf(_SC_PAGESIZE);
    /* There's a limit on how much memory we can map, and our various mmap
       buffers (main, aux etc.) have to come out of that. So we adjust the
       sizes of the requests so that we don't get an error. */
    if (e->mmap_data_size == MMAP_DATA_SIZE_DEFAULT) {
        unsigned int data_size;
        data_size = perf_event_mlock_size() - page_size;
        if (e->need_aux) {
            data_size /= 2;
        }
        e->mmap_data_size = data_size;
    }    
    /* Round up to a page size (it should be already) */
    if (e->mmap_data_size < page_size) {
        e->mmap_data_size = page_size;
    }
    /* Size actually mapped should be 1 + 2^n pages, to allow for the header */
    e->mmap_size = page_size + e->mmap_data_size;
    /* Provide the buffer to the file handle */
#ifdef PRINTF_DIAGNOSTICS
    if (e->verbose >= 2) {
        fprintf(stderr, "mmap(size=%#lx,fd=%d)", e->mmap_size, e->fd);
    }
#endif /* PRINTF_DIAGNOSTICS */
    pmap = mmap(NULL, e->mmap_size, PROT_READ|PROT_WRITE, MAP_SHARED, e->fd, 0);
    int const mmap_errno = errno;
#ifdef PRINTF_DIAGNOSTICS
    if (e->verbose >= 2) {
        fprintf(stderr, " => %p\n", pmap);
    }
#endif /* PRINTF_DIAGNOSTICS */
    if (pmap == MAP_FAILED) {
        if (!quiet) {
            errno = mmap_errno;
            perror("mmap");
            if (event_is_subordinate(e)) {
                fprintf(stderr, "  (event is subordinate to fd=%d)\n", e->buffer_owner->fd);
            }
            if (mmap_errno == EINVAL || 1) {
                fprintf(stderr, "  size = %lu/0x%lx, page size = %u/0x%x, fd=%d\n",
                    e->mmap_size, e->mmap_size,
                    page_size, page_size,
                    e->fd);
            }
            if (mmap_errno == EPERM) {
                fprintf(stderr, "  maybe exceeded process's MEMLOCK limit?\n");
            }
        }
        return 0;
    }
    e->mmap_page = (struct perf_event_mmap_page *)pmap;
    e->mmap_data_start = (unsigned char *)pmap + page_size;
    e->mmap_data_end = (unsigned char *)pmap + e->mmap_size;
    e->mmap_cursor = (unsigned char *)pmap + page_size;
#ifdef PRINTF_DIAGNOSTICS
    /* The kernel will have filled in some information fields. */
    if (e->verbose >= 2) {
        fprintf(stderr, "%p: perf buffer size = 0x%lx, ", pmap, e->mmap_size);
        fprint_mmap_page(stderr, e->mmap_page);
    }
#endif /* PRINTF_DIAGNOSTICS */
#if defined(PERF_RECORD_AUX) || !defined(PERF_RECORD_MMAP)
    if (e->need_aux) {
        void *paux;
        assert(e->aux_area == 0);
        if (e->aux_size == MMAP_DATA_SIZE_DEFAULT) {
            // TBD follow perf in halving the size
            e->aux_size = e->mmap_data_size / 2;
        }
        /* "To set up an AUX area, first aux_offset nedes to be set with
           an offset greater than data_offset+data_size and aux_size
           needs to be set to the desired buffer size. The desired offset
           and size must be page aligned, and the size must be a power of two." */
        unsigned long aux_offset = e->mmap_size;
        assert(aux_offset % page_size == 0);
        assert(e->aux_size % page_size == 0);
        e->mmap_page->aux_offset = aux_offset;
        e->mmap_page->aux_size = e->aux_size;
#ifdef PRINTF_DIAGNOSTICS
        if (e->verbose >= 2) {
            fprintf(stderr, "mmap(%#lx, fd=%d, %#llx) for aux buffer\n", e->aux_size, e->fd, (unsigned long long)aux_offset);
        }
#endif /* PRINTF_DIAGNOSTICS */
        paux = mmap(NULL, e->aux_size, PROT_READ|PROT_WRITE, MAP_SHARED, e->fd, aux_offset);
        if (paux == MAP_FAILED) {
            perror("mmap(aux)");
            fprintf(stderr, "  failed to allocate AUX buffer: %lu/0x%lx, fd=%d, type=%d\n",
                e->aux_size, e->aux_size,
                e->fd, e->attr.type);
            return 0;
        }
        e->aux_area = paux;
        if (e->verbose >= 2) {
            fprintf(stderr, "%p: aux buffer size = 0x%lx\n", e->aux_area, e->aux_size);
        }
    }
#endif /* PERF_RECORD_AUX */
    return 1;     
}


static int event_setup_buffer(EventObject *e)
{
    return event_setup_buffer_aux(e, /*quiet=*/0);
}


/*
 * Rebind an event object to a new perf_event_attr.
 * Not well tested.
 */
static PyObject *event_bind(PyObject *x, PyObject *args, PyObject *kwds)
{
    EventObject *e = (EventObject *)x;
    int rc;
    (void)event_close(x);
    rc = event_init(x, args, kwds);
    if (rc != 0) {
        PyErr_SetString(PyExc_ValueError, "bad value");
        return NULL;
    }
    return (PyObject *)e;
}


/*
 * Given a leader event and an event id, find the event object.
 * We might do this every time we read a sample record from the memory buffer,
 * so it needs to be reasonably fast. Currently we scan a linked list of
 * the leader's subordinates. Typically we only call this when the id
 * is known to be a subordinate (or the leader itself) so we won't be doing
 * a lot of full-length scans unnecessarily.
 * Chaining lots of events together is likely to hit various other performance
 * issues so currently we don't focus on making the lookup quicker.
 */
#if 0
static EventObject *event_find_subordinate(EventObject const *e, unsigned long long id)
{
    assert(id != 0);
    assert(e->buffer_owner == NULL);
    while (e) {
        /* As this is a frequently called routine, we shouldn't be
           discovering the events' ids only now, we should have done
           this as soon as we knew we might have to do this lookup. */
        assert(e->id != 0);
        if (e->id == id) {
            break;
        }
        e = e->next_sub;
    }
    /* Generally this will not be NULL. */
    return (EventObject *)e;
}
#endif


/*
 * The __repr__ should be unambiguous.
 * So it should contain the full contents of the object. TBD.
 */
static PyObject *event_repr(PyObject *x)
{
    EventObject const *e = (EventObject *)x;
    return PyString_FromFormat("Event(type=%u,n=0x%x,cpu=%d)",
        (unsigned int)e->attr.type, (unsigned int)e->attr.config, (int)e->cpu);
}


/*
 * Return the byte string corresponding to the event's perf_event_attr.
 * In Python2, this should be a bytearray().
 * In Python3, this should be a bytes() object.
 *
 * Better would be to return a PerfEventAttr object, but that would involve this
 * module knowing about perf_attr. Instead, caller can do:
 *   PerfEventAttr(e.attr_struct)
 */
static PyObject *event_attr_struct(PyObject *x)
{
    EventObject const *e = (EventObject *)x;
    return MyBytes_FromStringAndSize((char const *)&e->attr, sizeof e->attr);
}


/*
 * Check whether new event(s) are available in the ring buffer,
 * by inspecting the head pointer.
 */
static int event_available(EventObject *e)
{
    assert(e->mmap_page != NULL);
    return e->mmap_page->data_tail != e->mmap_page->data_head;
}


/*
 * Check whether an event record is available in the mmap buffer -
 * first making sure that events are actually enabled. Non-blocking.
 */
static PyObject *event_poll(PyObject *x)
{
    EventObject *e = (EventObject *)x;
    if (!e->mmap_page) {
        if (!event_setup_buffer(e)) {
            PyErr_SetString(PyExc_ValueError, "no buffer allocated");
            return NULL;
        }
        assert(e->mmap_page != NULL);
        /* Very unlikely the event occurred between allocation and the below test,
           but maybe perf has been waiting to put some events in. */
    }
    if (e->attr.disabled && !e->attr.enable_on_exec) {
        int rc = ioctl(e->fd, PERF_EVENT_IOC_ENABLE, 0);
        if (rc < 0) {
            perror("ioctl(ENABLE)");
        }
        e->attr.disabled = 0;
    }
    if (e->verbose >= 1) {
        fprintf(stderr, "[%d] polling: ", e->fd);
        fprint_mmap_page_status(stderr, e->mmap_page);
    }
    return PyBool_FromLong(event_available(e));
}


/*
 * Test if an event still 'exists'. An event bound to a terminated subprocess will
 * cease to exist, and our copy of its file descriptor will report ready when polled.
 */
static PyObject *event_is_active(PyObject *x)
{
    EventObject *e = (EventObject *)x;
    static struct timespec const fto = { 0, 0 };
    struct pollfd fds = { e->fd, POLLIN, 0 };
    int rc = ppoll(&fds, 1, &fto, NULL);
    if (e->verbose) {
        if (rc == 1) {
            fprintf(stderr, "poll(%d): revents=0x%x\n", e->fd, (unsigned int)fds.revents);
        } else {
            fprintf(stderr, "poll(%d): rc=%d\n", e->fd, rc);
            perror("poll");
        }
    }
    /* It's active if the event's not ready, or if it's got some data, or if it's just there and hasn't hung up */
    /* Note that POLLIN takes priority over POLLHUP in case there's unconsumed data ready. */
    int is_active = (rc == 0) || (rc == 1 && ((fds.revents & POLLIN) != 0 || (fds.revents & POLLHUP) == 0));
    if (e->verbose) {
        fprintf(stderr, "is_active: %d\n", is_active);
    }
    return PyBool_FromLong(is_active);
}


/*
 * To read events from userspace, or to get the capabilities that say
 * whether we can do that, we need at least the header page of the mmap buffer.
 */
static void ensure_minimal_mmap_page(EventObject *e)
{
    if (!e->mmap_page) {
        if (e->verbose) {
            fprintf(stderr, "[%d] ensuring mmap page\n", e->fd);
        }
        event_setup_buffer(e);
    }
    assert(e->mmap_page != NULL);
}


/*
 * Get the event capabilities from the mmap page. If we haven't already
 * allocated a page, allocate a minimal one now.
 */
static PyObject *event_capabilities(PyObject *x)
{
    EventObject *e = (EventObject *)x;
    ensure_minimal_mmap_page(e);
  //    fprint_mmap_page(stderr, e->mmap_page);  // TBD remove
    return PyInt_FromLong(e->mmap_page->capabilities);
}


/*
 * Close the event's file descriptor. We may need to do this to cause events
 * to be flushed into the ring buffer.
 */
static PyObject *event_close(PyObject *x)
{
    EventObject *e = (EventObject *)x;
    if (e->fd != -1) {
#ifdef PRINTF_DIAGNOSTICS
        if (e->verbose) {
            fprintf(stderr, "[%d] close event\n", e->fd);
        }
#endif /* PRINTF_DIAGNOSTICS */
        int rc = close(e->fd);
        if (rc != 0) {
            PyErr_SetFromErrno(PyExc_OSError);
            perror("close");
            return NULL;            
        }
        /* Now delete the file handle from the map before it goes away. */
        fileno_events[e->fd] = NULL;
        e->fd = -1;
        if (e->group_leader) {
            Py_DECREF(e->group_leader);
            e->group_leader = NULL;
        }
    }
    Py_RETURN_NONE;
}


static void event_free_buffers(PyObject *x)
{
    EventObject *e = (EventObject *)x;
    if (e->mmap_page != NULL) {
        int rc = munmap(e->mmap_page, e->mmap_size);
        if (rc) {
            perror("munmap");
        }
        e->mmap_page = NULL;
    }
    if (e->aux_area != NULL) {
        int rc = munmap(e->aux_area, e->aux_size);
        if (rc) {
            perror("munmap(aux)");
        }
        e->aux_area = NULL;
    }
}


static void event_dealloc(PyObject *x)
{
    EventObject *e = (EventObject *)x;
    //fprintf(stderr, "-- event %p dealloc\n", e);
#ifdef PRINTF_DIAGNOSTICS
    if (e->verbose && e->fd >= 0) {
        fprintf(stderr, "[%d] event object deallocated when event open: closing\n", e->fd);
    }
#endif /* PRINTF_DIAGNOSTICS */
    (void)event_close(x);
    if (e->buffer_owner) {
        Py_DECREF(e->buffer_owner);
        e->buffer_owner = NULL;
    }
    event_free_buffers(x);
    if (e->datasnap) {
        Py_DECREF(e->datasnap);
    }
    x->ob_type->tp_free(x);
}


static void fprint_event_sample(FILE *fd, event_sample_t const *ed)
{
    fprintf(fd, "value: %llx, enabled: %llx, running: %llx\n",
        ed->value, ed->time_enabled, ed->time_running);
}


static void subtract_base_event_values(event_sample_t *a, event_sample_t const *b)
{
    a->value -= b->value;
    a->time_enabled -= b->time_enabled;
    a->time_running -= b->time_running;
}


static void subtract_event_values(PyObject *a, PyObject const *b, EventObject *e)
{
    if (b) {
        BaseReadingObject *ae = (BaseReadingObject *)a;
        BaseReadingObject const *be = (BaseReadingObject const *)b;
        subtract_base_event_values(&ae->sample, &be->sample);
        if (e->attr.read_format & PERF_FORMAT_GROUP) {
            GroupReadingObject *ag = (GroupReadingObject *)a;
            GroupReadingObject const *bg = (GroupReadingObject const *)b;
            unsigned int i;
            for (i = 0; i < bg->n_values; ++i) {
                subtract_base_event_values(&ag->samples[i], &bg->samples[i]);
            }
        }
    }
}


/*
 * Adjust a single value using the time enabled/running counters.
 * After this call, the value is either a float, or None.
 */
static void postprocess_value(PyObject **vp, unsigned long long value, BaseReadingObject *br)
{
    if (*vp && *vp != Py_None) {
        Py_DECREF(*vp);
    }
    if (br->fraction_running != 0.0) {
        *vp = PyFloat_FromDouble(value / br->fraction_running);
        Py_INCREF(*vp);
    } else {
        *vp = Py_None;
    }
}


static void postprocess_reading(PyObject *x)
{
    BaseReadingObject *br = (BaseReadingObject *)x;
    if (br->sample.time_enabled != 0.0) {
        br->fraction_running = (double)br->sample.time_running / br->sample.time_enabled;
    } else {
        br->fraction_running = 0.0;
    }
    if (!(br->event->attr.read_format & PERF_FORMAT_GROUP)) {
        ReadingObject *r = (ReadingObject *)x;
        postprocess_value(&r->adjusted_value, br->sample.value, br);
    }
    if (br->event->verbose) {
        double scale = (br->fraction_running ? (1.0 / br->fraction_running) : 0.0);
        fprintf(stderr, "perf event %p: enabled=%llu running=%llu=%.5f",
            br->event,
            (unsigned long long)br->sample.time_enabled,
            (unsigned long long)br->sample.time_running,
            br->fraction_running);
        if (!(br->event->attr.read_format & PERF_FORMAT_GROUP)) {
            ReadingObject *r = (ReadingObject *)x;
            fprintf(stderr, " value=%llu (adj=%.2f)\n",
                (unsigned long long)r->base.sample.value,
                r->base.sample.value * scale);
        } else {
            GroupReadingObject *r = (GroupReadingObject *)x;
            unsigned int i;
            fprintf(stderr, " values=%u:", (unsigned int)r->n_values);
            for (i = 0; i < r->n_values; ++i) {
                fprintf(stderr, " value=%llu (adj=%.2f)",
                    (unsigned long long)r->samples[i].value,
                    r->samples[i].value * scale);
            }
            fprintf(stderr, "\n");
        }
    }
}


static PyObject *populate_reading_object_from_data(PyObject *x, void const *data, EventObject *e);

static PyObject *perf_read_count_using_read(PyObject *x)
{
    /*
     * Read counter data for an event using read().
     * x is a Reading or GroupReading object.
     * If the event was PERF_FORMAT_GROUP, we are collecting multiple
     * events in one operation on this file descriptor.
     */
    int n;
    int size_expected = -1;
    unsigned long long buf[20];    /* 8 counters * (1+1) + 4 header */
    BaseReadingObject *base = (BaseReadingObject *)x;
    EventObject *e = base->event;
    size_t tr = perf_reading_size(e);
    if (e->fd == -1) {
        PyErr_SetString(PyExc_ValueError, "counter error - attempt to read closed counter");
        return 0;
    }
    assert(tr <= sizeof buf);
    if (e->attr.read_format & PERF_FORMAT_GROUP) {
        /* The amount to read is determined by the size of the group.
           The kernel will return ENOSPC if we don't supply a big enough buffer. */
        n = read(e->fd, buf, sizeof buf);
        if (n > 0) {
            unsigned int n_values = (unsigned int)buf[0];
            size_expected = perf_reading_size_group(e, n_values);
        }
    } else {
        n = read(e->fd, buf, tr);
        size_expected = tr;
    }
    if (n > 0 && n != size_expected) {
        PyErr_SetString(PyExc_ValueError, "unexpected size from read()");
        return 0;
    } else if (n > 0) {
        /* Observation was read as expected. */
        return populate_reading_object_from_data(x, buf, e);
    } else if (n == 0) {
        fprintf(stderr, "perf_events: tried to read while event in error state\n");
        /* Counter is in error state - see perf_event_open man page under "pinned" */
        PyErr_SetString(PyExc_ValueError, "counter error - event is in error state");
        return 0;
    } else if (errno == ENOSPC) {
        fprintf(stderr, "perf_events: event reading buffer too small\n"); 
        PyErr_SetString(PyExc_ValueError, "counter error - buffer not big enough");
        return 0;
    } else {
        fprintf(stderr, "perf_events: unexpected error, errno=%u\n", errno);
        PyErr_SetString(PyExc_ValueError, "counter error");
        return 0;
    }
}


static inline void barrier(void)
{
#if defined(__arm64__) || defined(__AARCH64EL__) || defined(__ARM_ARCH_ISA_A64) || defined(__ARMEL__)
    __asm__ __volatile__("isb":::);
#elif defined(__x86_64__)
    __asm__ __volatile__("sfence":::);
#endif
}


/*
 * Read the hardware timestamp.
 * Note that 0 is a valid (if very unlikely) value of a working timestamp.
 */
static inline unsigned long long hardware_timestamp(void)
{
    unsigned long long cyc = 0;
#if defined(__arm64__) || defined(__AARCH64EL__) || defined(__ARM_ARCH_ISA_A64)
    __asm__ __volatile__("mrs %0,cntvct_el0":"=r"(cyc)::);
#elif defined(__ARMEL__)
    unsigned int hi, lo;
    __asm__ __volatile__("mrrc p15,1,%0,%1,c14":"=r"(lo),"=r"(hi)::);
    cyc = ((unsigned long long)hi << 32) | lo;
#elif defined(__x86_64__)
    unsigned int hi, lo;
    __asm__ __volatile__("rdtsc":"=a"(lo),"=d"(hi));
    cyc = ((unsigned long long)hi << 32) | lo;
#else
    cyc = 0xBADBAD;
#endif
    return cyc;
}


/*
 * Frequency of the hardware timestamp (not the core clock frequency).
 * This could be derived from the time conversion parameters in the
 * mmap header, but we may also be able to get it directly.
 */
static inline unsigned long long hardware_timestamp_frequency(void)
{
    unsigned long long freq = 0;
#if defined(__arm64__) || defined(__AARCH64EL__) || defined(__ARM_ARCH_ISA_A64)
    __asm__ __volatile__("mrs %0,cntfrq_el0":"=r"(freq)::);
#elif defined(__ARMEL__)
    unsigned int lo;    /* In AArch32 we only see a 32-bit CNTFRQ */
    __asm__ __volatile__("mrc p15,0,%0,c14,c0,0":"=r"(lo)::);
    freq = lo;
#else
    // TBD
#endif
    return freq;
}


/*
 * Read a hardware PMU register, given the register selector.
 *
 * For general events this will be a small integer from 0 upwards.
 * In the mmap header, the number is offset by 1 (because 0 indicates the
 * event can't be read from a counter). We are called with the actual
 * counter number i.e. 0 means counter #0.
 *
 * For x86, fixed-function register selectors are:
 *   40000000: IA32_FIXED_CTR0  INST_RETIRED.ANY
 *   40000001: IA32_FIXED_CTR1  CPU_CLK_UNHALTED.THREAD
 *   40000002: IA32_FIXED_CTR2  CPU_CLK_UNHALTED.REF_TSC
 *   40000003: IA32_FIXED_CTR3  TOPDOWN.SLOTS
 */
static inline unsigned long long rdpmc(unsigned int idx)
{
    unsigned long long count = 0;
#if defined(__arm64__) || defined(__AARCH64EL__) || defined(__ARM_ARCH_ISA_A64)
    switch (idx) {
    /* ARM PMU architecture defines up to 31 counter registers */
#define REV(idx) case (idx): __asm__ __volatile__("mrs %0,pmevcntr" #idx "_el0":"=r"(count)); break
    REV(0);
    REV(1);
    REV(2);
    REV(3);
    REV(4);
    REV(5);
    REV(6);
    REV(7);
    REV(8);
    REV(9);
    REV(10);
    REV(11);
    REV(12);
    REV(13);
    REV(14);
    REV(15);
    REV(16);
    REV(17);
    REV(18);
    REV(19);
    REV(20);
    REV(21);
    REV(22);
    REV(23);
    REV(24);
    REV(25);
    REV(26);
    REV(27);
    REV(28);
    REV(29);
    REV(30);
#undef REV
    case 31:
        /* Counter 31 is the dedicated cycle counter */
        __asm__ __volatile__("mrs %0,pmccntr_el0":"=r"(count));
        break;
    default:
        assert(0);
    } 
#elif defined(__x86_64__)
    unsigned int hi, lo;
    __asm__ __volatile__("rdpmc":"=a"(lo),"=d"(hi):"c"(idx));
    count = ((unsigned long long)hi << 32) | lo;
#endif
    return count;
}


/*
 * Try to read the current event value from userspace, into an event_sample_t.
 * Return 1 if successful, 0 if unsuccessful.
 *
 * This only makes sense if we're monitoring either
 *  - our own thread, and nothing else
 *  - this core, and nothing else
 * It probably doesn't make sense for grouped objects either, since we
 * can't guarantee to read all counters in a group without a schedule.
 */
static int perf_read_count_userspace(event_sample_t *ed, EventObject *e)
{
    if (!e->mmap_page) {
        fprintf(stderr, "perf event: read_count_userspace called with no buffer\n");
        ensure_minimal_mmap_page(e);
    }
    struct perf_event_mmap_page volatile *mp = e->mmap_page;
    /* First test to see if this event is readable from userspace.
       We assume the 'capabilities' don't change during the lifetime of the event. */
    const unsigned int caps_needed = _cap_user_rdpmc|_cap_user_time;
    if ((mp->capabilities & caps_needed) != caps_needed) {
        /* Can't read PMU from userspace */
        return 0;
    }
    unsigned int seq;
    unsigned long long enabled, running;
    unsigned int time_mult, time_shift;
    unsigned long long cyc, time_offset;
    unsigned int idx, width=0;
    unsigned long long count_offset, count_value=0;
    do {
        seq = mp->lock;
        barrier();
        enabled = mp->time_enabled;
        running = mp->time_running;
        if (1 /* mp->capabilities & _cap_user_time */) {
            cyc = hardware_timestamp();
            time_offset = mp->time_offset;
            time_mult = mp->time_mult;
            time_shift = mp->time_shift;
        }
        count_offset = mp->offset;
        idx = mp->index;
        if ((mp->capabilities & _cap_user_rdpmc) && idx != 0) {    /* cap_user_rdpmc */
            /* This event is currently scheduled on to a hardware counter. */
            width = mp->pmc_width;
            count_value = rdpmc(idx - 1);
        }
        barrier();
    } while (mp->lock != seq);
    if (0) {
        fprintf(stderr, "-- [%u] read_count_userspace, caps=0x%x:\n", e->fd, (unsigned int)mp->capabilities);
        if (mp->capabilities & _cap_user_time) {
            fprintf(stderr, "-- cyc=0x%llx time_offset=0x%llx 0x%llx time_mult=%u/0x%x time_shift=%u\n",
                cyc, time_offset, cyc-time_offset, time_mult, time_mult, time_shift);
        }
        if ((mp->capabilities & _cap_user_rdpmc) && idx != 0) {
            fprintf(stderr, "-- idx=%u/0x%x width=%u count_offset=0x%llx count_value=0x%llx adj-count=0x%llx\n",
                idx, idx, width, count_offset, count_value, count_offset+count_value);
        }
        fprintf(stderr, "-- enabled=%llx running=%llx\n", enabled, running);
    }
    if (idx != 0) {
        /* The hardware counter value needs to be sign-extended before use. */
        count_value = (signed long long)(count_value << (64-width)) >> (64-width);
        count_value += count_offset;
    } else {
        count_value = count_offset;
    }
    {
        unsigned long long quot, rem, delta;
        quot = (cyc >> time_shift);
        rem = cyc & ((1ULL << time_shift) - 1);
        delta = time_offset + quot*time_mult + ((rem*time_mult) >> time_shift);
        enabled += delta;
        if (idx != 0) {
            running += delta;
        }
    }
    ed->value = count_value;
    ed->time_enabled = enabled;    /* should add current time to this? */
    ed->time_running = running;    /* ditto */
    ed->id = e->id;
    if (0) {
        fprintf(stderr, "-- value in mmap:   ");
        fprint_event_sample(stderr, ed);
    }
    return 1;
}


/*
 * Read a counter event's value(s).
 * Use userspace if available, else use read().
 */
static int perf_read_count(PyObject *x)
{
    int ok;
    BaseReadingObject *base = (BaseReadingObject *)x;
    EventObject *e = base->event;
    if (e->try_userspace_read) {
        ok = perf_read_count_userspace(&base->sample, e);
        if (ok) {
            if (0) {
                /* Consistency check against read() values */
                event_sample_t *ed = &base->sample;
                event_sample_t edr = *ed;
                perf_read_count_using_read(x);
                if (ed->value < edr.value) {
                    fprintf(stderr, "[%u] mmap: ", e->fd);
                    fprint_event_sample(stderr, ed);
                    fprintf(stderr, "[%u] read: ", e->fd);
                    fprint_event_sample(stderr, &edr);
                }
                assert(ed->value >= edr.value);
            }
            postprocess_reading(x);
            return ok;
        }
    }
    ok = !!perf_read_count_using_read(x);
    if (e->datasnap != NULL) {
        /* Subtract event data from the baseline. */
        subtract_event_values(x, e->datasnap, e);
    }
    postprocess_reading(x);
    return ok;
}


/*
Reset an event's value to zero - at least as far as the API is concerned.

About PERF_EVENT_IOC_RESET:
"This resets only the counts; there is no way to reset the multiplexing
time_enabled or time_running values."

So instead, we snapshot the counter values, and present future values as
a delta against that.
*/
static PyObject *event_read(PyObject *x);
static PyObject *event_reset(PyObject *x)
{
    EventObject *e = (EventObject *)x;
    if (0) {
        int rc = ioctl(e->fd, PERF_EVENT_IOC_RESET, 0);
        if (rc != 0) {
            PyErr_SetString(PyExc_ValueError, "bad ioctl(RESET)");
            return NULL;
        }
        return PyInt_FromLong(rc);
    } else {
        if (e->datasnap) {
            Py_DECREF(e->datasnap);
            e->datasnap = NULL;
        }
        e->datasnap = event_read(x);
        //fprintf(stderr, "-- [%u] snapshot %llx\n", e->fd, e->datasnap.value);
        return PyInt_FromLong(0);  /* success */
    }
}


static PyObject *event_refresh(PyObject *x, PyObject *arg)
{
    EventObject *e = (EventObject *)x;
    int rc;
    int n = PyLong_AsLong(arg);
    rc = ioctl(e->fd, PERF_EVENT_IOC_REFRESH, n);
    if (rc < 0) {
        return NULL;
    }
    return PyInt_FromLong(rc);
}


static PyObject *event_pause(PyObject *x)
{
    EventObject *e = (EventObject *)x;
    int rc = ioctl(e->fd, PERF_EVENT_IOC_PAUSE_OUTPUT, 1);
    return PyInt_FromLong(rc);
}


static PyObject *event_resume(PyObject *x)
{
    EventObject *e = (EventObject *)x;
    int rc = ioctl(e->fd, PERF_EVENT_IOC_PAUSE_OUTPUT, 0);
    return PyInt_FromLong(rc);
}


/*
 * fileno() method - as for Python file objects.
 * This means the Event object can be used directly in e.g. select.poll.register().
 */
static PyObject *event_fileno(PyObject *x)
{
    EventObject *e = (EventObject *)x;
    return PyInt_FromLong(e->fd);
}


static PyObject *event_enable(PyObject *x)
{
    EventObject *e = (EventObject *)x;
    int rc = ioctl(e->fd, PERF_EVENT_IOC_ENABLE, 0);
    if (rc != 0) {
        PyErr_SetString(PyExc_ValueError, "bad ioctl(ENABLE)");
        return NULL;
    }
    e->attr.disabled = 0;
    return PyInt_FromLong(rc);
}


static PyObject *event_disable(PyObject *x)
{
    EventObject *e = (EventObject *)x;
    int rc = ioctl(e->fd, PERF_EVENT_IOC_DISABLE, 0);
    if (rc != 0) {
        PyErr_SetString(PyExc_ValueError, "bad ioctl(DISABLE)");
        return NULL;
    }
    e->attr.disabled = 1;
    return PyInt_FromLong(rc);
}


/*
 * Pass a filter to the kernel. See kernel/events/core.c for syntax.
 * Address filters for hardware trace have this syntax:
 *
 *   filter  <range>    - limit tracing to a range
 *   start   <range>    - start when execution enters this range
 *   stop    <range>    - stop when execution enters this range
 *
 * <range> is
 *
 *   <addr>[/<size>]         - kernel address
 *   <addr>[/<size>]@object  - address in module
 *
 * "perf record" accepts a more general syntax that can resolve
 * symbol names: see tools/perf/Documentation/perf-record.txt
 */
static PyObject *event_set_filter(PyObject *x, PyObject *s)
{
    EventObject *e = (EventObject *)x;
    char const *str = PyString_AsString(s);
    if (str) {
        if (e->verbose) {
            fprintf(stderr, "[%d]: set filter \"%s\"\n", e->fd, str);
        }
        int rc = ioctl(e->fd, PERF_EVENT_IOC_SET_FILTER, str);
        if (rc != 0) {
            if (e->verbose) {
                perror("ioctl");
            }
            PyErr_SetString(PyExc_ValueError, "bad ioctl(SET_FILTER)");
            return NULL;
        } else {
            /* filter was set successfully: return self */
            Py_INCREF(x);
            return x;
        }
    } else {
        PyErr_SetString(PyExc_ValueError, "expected string");
        return NULL;
    }
}


static int event_get_id(EventObject *e)
{
    if (e->id == 0) {
        int rc = ioctl(e->fd, PERF_EVENT_IOC_ID, &e->id);
        if (rc != 0) {
            return 0;
        }
    }
    return 1;
}


static PyObject *event_id(PyObject *x)
{
    EventObject *e = (EventObject *)x;
    if (!event_get_id(e)) {
        PyErr_SetString(PyExc_ValueError, "bad ioctl");
        return NULL;
    }
    return PyLong_FromUnsignedLongLong(e->id);
}


static PyObject *basereading_new(PyTypeObject *ot, PyObject *args, PyObject *kwds)
{
    assert(0);
    return NULL;
}

static PyObject *reading_new(PyTypeObject *ot, PyObject *args, PyObject *kwds)
{
    ReadingObject *t = (ReadingObject *)ot->tp_alloc(ot, 0);
    assert(t->base.event == NULL);
    //fprintf(stderr, "-- reading %p alloc\n", t);
    return (PyObject *)t;
}


static PyObject *groupreading_new(PyTypeObject *ot, PyObject *args, PyObject *kwds)
{
    GroupReadingObject *t = (GroupReadingObject *)ot->tp_alloc(ot, 0);
    assert(t->base.event == NULL);
    t->samples = NULL;
    return (PyObject *)t;
}


static void basereading_dealloc(PyObject *x)
{
    assert(0);
}

static void reading_dealloc(PyObject *x)
{
    ReadingObject *r = (ReadingObject *)x;
    //fprintf(stderr, "-- reading %p dealloc\n", r);
    Py_DECREF(r->base.event);
    r->base.event = NULL;
    if (r->adjusted_value != Py_None) {
        Py_DECREF(r->adjusted_value);
    }
    x->ob_type->tp_free(x);
}

static void groupreading_dealloc(PyObject *x)
{
    GroupReadingObject *r = (GroupReadingObject *)x;
    Py_DECREF(r->base.event);
    free(r->samples);
    x->ob_type->tp_free(x);
}


/*
 * A reading is missing if the task was never scheduled, or if it was
 * scheduled but the event was never assigned to a counter.
 */
static PyObject *reading_is_missing(PyObject *x)
{
    BaseReadingObject *r = (BaseReadingObject *)x;
    return PyBool_FromLong(r->sample.time_running == 0);
}

/*
 * A reading is incomplete if the task was scheduled but the event
 * was not always assigned to a counter. This includes both the case
 * where the event was sometimes assigned to a counter (and 'value'
 * will be scaled) and when it was never assigned to a counter
 * (and 'value' is None).
 */
static PyObject *reading_is_incomplete(PyObject *x)
{
    BaseReadingObject *r = (BaseReadingObject *)x;
    assert(r->sample.time_running <= r->sample.time_enabled);
    return PyBool_FromLong(r->sample.time_running != r->sample.time_enabled);
}


static PyObject *reading_str(PyObject *x)
{
    ReadingObject *r = (ReadingObject *)x;
    if (!r->base.sample.time_enabled) {
        return PyString_FromFormat("<not counted>");
    } else if (r->base.fraction_running == 1.0) {
        return PyString_FromFormat("%llu", r->base.sample.value);
    } else {
        /* PyString_FromFormat doesn't support floats, so we print
           the raw enabled/running values */         
        char adj_value_string[100];
        if (r->adjusted_value != Py_None) {
            sprintf(adj_value_string, "adjusted value %f", PyFloat_AsDouble(r->adjusted_value));
        } else {
            strncpy(adj_value_string, "no value", 10);
        }
        return PyString_FromFormat("%llu (running %llu enabled %llu, %s)",
            r->base.sample.value, r->base.sample.time_running, r->base.sample.time_enabled, adj_value_string);
    }
}


static PyObject *groupreading_str(PyObject *x)
{
    GroupReadingObject *r = (GroupReadingObject *)x;
    if (!r->base.sample.time_enabled) {
        return PyString_FromFormat("<not counted>");
    } else {
        return PyString_FromFormat("GroupReading(%u values)", r->n_values);
    }
}


static Py_ssize_t groupreading_seq_length(PyObject *x)
{
    GroupReadingObject *r = (GroupReadingObject *)x;
    return r->n_values;
}


static ReadingObject *create_reading_object(EventObject *e);

static PyObject *groupreading_seq_item(PyObject *x, Py_ssize_t i)
{
    GroupReadingObject *r = (GroupReadingObject *)x;
    ReadingObject *nr;
    if (i < 0 || i >= r->n_values) {
        return NULL;
    }
    nr = create_reading_object(r->base.event);
    nr->base.sample = r->samples[i];
    postprocess_value(&nr->adjusted_value, nr->base.sample.value, &r->base); 
    return (PyObject *)nr;
}


/*
 * Create a ReadingObject and bind it to an event.
 */
static ReadingObject *create_reading_object(EventObject *e)
{
    ReadingObject *r = (ReadingObject *)PyObject_CallObject((PyObject *)&ReadingType, NULL);
    assert(e != NULL);
    Py_INCREF(e);
    r->base.event = e;
    return r;
}


/*
 * Create a GroupReading object, but don't yet create the array of samples.
 */
static GroupReadingObject *create_group_reading_object(EventObject *e)
{
    GroupReadingObject *r = (GroupReadingObject *)PyObject_CallObject((PyObject *)&GroupReadingType, NULL);
    assert(e != NULL);
    Py_INCREF(e);
    r->base.event = e;
    return r;
}


static PyObject *create_correct_reading_object(EventObject *e)
{
    return (e->attr.read_format & PERF_FORMAT_GROUP) ?
        (PyObject *)create_group_reading_object(e) :
        (PyObject *)create_reading_object(e);
}


/*
 * Create a Reading object from a 'struct read_format', either read from
 * the file descriptor (perf stat) or from a PERF_SAMPLE_READ (perf record).
 * Fill in the 'adjusted_value' field with a Python object to represent
 * the value adjusted for the event's running-fraction - None if the event
 * was never running.
 */
#if 0
static void perf_update_reading(ReadingObject *, event_sample_t const *);
static ReadingObject *reading_from_data(event_sample_t const *ed, EventObject const *e, ReadingObject *r)
{
    assert(!(e->attr.read_format & PERF_FORMAT_GROUP));
    EventObject *re = (EventObject *)e;
    if (ed->id != 0 && ed->id != e->id) {
        /* A sampling event might have been written to its parent's buffer.
           Look up the actual event. */
        re = event_find_subordinate(e, ed->id);
        if (!re) {
            fprintf(stderr, "perf event: unexpected event id: 0x%llX\n", ed->id);
            assert(0);
        }
    }
    if (!r) {
        r = create_reading_object(re);
    }
    assert(r->base.event == e);
    perf_update_reading(r, ed);
    return r;
}
#endif


#if 0
static void perf_update_reading(ReadingObject *r, event_sample_t const *ed)
{
    EventObject const *e = r->base.event;
    //fprintf(stderr, "-- [%u] org value =%llx  datasnap = %llx\n", e->fd, ed->value, e->datasnap.value);
    /* Because we implement event_reset() by taking a snapshot, we now recalulate
       all the event metrics as a delta against that snapshot. */
    assert(ed->value >= e->datasnap.value);
    r->base.sample.value = ed->value - e->datasnap.value;
    r->base.sample.time_enabled = ed->time_enabled - e->datasnap.time_enabled;
    r->base.sample.time_running = ed->time_running - e->datasnap.time_running;
    if (r->base.sample.time_enabled != 0.0) {
        r->base.fraction_running = (double)r->base.sample.time_running / r->base.sample.time_enabled;
        if (r->base.fraction_running != 0.0) {
            /* Increase the value to compensate for partial sampling */
            if (r->adjusted_value && r->adjusted_value != Py_None) {
                Py_DECREF(r->adjusted_value);
            }
            r->adjusted_value = PyFloat_FromDouble(r->base.sample.value / r->base.fraction_running);
        } else {
            r->adjusted_value = Py_None;
        }
    } else {
        r->base.fraction_running = 0.0;
        r->adjusted_value = Py_None;
    }
    if (e->verbose) {
        double adjv = (r->base.fraction_running ? (r->base.sample.value / r->base.fraction_running) : 0.0);
        fprintf(stderr, "perf event %p: value=%llu enabled=%llu running=%llu=%.5f adjusted=%.2f\n",
            e,
            (unsigned long long)r->base.sample.value,
            (unsigned long long)r->base.sample.time_enabled,
            (unsigned long long)r->base.sample.time_running,
            r->base.fraction_running,
            adjv);
    }        
    Py_INCREF(r->adjusted_value);
}
#endif


static unsigned long long const *read_data_to_sample(event_sample_t *ed, unsigned long long const *p, EventObject const *e)
{
    if (e->attr.read_format & PERF_FORMAT_TOTAL_TIME_ENABLED) {
        ed->time_enabled = *p++;
    } else {
        ed->time_enabled = 0xCCCCCCCC;
    }
    if (e->attr.read_format & PERF_FORMAT_TOTAL_TIME_RUNNING) {
        ed->time_running = *p++;
    } else {
        ed->time_running = 0xCCCCCCCC;
    }
    return p;
}


/*
 * Populate a Reading or GroupReading object from a data buffer either read from read()
 * or in a record in the mmap buffer.
 */
static PyObject *populate_reading_object_from_data(PyObject *x, void const *data, EventObject *e)
{
    unsigned long long const *p = (unsigned long long const *)data;
    if (!(e->attr.read_format & PERF_FORMAT_GROUP)) {
        ReadingObject *r = (ReadingObject *)x;
        event_sample_t *ed = &r->base.sample;
        ed->value = *p++;
        p = read_data_to_sample(ed, p, e);
        if (e->attr.read_format & PERF_FORMAT_ID) {
            ed->id = *p++;
        } else {
            ed->id = 0xCCCCCCCC;
        }
    } else {
        GroupReadingObject *g = (GroupReadingObject *)x;
        event_sample_t *ed = &g->base.sample;
        g->n_values = (unsigned int)*p++;
        unsigned int i;
        p = read_data_to_sample(ed, p, e);
        g->samples = (event_sample_t *)malloc(g->n_values * sizeof(event_sample_t));
        for (i = 0; i < g->n_values; ++i) {
            event_sample_t *sed = &g->samples[i];   /* array entry to write into */
            *sed = *ed;
            sed->value = *p++;
            if (e->attr.read_format & PERF_FORMAT_ID) {
                sed->id = *p++;
            } else {
                sed->id = 0xCCCCCCCC;
            }
        }
    }
    return x;
}


/*
 * Create a Reading or GroupReading object from data.
 */
static PyObject *create_reading_object_from_data(void const *data, EventObject *e)
{
    PyObject *x = create_correct_reading_object(e);
    x = populate_reading_object_from_data(x, data, e);
    return x;
}


/*
Read the current value of an event counter, as a Reading object.
We hope it hasn't wrapped. A Reading object may be passed in - its data
will be discarded.

We only expect to call this on a counting event, not a sampling event.
*/
static PyObject *take_reading(EventObject *e, PyObject *r)
{
    if (!r) {
        r = create_correct_reading_object(e);
    }
    if (perf_read_count(r)) {
        return (PyObject *)r;
    } else {
        return NULL;
    }
}


/*
 * Return a new Reading or GroupReading object
 */
static PyObject *event_read(PyObject *x)
{
    EventObject *e = (EventObject *)x;
    PyObject *r = take_reading(e, NULL);
    //Py_INCREF(r);
    return r;
}


/*
 * Update a Reading object
 */
static PyObject *reading_update(PyObject *x)
{
    ReadingObject *r = (ReadingObject *)x;
    //fprintf(stderr, "reading %p event %p\n", r, r->event);
    assert(r->base.event != NULL);
    PyObject *rn = take_reading(r->base.event, x);
    /* Returning 'self' creates a new reference, so we must increment the
       reference count to avoid being discarded. */
    Py_INCREF(rn);
    return rn;
}


/*
 * The __str__ should be human-readable.
 */
static PyObject *event_str(PyObject *x)
{
    EventObject const *e = (EventObject *)x;
    return PyString_FromFormat("event[%d](type=%u,n=0x%x,cpu=%d)[%sabled]",
        e->fd, (unsigned int)e->attr.type, (unsigned int)e->attr.config, (int)e->cpu, (e->attr.disabled ? "dis" : "en"));
}


/*
 * Python object corresponding to a sampled event or other record - as acquired
 * from the memory mapped buffer.  This object is logically read-only
 * once acquired.
 */
typedef struct {
    PyObject_HEAD
    EventObject *event;             /* back-pointer to the sampling event we got this from */
    unsigned long offset;           /* original offset of this record in the buffer */
    size_t data_size;               /* total size of the collected data */
    struct perf_event_header head;  /* includes type, size etc. */
    void *data;                     /* variable amount of data, specific to record type: includes header */
    PyObject *aux;                  /* for PERF_RECORD_AUX: AUX buffer segment corresponding to this record */
} RecordObject;


/*
 * Return the previously collected record payload as a binary string object.
 * This includes:
 *   the standard record header (perf_event_header)
 *   the payload
 *   the 'struct sample_id' possibly present at the end of the payload
 * This matches the record that gets written to perf.data.
 *
 * N.b. this means we copy twice: once out of perf's mmap ring buffer into the
 * Record's malloc'ed buffer, and then again when we return the string object.
 * We could do better by creating a string to start with.
 */
static PyObject *record_data(PyObject *x)
{
    RecordObject *s = (RecordObject *)x;
    return MyBytes_FromStringAndSize(s->data, s->data_size);
}


static PyObject *record_new(PyTypeObject *t, PyObject *args, PyObject *kwds)
{
    RecordObject *s = (RecordObject *)t->tp_alloc(t, 0);
    assert(s != NULL);
    assert(s->event == NULL);
    s->data = NULL;
    s->aux = Py_None;
    Py_INCREF(s->aux);
    return (PyObject *)s;
}


static char const *record_type_name(unsigned int type)
{
    unsigned int i;
    char const *typestr = NULL;
    static struct {
        char const *name;
        unsigned int type;
    } const typestrs[] = {
#define TYPE(x) { #x, PERF_RECORD_##x }
        TYPE(MMAP),
        TYPE(LOST),
        TYPE(COMM),
        TYPE(EXIT),
        TYPE(THROTTLE),
        TYPE(UNTHROTTLE),
        TYPE(FORK),
        TYPE(READ),
        TYPE(SAMPLE),
        TYPE(MMAP2),
#ifdef PERF_RECORD_MMAP
        /* Older kernels have these as macros, so we can test each one. */
#ifdef PERF_RECORD_AUX
        TYPE(AUX),
#endif /* PERF_RECORD_AUX */
#ifdef PERF_RECORD_ITRACE_START
        TYPE(ITRACE_START),
#endif /* PERF_RECORD_ITRACE_START */
#ifdef PERF_RECORD_LOST_SAMPLES
        TYPE(LOST_SAMPLES),
#endif /* PERF_RECORD_LOST_SAMPLES */
#ifdef PERF_RECORD_SWITCH
        TYPE(SWITCH),
#endif /* PERF_RECORD_SWITCH */
#ifdef PERF_RECORD_SWITCH_CPU_WIDE
        TYPE(SWITCH_CPU_WIDE)
#endif /* PERF_RECORD_SWITCH_CPU_WIDE */
#else
        /* PERF_RECORD_MMAP not being #define'd, indicates we're now using enumerators.
           So we have to hope that we have the full set. */
        TYPE(AUX),
        TYPE(ITRACE_START),
        TYPE(LOST_SAMPLES),
        TYPE(SWITCH),
        TYPE(SWITCH_CPU_WIDE)
#endif
#undef TYPE            
    };
    for (i = 0; i < sizeof(typestrs)/sizeof(typestrs[0]); ++i) {
        if (type == typestrs[i].type) {
            typestr = typestrs[i].name;
            break;
        }
    }
    return typestr;
}




/*
 * Create a Reading or GroupReading object from data in a record.
 */
static PyObject *record_reading(PyObject *x)
{
    RecordObject const *s = (RecordObject *)x;
    if (s->head.type == PERF_RECORD_SAMPLE &&
        s->event->attr.sample_type & PERF_SAMPLE_READ) {
        /* Find the read object in the sample payload */
        unsigned int const offs = sample_offset_to_read(&s->event->attr);
        return create_reading_object_from_data((unsigned char const *)s->data + offs, s->event);
    } else {
        /* Presence/absence of a read object is a property of the sample type and configuration.
           It's reasonable to throw an exception if we ask for one invalidly. */
        PyErr_SetString(PyExc_ValueError, "record does not contain a counter reading");
        return NULL;
    }
}


static PyObject *record_is_sample(PyObject *x)
{
    RecordObject const *s = (RecordObject *)x;
    return PyBool_FromLong(s->head.type == PERF_RECORD_SAMPLE);
}


static PyObject *record_str(PyObject *x)
{
    RecordObject const *s = (RecordObject *)x;
    char const *typestr = record_type_name(s->head.type);
    if (!typestr) {
        typestr = "?";
    }
    return PyString_FromFormat("Record(%s,databytes=%u)",
        typestr, (unsigned int)s->data_size);
}


static void record_dealloc(PyObject *x)
{
    RecordObject *s = (RecordObject *)x;
    free(s->data);
    Py_DECREF(s->event);
    Py_DECREF(s->aux);
    x->ob_type->tp_free(x);
}


static struct PyMethodDef Record_methods[] = {
    {"data", (PyCFunction)&record_data, METH_NOARGS, "string: raw data"},
    {"reading", (PyCFunction)&record_reading, METH_NOARGS, "Reading: event value reading"},
    {"is_sample", (PyCFunction)&record_is_sample, METH_NOARGS, "bool: record is a sample"},
    {"__bytes__", (PyCFunction)&record_data, METH_NOARGS, "string: raw data"}, 
    {NULL}
};

static struct PyMemberDef Record_members[] = {
    {"event", T_OBJECT, offsetof(RecordObject, event), 0, "event that record was read from"},
    {"offset", T_LONG, offsetof(RecordObject, offset), 0, "byte offset in mmap buffer"},
    {"type", T_INT, offsetof(RecordObject, head.type), 0, "record type"},
    {"misc", T_SHORT, offsetof(RecordObject, head.misc), 0, "record mode and flags"},
    {"size", T_SHORT, offsetof(RecordObject, head.size), 0, "record total size (including header)"},
    {"aux", T_OBJECT, offsetof(RecordObject, aux), 0, "AUX buffer contents"},
    {NULL}
};


static PyTypeObject RecordType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_basicsize = sizeof(RecordObject),
    .tp_name = "perf_events.Record",
    .tp_doc = "perf event",
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = Record_methods,
    .tp_members = Record_members,
//    tp_repr: sample_repr,
    .tp_str = record_str,
    .tp_alloc = 0,
    .tp_new = record_new,
//    tp_init: sample_init,
    .tp_dealloc = record_dealloc
};


/*
 * Copy 'len' bytes from a buffer which may have wrapped.
 * 'offset' is the non-wrapping offset into the buffer - continually increasing
 * (as for perf buffers) which must be wrapped into a real offset before use.
 */
void copy_from_wrapped_buffer(void *dst, void const *vbuf, size_t buf_size, size_t virtual_offset, size_t len)
{
    unsigned int real_offset = virtual_offset % buf_size;
    unsigned int fs = buf_size - real_offset;
    assert(len <= buf_size);
    if (fs > len) {
        fs = len;
    }
    /* fs is now the first part of the data to copy (all of it, unless wrapped) */
    memcpy(dst, (unsigned char const *)vbuf + real_offset, fs);
    if (fs < len) {
        /* data wrapped, copy remainder from the beginning */
        memcpy((unsigned char *)dst + fs, vbuf, len - fs);
    }
}


#if defined(PERF_RECORD_AUX) || !defined(PERF_RECORD_MMAP)
/*
 * There are two ways we could retrieve data from the AUX buffer:
 *
 *  - we can directly poll and update the head/tail pointers like we do for the
 *    main mmap ring buffer
 *
 *  - we can use the (offset,size) information that we are given in PERF_RECORD_AUX
 *    records that we get from the main mmap ring buffer. We would expect these
 *    notifications to occur in order, so that the oldest such outstanding notification
 *    has the data at the tail pointer.
 */

/* Manage the AUX buffer tail - factored out in case we use a read-only AUX buffer */

static unsigned long event_aux_tail(EventObject *e)
{
    return e->mmap_page->aux_tail;
}


static void event_update_aux_tail(EventObject *e, unsigned long na)
{
    if (e->verbose) {
        fprintf(stderr, "updating tail from 0x%llx to 0x%lx; head at 0x%llx\n", e->mmap_page->aux_tail, na, e->mmap_page->aux_head);
    }
    assert(na <= e->mmap_page->aux_head);   /* tail must not advance past head */
    e->mmap_page->aux_tail = na;
    __sync_synchronize();
}
#endif /* PERF_RECORD_AUX */


/*
 * Get a chunk of AUX data, of a given length.
 * This consumes data directly from the mmap'ed AUX buffer.
 * Normally it's better to have the data from the AUX buffer consumed by
 * reading the corresponding PERF_RECORD_AUX from the main mmap buffer.
 */
static PyObject *get_aux_data(EventObject *e, unsigned int len)
{
    PyObject *s;
    if (!e->mmap_page || !e->aux_area) {
        Py_RETURN_NONE;
    }
    if (len == 0) {
        Py_RETURN_NONE;
    }
    /* Get the AUX data. Python lets us construct a buffer object of the required
       length and then write directly into it. */
    s = MyBytes_FromStringAndSize(NULL, len);
    copy_from_wrapped_buffer(MyBytes_AsString(s), e->aux_area, e->aux_size, event_aux_tail(e), len);
    event_update_aux_tail(e, e->mmap_page->aux_tail + len);
    return s;
}


/*
 * Get as big a chunk of AUX data as we can.
 */
static PyObject *event_get_aux(PyObject *x)
{
#if defined(PERF_RECORD_AUX) || !defined(PERF_RECORD_MMAP)
    EventObject *e = (EventObject *)x;
    /* Make sure to read aux_head just once, in case it moves on while we're consuming the tail */
    unsigned int len = e->mmap_page->aux_head - e->mmap_page->aux_tail;
    return get_aux_data(e, len);
#else /* !PERF_RECORD_AUX */
    Py_RETURN_NONE;
#endif /* PERF_RECORD_AUX */
}


/*
 * Collect one record from the mmap buffer - return a Record event if
 * an event is available, otherwise return None.
 * Event source must already have been set up for sampling, e.g. by calling poll().
 */
static PyObject *event_get_record(PyObject *x)
{
    EventObject *e = (EventObject *)x;
    if (!e->mmap_page) {
        /* Never going to return an event until we set up the buffer -
           should we raise an exception? */
        Py_RETURN_NONE;
    }        
    if (!event_available(e)) {
        Py_RETURN_NONE;
    }
    /* A sample is available. Construct a new Record object. */
    struct perf_event_header head;
    /* Populate the sample object from the mmap'ed sample buffer.
       This is slightly tricky as the buffer may wrap around in the
       sample - in the header, between header and data or in the data. */
    copy_from_wrapped_buffer(&head, e->mmap_data_start, e->mmap_data_size, e->mmap_page->data_tail, sizeof(struct perf_event_header));
#ifdef PRINTF_DIAGNOSTICS
    if (e->verbose) {
        char const *typestr = record_type_name(head.type);
        if (!typestr) {
            typestr = "?";
        }
        fprintf(stderr, "%u: event record available: total record size: %u type: %u (%s); samples expected: ",
            e->fd, head.size, head.type, typestr);
        fprint_flags(stderr, e->attr.sample_type, FLAG_NAMES(sample_flagnames));
        fprintf(stderr, "\n");
    }
#endif /* PRINTF_DIAGNOSTICS */
    if (head.size < sizeof(struct perf_event_header) ||
        head.size > e->mmap_data_size) {     
        /* Corrupt */
        fprintf(stderr, "sample corrupt: length = %ld\n", (long)head.size);
        return NULL;
    }   
    /* We could make Record flexibly sized, but for now we have a separate buffer */
    RecordObject *s = (RecordObject *)PyObject_CallObject((PyObject *)&RecordType, NULL);
    s->offset = e->mmap_page->data_tail;
    s->head = head;    /* Copy the (small) head structure */
    s->data_size = head.size;
    s->data = malloc(s->data_size);
    /* Copy the complete record, including the header (again) */
    copy_from_wrapped_buffer(s->data, e->mmap_data_start, e->mmap_data_size, e->mmap_page->data_tail, s->data_size);
    /* data is in s->data in the Record object */
    /* Now update the read pointer in the shared metadata */
    if (1) {
        /* Page is writeable - update the tail pointer so kernel can now
           overwrite the record we've just collected. */
        e->mmap_page->data_tail += head.size;
        __sync_synchronize();
    }
    if (e->aux_area) {
        //fprintf(stderr, "aux_head = %llx, aux_tail = %llx\n", e->mmap_page->aux_head, e->mmap_page->aux_tail);
    }
    if (head.type == PERF_RECORD_AUX) {
        /* Get the AUX data now */
        typedef struct {
            struct perf_event_header header;
            unsigned long long aux_offset;
            unsigned long long aux_size;
            unsigned long long flags;
        } aux_header_t;
        aux_header_t const *ah = (aux_header_t const *)s->data;
        if (1 || e->verbose) {
            /* Show this PERF_RECORD_AUX segment in relation to the AUX ring buffer.
               The offset, tail and head pointers are "infinite". */
            fprintf(stderr, "[%d] AUX flags=0x%lx offset 0x%lx size 0x%lx, current AUX tail 0x%lx head 0x%lx size 0x%lx",
                e->fd,
                (unsigned long)ah->flags,
                (unsigned long)ah->aux_offset, (unsigned long)ah->aux_size,
                (unsigned long)e->mmap_page->aux_tail,
                (unsigned long)e->mmap_page->aux_head,
                (unsigned long)e->mmap_page->aux_size);
            if (ah->flags & PERF_AUX_FLAG_TRUNCATED) {
                fprintf(stderr, " TRUNCATED");
            }
            if (ah->flags & PERF_AUX_FLAG_OVERWRITE) {
                fprintf(stderr, " OVERWRITE");
            }
            if (ah->flags & PERF_AUX_FLAG_PARTIAL) {
                fprintf(stderr, " PARTIAL");
            }
            if (ah->flags & PERF_AUX_FLAG_COLLISION) {
                fprintf(stderr, " COLLISION");
            }
            fprintf(stderr, "\n");
        }
        /* Guard in case AUX chunk was already consumed by get_aux() */
        if (ah->aux_size == 0) {
            /* No data - possibly a TRUNCATED indication */
            event_update_aux_tail(e, ah->aux_offset);
        } else if (ah->aux_offset == e->mmap_page->aux_tail) {
            /* This AUX record describes the next segment available in the AUX buffer. */
#ifndef NDEBUG
            unsigned long available = e->mmap_page->aux_head - e->mmap_page->aux_tail;
#endif
            assert(available >= ah->aux_size);
            Py_DECREF(s->aux);    /* it was None */
            s->aux = get_aux_data(e, ah->aux_size);
            Py_INCREF(s->aux);
        } else {
            /* Mismatch */
            fprintf(stderr, "** AUX record mismatch\n");
        }
    }
    /* the Record has a back-pointer to the buffer-owning Event */
    Py_INCREF(e);
    s->event = e;
    return (PyObject *)s;
}


static PyMethodDef Event_methods[] = {
    {"attr_struct", (PyCFunction)&event_attr_struct, METH_NOARGS, "string: event attributes as raw string"},
    {"fileno", (PyCFunction)&event_fileno, METH_NOARGS, "int: file handle - not for general use"},  /* this makes it a "waitable object" */
    {"id", (PyCFunction)&event_id, METH_NOARGS, "int: unique id"},
    {"close", (PyCFunction)&event_close, METH_NOARGS, "close the event"},
    {"capabilities", (PyCFunction)&event_capabilities, METH_NOARGS, "int: get capability flags"},
    {"bind", (PyCFunction)&event_bind, METH_VARARGS|METH_KEYWORDS, "int, int -> start collecting an event"},
    {"set_filter", (PyCFunction)&event_set_filter, METH_O, "str: set filter on event"},
    {"reset", (PyCFunction)&event_reset, METH_NOARGS, "int: reset the event count"},
    {"enable", (PyCFunction)&event_enable, METH_NOARGS, "int: enable the event"},
    {"disable", (PyCFunction)&event_disable, METH_NOARGS, "int: disable the event"},
    {"refresh", (PyCFunction)&event_refresh, METH_O, "int -> refresh the wakeup counter"},
    {"pause", (PyCFunction)&event_pause, METH_NOARGS, "pause a sampling event"},
    {"resume", (PyCFunction)&event_resume, METH_NOARGS, "resume a sampling event"},
    {"read", (PyCFunction)&event_read, METH_NOARGS, "Reading: read the current value of a counting event"},
    {"poll", (PyCFunction)&event_poll, METH_NOARGS, "bool: test if event record is available"},
    {"is_active", (PyCFunction)&event_is_active, METH_NOARGS, "bool: test if event was closed by kernel"},
    {"get_record", (PyCFunction)&event_get_record, METH_NOARGS, "Record: get next record from a sampling event"},
    {"get_aux", (PyCFunction)&event_get_aux, METH_NOARGS, "string: get AUX data"},
    {NULL}
};


/*
 * Need to explicitly include structmember.h for this?
 */
static struct PyMemberDef Event_members[] = {
    {"type", T_INT, offsetof(EventObject, attr.type), 0, "perf event type"},
    {"code", T_INT, offsetof(EventObject, attr.config), 0, "perf event code"},
    {"sample_type", T_ULONGLONG, offsetof(EventObject, attr.sample_type), 0, "mask of data in sample records"},
    {"cpu", T_INT, offsetof(EventObject, cpu), 0, "cpu that this event is bound to, or -1"},
    {"verbose", T_INT, offsetof(EventObject, verbose), 0, "verbosity level"}, 
    {NULL}
};


static PyTypeObject EventType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_basicsize = sizeof(EventObject),
    .tp_name = "perf_events.Event",
    .tp_doc = "perf event",
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = Event_methods,
    .tp_members = Event_members,
    .tp_repr = event_repr,
    .tp_str = event_str,
    .tp_alloc = 0,
    .tp_new = event_new,
    .tp_init = event_init,
    .tp_dealloc = event_dealloc
};


static PyMethodDef BaseReading_methods[] = {
    {"is_incomplete", (PyCFunction)&reading_is_incomplete, METH_NOARGS, "bool: only partial data is available"},
    {"is_missing", (PyCFunction)&reading_is_missing, METH_NOARGS, "bool: no data is available"},
    {"update", (PyCFunction)&reading_update, METH_NOARGS, "take another reading"},
    {NULL}
};


static struct PyMemberDef BaseReading_members[] = {
    {"time_enabled_ns", T_LONGLONG, offsetof(BaseReadingObject, sample.time_enabled), 0, "time enabled (ns)"},
    {"time_running_ns", T_LONGLONG, offsetof(BaseReadingObject, sample.time_running), 0, "time running (ns)"},
    {"fraction_running", T_DOUBLE, offsetof(BaseReadingObject, fraction_running), 0, "float: fraction of time event was running"},
    {"event", T_OBJECT, offsetof(BaseReadingObject, event), 0, "Event object this reading was taken from"},
    {"id", T_LONGLONG, offsetof(ReadingObject, base.sample.id), 0, "event id"},
    {NULL}
};


static PyMethodDef Reading_methods[] = {
    {NULL}
};


static struct PyMemberDef Reading_members[] = {
    {"raw_value", T_LONGLONG, offsetof(ReadingObject, base.sample.value), 0, "raw count"},
    {"value", T_OBJECT, offsetof(ReadingObject, adjusted_value), 0, "adjusted count - None if event was never scheduled"},
    {NULL}
};


static PyMethodDef GroupReading_methods[] = {
    {NULL}
};


static PySequenceMethods GroupReading_seqmethods = {
    .sq_length = &groupreading_seq_length,
    .sq_item = &groupreading_seq_item
};


static struct PyMemberDef GroupReading_members[] = {
    {NULL}
};


static PyTypeObject BaseReadingType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_basicsize = sizeof(BaseReadingObject),
    .tp_name = "perf_events.BaseReading",
    .tp_doc = "Base reading class",
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = BaseReading_methods,
    .tp_members = BaseReading_members,
    //.tp_str = basereading_str,
    .tp_new = basereading_new,   /* should not be called */
    .tp_dealloc = basereading_dealloc
};


static PyTypeObject ReadingType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_base = &BaseReadingType,
    .tp_basicsize = sizeof(ReadingObject),
    .tp_name = "perf_events.Reading",
    .tp_doc = "A reading taken from a counting perf event",
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = Reading_methods,
    .tp_members = Reading_members,
    .tp_str = reading_str,
    .tp_new = reading_new,
    .tp_dealloc = reading_dealloc
};


static PyTypeObject GroupReadingType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_base = &BaseReadingType,
    .tp_basicsize = sizeof(GroupReadingObject),
    .tp_name = "perf_events.GroupReading",
    .tp_doc = "A reading taken from a counting perf event group",
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = GroupReading_methods,
    .tp_members = GroupReading_members,
    .tp_as_sequence = &GroupReading_seqmethods,
    .tp_str = groupreading_str,
    .tp_new = groupreading_new,
    .tp_dealloc = groupreading_dealloc
};


/*
 * Convert a hardware timestamp to a kernel timestamp as found in perf.data.
 */
static PyObject *timeconv_to_time(PyObject *x, PyObject *v)
{
    TimeConvObject *c = (TimeConvObject *)x;
    unsigned long cyc = PyLong_AsLong(v);
    unsigned long long quot, rem, st;
    quot = (cyc >> c->time_shift);
    rem = cyc & ((1ULL << c->time_shift) - 1);
    st = c->time_zero + quot*c->time_mult + ((rem*c->time_mult) >> c->time_shift);
    return PyLong_FromLong(st); 
}


static int timeconv_from_mmap(TimeConvObject *c, struct perf_event_mmap_page volatile *mp)
{ 
    if (!(mp->capabilities & _cap_user_time_zero)) {
        /* Can't read timebase from userspace */
        return 0;
    }
    unsigned int seq;
    do {
        seq = mp->lock;
        barrier();
        if (mp->capabilities & _cap_user_time_zero) {
            //cyc = hardware_timestamp();
            c->time_zero = mp->time_zero;
            c->time_mult = mp->time_mult;
            c->time_shift = mp->time_shift;
        }
        barrier();
    } while (mp->lock != seq);
    return 1;
}


/*
 * Get the time conversion parameters - these are global, but if we don't
 * already have an event we need to open a dummy event to get them.
 */
static int timeconv_from_dummy(TimeConvObject *c)
{
    int ok = 0;
    struct perf_event_attr pa;
    memset(&pa, 0, sizeof pa);
    pa.size = sizeof pa;
    pa.type = PERF_TYPE_SOFTWARE;
    pa.config = PERF_COUNT_SW_DUMMY;
    /* Open a temporary dummy event, which we will close before returning. */
    int e_fd = perf_event_open(&pa, /*pid=*/0, -1, -1, 0);
    if (e_fd < 0) {
        perror("dummy event");
        return 0;
    }
    unsigned int size = sysconf(_SC_PAGESIZE);
    void *pmap = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, e_fd, 0);
    if (pmap != MAP_FAILED) {
        ok = timeconv_from_mmap(c, (struct perf_event_mmap_page *)pmap);
        if (!ok) {
            PyErr_SetString(PyExc_ValueError, "mmap buffer doesn't contain system time base");
        }
        munmap(pmap, size);
    } else {
        perror("mmap");
    }
    close(e_fd);    /* Close the temporary dummy event. */
    return ok;
}


static PyObject *timeconv_new(PyTypeObject *t, PyObject *args, PyObject *kwds)
{
    TimeConvObject *c = (TimeConvObject *)t->tp_alloc(t, 0);
    int ok = timeconv_from_dummy(c);
    if (!ok) {
        t->tp_free(c);
        return NULL;
    }
    return (PyObject *)c;
}


static void timeconv_dealloc(PyObject *x)
{
    x->ob_type->tp_free(x);
}


static PyMethodDef TimeConv_methods[] = {
    {"to_time", (PyCFunction)&timeconv_to_time, METH_O, "convert hardware timestamp to user time"},
    {NULL}
};

static struct PyMemberDef TimeConv_members[] = {
    {"time_zero", T_LONG, offsetof(TimeConvObject, time_zero), 0, "time zero"},
    {"time_mult", T_LONG, offsetof(TimeConvObject, time_mult), 0, "time multiplier"},
    {"time_shift", T_LONG, offsetof(TimeConvObject, time_shift), 0, "time shift"},
    {NULL}
};

static PyTypeObject TimeConvType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_basicsize = sizeof(TimeConvObject),
    .tp_name = "perf_events.TimeConv",
    .tp_doc = "time conversion parameters",
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = TimeConv_methods,
    .tp_members = TimeConv_members,
    //.tp_str = timeconv_str,
    .tp_new = timeconv_new,
    .tp_dealloc = timeconv_dealloc
};


static int sysctl_value(char const *s, int dflt)
{
    FILE *fd = fopen(s, "r");
    if (fd) {
        int n;
        if (fscanf(fd, "%d", &n) == 1) {
            return n;
        }
        fclose(fd);
    }
    return dflt;
}


/*
 * Check if the overall perf_events API is available - might have been configured out.
 *
 * 2: no measurements
 * 1: kernel events
 * 0: CPU events
 * -1: kernel tracepoints
 */
static PyObject *is_available(void)
{
    int ok = (sysctl_value("/proc/sys/kernel/perf_event_paranoid", 3) <= 2);
    return PyBool_FromLong(ok);
}


static unsigned int perf_event_mlock_size(void)
{
    static unsigned int size_memo = 0;
    if (!size_memo) {
        size_memo = 1024 * sysctl_value("/proc/sys/kernel/perf_event_mlock_kb", 0);
    }
    return size_memo;
}


/*
 * Get the TID of the current thread.
 * This is provided as a convenience function for when we need a TID
 * to set up a perf event on a thread.
 *
 * glibc doesn't have a wrapper for this so we must call via syscall().
 *
 * Note that the pthread_t value we get when we start a thread,
 * isn't necessarily the thread's TID.
 */
static PyObject *perf_gettid(void)
{
    pid_t tid = (pid_t)syscall(SYS_gettid);
    return PyInt_FromLong(tid);
}


/*
 * Increment the "software increment" event(s), where available.
 * This may cause a hardware exception (trap to kernel) if not available from userspace.
 *
 * On ARM we may have multiple counters configured to count software increments.
 * Each counter is incremented by writing a 1 bit to the corresponding bit of SWINC.
 * However this doesn't work well when events are dynamically mapped to counters
 * by the OS.  So we play safe and increment all available s/w increment events.
 */
static PyObject *perf_swinc(PyObject *x)
{
    int ok = 0;
#if defined(__arm64__) || defined(__AARCH64EL__) || defined(__ARM_ARCH_ISA_A64) 
    /* AArch64: this will get Illegal Instruction if not enabled at EL0 */
    __asm__ __volatile__("msr pmswinc_el0,%0"::"r"((unsigned long long)0x7fffffff));
    ok = 1;
#endif
    return PyBool_FromLong(ok);
}


/*
 * Read a value directly from a hardware event counter (may fault) on the current core.
 * This is not generally a safe way to read event values.
 * It may fault (if userspace access is not permitted), or race with event scheduling.
 */
static PyObject *perf_rdpmc(PyObject *x, PyObject *vo)
{
    unsigned long ix = PyLong_AsLong(vo);
    unsigned long long v = rdpmc(ix);
    return PyLong_FromLong(v);
}


/*
 * Write a value to hardware trace, where supported.
 */
static PyObject *perf_swtrace(PyObject *x, PyObject *vo)
{
    int ok = 0;
    unsigned long v = PyLong_AsLong(vo);
#if defined(__x86_64__)
    /* This may SIGILL on older cores. We could do a feature check beforehand (TBD). */
    __asm__ __volatile__("ptwrite %0"::"r"(v));
    ok = 1;
#else
    (void)v;      /* trace is a no-op */
#endif
    return PyBool_FromLong(ok);
}


static PyObject *addr_no_randomize(PyObject *x)
{   
    int ok = 0;
    int prev = personality(0xffffffff);
    if (prev == -1) {
        perror("personality(-1)");
    } else {
        prev = personality(prev | ADDR_NO_RANDOMIZE);
        if (prev == -1) {
            perror("personality update");
        } else {
            ok = 1;
        }
    }
    if (!ok) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    Py_RETURN_NONE;
}


/*
 * Return the platform-specific hardware timestamp. This can be converted to
 * a kernel timestamp using the time conversion factors in the mmap buffer.
 * It may appear directly in AUX buffers for various kinds of hardware trace.
 */
static PyObject *perf_hardware_timestamp(PyObject *x)
{
    return PyLong_FromUnsignedLongLong(hardware_timestamp());
}


/*
 * Return the platform-specific hardware timestamp frequency,
 * if directly available. If not available this way, it might be
 * available in the mmap header.
 */
static PyObject *perf_hardware_timestamp_frequency(PyObject *x)
{
    unsigned long long freq = hardware_timestamp_frequency();
    if (freq > 0) {
        return PyLong_FromUnsignedLongLong(freq);
    } else {
        Py_RETURN_NONE;
    }
}


/*
 * Return an integer timestamp with the same timebase as the timestamp in
 * perf records (PERF_SAMPLE_TIME). This is typically a count of nanoseconds
 * since boot, as returned by sched_clock().
 */
static PyObject *perf_kernel_timestamp(PyObject *x)
{
    unsigned long long t;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    t = (unsigned long long)ts.tv_sec * 1000000000 + ts.tv_nsec;
    return PyLong_FromUnsignedLongLong(t);
}


static PyObject *perf_fileno_event(PyObject *x, PyObject *ixo)
{
    int ix = PyLong_AsLong(ixo);
    if (ix < 0 || (unsigned int)ix >= n_fileno_events) {
        Py_RETURN_NONE;
    } else if (fileno_events[ix] == NULL) {
        Py_RETURN_NONE;
    } else {
        PyObject *e = (PyObject *)fileno_events[ix];
        Py_INCREF(e);
        return e;
    }
}


/*
 * General useful global functions.
 */
static PyMethodDef funcs[] = {
    {"is_available", (PyCFunction)&is_available, METH_NOARGS, PyDoc_STR("None -> bool: test if perf events is available")},
    {"gettid", (PyCFunction)&perf_gettid, METH_NOARGS, PyDoc_STR("None -> int: get current OS thread id")},
    {"swinc", (PyCFunction)&perf_swinc, METH_NOARGS, PyDoc_STR("None -> bool: increment the Software Increment register")},
    {"swtrace", (PyCFunction)&perf_swtrace, METH_O, PyDoc_STR("int -> write a value to hardware trace")},
    {"rdpmc", (PyCFunction)&perf_rdpmc, METH_O, PyDoc_STR("int -> read a value from a hardware event counter")},
    {"addr_no_randomize", (PyCFunction)&addr_no_randomize, METH_NOARGS, PyDoc_STR("disable ASLR in this and child processes")},
    {"hardware_timestamp", (PyCFunction)&perf_hardware_timestamp, METH_NOARGS, PyDoc_STR("None -> int: read hardware timestamp")},
    {"hardware_timestamp_frequency", (PyCFunction)&perf_hardware_timestamp_frequency, METH_NOARGS, PyDoc_STR("None -> int: read hardware timestamp frequency (Hz)")},
    {"kernel_timestamp", (PyCFunction)&perf_kernel_timestamp, METH_NOARGS, PyDoc_STR("None -> int: read kernel timestamp")},
    {"fileno_event", (PyCFunction)&perf_fileno_event, METH_O, PyDoc_STR("int -> get perf event for an OS file handle")},
    {NULL}
};


#define CON(x) { #x, x }
static struct {
    char const *name;
    unsigned long value;
} constants[] = {
    CON(PERF_FLAG_READ_USERSPACE),
    CON(PERF_FLAG_NO_READ_USERSPACE),
    CON(PERF_FLAG_WEAK_GROUP),
};


#if PY_MAJOR_VERSION < 3
#define INIT_NAME2(m) init##m
#else
#define INIT_NAME2(m) PyInit_##m
#endif
#define INIT_NAME(m) INIT_NAME2(m)

PyMODINIT_FUNC INIT_NAME(MODULE_NAME)(void)
{
    PyObject *pmod;
#if PY_MAJOR_VERSION < 3
    pmod = Py_InitModule3(MODULE_NAME_STRING, funcs, "perf_events API");
#else
    static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        .m_name = MODULE_NAME_STRING,
        .m_doc = PyDoc_STR("perf_events API"),
        .m_size = -1,
        .m_methods = funcs
    };
    pmod = PyModule_Create(&moduledef);
#endif
    PyType_Ready(&EventType);
    PyObject_SetAttrString(pmod, "Event", (PyObject *)&EventType);
    PyType_Ready(&RecordType);
    PyObject_SetAttrString(pmod, "Record", (PyObject *)&RecordType);
    PyType_Ready(&ReadingType);
    PyObject_SetAttrString(pmod, "Reading", (PyObject *)&ReadingType);
    PyType_Ready(&GroupReadingType);
    PyObject_SetAttrString(pmod, "GroupReading", (PyObject *)&ReadingType);
    PyType_Ready(&TimeConvType);
    PyObject_SetAttrString(pmod, "TimeConv", (PyObject *)&TimeConvType);
    {
        unsigned int i;
        for (i = 0; i < (sizeof constants / sizeof constants[0]); ++i) {
            PyModule_AddIntConstant(pmod, constants[i].name, constants[i].value);
        }
    }
#if PY_MAJOR_VERSION >= 3
    return pmod;
#endif
}

/* end of pyperf_events.c */
