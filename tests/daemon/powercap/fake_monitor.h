/*********************************************************************
 * Copyright (c) 2026 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

/*
 * fake_monitor.h -- deterministic replacement for common/system/monitor.c
 * (implemented in common_stubs.c). Subscriptions are recorded, never run by a
 * thread: the test drives them with fake_init_all()/fake_tick_all().
 */
#ifndef FAKE_MONITOR_H
#define FAKE_MONITOR_H

#include <common/system/monitor.h>

#define FAKE_MAX_SUBS 16

extern suscription_t *fake_subs[FAKE_MAX_SUBS];
extern int fake_sub_state[FAKE_MAX_SUBS];
extern int fake_num_subs;
extern int fake_unregister_calls;

suscription_t *fake_find_sub(suscall_f main_fn);
void fake_init_all(void);
void fake_tick_all(void);

/* Frozen clock (common_stubs.c): -1 = real time, otherwise nanoseconds */
extern long long fake_clock_ns;

#endif
