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

#ifndef _RAPL_METRICS_H_
#define _RAPL_METRICS_H_

#include <metrics/common/omsr.h>

#define RAPL_POWER_EVS		2
#define RAPL_DRAM0          0
#define RAPL_DRAM1          1
#define RAPL_PACKAGE0       2
#define RAPL_PACKAGE1       3
#define RAPL_MSR_UNITS		1000000000.0

#define RAPL_ENERGY_EV        2
#define RAPL_DRAM_EV        0
#define RAPL_PCK_EV         1

/*Intel® 64 and IA-32 Architectures
 * Software Developer’s Manual
 * Volume 3B:
 * System Programming Guide, Part 2
 */

/** Opens the necessary fds to read the MSR registers. Returns 0 on success
* 	and -1 on error. */
int init_rapl_msr(int *fd_map);

/** */
void dispose_rapl_msr(int *fd_map);

/** Reads rapl counters and stores them in values array. Returns 0 on success 
*	and -1 on error. */
int read_rapl_msr(int *fd_map,ullong *_values);

/** Reads the PCK TDP and copy in tdps. Returns EAR_SUCCESS or EAR_ERROR */
int read_rapl_pck_tdp(int *fd_map, double *tdps);
/** Reads the DRAM TDP and copy in tdps. Returns EAR_SUCCESS or EAR_ERROR */
int read_rapl_dram_tdp(int *fd_map, double *tdps);


void rapl_msr_energy_to_str(char *b,ullong *values);

ullong acum_rapl_energy(ullong *values);

void diff_rapl_msr_energy(ullong *diff, ullong *end, ullong *init);


#endif
