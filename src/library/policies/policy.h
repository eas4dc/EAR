/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING
*/

#ifndef EAR_POLICIES_H
#define EAR_POLICIES_H

#include <common/states.h>
#include <common/types/loop.h>
#include <common/types/projection.h>
#include <daemon/shared_configuration.h>

state_t init_power_policy(settings_conf_t *app_settings,resched_t *res);
state_t policy_init();
state_t policy_apply(signature_t *my_sig,ulong *freq_set, int *ready);
state_t policy_get_default_freq(ulong *freq_set);
state_t policy_set_default_freq();
state_t policy_ok(signature_t *c_sig,signature_t *l_sig,int *ok);
state_t policy_max_tries(int *intents);
state_t policy_end();
state_t policy_loop_init(loop_id_t *loop_id);
state_t policy_loop_end(loop_id_t *loop_id);
state_t policy_new_iteration(loop_id_t *loop_id);
state_t policy_mpi_init();
state_t policy_mpi_end();

state_t policy_force_global_frequency(ulong new_f);

/* These two functions needs to be reconsidered*/
state_t policy_configure();
#endif //EAR_POLICIES_H
