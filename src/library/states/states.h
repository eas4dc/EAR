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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef _EAR_BASE_H_
#define _EAR_BASE_H
#include <common/types.h>

/** Executed at application start. Reads EAR configuration environment variables. */
void states_begin_job(int my_id, char *app_name);

/** Executed at NEW_LOOP or END_NEW_LOOP DynAIS event */
void states_begin_period(int my_id, ulong event, ulong size, ulong level);

/** Executed at NEW_ITERATION DynAIS event */
void states_new_iteration(int my_id, uint size, uint iterations, uint level, ulong event,ulong mpi_calls_iter,uint dynais_used);

/** Executed at END_LOOP or END_NEW_LOOP DynAIS event */
void states_end_period(uint iterations);

/** Executed at application end */
void states_end_job(int my_id, char *app_name);
#endif
