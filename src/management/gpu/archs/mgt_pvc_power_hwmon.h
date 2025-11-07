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

#ifndef MGT_ENERGY_CPU_PVC_HWMON_METER_H
#define MGT_ENERGY_CPU_PVC_HWMON_METER_H

#include <management/gpu/gpu.h>

state_t mgt_gpu_pvc_hwmon_load(mgt_gpu_ops_t *ops);
state_t mgt_pvc_hwmon_init(ctx_t *c);
void mgt_pvc_hwmon_get_api(uint *api);
state_t mgt_pvc_hwmon_get_devices(ctx_t *c, gpu_devs_t **devs_in, uint *devs_count_in);
state_t mgt_pvc_hwmon_dispose(ctx_t *c);
state_t mgt_pvc_hwmon_count_devices(ctx_t *c, uint *count);

/* DUMMY */
state_t mgt_pvc_hwmon_freq_limit_get_current(ctx_t *c, ulong *khz);
state_t mgt_pvc_hwmon_freq_limit_get_default(ctx_t *c, ulong *khz);
state_t mgt_pvc_hwmon_freq_limit_get_max(ctx_t *c, ulong *khz);
state_t mgt_pvc_hwmon_freq_limit_reset(ctx_t *c);
state_t mgt_pvc_hwmon_freq_limit_set(ctx_t *c, ulong *khz);
state_t mgt_pvc_hwmon_freq_get_available(ctx_t *c, const ulong ***list_khz, const uint **list_len);
state_t mgt_pvc_hwmon_power_cap_get_current(ctx_t *c, ulong *watts);
state_t mgt_pvc_hwmon_power_cap_get_default(ctx_t *c, ulong *watts);
state_t mgt_pvc_hwmon_power_cap_get_rank(ctx_t *c, ulong *watts_min, ulong *watts_max);
state_t mgt_pvc_hwmon_power_cap_reset(ctx_t *c);
state_t mgt_pvc_hwmon_power_cap_set(ctx_t *c, ulong *watts);

#endif
