/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#ifndef COMMON_APIS_SUSCRIPTOR_H
#define COMMON_APIS_SUSCRIPTOR_H

#include <common/types.h>
#include <common/states.h>
#include <common/system/time.h>

typedef state_t (*suscall_f) (void *);
typedef state_t (*suscribe_f) (void *);

typedef struct suscription_s
{
	suscribe_f 	suscribe;
	suscall_f	call_init;
	suscall_f	call_main;
	void		*memm_init;
	void		*memm_main;
	int			time_relax; // In miliseconds.
	int			time_burst; // In miliseconds.
	int 		*bursting;
    int			id;
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
//	s = monitor_burst(sus);
//	s = monitor_relax(sus);

state_t monitor_init();

state_t monitor_dispose();

state_t monitor_register(suscription_t *suscription);

state_t monitor_unregister(suscription_t *suscription);

state_t monitor_burst(suscription_t *suscription);

state_t monitor_relax(suscription_t *suscription);

int monitor_is_bursting(suscription_t *s);

suscription_t *suscription();

#endif //EAR_STASH_MONITOR_H
