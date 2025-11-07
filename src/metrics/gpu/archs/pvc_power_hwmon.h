/*

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

#ifndef METRICS_ENERGY_CPU_PVC_HWMON_METER_H
#define METRICS_ENERGY_CPU_PVC_HWMON_METER_H

#include <metrics/gpu/gpu.h>

state_t gpu_pvc_hwmon_load(gpu_ops_t *ops, int force_api);
state_t pvc_hwmon_init(ctx_t *c);
void pvc_hwmon_get_info(apinfo_t *info);
void pvc_hwmon_set_monitoring_mode(int mode_in);
void pvc_hwmon_get_devices(gpu_devs_t **devs, uint *devs_count);
state_t pvc_hwmon_dispose(ctx_t *c);
state_t pvc_hwmon_count_devices(ctx_t *c, uint *count);
state_t pvc_hwmon_read(ctx_t *c, gpu_t *data);
state_t pvc_hwmon_read_raw(ctx_t *c, gpu_t *data);
state_t pvc_hwmon_set_mode(ctx_t *c, uint32_t mode);

#endif
