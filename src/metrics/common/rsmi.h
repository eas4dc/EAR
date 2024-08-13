/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_COMMON_RSMI_H
#define METRICS_COMMON_RSMI_H

#include <common/types.h>
#include <common/states.h>
#ifdef RSMI_BASE
#include <rocm_smi/rocm_smi.h>
#endif

#ifndef RSMI_BASE
typedef int rsmi_temperature_metric_t;
typedef int rsmi_clk_type_t;
typedef int rsmi_status_t;

typedef struct rsmi_process_info_s {
    uint    process_id;
    uint    pasid;
    ullong  vram_usage;
    ullong  sdma_usage;
    uint    cu_occupancy;
} rsmi_process_info_t;

typedef struct rsmi_frequencies_s {
    uint    num_supported;
    uint    current;
    ullong *frequency;
} rsmi_frequencies_t;

typedef struct rsmi_utilization_counter_s {
    int    type;
    ullong value;
} rsmi_utilization_counter_t;

typedef struct {
    rsmi_frequencies_t transfer_rate;
    uint *lanes;
} rsmi_pcie_bandwidth_t;

#define RSMI_STATUS_SUCCESS             0
#define RSMI_TEMP_CURRENT               0
#define RSMI_TEMP_TYPE_EDGE             0
#define RSMI_TEMP_TYPE_JUNCTION         0
#define RSMI_COARSE_GRAIN_GFX_ACTIVITY  0
#define RSMI_CLK_TYPE_SYS               0
#define RSMI_CLK_TYPE_MEM               0
#define RSMI_FREQ_IND_MAX               0
#endif
typedef rsmi_utilization_counter_t rsmi_util_t;
typedef rsmi_frequencies_t         rsmi_freqs_t;
typedef rsmi_status_t              rsmi_status_t;
typedef int                        rsmi_enum_t;

typedef struct rsmi_s
{
    rsmi_status_t (*init)                 (ullong flags);
    rsmi_status_t (*devs_count)           (uint *devs_count);
    rsmi_status_t (*get_serial)           (uint dev_idx, char *serial_num, uint len);
    rsmi_status_t (*get_energy)           (uint dev_idx, ullong *power, float *resolution, ullong *ts);
    rsmi_status_t (*get_power)            (uint dev_idx, uint sensor_idx, ullong *power); // uW
    rsmi_status_t (*get_powercap)         (uint dev_idx, uint sensor_ind, ulong *cap); // uW
    rsmi_status_t (*get_powercap_default) (uint dev_idx, ulong *cap); // uW
    rsmi_status_t (*get_powercap_range)   (uint dev_idx, uint sensor_ind, ulong *max, ulong *min); // uW
    rsmi_status_t (*set_powercap)         (uint dev_idx, uint sensor_ind, ulong cap); // uW
    rsmi_status_t (*get_temperature)      (uint dev_idx, uint sensor_type, rsmi_enum_t metric, ullong *temp);
    rsmi_status_t (*get_clock)            (uint dev_idx, rsmi_enum_t type, rsmi_freqs_t *f); // Hz
    rsmi_status_t (*set_clock)            (uint dev_idx, rsmi_enum_t level, ulong mhz, rsmi_enum_t type); // MHz
    rsmi_status_t (*set_clock_range)      (uint dev_idx, ulong min, ulong max, rsmi_enum_t type); // MHz
    rsmi_status_t (*get_utilization)      (uint dev_idx, rsmi_util_t *counters, uint length, ullong *ts);
    rsmi_status_t (*get_busy_gpu)         (uint dev_idx, uint *busy_percent);
    rsmi_status_t (*get_busy_mem)         (uint dev_idx, uint *busy_percent);
    rsmi_status_t (*get_mem_total)        (uint dev_idx, rsmi_enum_t type, ullong *total); // B
    rsmi_status_t (*get_mem_used)         (uint dev_idx, rsmi_enum_t type, ullong *used); // B
    rsmi_status_t (*get_pci_bandwidth)    (uint dev_idx, rsmi_pcie_bandwidth_t *bandwidth);
    rsmi_status_t (*get_procs)            (rsmi_process_info_t *procs, uint *num_items);
    rsmi_status_t (*get_procs_devs)       (uint pid, uint *dev_idx, uint *num_devices);
} rsmi_t;

#define rsmi_fail(state) \
    ((state) != RSMI_STATUS_SUCCESS) 

state_t rsmi_open(rsmi_t *rsmi);

state_t rsmi_close();

int rsmi_is_privileged();

#endif //METRICS_COMMON_RSMI_H


