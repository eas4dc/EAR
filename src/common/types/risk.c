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

#include <common/config.h>
#include <common/states.h>
#include <common/types/risk.h>



state_t set_risk(risk_t *r,risk_t new_r)
{
	*r=new_r;
	return EAR_SUCCESS;
}

int is_risk_set(risk_t r,risk_t value)
{
	return  (r&value);
}

state_t add_risk(risk_t *r,risk_t value)
{
	risk_t new_r;
	new_r=*r;
	*r=new_r|value;
	return EAR_SUCCESS;
}

state_t del_risk(risk_t *r,risk_t value)
{
	risk_t new_r;
	new_r=*r;
	*r=new_r&~value;
	return EAR_SUCCESS;
}

risk_t get_risk(char *risk)
{
    if (!strcasecmp(risk, "WARNING1")) return WARNING1;
    if (!strcasecmp(risk, "WARNING2")) return WARNING2;
    if (!strcasecmp(risk, "PANIC")) return PANIC;
    return 0;
}

unsigned int get_target(char *target)
{
    if (!strcasecmp(target, "ENERGY")) return ENERGY;
    if (!strcasecmp(target, "POWER")) return POWER;
    return 0;

}
