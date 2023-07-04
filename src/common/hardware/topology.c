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

//#define SHOW_DEBUGS 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <asm/sigcontext.h>
#include <common/sizes.h>
#include <common/states.h>
#include <common/system/file.h>
#include <common/output/debug.h>
#include <common/hardware/topology_asm.h>

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
	
	// Just means merge
	just = (group == TPGroup.merge);
    // Component to merge (by now L3, CORE and SOCKET, but expandable)	
	if (component == TPSelect.l3) {
		addr_offset = ((ulong) &t->cpus[0].l[3].id) - ((ulong) t->cpus);
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
    // Copying base topology data
	memcpy(s, t, sizeof(topology_t));
	// Allocating newer CPUs (more room than required, but OK).
	s->cpus = calloc(t->cpu_count, sizeof(cpu_t));
	// Iterating 
	for (i = 0, c = 0; i < t->cpu_count; ++i)
	{
		// Getting pointer and value of the CPU I
		addr_param = ((ulong) &t->cpus[i]) + addr_offset;
		val1 = (int *) addr_param;
		// Iterating from 0 to I
		for (j = 0; just && j < i; ++j) {
			// Getting pointer and value of the CPU J
			addr_param = (ulong) &t->cpus[j] + addr_offset;
			val2 = (int *) addr_param;
			// If values are equal, break
			if (*val1 == *val2) {
				break;
			}
		}
		// If is merge and the end is reached, then the CPU
		// is copied and the CPUs counter is increased
		if ((just && j == i) || (!just && *val1 == val)) {
			memcpy(&s->cpus[c], &t->cpus[i], sizeof(cpu_t));
			c++;
		}
	}
    // Replacing number of CPUs
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
    int aux3 = 0;
    int fd;
    int i;

    // First settings	
    topo->cpus[thread].id         = thread;
    topo->cpus[thread].apicid     = -1;
    topo->cpus[thread].socket_id  = -1;
    topo->cpus[thread].sibling_id = thread;
    topo->cpus[thread].core_id    = thread;
    topo->cpus[thread].is_thread  =  0;
    // Getting the sibling_id and is_thread
    sprintf(path, "/sys/devices/system/cpu/cpu%d/topology/thread_siblings_list", thread);

    if ((fd = open(path, F_RD)) >= 0) {
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
    		topo->cpus[thread].core_id    = thread;

            if (thread > aux1) {
		        topo->cpus[thread].core_id   = aux1;
		        topo->cpus[thread].is_thread = 1;
		    }
	    }
	    tok = strtok(NULL, ",");
	}
	close(fd);
    } else {
        debug("Failed while opening '%s': %s", path, strerror(errno));
    }
    // Getting the socket_id
	sprintf(path, "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", thread);
	aux1 = 0;
	aux2 = 0;

	if ((fd = open(path, O_RDONLY)) >= 0) {
		do {
			aux2  = pread(fd, (void*) &buffer[aux1], SZ_NAME_LARGE, aux1);
			aux1 += aux2;
		} while(aux2 > 0);

		topo->cpus[thread].socket_id = atoi(buffer);
		close(fd);
	} else {
        debug("Failed while opening '%s': %s", path, strerror(errno));
    }
    // Cleaning caches
	for (i = 0; i < TOPO_CL_COUNT; ++i) {
        topo->cpus[thread].l[i].id = TOPO_UNDEFINED;
    }
    // Getting cache information, iterating by index
    for (i = 0; i < TOPO_CL_COUNT; ++i) {
        sprintf(path, "/sys/devices/system/cpu/cpu%d/cache/index%d/level", thread, i);
        aux1 = 0; // Total bytes
        aux2 = 0; // Returned bytes
        aux3 = 0; // level
        // Getting level
        if ((fd = open(path, O_RDONLY)) >= 0) {
            do {
                aux2  = pread(fd, (void*) &buffer[aux1], SZ_NAME_LARGE, aux1);
                aux1 += aux2;
            } while(aux2 > 0);
            aux3 = atoi(buffer);
	    close(fd);
        } else {
            //debug("Failed while opening '%s': %s", path, strerror(errno));
            continue;
        }
        sprintf(path, "/sys/devices/system/cpu/cpu%d/cache/index%d/id", thread, i);
        aux1 = 0; // Total bytes
        aux2 = 0; // Returned bytes
        // Getting id
        if ((fd = open(path, O_RDONLY)) >= 0) {
            do {
                aux2  = pread(fd, (void*) &buffer[aux1], SZ_NAME_LARGE, aux1);
                aux1 += aux2;
            } while(aux2 > 0);
            topo->cpus[thread].l[aux3].id = (atoi(buffer));
            close(fd);
        } else {
            // We are supposing, if the id file doesn't exists, it is
            //  - L1 and L2, per core
            //  - L3 and L4, per socket
            if (aux3 >= 3) {
                topo->cpus[thread].l[aux3].id = topo->cpus[thread].socket_id;
            } else {
                topo->cpus[thread].l[aux3].id = topo->cpus[thread].core_id;
            }
            //debug("Failed while opening '%s': %s", path, strerror(errno));
        }
        debug("CPU%d L%d id: %d", thread, aux3, topo->cpus[thread].l[aux3].id);
    }
	return EAR_SUCCESS;
}

state_t topology_copy(topology_t *dst, topology_t *src)
{
	void *p;
	p = memcpy(dst, src, sizeof(topology_t));
	p = malloc(sizeof(cpu_t) * src->cpu_count);
	p = memcpy(p, src->cpus, sizeof(cpu_t) * src->cpu_count);
	dst->cpus = (cpu_t *) p;
	return EAR_SUCCESS;
}

static void topology_simd(topology_t *topo)
{
    if (topo->vendor == VENDOR_ARM) {
        #ifdef __SVE_VQ_MAX
        topo->sve_bits = __SVE_VQ_MAX;
        topo->sve = (topo->sve_bits > 0);
        #else
        topo->sve = 0;
        #endif
    }
}

static void topology_watchdog(topology_t *topo)
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

static void topology_cpuinfo(topology_t *topo)
{
    ulong base_freq = 0;
    char *line      = NULL;
    char *col       = NULL; //column
    char *aux       = NULL;
    size_t len      = 0;
    int cpu         = 0;

    // If not found 1GHz --> Replaced by a constant DUMMY to
    // be able to compare in the EARL
    //base_freq = 1000000;
    base_freq = DUMMY_BASE_FREQ;

    FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
    while(getline(&line, &len, cpuinfo) != -1) {
        if (strncmp(line, "apicid", 6) == 0) {
            if ((col = strchr(line, ':')) != NULL) {
                ++col;
                ++col;
                // Now points to the number
                topo->cpus[cpu].apicid = atoi(col);
                ++cpu;
            }
        }
        if ((strncmp(line, "flags", 5) == 0) && !topo->avx512) {
            if ((col = strchr(line, ':')) != NULL) {
                ++col;
                ++col;
                topo->avx512 = (strstr(col, "avx512f") != NULL);
            }
        }
        if (strncmp(line,"model name", 10) == 0){
            if ((col = strchr(line, '@')) != NULL) {
                col++;
                while (*col == ' ') col++;
                aux = col;
                while (*col != 'G') col++;
                *col = '\0';
                // Converted to KHz
                base_freq = atof(aux)*1000000;
            }
        }
    }
    // In case we don't find the base frequency by low level procedures
    if (topo->base_freq == 0) {
        topo->base_freq = base_freq;
    }
    topo->apicids_found = (cpu == topo->cpu_count);
    fclose(cpuinfo);
    free(line);
}

static int is_online(const char *path)
{
	char c = '0';
	if (access(path, F_OK) != 0) {
		return 0;
	}
	if (state_fail(ear_file_read(path, &c, sizeof(char)))) {
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

    if (topo_static.initialized) {
        topology_copy(topo, &topo_static);
        return EAR_SUCCESS;
    }
    // Initial values
    topo->cpu_count        = 0;
    topo->core_count       = 0;
    topo->socket_count     = 0;
    topo->threads_per_core = 1;
    topo->smt_enabled      = 0;
    topo->l2_count         = 0;
    topo->l3_count         = 0;
    topo->l4_count         = 0;
    topo->vendor           = 0;
    topo->family           = 0;
    topo->model            = 0;
    topo->cache_last_level = 0;
    topo->cache_line_size  = 0;
    topo->brand[0]         = '\0';
    topo->avx512           = 0;
    topo->sve              = 0;
    topo->sve_bits         = 0;
    topo->base_freq        = 0;
    topo->boost_enabled    = 0;
    topo->initialized      = 1;
    // Counting number of CPUs
    do {
        sprintf(path, "/sys/devices/system/cpu/cpu%d/online", topo->cpu_count + 1);
	topo->cpu_count += 1;
    } while(is_online(path));
    // Allocating individual CPU memory
    topo->cpus = calloc(topo->cpu_count, sizeof(cpu_t));
    // First assembly characteristics (random CPU, we are supposing all CPUs are the same).
    // In the future, use smp_call_function_single to run ASM code in each CPU.
    topology_asm(topo);	
    // Travelling through all CPUs
    for (i = 0; i < topo->cpu_count; ++i) {
    	topology_init_thread(topo, i);
	// Counting cores
        topo->core_count += !topo->cpus[i].is_thread;
	    if (topo->cpus[i].is_thread) {
	        topo->threads_per_core = 2;
	        topo->smt_enabled = 1;
        }
        // Counting sockets
        for (j = 0; j < i; ++j) {
            if (topo->cpus[j].socket_id == topo->cpus[i].socket_id) {
                break;
            }
        }
        topo->socket_count += (j == i);
        // Counting cache banks
        #define cache_count(level, var) \
        for (j = 0; j <= i; ++j) { \
            if (topo->cpus[j].l[level].id != TOPO_UNDEFINED && \
                topo->cpus[j].l[level].id == topo->cpus[i].l[level].id) { \
                break; \
            } \
        } \
        var += (j == i);

        cache_count(2, topo->l2_count)
        cache_count(3, topo->l3_count)
        cache_count(4, topo->l4_count)
    }
    if (topo->l4_count) {
        topo->cache_last_level = 4;
    } else if (topo->l3_count) {
        topo->cache_last_level = 3;
    } else if (topo->l2_count) {
        topo->cache_last_level = 2;
    } else {
        topo->cache_last_level = 1;
    }
    // TDP definition (to avoid redundancy)
    void topology_tdp(topology_t *topo);

    topology_cpuinfo(topo);
    topology_watchdog(topo);
    topology_simd(topo);
    topology_tdp(topo);

    topology_copy(&topo_static, topo);

    return EAR_SUCCESS;
}

state_t topology_close(topology_t *topo)
{
	return EAR_SUCCESS;
}

#define f_print(f, ...) \
    f (__VA_ARGS__, \
    "cpu_count        : %d\n" \
    "core_count       : %d\n" \
    "socket_count     : %d\n" \
    "threads_per_core : %d\n" \
    "smt_enabled      : %d\n" \
    "l2_count         : %d\n" \
    "l3_count         : %d\n" \
    "l4_count         : %d\n" \
    "cache_line_size  : %d\n" \
    "vendor           : %d\n" \
    "family           : %d\n" \
    "model            : %d\n" \
    "nmi_watchdog     : %d\n" \
    "avx512           : %d\n" \
    "sve              : %d\n" \
    "sve_bits         : %d\n" \
    , \
    topo->cpu_count, \
    topo->core_count, \
    topo->socket_count, \
    topo->threads_per_core, \
    topo->smt_enabled, \
    topo->l2_count, \
    topo->l3_count, \
    topo->l4_count, \
    topo->cache_line_size, \
    topo->vendor, \
    topo->family, \
    topo->model, \
    topo->nmi_watchdog, \
    topo->avx512, \
    topo->sve, \
    topo->sve_bits);

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
    static uint read_from_file = 0;
    char path[1024];
    int fd;

    if (topo_static.base_freq > 0 && read_from_file) {
        *freq_base = topo_static.base_freq;
        return EAR_SUCCESS;
    }
    // Using an alternative method
    sprintf(path,"/sys/devices/system/cpu/cpu%d/cpufreq/scaling_available_frequencies", cpu);
    if ((fd = open(path, O_RDONLY)) < 0) {
        *freq_base = topo_static.base_freq;
        return EAR_ERROR;
    }
    read_from_file = 1;
    ulong f0 = read_freq(fd);
    ulong f1 = read_freq(fd);
    ulong fx = f0 - f1;

    if (fx == 1000) {
        *freq_base = f1;
    } else {
        *freq_base = f0;
    }
    close(fd);

    topo_static.base_freq = *freq_base;

    return EAR_SUCCESS;
}

#if TEST
static topology_t tp;
int main(int argc, char *argv[])
{
    topology_init(&tp);
    topology_print(&tp, verb_channel);
    return 0;
}
#endif
