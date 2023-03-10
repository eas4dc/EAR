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

#ifndef METRICS_CPUFREQ_BASE_H
#define METRICS_CPUFREQ_BASE_H

/*
 * Provisional files to discover the base frequency by low level calls.
 */

#include <common/types/types.h>
#include <common/hardware/topology.h>

void cpufreq_get_base(topology_t *tp, ulong *freq_base);

void cpufreq_get_boost_enabled(topology_t *tp, uint *boost_enabled);

#endif
