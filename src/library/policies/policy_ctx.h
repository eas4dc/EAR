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

#ifndef EAR_POLICIES_CTX_H
#define EAR_POLICIES_CTX_H

#include <common/states.h>
#include <common/types/loop.h>
#include <common/types/application.h>
#include <common/types/signature.h>
#include <common/types/projection.h>
#include <daemon/shared_configuration.h>
#include <mpi.h>

typedef struct mpi_ctx{
	MPI_Comm 	master_comm;
	MPI_Comm 	comm;
}mpi_ctx_t;

typedef struct policy_context {
	settings_conf_t *app;
	resched_t *reconfigure;
	unsigned long user_selected_freq;
	unsigned long reset_freq_opt;
	unsigned long *ear_frequency;
	unsigned int  num_pstates;
	unsigned int use_turbo;
	mpi_ctx_t mpi;
	//job_t 		*my_job
} polctx_t;

void print_policy_ctx(polctx_t *p);

#endif //EAR_POLICIES_H
