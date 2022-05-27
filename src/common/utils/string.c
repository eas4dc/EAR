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

#include <stdio.h>
#include <stdlib.h>
#include <common/sizes.h>
#include <common/utils/string.h>

size_t xsnsize(size_t size)
{
	return size;
}

char *xsnbuffer(char *buffer)
{
    return buffer;
}

int appenv(const char *var, const char *string)
{
	char buffer[SZ_PATH];
	char *p = getenv(var);

	if (p != NULL) {
		sprintf(buffer, "%s:%s", string, p);
	} else {
		sprintf(buffer, "%s", string);
	}
	return setenv(var, buffer, 1);
}
