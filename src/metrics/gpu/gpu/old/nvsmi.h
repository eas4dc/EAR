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

#ifndef METRICS_ENERGY_NVSMI_GPU_H
#define METRICS_ENERGY_NVSMI_GPU_H

#include <metrics/energy/energy_gpu.h>

/** Reveals if the system is 'nvidia-smi' compatible. **/
state_t nvsmi_gpu_status();

/** Initializes and allocates the GPU context (c) memory for the data readings (data_read).
  * Also the data average structure (data_avrg) is allocated if its pointer it is not NULL.
  * This data computes the average of the current read metric and the previous one, and also
  * computes the energy and time between two different readings. **/
state_t nvsmi_gpu_init(pcontext_t *c, uint loop_ms);

/** Frees the allocated memory of the context and data readings structures. **/
state_t nvsmi_gpu_dispose(pcontext_t *c);

/** Counts the number of GPUs. **/
state_t nvsmi_gpu_count(pcontext_t *c, uint *count);

/** Reads the GPU metrics and returns its values (data_read). If previous data reading is
  * given (data_read), the averages of the current reading and the previous is computed and
  * returned (data_avrg). You can get the number of samples contained in the arrays by
  * calling 'nvsmi_gpu_count()'. **/
state_t nvsmi_gpu_read(pcontext_t *c, gpu_energy_t *data_read);

/** **/
state_t nvsmi_gpu_data_alloc(pcontext_t *c, gpu_energy_t **data_read);

/** **/
state_t nvsmi_gpu_data_free(pcontext_t *c, gpu_energy_t **data_read);

/** **/
state_t nvsmi_gpu_data_null(pcontext_t *c, gpu_energy_t *data_read);

/** **/
state_t nvsmi_gpu_data_diff(pcontext_t *c, gpu_energy_t *data_read1, gpu_energy_t *data_read2, gpu_energy_t *data_avrg);

#endif // METRICS_ENERGY_NVSMI_GPU_H
