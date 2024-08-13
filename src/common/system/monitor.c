/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <error.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <common/output/verbose.h>
#include <common/output/debug.h>
#include <common/system/monitor.h>

#define N_QUEUE  128
#define SIGNAL   SIGCONT

#define debugv(...) verbose(0,__VA_ARGS__)
//#define debugv(...) debug(__VA_ARGS__)

typedef struct wait_s {
    int relax;
    int burst;
} wait_t;

// Register queue (suscription + meta data)
typedef struct queue_s {
    suscription_t suscription;
    wait_t sleep_units;
    wait_t saved_units;
    int delivered; // If the suscription has been given but maybe not registered
    int registered; // if the suscription has been given and registered
    int aligned;
    int is_bursting;
    int ok_init; // Ready to call init
    int ok_main; // Ready to call main
} queue_t;

static queue_t         queue[N_QUEUE];
static uint            queue_last;
static pthread_t       thread;
struct sigaction       action_new;
struct sigaction       action_old;
static pthread_mutex_t lock_gen = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t locks[N_QUEUE];
static uint            is_allocated;
static uint            is_atforked = 1;
static uint            is_running;

static void goto_handler(int sig)
{
    if (sigaction(SIGNAL, &action_old, NULL) < 0) {
        error("SIGACTION2 on signal SIGCONT (%s)", strerror(errno));
    }
}

static void goto_interrupt()
{
    action_new.sa_handler = goto_handler;
    action_new.sa_flags = 0;

    if (sigaction(SIGNAL, &action_new, &action_old) < 0) {
        error("SIGACTION1 on signal SIGCONT (%s)", strerror(errno));
    }
    if (is_running) {
        debug("sending signal %d to thread %ld", SIGNAL, thread);
        pthread_kill(thread, SIGNAL);
    }
}

static void goto_sleep(int sleep_units, int sleep_reason, int *passed_units, int *alignment)
{
#ifdef SHOW_DEBUGS
    char       *sleep_str[3] = { "next call", "alignment", "wandering" };
    char       *wake_str[2]  = { "time passed", "interruption" };
#endif
    ATTR_UNUSED int wake_reason;
    ullong      passed_time;
    ullong      deciseconds;
    timestamp_t tW; // Wait time
    timestamp_t t2;
    timestamp_t t1;

    // Converting sleep units to timesampt.
    deciseconds = ((ullong) sleep_units) * 100LLU;
    timestamp_revert(&tW, deciseconds, TIME_MSECS);
    debug("going to sleep %d units (reason '%s')",
          sleep_units, sleep_str[sleep_reason]);

    // Last check
    if (!is_running) {
        return;
    }
    // Sleeping
    timestamp_getfast(&t1);
    wake_reason = nanosleep(&tW, NULL);
    timestamp_getfast(&t2);

    // If s is 0 then all went ok and the time processing continues normally.
    passed_time   = timestamp_diff(&t2, &t1, TIME_MSECS) + 1LLU;
    *passed_units = ((int) passed_time);
    *passed_units = *passed_units / 100;
    *alignment   += *passed_units;
    *alignment   %= 10;

    debug("sleeped during %llu ms (%d units, alignment %d, reason '%s')",
          passed_time, *passed_units, *alignment, wake_str[wake_reason != 0]);
}

static void goto_time(queue_t *reg, int *sleep_units, int *sleep_reason, int passed_units, int alignment)
{
    int is_bursting = reg->is_bursting;
    int sleep_units_required;

    // Alignment processing. If not aligned and impossible to align yet,
    // set the sleep units to align.
    if (!reg->aligned) {
        reg->aligned = (alignment == 0);
        reg->ok_init = reg->aligned && (reg->suscription.call_init != NULL);
        reg->ok_main = reg->aligned;

        if (!reg->ok_main) {
            sleep_units_required = 10 - alignment;
            if (sleep_units_required < *sleep_units) {
                *sleep_units = sleep_units_required;
                *sleep_reason = 1; // Alignment
            }
            return;
        }
        debug("S%d aligned", reg->suscription.id);
    }
    // The time is discounted from both counters (burst and relax). If currently
    // the state has been changed, the wait counter will be switched too. 
    reg->sleep_units.burst -= passed_units;
    reg->sleep_units.relax -= passed_units;
    if (reg->sleep_units.burst <= 0 &&  is_bursting) reg->ok_main = 1;
    if (reg->sleep_units.relax <= 0 && !is_bursting) reg->ok_main = 1;
    if (reg->sleep_units.burst <= 0) reg->sleep_units.burst = reg->saved_units.burst;
    if (reg->sleep_units.relax <= 0) reg->sleep_units.relax = reg->saved_units.relax;

    // If current sleep units is greater than our burst/relax pending units,
    // current wait units will be replaced by the suscription counter.
    if (is_bursting && reg->sleep_units.burst < *sleep_units) {
        *sleep_units = reg->sleep_units.burst;
        *sleep_reason = 0; // Next call
    } else if (!is_bursting && reg->sleep_units.relax < *sleep_units) {
        *sleep_units = reg->sleep_units.relax;
        *sleep_reason = 0; // Next call
    }
}

static void goto_init()
{
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGNAL);
    pthread_sigmask(SIG_SETMASK, &set, NULL);
    pthread_setschedprio(thread, 10);
    debug("ready");
}

static void *monitor_main(void *p)
{
    queue_t *reg;
    suscription_t *sus;
    int sleep_reason = 0;
    int sleep_units  = 0;
    int passed_units = 0; // How many units have been passed
    int alignment    = 0;
    int i;

    // Initializing thread signals
    goto_init();
    // Main loop
    while (is_running) {
        debug("running");
        for (i = 0, sleep_units = 1000, sleep_reason = 0; i < queue_last && is_running; ++i) {
            // Locking the suscription [i]
            while (pthread_mutex_trylock(&locks[i]));
            // Getting pointers
            reg = &queue[i];
            sus = &reg->suscription;
            // If the suscription is not registered, discard
            if (!reg->registered) {
                pthread_mutex_unlock(&locks[i]);
                continue;
            }
            // Time calculation, a unit is 100 ms:
            //     - sleep_units: units to sleep
            //     - passed_units: passed units since last sleep
            //     - alignment: a number from 0 to 9
            // The upper for while, set sleeping time to 100 seconds (1000 units)
            goto_time(reg, &sleep_units, &sleep_reason, passed_units, alignment);
            // Preparing sus-calls to avoid race conditions. Because someone can
            // clear subscriptions between both ifs and the following calls.
            suscall_f call_init = sus->call_init;
            suscall_f call_main = sus->call_main;
            void     *memm_init = sus->memm_init;
            void     *memm_main = sus->memm_main;

            // Unlocking suscription i because time is calculated
            pthread_mutex_unlock(&locks[i]);

            if (reg->ok_init && is_running) {
                //debug("S%d, called init", i);
                call_init(memm_init);
                reg->ok_init = 0;
            }
            if (reg->ok_main && is_running) {
                //debug("S%d, called main", i);
                call_main(memm_main);
                reg->ok_main = 0;
            }
        }
        if (queue_last == 0) {
            sleep_reason = 2;
        }
        goto_sleep(sleep_units, sleep_reason, &passed_units, &alignment);
    }
    return NULL;
}

static state_t init_allocation()
{
    int error = 0;
    int i;

    for (i = 0; i < N_QUEUE && !error; ++i) {
        error = (pthread_mutex_init(&locks[i], NULL) != 0);
    }
    for (; i > 0 && error; --i) {
        pthread_mutex_destroy(&locks[i-1]);
    }
    if (error) {
        return_msg(EAR_ERROR, Generr.lock);
    }
    return EAR_SUCCESS;
}

static state_t init_thread()
{
    int m_errno;
    if ((m_errno = pthread_create(&thread, NULL, monitor_main, NULL)) != 0) {
        return_msg(EAR_ERROR, strerror(m_errno));
    }
    return EAR_SUCCESS;
}

static void init_atfork()
{
    int i;
    // Atfork enters before fork() exits
    pthread_mutex_unlock(&lock_gen);
    // If is allocated, we just unlock the possible locked mutexes
    if (is_allocated) {
        for (i = 0; i < N_QUEUE; ++i) {
            pthread_mutex_unlock(&locks[i]);
        }
    }
    // If is already running, we start the monitor again
    if (is_running) {
        if (state_fail(init_thread())) {
            is_running = 0;
        }
    }
    return;
}

state_t monitor_init()
{
    int m_errno;
    state_t s;

    // We assume this statically initialized lock still working after fork
    while (pthread_mutex_trylock(&lock_gen));
    // If monitor is already running, and thus is_allocated
    if (is_running != 0) {
        pthread_mutex_unlock(&lock_gen);
        return EAR_SUCCESS;
    }
    if (!is_allocated) {
        if (state_fail(s = init_allocation())) {
            pthread_mutex_unlock(&lock_gen);
            return s;
        }
        is_allocated = 1;
    }
    if (!is_atforked) {
        if ((m_errno = pthread_atfork(NULL, NULL, init_atfork)) != 0) {
            pthread_mutex_unlock(&lock_gen);
            return_msg(EAR_ERROR, strerror(m_errno));
        }
        is_atforked = 1;
    }
    if (!is_running) {
        // We set this variable before because we want the following thread to continue.
        is_running = 1;
        if (state_fail(s = init_thread())) {
            is_running = 0;
            pthread_mutex_unlock(&lock_gen);
            return s;
        }
    }
    pthread_mutex_unlock(&lock_gen);
    debug("monitor_init");
    return EAR_SUCCESS;
}

void monitor_wait()
{
    if (!pthread_equal(pthread_self(), thread)) {
        pthread_join(thread, NULL);
    }
}

static void suscriptions_unregister_all()
{
    int i;
    for (i = 0; is_allocated && i < N_QUEUE; ++i) {
        while (pthread_mutex_trylock(&locks[i]));
    }
    memset(queue, 0, sizeof(queue_t) * N_QUEUE);
    queue_last = 0;
    for (i = 0; is_allocated && i < N_QUEUE; ++i) {
        pthread_mutex_unlock(&locks[i]);
    }
}

state_t monitor_dispose()
{
    if (!is_running) {
        return_msg(EAR_ERROR, "not initialized");
    }
    while (pthread_mutex_trylock(&lock_gen));
    goto_interrupt();
    is_running = 0;
    suscriptions_unregister_all();
    pthread_mutex_unlock(&lock_gen);
    return EAR_SUCCESS;
}

static state_t monitor_register_void(void *p)
{
    suscription_t *s = (suscription_t *) p;
    return monitor_register(s);
}

state_t monitor_register(suscription_t *s)
{
    int retval;

    if (!is_running         ) monitor_init();
    if (s == NULL           ) return_msg(EAR_ERROR, "the suscription can't be NULL");
    if (s->time_relax < 100 ) return_msg(EAR_ERROR, "time can't be zero");
    if (s->call_main == NULL) return_msg(EAR_ERROR, "reading call is NULL");
    if (s->id         < 0   ) return_msg(EAR_ERROR, "incorrect suscription index");

    while ((retval = pthread_mutex_trylock(&locks[s->id])) != 0) {
        if (retval != EBUSY) {
            return_msg(EAR_ERROR, strerror(retval));
        }
    }
    queue[s->id].registered  = 1;
    queue[s->id].aligned     = 0;
    queue[s->id].is_bursting = 0;
    queue[s->id].saved_units.relax = (s->time_relax / 100);
    queue[s->id].saved_units.burst = (s->time_burst / 100);
    debug("registered S%d (%d/%d)", s->id, s->time_relax, s->time_burst);
    pthread_mutex_unlock(&locks[s->id]);
    // Interrupt to wake monitor thread
    goto_interrupt();

    return EAR_SUCCESS;
}

state_t monitor_unregister(suscription_t *s)
{
    int id = s->id;
    int retval;

    if (!is_running        ) monitor_init();
    if (s     == NULL      ) return_msg(EAR_ERROR, "the suscription can't be NULL");
    if (s->id  > queue_last) return_msg(EAR_ERROR, "incorrect suscription index");
    if (s->id  < 0         ) return_msg(EAR_ERROR, "incorrect suscription index");

    while ((retval = pthread_mutex_trylock(&locks[id])) != 0) {
        if (retval != EBUSY) {
            return_msg(EAR_ERROR, strerror(retval));
        }
    }
    memset(&queue[id], 0, sizeof(queue_t));
    pthread_mutex_unlock(&locks[id]);
    return EAR_SUCCESS;
}

int monitor_is_initialized()
{
    return is_running;
}

int monitor_is_bursting(suscription_t *s)
{
    return queue[s->id].is_bursting;
}

state_t monitor_burst(suscription_t *s, uint interrupt)
{
    int retval;

    if (s             == NULL) return_msg(EAR_ERROR, "the suscription can't be NULL");
    if (s->time_burst == 0   ) return_msg(EAR_ERROR, "time cant be zero");
    if (s->time_burst  < 0   ) return_msg(EAR_ERROR, "burst time cant be less than relax time");

    while ((retval = pthread_mutex_trylock(&locks[s->id])) != 0) {
        if (retval != EBUSY) {
            return_msg(EAR_ERROR, strerror(retval));
        }
    }
    if (!queue[s->id].is_bursting) {
        queue[s->id].is_bursting = 1;
        // Not sure if this have to be there or after unlocking
        if (interrupt) {
            goto_interrupt();
        }
    }
    pthread_mutex_unlock(&locks[s->id]);
    return EAR_SUCCESS;
}

state_t monitor_relax(suscription_t *s)
{
    if (s == NULL) return_msg(EAR_ERROR, "the suscription can't be NULL");
    queue[s->id].is_bursting = 0;
    return EAR_SUCCESS;
}

suscription_t *suscription()
{
    int i = 0;

    while (pthread_mutex_trylock(&lock_gen));
    // Finding an empty suscription
    for (; i < N_QUEUE; ++i) {
        if (queue[i].delivered == 0) {
            break;
        }
    }
    if (queue_last == N_QUEUE) {
        pthread_mutex_unlock(&lock_gen);
        return NULL;
    }
    queue[i].suscription.id = i;
    queue[i].delivered      = 1;
    queue[i].suscription.suscribe = monitor_register_void;
    queue[i].is_bursting    = 0;
    //
    if (queue[i].suscription.id >= queue_last) {
        queue_last = queue[i].suscription.id + 1;
    }
    pthread_mutex_unlock(&lock_gen);
    return &queue[i].suscription;
}
