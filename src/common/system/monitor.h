/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_APIS_SUSCRIPTOR_H
#define COMMON_APIS_SUSCRIPTOR_H

#include <common/states.h>
#include <common/system/time.h>
#include <common/types.h>

typedef state_t (*suscall_f)(void *);
typedef state_t (*suscribe_f)(void *);

typedef struct suscription_s {
    suscribe_f suscribe;
    suscall_f call_init;
    suscall_f call_main;
    void *memm_init;
    void *memm_main;
    int time_relax; // In miliseconds.
    int time_burst; // In miliseconds.
    int id;
} suscription_t;

// Example (test will receive NULL):
//	state_t test(void *arg);
//
// 	suscription_t *sus;
// 	state_t s;
//
// 	sus = suscription();
//	sus->call_main  = test;
//	sus->time_relax = 1000;
//	sus->time_burst =  300;
//
//	s = monitor_init();
//	s = monitor_register(sus);
//	s = monitor_burst(sus, MON_NO_INTERRUPT);
//	s = monitor_relax(sus);

state_t monitor_init();

state_t monitor_dispose();

// Waits until monitor finalizes
void monitor_wait();

state_t monitor_register(suscription_t *suscription);

state_t monitor_unregister(suscription_t *suscription);

// If interrupt = 1, then wakes up the monitor to compute
state_t monitor_burst(suscription_t *s, uint interrupt);

state_t monitor_relax(suscription_t *suscription);

int monitor_is_initialized();

int monitor_is_bursting(suscription_t *s);

suscription_t *suscription();

#endif // EAR_STASH_MONITOR_H
