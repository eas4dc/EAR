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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <common/config.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/hardware/frequency.h>
#include <common/types/projection.h>
#include <library/policies/policy_api.h>
#if POWERCAP
#include <daemon/powercap_status.h>
#endif

static uint last_pc=0;

#ifdef EARL_RESEARCH
extern unsigned long ext_def_freq;
#define DEF_FREQ(f) (!ext_def_freq?f:ext_def_freq)
#else
#define DEF_FREQ(f) f
#endif



state_t policy_init(polctx_t *c)
{
	return EAR_SUCCESS;
}
state_t policy_apply(polctx_t *c,signature_t *my_sig, ulong *new_freq,int *ready)
{
	ulong eff_f,f;
	
	*ready=1;
	f=DEF_FREQ(c->app->def_freq);
	*new_freq=f;
	
	return EAR_SUCCESS;
}
state_t policy_ok(polctx_t *c, signature_t *curr_sig,signature_t *prev_sig,int *ok)
{
	ulong eff_f;
	uint power_status,next_status;
	*ok=1;

	return EAR_SUCCESS;
}


state_t policy_get_default_freq(polctx_t *c, ulong *freq_set)
{
	*freq_set=DEF_FREQ(c->app->def_freq);
	return EAR_SUCCESS;
}

state_t policy_max_tries(polctx_t *c,int *intents)
{
	*intents=0;
	return EAR_SUCCESS;
}


