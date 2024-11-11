/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <common/sizes.h>
#include <common/output/debug.h>
#include <metrics/common/hwmon.h>
#include <metrics/temperature/archs/amd17.h>

static uint  socket_count;
static uint  id_count;
static uint *fd_count;
static int **fds;

TEMP_F_LOAD(amd17)
{
	if (tp->vendor != VENDOR_AMD || tp->family < FAMILY_ZEN) {
		return;
	}
    if (state_fail(hwmon_find_drivers("k10temp", NULL, NULL))) {
        return;
    }
    socket_count = tp->socket_count;
    apis_set(ops->get_info, temp_amd17_get_info);
    apis_set(ops->init,     temp_amd17_init);
    apis_set(ops->dispose,  temp_amd17_dispose);
    apis_set(ops->read,     temp_amd17_read);
}

TEMP_F_GET_INFO(amd17)
{
    info->api         = API_AMD17;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_SOCKET;
    info->devs_count  = socket_count;
}

TEMP_F_INIT(amd17)
{
	uint *ids;
	state_t s;
	int i;

	// Looking for ids
	if (state_fail(s = hwmon_find_drivers("k10temp", &ids, &id_count))) {
		return s;
	}
    fds = calloc(id_count, sizeof(int *));
    fd_count = calloc(id_count, sizeof(uint));
	//
	for (i = 0; i < id_count; ++i) {
		// Starting at id=3 because 1 is Tctl, 2 is Tdie (if available), used for cooling control.
		if (state_fail(s = hwmon_open_files(ids[i], Hwmon.temp_input, &fds[i], &fd_count[i], 3))) {
			return s;
		}
	}
	free(ids);
	return EAR_SUCCESS;
}

TEMP_F_DISPOSE(amd17)
{
    int i;
	if (fds != NULL) {
		for (i = 0; i < id_count; ++i) {
            hwmon_close_files(fds[i], fd_count[i]);
        }
        free(fd_count);
        free(fds);
        // Pointing to NULL to show is not allocated
        fd_count = NULL;
        fds = NULL;
    }
}

static llong parse(char *buffer, int n)
{
	char aux[SZ_NAME_SHORT];
	int i = 0;
	if (n < 0) {
		n = -n;
		i = sizeof(buffer) - n;
	}
	while(n > 0)
	{
		aux[i] = buffer[i];
		++i;
		--n;
	}
	aux[i] = '\0';
	debug("parsed word '%s'", aux);
	return atoll(aux);
}

TEMP_F_READ(amd17)
{
	char data[SZ_PATH];
    llong aux1 = 0LL;
    llong aux2 = 0LL;
	llong aux3 = 0LL;
	int i, j, k;
	state_t s;

	for (k = 0; k < socket_count && temp != NULL; ++k) {
		temp[k] = 0;
	}
	for (i = 0, k = 0; i < id_count && k < socket_count; ++i) {
		for (j = 0; j < fd_count[i]; ++j)
		{
			if (state_fail(s = hwmon_read(fds[i][j], data, SZ_PATH))) {
				return s;
			}
			debug("read %s", data);
			aux1  = parse(data, 2);
			aux2 += aux1;
		}
		aux2 /= (llong)fd_count[i];
		debug("avg temperature for socket %d: %lld", i, aux2);
		if (temp != NULL)
		{
			temp[k] = aux2;
			}
		++k;
		aux3 += aux2;
	}
	if (average != NULL)
	{
		*average = aux3 / (llong) socket_count;
		debug("average temperature of all sockets: %llu", *average);
	}
	return EAR_SUCCESS;
}
