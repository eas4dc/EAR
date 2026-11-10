/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _POWERCAP_STATUS_H
#define _POWERCAP_STATUS_H
#include <common/types/pc_app_info.h>
#include <daemon/powercap/powercap_status_conf.h>

uint util_changed(ulong curr, ulong prev);

/** Given a powercap settings and a given power consumption computes the power to be released */
uint compute_power_to_release(node_powercap_opt_t *pc_opt, uint current);

/** Given a powercap settings and a given power consumption estimates if the application needs more power: GREEDY state
 */
uint more_power(node_powercap_opt_t *pc_opt, uint current);

/** Given a powercap settings and a given power consumption estimates if the application can release power :RELEASE
 * state */
uint free_power(node_powercap_opt_t *pc_opt, uint current);

/** Given a powercap settings and a given power consumption estimates if the application is ok with the allocated power
 */
uint ok_power(node_powercap_opt_t *pc_opt, uint current);

/** Given a powercap settings returns true when a powercap limit is defined */
int is_powercap_set(node_powercap_opt_t *pc_opt);

/** Checks the default powercap in powermon */
#define is_powercap_unlimited() (powermon_get_baseline_powercap() == POWER_CAP_UNLIMITED)

/** returns true if the powercap management is enabled and a powercap limit is set */
int is_powercap_on(node_powercap_opt_t *pc_opt);

/** Returns the powercap limit */
uint get_powercapopt_value(node_powercap_opt_t *pc_opt);
uint get_powercap_allocated(node_powercap_opt_t *pc_opt);
uint compute_extra_gpu_power(uint current, uint diff, uint target);

/** Given a current power , when running an application, returns the powercap status. It must be used only when powercap
 * is set */
uint compute_power_status(node_powercap_opt_t *pc, uint current_power);

void powercap_status_to_str(uint s, char *b);
uint compute_next_status(node_powercap_opt_t *pc, uint current_power, ulong eff_f, ulong req_f);

/** Given a max, min, current and target frequencies it returns the stress level of the subsystem. */
uint powercap_get_stress(uint max_freq, uint min_freq, uint t_freq, uint c_freq);

/** Power/frequency estimator of one device or domain (#814).
 * Power is taken as linear in frequency around the current point:
 *     P(f) = P_now + (f - f_now) * w_per_khz
 * w_per_khz is seeded from the device range, (max_w - min_w) / (max_khz - min_khz),
 * and refined from the measured (power, effective frequency) of each move. */
typedef struct power_estimator {
    double seed;      /* W per kHz from the device range */
    double w_per_khz; /* current slope, kept within [seed / 16, seed * 4] */
    double last_w;    /* last sample */
    ulong last_khz;
} power_estimator_t;

/** min_w may be 0 (unknown): the slope then assumes power falls to 0 at min_khz. */
void power_estimator_init(power_estimator_t *e, uint min_w, uint max_w, ulong min_khz, ulong max_khz);

/** Feeds the power drawn at an effective frequency; refines the slope after a move.
 * Power and frequency moving in opposite directions is a load change, not a clock
 * effect: such a sample is skipped. */
void power_estimator_sample(power_estimator_t *e, double w, ulong khz);

/** Power the device would draw at khz, from what it draws now (w_now at khz_now). */
double power_estimator_at(const power_estimator_t *e, double w_now, ulong khz_now, ulong khz);

/** Watts a device capped at cap_w asks for to draw need_w more: never more than its
 * headroom to max_w, 0 when it needs nothing. */
uint powercap_device_request(uint max_w, uint cap_w, uint need_w);

/** Devices whose hardware reports no maximum cap (wmax 0, e.g. RAPL POWER_INFO without
 * max_power) take their TDP as the maximum. */
void powercap_max_or_tdp(uint32_t *wmax, const uint32_t *tdp, uint n);

#endif
