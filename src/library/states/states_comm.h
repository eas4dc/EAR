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

#ifndef _STATE_COMMON_H
#define _STATE_COMMON_H
//#define SHOW_DEBUGS 1
#include <common/config.h>
#include <common/types/loop.h>

#define NO_PERIOD       0
#define FIRST_ITERATION     1
#define EVALUATING_LOCAL_SIGNATURE  2
#define SIGNATURE_STABLE    3
#define PROJECTION_ERROR    4
#define RECOMPUTING_N     5
#define SIGNATURE_HAS_CHANGED 6
#define TEST_LOOP   7
#define EVALUATING_GLOBAL_SIGNATURE 8


void state_verbose_signature(loop_t *sig,int master_rank,int show,char *aname,char *nname,int iterations,ulong prevf,ulong new_freq,char *mode);
void state_report_traces(int master_rank, int my_rank,int lid, loop_t *lsig,ulong freq,ulong status);
void state_report_traces_state(int master_rank, int my_rank,int lid,ulong status);
void state_print_policy_state(int master_rank,int st);
#endif

