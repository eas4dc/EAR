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

#include <common/states.h>
#include <common/types/types.h>
#include <common/output/verbose.h>
#include <daemon/local_api/eard_api.h>

state_t report_init()
{
	debug("eard report_init");
	return EAR_SUCCESS;
}

state_t report_applications(application_t *apps, uint count)
{
	int i;
	debug("eard report_applications");
	if (!eards_connected()) return EAR_ERROR;
	if ((apps == NULL) || (count == 0)) return EAR_SUCCESS;
	for (i=0;i<count;i++){
		eards_write_app_signature(&apps[i]);
	}
	return EAR_SUCCESS;
}

state_t report_loops(loop_t *loops, uint count)
{
	int i;
	debug("eard report_loops");
	if (!eards_connected()) return EAR_ERROR;
	if ((loops == NULL) || (count == 0)) return EAR_SUCCESS;
	for (i=0;i<count;i++){
		eards_write_loop_signature(&loops[i]);
	}
	return EAR_SUCCESS;
}
