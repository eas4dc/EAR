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

#include <management/cpufreq/governor.h>

state_t mgt_governor_tostr(uint governor, char *buffer)
{
	if (governor == Governor.conservative) {
		sprintf(buffer, "%s", Goverstr.conservative);
	} else if (governor == Governor.performance) {
		sprintf(buffer, "%s", Goverstr.performance);
	} else if (governor == Governor.userspace) {
		sprintf(buffer, "%s", Goverstr.userspace);
	} else if (governor == Governor.powersave) {
		sprintf(buffer, "%s", Goverstr.powersave);
	} else if (governor == Governor.ondemand) {
		sprintf(buffer, "%s", Goverstr.ondemand);
	} else {
		sprintf(buffer, "%s", Goverstr.other);
		return_msg(EAR_ERROR, "undefined governor");
	}
	return EAR_SUCCESS;
}

state_t mgt_governor_toint(char *buffer, uint *governor)
{
	if (strncmp(buffer, Goverstr.conservative, 12) == 0) {
		*governor = Governor.conservative;
	} else if (strncmp(buffer, Goverstr.performance, 11) == 0) {
		*governor = Governor.performance;
	} else if (strncmp(buffer, Goverstr.userspace, 9) == 0) {
		*governor = Governor.userspace;
	} else if (strncmp(buffer, Goverstr.powersave, 9) == 0) {
		*governor = Governor.powersave;
	} else if (strncmp(buffer, Goverstr.ondemand, 8) == 0) {
		*governor = Governor.ondemand;
	} else {
		*governor = Governor.other;
		return_msg(EAR_ERROR, "undefined governor");
	}
	return EAR_SUCCESS;
}

int mgt_governor_is(char *buffer, uint governor)
{
    uint aux;
    if (state_fail(mgt_governor_toint(buffer, &aux))) {
        return 0;
    }
    return (aux == governor);
}
