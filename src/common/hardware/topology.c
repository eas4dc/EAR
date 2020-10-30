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
#include <string.h>
#include <unistd.h>
#include <common/sizes.h>
#include <common/states.h>
#include <common/system/file.h>
#include <common/hardware/cpuid.h>
#include <common/hardware/topology.h>
#include <common/hardware/topology_frequency.h>

// TODO: clean, just for fix spaguettis
static topology_t topo_static;

static int file_is_accessible(const char *path)
{
	return (access(path, F_OK) == 0);
}

static int is_online(const char *path)
{
	char c = '0';
	if (access(path, F_OK) != 0) {
		return 0;
	}
	if (state_fail(file_read(path, &c, sizeof(char)))) {
		return 0;
	}
	if (c != '1') {
		return 0;
	}
	return 1;
}



state_t topology_select(topology_t *t, topology_t *s, int component, int group, int val)
{
	ulong addr_offset;
	ulong addr_param;
	int *val1;
	int *val2;
	int just;
	int i;
	int j;
	int c;

	just = (group == TPGroup.merge);

	if (component == TPSelect.l3) {
		addr_offset = ((ulong) &t->cpus[0].l3_id) - ((ulong) t->cpus);
	}
	if (component == TPSelect.core && group == TPGroup.merge)
	{
		addr_offset = ((ulong) &t->cpus[0].is_thread) - ((ulong) t->cpus);
		just = 0;	
		val  = 0;
	}
	if (component == TPSelect.socket) {
		addr_offset = ((ulong) &t->cpus[0].socket_id) - ((ulong) t->cpus);
	}

	s->cpus = malloc(t->cpu_count * sizeof(core_t));

	for (i = 0, c = 0; i < t->cpu_count; ++i)
	{		
		addr_param = ((ulong) &t->cpus[i]) + addr_offset;
		val1 = (int *) addr_param;

		for (j = 0; just && j < i; ++j) {
			addr_param = (ulong) &t->cpus[j] + addr_offset;
			val2 = (int *) addr_param;
			if (*val1 == *val2) {
				break;
			}
		}
	
		if ((just && j == i) || (!just && *val1 == val)) {
			memcpy(&s->cpus[c], &t->cpus[i], sizeof(core_t));
			c++;
		}

		s->cpus[i].freq_base = t->cpus[i].freq_base;
	}

	s->socket_count = t->socket_count;
	s->cpu_count = c;
	
	return EAR_SUCCESS;
}

static state_t topology_init_thread(topology_t *topo, uint thread)
{
	char buffer[SZ_NAME_LARGE];
	char path[SZ_NAME_LARGE];
	int aux1 = 0;
	int aux2 = 0;
	int fd;

	// First settings	
	topo->cpus[thread].id         = thread;
	topo->cpus[thread].socket_id  =  0;
	topo->cpus[thread].sibling_id = -1;
	topo->cpus[thread].is_thread  =  0;
	topo->cpus[thread].l3_id      = -1;

	// Getting the sibling_id and is_thread
	sprintf(path, "/sys/devices/system/cpu/cpu%d/topology/thread_siblings_list", thread);
	fd = open(path, F_RD);

	if (fd >= 0)
	{	
		do {
			aux2  = pread(fd, (void*) &buffer[aux1], SZ_NAME_LARGE, aux1);
			aux1 += aux2;
		} while(aux2 > 0);

		// Parsing
		char *tok = strtok(buffer, ",");

		//
		while (tok != NULL) {
			if ((aux1 = atoi(tok)) != thread) {
				topo->cpus[thread].sibling_id = aux1;
				if (thread > aux1) {
					topo->cpus[thread].is_thread = 1;
				}
			}
			tok = strtok(NULL, ",");
		}

		close(fd);
	}
	
	// Getting the socket_id
	sprintf(path, "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", thread);

	fd = open(path, F_RD);
	aux1 = 0;
	aux2 = 0;

	if (fd >= 0)
	{	
		do {
			aux2  = pread(fd, (void*) &buffer[aux1], SZ_NAME_LARGE, aux1);
			aux1 += aux2;
		} while(aux2 > 0);

		topo->cpus[thread].socket_id = atoi(buffer);
	
		close(fd);
	}

	// Getting the l3_id (TODO: explore all the indexes in the future)
	sprintf(path, "/sys/devices/system/cpu/cpu%d/cache/index3/id", thread);

	fd = open(path, F_RD);
	aux1 = 0;
	aux2 = 0;

	if (fd >= 0)
	{
		do {
			aux2  = pread(fd, (void*) &buffer[aux1], SZ_NAME_LARGE, aux1);
			aux1 += aux2;
		} while(aux2 > 0);

		topo->cpus[thread].l3_id = atoi(buffer);
		
		close(fd);
	}

	return EAR_SUCCESS;
}

static void topology_copy(topology_t *dst, topology_t *src)
{
	void *p;
	p = memcpy(dst, src, sizeof(topology_t));
	p = malloc(sizeof(core_t) * src->cpu_count);
	p = memcpy(p, src->cpus, sizeof(core_t) * src->cpu_count);
	src->cpus = (core_t *) p;
}

static void topology_cpuid(topology_t *topo)
{
	int buffer[4];
	cpuid_regs_t r;

	/* Vendor */
	CPUID(r,0,0);
	buffer[0] = r.ebx;
	buffer[1] = r.edx;
	buffer[2] = r.ecx;
	topo->vendor = !(buffer[0] == 1970169159);
	
	/* Family */
	CPUID(r,1,0);
	buffer[0] = cpuid_getbits(r.eax, 11,  8);
	buffer[1] = cpuid_getbits(r.eax, 27, 20);
	topo->family = (buffer[1] << 4) | buffer[0];

	/* Model */
	CPUID(r,1,0);
	buffer[0] = cpuid_getbits(r.eax,  7,  4);
	buffer[1] = cpuid_getbits(r.eax, 19, 16);
	topo->model = (buffer[1] << 4) | buffer[0];
}

state_t topology_init(topology_t *topo)
{
	char path[SZ_NAME_LARGE];
	int i;

	// TODO: spaguettis
	if (topo_static.cpu_count != 0) {
		topology_copy(topo, &topo_static);
	}

	topo->cpu_count = 0;
	topo->core_count = 0;
	topo->socket_count = 0;
	topo->threads_per_core = 1;
	topo->smt_enabled = 0;
	topo->l3_count = 0;

	/* First characteristics */
	topology_cpuid(topo);

	/* Number of CPUs */
	do {
		sprintf(path, "/sys/devices/system/cpu/cpu%d/online", topo->cpu_count + 1);
		topo->cpu_count += 1;
	}
	while(is_online(path));
	
	//
	topo->cpus = calloc(topo->cpu_count, sizeof(core_t));
	
	for (i = 0; i < topo->cpu_count; ++i)
	{
		topology_init_thread(topo, i);

		topo->core_count += !topo->cpus[i].is_thread;
		if (topo->cpus[i].socket_id > (topo->socket_count-1)) {
			topo->socket_count = topo->cpus[i].socket_id + 1;
		}
		if (topo->cpus[i].is_thread) {
			topo->threads_per_core = 2;
			topo->smt_enabled = 1;
		}
		if (topo->cpus[i].l3_id > (topo->l3_count-1)) {
			topo->l3_count = topo->cpus[i].l3_id + 1;
		}
	
		// Base frequency	
		topology_freq_getbase(i, &topo->cpus[i].freq_base);
	}

	// TODO: spaguettis
	topology_copy(&topo_static, topo);

	return EAR_SUCCESS;
}

state_t topology_close(topology_t *topo)
{
	// TODO: spaguettis
	return EAR_SUCCESS;

	if (topo == NULL) {
		return EAR_ERROR;
	}
	
	free(topo->cpus);
	memset(topo, 0, sizeof(topology_t));
	return EAR_SUCCESS;
}

