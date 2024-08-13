/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_COMMON_HSMP_LIB_H
#define METRICS_COMMON_HSMP_LIB_H

#ifdef HSMP_BASE
#include <e_smi/e_smi.h>
#endif
#include <metrics/common/hsmp.h>

#ifndef HSMP_BASE
#define ESMI_SUCCESS 0

typedef int esmi_status_t;

struct smu_fw_version {
    uchar debug;
    uchar minor;
    uchar major;
    uchar unused;
};

struct ddr_bw_metrics {
    uint max_bw;
    uint utilized_bw;
    uint utilized_pct;
};
#endif

typedef struct esmi_s {
    esmi_status_t (*init)                      (void);
    esmi_status_t (*exit)                      (void);
    char *        (*get_err_msg)               (esmi_status_t esmi_err);
    esmi_status_t (*smu_fw_version_get)        (struct smu_fw_version *smu_fw);//HSMP_GET_SMU_VERSION
    esmi_status_t (*proto_ver_get)             (uint *proto_ver); //HSMP_GET_INTERFACE_VERSION
    esmi_status_t (*socket_power_get)          (uint socket_idx, uint *ppower); //HSMP_READ_SOCKET_POWER
    esmi_status_t (*socket_power_cap_set)      (uint socket_idx, uint pcap); //HSMP_WRITE_SOCKET_POWER_LIMIT
    esmi_status_t (*socket_power_cap_get)      (uint socket_idx, uint *pcap); //HSMP_READ_SOCKET_POWER_LIMIT
    esmi_status_t (*socket_power_cap_max_get)  (uint socket_idx, uint *pmax); //HSMP_READ_MAX_SOCKET_POWER_LIMIT
    esmi_status_t (*core_boostlimit_set)       (uint cpu_ind, uint boostlimit); //HSMP_WRITE_BOOST_LIMIT
    esmi_status_t (*socket_boostlimit_set)     (uint socket_idx, uint boostlimit); //HSMP_WRITE_BOOST_LIMIT_ALL_CORES
    esmi_status_t (*core_boostlimit_get)       (uint cpu_ind, uint *pboostlimit); //HSMP_READ_BOOST_LIMIT
    esmi_status_t (*prochot_status_get)        (uint socket_idx, uint *prochot); //HSMP_READ_PROCHOT_STATUS
    esmi_status_t (*xgmi_width_set)            (uchar min, uchar max); //HSMP_SET_XGMI_LINK_WIDTH_RANGE
    esmi_status_t (*apb_disable)               (uint sock_ind, uchar pstate); //HSMP_APB_DISABLE
    esmi_status_t (*apb_enable)                (uint sock_ind); //HSMP_APB_ENABLE
    esmi_status_t (*fclk_mclk_get)             (uint socket_idx, uint *fclk, uint *mclk); //HSMP_READ_CURRENT_FCLK_MEMCLK
    esmi_status_t (*cclk_limit_get)            (uint socket_idx, uint *cclk); //HSMP_READ_CCLK_FREQUENCY_LIMIT
    esmi_status_t (*socket_c0_residency_get)   (uint socket_idx, uint *pc0_residency); //HSMP_SOCKET_C0_RESIDENCY
    esmi_status_t (*socket_lclk_dpm_level_set) (uint sock_ind, uchar nbio_id, uchar min, uchar max); //HSMP_SET_LCLK_DPM_LEVEL_RANGE
    esmi_status_t (*ddr_bw_get)                (struct ddr_bw_metrics *ddr_bw); //HSMP_GET_MAX_DDR_BANDIWDTH
} esmi_t;

// esmi_init
// esmi_exit
// esmi_socket_current_active_freq_limit_get
// esmi_socket_freq_range_get
// esmi_current_freq_limit_core_get
// esmi_pwr_svi_telemetry_all_rails_get
// esmi_pwr_efficiency_mode_set
// esmi_socket_temperature_get
// esmi_dimm_temp_range_and_refresh_rate_get
// esmi_dimm_power_consumption_get
// esmi_dimm_thermal_sensor_get
// esmi_gmi3_link_width_range_set
// esmi_socket_lclk_dpm_level_get
// esmi_pcie_link_rate_set
// esmi_df_pstate_range_set
// esmi_current_io_bandwidth_get
// esmi_current_xgmi_bw_get
// esmi_cpu_family_get
// esmi_core_energy_get
// esmi_socket_energy_get

state_t hsmp_lib_open(topology_t *tp);

state_t hsmp_lib_close();

state_t hsmp_lib_send(int socket, uint function, uint *args, uint *reps);

#endif
