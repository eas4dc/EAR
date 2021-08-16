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
#include <common/output/debug.h>
#include <common/hardware/cpuid.h>
#include <common/hardware/topology.h>

static topology_t topo_static;

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
	}

	s->socket_count = t->socket_count;
	s->cpu_count = c;

	if (s->cpu_count <= 0) {
		return_msg(EAR_ERROR, "invalid topology");
	}
	
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

state_t topology_copy(topology_t *dst, topology_t *src)
{
	void *p;
	p = memcpy(dst, src, sizeof(topology_t));
	p = malloc(sizeof(core_t) * src->cpu_count);
	p = memcpy(p, src->cpus, sizeof(core_t) * src->cpu_count);
	dst->cpus = (core_t *) p;
	return EAR_SUCCESS;
}

static void topology_extras(topology_t *topo)
{
	char path[SZ_NAME_LARGE];
	char c[2];
	int fd;
	
	// NMI Watchdog
	sprintf(path, "/proc/sys/kernel/nmi_watchdog");

	if ((fd = open(path, F_RD)) >= 0) {
		if (read(fd, c, sizeof(char)) > 0) {
			c[1] = '\0';
			topo->nmi_watchdog = atoi(c);
		}
		close(fd);
	}
}

static void topology_cpuid(topology_t *topo)
{
	cpuid_regs_t r;
	int buffer[4];

	/* Vendor */
	CPUID(r,0,0);
	buffer[0] = r.ebx;
	buffer[1] = r.edx;
	buffer[2] = r.ecx;
	topo->vendor = !(buffer[0] == 1970169159); // Intel
	
	/* Family */
	CPUID(r,1,0);
	buffer[0] = cpuid_getbits(r.eax, 11,  8);
	buffer[1] = cpuid_getbits(r.eax, 27, 20); // extended
	buffer[2] = buffer[0]; // auxiliar
	
	if (buffer[0] == 0x0F) {
		topo->family = buffer[1] + buffer[0];
	} else {
		topo->family = buffer[0];
	}

	/* Model */
	CPUID(r,1,0);
	buffer[0] = cpuid_getbits(r.eax,  7,  4);
	buffer[1] = cpuid_getbits(r.eax, 19, 16); // extended
	
	if (buffer[2] == 0x0F || (buffer[2] == 0x06 && topo->vendor == VENDOR_INTEL)) {
		topo->model = (buffer[1] << 4) | buffer[0];
	} else {
		topo->model = buffer[0];
	}

	/* Cache line size */
	uint max_level = 0;
	uint cur_level = 0;
	int index      = 0;

	if (topo->vendor == VENDOR_INTEL)
	{
		while (1)
		{
			CPUID(r,4,index);

			if (!(r.eax & 0x0F)) break;
			cur_level = cpuid_getbits(r.eax, 7, 5);

			if (cur_level >= max_level) {
				topo->cache_line_size = cpuid_getbits(r.ebx, 11, 0) + 1;
				max_level = cur_level;
			}

			index = index + 1;
		}
	} else {
		CPUID(r,0x80000005,0);
		topo->cache_line_size = r.edx & 0xFF;
	}

	/* General-purpose/fixed registers */
    CPUID(r, 0x0a, 0);

	if (topo->vendor == VENDOR_INTEL) {
		//  Intel Vol. 2A
		// 	bits   7,0: version of architectural performance monitoring
		// 	bits  15,8: number of GPR counters per logical processor
		//	bits 23,16: bit width of GPR counters
		topo->gpr_count = cpuid_getbits(r.eax, 15,  8);
		topo->gpr_bits  = cpuid_getbits(r.eax, 23, 16);
	} else {
		if (topo->family >= FAMILY_ZEN) {
			topo->gpr_count = 6;
			topo->gpr_bits  = 48;
		}
	}
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

state_t topology_init(topology_t *topo)
{
	char path[SZ_NAME_LARGE];
	int i, j;

	if (topo_static.cpu_count != 0) {
		topology_copy(topo, &topo_static);
	}

	topo->cpu_count = 0;
	topo->core_count = 0;
	topo->socket_count = 0;
	topo->threads_per_core = 1;
	topo->cache_line_size = 0;
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
		// 0 1 2 3
		for (j = 0; j < i; ++j) {
			if (topo->cpus[j].l3_id == topo->cpus[i].l3_id) {
				break;
			}
		}
		topo->l3_count += (j == i);
		//if (topo->cpus[i].l3_id > (topo->l3_count-1)) {
		//	topo->l3_count = topo->cpus[i].l3_id + 1;
		//}
	}

	topology_extras(topo);

	topology_copy(&topo_static, topo);

	return EAR_SUCCESS;
}

state_t topology_close(topology_t *topo)
{
	return EAR_SUCCESS;

	if (topo == NULL) {
		return EAR_ERROR;
	}
	
	free(topo->cpus);
	memset(topo, 0, sizeof(topology_t));
	return EAR_SUCCESS;
}

#define f_print(f, ...) \
	f (__VA_ARGS__, \
		"cpu_count        : %d\n" \
    	"socket_count     : %d\n" \
		"threads_per_core : %d\n" \
		"smt_enabled      : %d\n" \
    	"l3_count         : %d\n" \
		"vendor           : %d\n" \
		"family           : %d\n" \
		"gpr_count        : %d\n" \
		"gpr_bits         : %d\n" \
		"nmi_watchdog     : %d\n" \
	, \
		topo->cpu_count, \
		topo->socket_count, \
		topo->threads_per_core, \
		topo->smt_enabled, \
		topo->l3_count, \
		topo->vendor, \
		topo->family, \
		topo->gpr_count, \
		topo->gpr_bits, \
		topo->nmi_watchdog);

state_t topology_print(topology_t *topo, int fd)
{
	f_print(dprintf, fd);

	return EAR_SUCCESS;
}

state_t topology_tostr(topology_t *topo, char *buffer, size_t n)
{
	f_print(snprintf, buffer, n);

	return EAR_SUCCESS;
}

static ulong read_freq(int fd)
{
	char freqc[8];
	ulong freq;
	int i = 0;
	char c;

	while((read(fd, &c, sizeof(char)) > 0) && ((c >= '0') && (c <= '9')))
	{
		freqc[i] = c;
    	i++;
	}

	freqc[i] = '\0';
	freq = atoi(freqc);
	return freq;
}

state_t topology_freq_getbase(uint cpu, ulong *freq_base)
{
	char path[1024];
	int fd;

	sprintf(path,"/sys/devices/system/cpu/cpu%d/cpufreq/scaling_available_frequencies", cpu);
	if ((fd = open(path, O_RDONLY)) < 0)
	{
		*freq_base = 1000000LU;
    	return EAR_ERROR;
	}

	ulong f0 = read_freq(fd);
	ulong f1 = read_freq(fd);
	ulong fx = f0 - f1;

	if (fx == 1000) {
		*freq_base = f1;
	} else {
		*freq_base = f0;
	}

	close(fd);

	return EAR_SUCCESS;
}

