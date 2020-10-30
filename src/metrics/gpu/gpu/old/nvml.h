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

#ifndef METRICS_ENERGY_NVML_GPU_H
#define METRICS_ENERGY_NVML_GPU_H

#include <metrics/energy/energy_gpu.h>

/** **/
state_t nvml_status();

/** **/
state_t nvml_init(pcontext_t *c);

/** **/
state_t nvsmi_dispose(pcontext_t *c);

/** Counts the number of GPUs. **/
state_t nvml_count(pcontext_t *c, uint *count);

/** **/
state_t nvml_read(pcontext_t *c, gpu_energy_t *data_read);

/** **/
state_t nvml_data_alloc(pcontext_t *c, gpu_energy_t **data_read);

/** **/
state_t nvml_data_free(pcontext_t *c, gpu_energy_t **data_read);

/** **/
state_t nvml_data_null(pcontext_t *c, gpu_energy_t *data_read);

/** **/
state_t nvml_data_diff(pcontext_t *c, gpu_energy_t *data_read1, gpu_energy_t *data_read2, gpu_energy_t *data_avrg);

#endif // METRICS_ENERGY_NVML_GPU_H