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



state_t init_power_models(uint user_type,conf_install_t *data,architecture_t *arch_desc)
{
	state_t st;
	st = projections_init(user_type,data,arch_desc);
	debug("Projections for %d pstates",arch_desc->pstates);
	projection_create(arch_desc->pstates);
	projection_reset(arch_desc->pstates);
	return st;
}



