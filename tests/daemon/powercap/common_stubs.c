/*
 * common_stubs.c -- the handful of libcommon symbols the unit tier needs.
 *
 * The unit tier must not need a built tree, so instead of linking libcommon.a
 * this file provides the time, environment and monitor entry points. The
 * monitor is the interesting one: instead of a thread it records every
 * subscription, so a test can fire call_init/call_main by hand, one tick at a
 * time, and the powercap threads become deterministic.
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <common/states.h>
#include <common/system/monitor.h>
#include <common/system/time.h>
#include <common/types/generic.h>

#include "fake_monitor.h"

/* ----------------------------------------------------------------- time */

/* When >= 0 the clock is frozen at this value (ns); tests advance it by hand */
long long fake_clock_ns = -1;

static void fake_now(timestamp *ts)
{
    if (fake_clock_ns < 0) {
        clock_gettime(CLOCK_MONOTONIC, ts);
        return;
    }
    ts->tv_sec  = fake_clock_ns / 1000000000LL;
    ts->tv_nsec = fake_clock_ns % 1000000000LL;
}

void timestamp_get(timestamp *ts)
{
    fake_now(ts);
}

void timestamp_getfast(timestamp *ts)
{
    fake_now(ts);
}

void timestamp_getprecise(timestamp *ts)
{
    fake_now(ts);
}

ullong timestamp_diff(timestamp *ts2, timestamp *ts1, ullong time_unit)
{
    ullong t2 = (ullong) ts2->tv_sec * 1000000000ULL + ts2->tv_nsec;
    ullong t1 = (ullong) ts1->tv_sec * 1000000000ULL + ts1->tv_nsec;
    return (t2 - t1) / time_unit;
}

ullong timestamp_diffnow(timestamp *ts1, ullong time_unit)
{
    timestamp now;
    timestamp_get(&now);
    return timestamp_diff(&now, ts1, time_unit);
}

/* ---------------------------------------------------------- environment */

char *ear_getenv(const char *name)
{
    return getenv(name);
}

/* -------------------------------------------------------------- monitor */

suscription_t *fake_subs[FAKE_MAX_SUBS];
int fake_sub_state[FAKE_MAX_SUBS]; /* 0 relax, 1 burst */
int fake_num_subs = 0;

static state_t fake_suscribe(void *p)
{
    suscription_t *s = p;
    for (int i = 0; i < fake_num_subs; i++)
        if (fake_subs[i] == s)
            return EAR_SUCCESS;
    if (fake_num_subs < FAKE_MAX_SUBS) {
        s->id                         = fake_num_subs;
        fake_subs[fake_num_subs++]    = s;
    }
    return EAR_SUCCESS;
}

suscription_t *suscription()
{
    suscription_t *s = calloc(1, sizeof(suscription_t));
    s->suscribe      = fake_suscribe;
    return s;
}

state_t monitor_burst(suscription_t *s, uint interrupt)
{
    if (s != NULL && s->id < FAKE_MAX_SUBS)
        fake_sub_state[s->id] = 1;
    return EAR_SUCCESS;
}

state_t monitor_relax(suscription_t *s)
{
    if (s != NULL && s->id < FAKE_MAX_SUBS)
        fake_sub_state[s->id] = 0;
    return EAR_SUCCESS;
}

int fake_unregister_calls = 0;

state_t monitor_unregister(suscription_t *s)
{
    fake_unregister_calls++;
    for (int i = 0; i < fake_num_subs; i++) {
        if (fake_subs[i] == s) {
            for (int j = i; j < fake_num_subs - 1; j++)
                fake_subs[j] = fake_subs[j + 1];
            fake_num_subs--;
            break;
        }
    }
    return EAR_SUCCESS;
}

suscription_t *fake_find_sub(suscall_f main_fn)
{
    for (int i = 0; i < fake_num_subs; i++)
        if (main_fn == NULL || fake_subs[i]->call_main == main_fn)
            return fake_subs[i];
    return NULL;
}

void fake_tick_all(void)
{
    for (int i = 0; i < fake_num_subs; i++)
        if (fake_subs[i]->call_main)
            fake_subs[i]->call_main(fake_subs[i]->memm_main);
}

void fake_init_all(void)
{
    for (int i = 0; i < fake_num_subs; i++)
        if (fake_subs[i]->call_init)
            fake_subs[i]->call_init(fake_subs[i]->memm_init);
}
