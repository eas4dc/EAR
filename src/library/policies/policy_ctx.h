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

#ifndef EAR_POLICIES_CTX_H
#define EAR_POLICIES_CTX_H

#include <common/states.h>
#include <common/types/loop.h>
#include <common/types/application.h>
#include <common/types/signature.h>
#include <common/types/projection.h>
#include <daemon/shared_configuration.h>
#if MPI
#include <mpi.h>
#endif

#if MPI
typedef struct mpi_ctx{
	MPI_Comm 	master_comm;
	MPI_Comm 	comm;
}mpi_ctx_t;
#else
typedef struct mpi_ctx{
  int  master_comm;
  int  comm;
}mpi_ctx_t;
#endif

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
