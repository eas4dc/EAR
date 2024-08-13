/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
