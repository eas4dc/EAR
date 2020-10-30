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

#ifndef METRICS_FREQUENCY_IMC_INTEL63_H
#define METRICS_FREQUENCY_IMC_INTEL63_H

#include <metrics/frequency/imc.h>

state_t ifreq_intel63_init(topology_t *tp);

state_t ifreq_intel63_dispose();

state_t ifreq_intel63_read(freq_imc_t *ef);

state_t ifreq_intel63_read_diff(freq_imc_t *ef2, freq_imc_t *ef1, ulong *freqs, ulong *average);

state_t ifreq_intel63_read_copy(freq_imc_t *ef2, freq_imc_t *ef1, ulong *freqs, ulong *average);

state_t ifreq_intel63_data_alloc(freq_imc_t *ef, ulong *freqs[], ulong *freqs_count);

state_t ifreq_intel63_data_count(uint *count);

state_t ifreq_intel63_data_copy(freq_imc_t *ef_dst, freq_imc_t *ef_src);

state_t ifreq_intel63_data_free(freq_imc_t *ef, ulong *freqs[]);

state_t ifreq_intel63_data_diff(freq_imc_t *ef2, freq_imc_t *ef1, ulong *freqs, ulong *average);

state_t ifreq_intel63_data_print(ulong *freqs, ulong *average);

#endif //EAR_FREQUENCY_UNCORE_H
