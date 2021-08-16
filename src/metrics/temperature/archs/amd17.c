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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <common/sizes.h>
#include <common/output/debug.h>
#include <metrics/common/hwmon.h>
#include <metrics/temperature/archs/amd17.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static uint socket_count;

typedef struct hwfds_s {
	uint  id_count;
	uint *fd_count;
	int **fds;
} hwfds_t;

state_t temp_amd17_status(topology_t *topo)
{
	state_t s;
	debug("asking for status");
	if (topo->vendor != VENDOR_AMD || topo->family < FAMILY_ZEN) {
		return EAR_ERROR;
	}
	while (pthread_mutex_trylock(&lock));
	if (socket_count == 0) {
		if (xtate_ok(s, hwmon_find_drivers("k10temp", NULL, NULL))) {
			socket_count = topo->socket_count;
		}
	}
	pthread_mutex_unlock(&lock);
	return s;
}

state_t temp_amd17_init(ctx_t *c)
{
	uint id_count;
	uint *ids;
	state_t s;
	int i;

	debug("asking for init");
	if (c == NULL) {
		return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
	}
	// Looking for ids
	if (xtate_fail(s, hwmon_find_drivers("k10temp", &ids, &id_count))) {
		return s;
	}
	// Allocating context
	if ((c->context = calloc(1, sizeof(hwfds_t))) == NULL) {
		return_msg(s, strerror(errno));
	}
	// Allocating file descriptors)
	hwfds_t *h = (hwfds_t *) c->context;
	
	if ((h->fds = calloc(id_count, sizeof(int *))) == NULL) {
		return_msg(s, strerror(errno));
	}
	if ((h->fd_count = calloc(id_count, sizeof(uint))) == NULL) {
		return_msg(s, strerror(errno));
	}
	h->id_count = id_count;
	//
	for (i = 0; i < h->id_count; ++i) {
		if (xtate_fail(s, hwmon_open_files(ids[i], Hwmon.temp_input, &h->fds[i], &h->fd_count[i]))) {
			return s;
		}
	}
	// Freeing ids space
	free(ids);
	debug("init succeded");
	return EAR_SUCCESS;
}

static state_t get_context(ctx_t *c, hwfds_t **h)
{
	if (c == NULL || c->context == NULL) {
		return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
	}
	*h = (hwfds_t *) c->context;
	return EAR_SUCCESS;
}

state_t temp_amd17_dispose(ctx_t *c)
{
	int i;
	hwfds_t *h;
	state_t s;

	if (xtate_fail(s, get_context(c, &h))) {
		return s;
	}
	for (i = 0; i < h->id_count; ++i) {
		hwmon_close_files(h->fds[i], h->fd_count[i]);
	}
	c->context = NULL;
	free(h->fd_count);
	free(h->fds);

	return EAR_SUCCESS;
}

// Data
state_t temp_amd17_count_devices(ctx_t *c, uint *count)
{
	hwfds_t *h;
	state_t s;

	if (xtate_fail(s, get_context(c, &h))) {
		return s;
	}
	if (count != NULL) {
		*count = socket_count;
	}

	return EAR_SUCCESS;
}

// Getters
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

state_t temp_amd17_read(ctx_t *c, llong *temp, llong *average)
{
	char data[SZ_PATH];
	llong aux1, aux2;
	int i, j, k;
	hwfds_t *h;
	state_t s;

	if (xtate_fail(s, get_context(c, &h))) {
		return s;
	}
	for (k = 0, aux1 = aux2 = 0; k < socket_count && temp != NULL; ++k) {
		temp[k] = 0;
	}
	for (i = 0, k = 0; i < h->id_count && k < socket_count; ++i) {
		for (j = 0; j < h->fd_count[i] && k < socket_count; ++j, ++k) {
			if (xtate_fail(s, hwmon_read(h->fds[i][j], data, SZ_PATH))) {
				return s;
			}
			debug("read %s", data);
			aux1  = parse(data, 2);
			aux2 += aux1;
			if (temp != NULL) {
				temp[k] = aux1;
			}
		}
	}
	//
	if (average != NULL) {
		*average = aux2 / (llong) socket_count;
	}

	return EAR_SUCCESS;
}
