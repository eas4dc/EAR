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

#ifndef METRICS_FREQUENCY_IMC_H
#define METRICS_FREQUENCY_IMC_H

#include <common/types.h>
#include <common/states.h>
#include <common/hardware/topology.h>

typedef struct freq_imc_s {
	void *data;
} freq_imc_t;

state_t freq_imc_init(topology_t *tp);

state_t freq_imc_dispose();

state_t freq_imc_read(freq_imc_t *ef);

state_t freq_imc_read_diff(freq_imc_t *ef2, freq_imc_t *ef1, ulong *freqs, ulong *average);

state_t freq_imc_read_copy(freq_imc_t *ef2, freq_imc_t *ef1, ulong *freqs, ulong *average);

state_t freq_imc_data_alloc(freq_imc_t *ef, ulong *freqs[], ulong *freqs_count);

state_t freq_imc_data_count(uint *count);

state_t freq_imc_data_copy(freq_imc_t *ef_dst, freq_imc_t *ef_src);

state_t freq_imc_data_free(freq_imc_t *ef, ulong *freqs[]);

state_t freq_imc_data_diff(freq_imc_t *ef2, freq_imc_t *ef1, ulong *freqs, ulong *average);

state_t freq_imc_data_print(ulong *freqs, ulong *average);

#endif //METRICS_FREQUENCY_IMC_H
