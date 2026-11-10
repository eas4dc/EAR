/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <common/colors.h>
#include <common/config.h>
#include <pthread.h>
// #define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/types/pc_app_info.h>
#include <daemon/power_monitor.h>
#include <daemon/powercap/powercap.h>
#include <daemon/powercap/powercap_status.h>
#include <management/cpufreq/frequency.h>

/* This function decides if we have to changed the utilization or not */
#define MAX_ALLOWED_DIFF 0.25

uint util_changed(ulong curr, ulong prev)
{
    float diff;
    int idiff, icurr, iprev;
    icurr = (int) curr;
    iprev = (int) prev;
    if ((prev == 0) && (curr))
        return 1;
    if ((curr == 0) && (prev))
        return 1;
    if ((!prev) && (!curr))
        return 0;
    idiff = (icurr - iprev);
    if (idiff < 0) {
        idiff = idiff * -1;
        diff  = (float) idiff / (float) iprev;
    } else {
        diff = (float) idiff / (float) icurr;
    }
    // debug("Current util and prev util differs in %.3f prev %lu curr %lu",diff,prev,curr);
    return (diff > MAX_ALLOWED_DIFF);
}

uint compute_power_to_release(node_powercap_opt_t *pc_opt, uint current)
{
    return pc_opt->th_release;
}

uint compute_extra_gpu_power(uint current, uint diff, uint target)
{
    /* This function mut be improved */
    return 10;
}

uint more_power(node_powercap_opt_t *pc_opt, uint current)
{
    int dist;
    dist = (int) pc_opt->current_pc - (int) current;
    debug("More Power check:Computed distance %d", dist);
    if (dist < 0)
        return 1;
    if (dist < pc_opt->th_inc)
        return 1;
    return 0;
}

uint free_power(node_powercap_opt_t *pc_opt, uint current)
{
    int dist;
    dist = (int) pc_opt->current_pc - (int) current;
    debug("Free Power check:Computed distance %d", dist);
    if (dist < 0)
        return 0;
    if (dist >= pc_opt->th_red)
        return 1;
    return 0;
}

uint ok_power(node_powercap_opt_t *pc_opt, uint current)
{
    int dist;
    dist = (int) pc_opt->current_pc - (int) current;
    debug("OK Power check:Computed distance %d", dist);
    if (dist < 0)
        return 0;
    if ((dist < pc_opt->th_red) && (dist > pc_opt->th_inc))
        return 1;
    return 0;
}

int is_powercap_set(node_powercap_opt_t *pc_opt)
{
    /* 0 means disabled*/
    /* we are not checking hw configuration in this function */
    return (pc_opt->current_pc != 0);
}

int is_powercap_on(node_powercap_opt_t *pc_opt)
{
    return ((pc_opt->powercap_status != PC_STATUS_ERROR) && is_powercap_set(pc_opt));
}

uint get_powercapopt_value(node_powercap_opt_t *pc_opt)
{
    /* we are not checking hw configuration in this function */
    return pc_opt->current_pc;
}

uint get_powercap_allocated(node_powercap_opt_t *pc_opt)
{
    /* we are not checking hw configuration in this function */
    return pc_opt->last_t1_allocated;
}

/* Returns the stress in the subsystem. The stress level represents the
 * percentual distance between the current freq and the target freq,
 * with 0 being functioning as expected and 100 being the worst case.
 * The calculation is:
 *      (target-current)/(max-min) * 100.
 *
 * If the max_freq and min_freq are the same (error), 0 is returned
 * as the distance cannot be computed. */
uint powercap_get_stress(uint max_freq, uint min_freq, uint t_freq, uint c_freq)
{
    debug("getting stress with %u(max_freq) %u(min_freq) %u(t_freq) and %u(c_freq)", max_freq, min_freq, t_freq,
          c_freq);
    /* at or above target, or no range: no stress */
    if (max_freq <= min_freq || c_freq >= t_freq)
        return 0;
    return ear_min((100 * (ullong) (t_freq - c_freq)) / (max_freq - min_freq), 100);
}

void power_estimator_init(power_estimator_t *e, uint min_w, uint max_w, ulong min_khz, ulong max_khz)
{
    memset(e, 0, sizeof(power_estimator_t));
    if (max_khz > min_khz && max_w > min_w)
        e->seed = (double) (max_w - min_w) / (double) (max_khz - min_khz);
    e->w_per_khz = e->seed;
}

void power_estimator_sample(power_estimator_t *e, double w, ulong khz)
{
    /* a move of less than ~1% of the clock is noise, not a slope */
    double dkhz = (double) khz - (double) e->last_khz;
    if (e->seed > 0 && e->last_khz && fabs(dkhz) * e->seed >= 1.0) {
        /* for the same load power follows the clock: opposite moves (e.g. under a cap, where
         * power stays put and the clock follows the load) say nothing about the slope */
        double slope = (w - e->last_w) / dkhz;
        if (slope > 0) {
            /* the floor is low: frequency-insensitive (memory-bound) loads have slopes
             * well under the range average */
            slope        = ear_min(ear_max(slope, e->seed / 16), e->seed * 4);
            e->w_per_khz = 0.5 * e->w_per_khz + 0.5 * slope;
        }
    }
    e->last_w   = w;
    e->last_khz = khz;
}

double power_estimator_at(const power_estimator_t *e, double w_now, ulong khz_now, ulong khz)
{
    return w_now + ((double) khz - (double) khz_now) * e->w_per_khz;
}

uint powercap_device_request(uint max_w, uint cap_w, uint need_w)
{
    if (cap_w >= max_w || need_w == 0)
        return 0;
    return ear_min(max_w - cap_w, need_w);
}

void powercap_max_or_tdp(uint32_t *wmax, const uint32_t *tdp, uint n)
{
    for (uint i = 0; i < n; i++)
        if (wmax[i] == 0)
            wmax[i] = tdp[i];
}

uint compute_power_status(node_powercap_opt_t *pc, uint current_power)
{
    if (pc->last_t1_allocated < pc->baseline_powercap)
        return PC_STATUS_ASK_DEF;
    if (ok_power(pc, current_power))
        return PC_STATUS_OK;
    if (more_power(pc, current_power))
        return PC_STATUS_GREEDY;
    if (free_power(pc, current_power))
        return PC_STATUS_RELEASE;
    return PC_STATUS_OK;
}

uint compute_next_status(node_powercap_opt_t *pc, uint current_power, ulong eff_f, ulong req_f)
{
    uint pstate = compute_power_status(pc, current_power);
    switch (pstate) {
        case PC_STATUS_RELEASE:
            if (eff_f == req_f)
                return PC_STATUS_RELEASE;
            break;
        case PC_STATUS_GREEDY:
            if (eff_f < req_f)
                return PC_STATUS_GREEDY;
            break;
        case PC_STATUS_OK:
            if (eff_f < req_f)
                return PC_STATUS_GREEDY;
            break;
        default:
            return pstate;
    }
    return PC_STATUS_OK;
}

void powercap_status_to_str(uint s, char *b)
{
    switch (s) {
        case PC_STATUS_OK:
            sprintf(b, "OK");
            break;
        case PC_STATUS_GREEDY:
            sprintf(b, "greedy");
            break;
        case PC_STATUS_RELEASE:
            sprintf(b, "release");
            break;
        case PC_STATUS_ASK_DEF:
            sprintf(b, "ask_def");
            break;
        case PC_STATUS_IDLE:
            sprintf(b, "idle");
            break;
        case PC_STATUS_STOP:
            sprintf(b, "stop");
            break;
        case PC_STATUS_START:
            sprintf(b, "start");
            break;
        case PC_STATUS_RUN:
            sprintf(b, "run");
            break;
        default:
            sprintf(b, "undef");
            break;
    }
}
