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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <common/config.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/hardware/architecture.h>
#include <common/types/projection.h>
#include <common/types/application.h>
#include <library/common/externs.h>
#include <library/models/models.h>

/* This variable enables/disables the utilization of energy models in power policies */
extern uint use_energy_models;

state_t init_power_models(uint user_type,conf_install_t *data,architecture_t *arch_desc)
{
	state_t st;
	st = projections_init(user_type,data,arch_desc);
	debug("Projections for %d pstates",arch_desc->pstates);
	projection_create(arch_desc->pstates);
	projection_reset(arch_desc->pstates);
	if (projections_available() == EAR_ERROR) use_energy_models = 0;
	return st;
}



