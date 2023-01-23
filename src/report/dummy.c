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

#include <common/states.h>
#include <common/types/types.h>
#include <common/output/verbose.h>

state_t report_init()
{
	verbose(0, "dummy report_init");
	return EAR_SUCCESS;
}

state_t report_applications(application_t *apps, uint count)
{
	verbose(0, "dummy report_applications");
	return EAR_SUCCESS;
}

state_t report_loops(loop_t *loops, uint count)
{
	verbose(0, "dummy report_loops");
	return EAR_SUCCESS;
}
