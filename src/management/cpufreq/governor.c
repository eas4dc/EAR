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

#include <common/plugins.h>
#include <management/cpufreq/governor.h>

char *governor_tostr(uint governor, char *buffer)
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
	}
	return buffer;
}

state_t governor_toint(char *buffer, uint *governor)
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

int governor_is(char *buffer, uint governor)
{
    uint aux;
    if (state_fail(mgt_governor_toint(buffer, &aux))) {
        return 0;
    }
    return (aux == governor);
}
