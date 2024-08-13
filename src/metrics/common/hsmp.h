/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_COMMON_HSMP_H
#define METRICS_COMMON_HSMP_H

#include <common/states.h>
#include <common/plugins.h>
#include <common/hardware/topology.h>

#define HSMP_TEST                        0x01
#define HSMP_GET_SMU_VERSION             0x02 //esmi_smu_fw_version_get
#define HSMP_GET_INTERFACE_VERSION       0x03 //esmi_hsmp_proto_ver_get
#define HSMP_READ_SOCKET_POWER           0x04 //esmi_socket_power_get
#define HSMP_WRITE_SOCKET_POWER_LIMIT    0x05 //esmi_socket_power_cap_set
#define HSMP_READ_SOCKET_POWER_LIMIT     0x06 //esmi_socket_power_cap_get
#define HSMP_READ_MAX_SOCKET_POWER_LIMIT 0x07 //esmi_socket_power_cap_max_get
#define HSMP_WRITE_BOOST_LIMIT           0x08 //esmi_core_boostlimit_set
#define HSMP_WRITE_BOOST_LIMIT_ALL_CORES 0x09 //esmi_socket_boostlimit_set
#define HSMP_READ_BOOST_LIMIT            0x0a //esmi_core_boostlimit_get
#define HSMP_READ_PROCHOT_STATUS         0x0b //esmi_prochot_status_get
#define HSMP_SET_XGMI_LINK_WIDTH_RANGE   0x0c //esmi_xgmi_width_set
#define HSMP_APB_DISABLE                 0x0d //esmi_apb_disable
#define HSMP_APB_ENABLE                  0x0e //esmi_apb_enable
#define HSMP_READ_CURRENT_FCLK_MEMCLK    0x0f //esmi_fclk_mclk_get
#define HSMP_READ_CCLK_FREQUENCY_LIMIT   0x10 //esmi_cclk_limit_get
#define HSMP_SOCKET_C0_RESIDENCY         0x11 //esmi_socket_c0_residency_get
#define HSMP_SET_LCLK_DPM_LEVEL_RANGE    0x12 //esmi_socket_lclk_dpm_level_set
#define HSMP_GET_MAX_DDR_BANDIWDTH       0x14 //esmi_ddr_bw_get

// Looks for HSMP devices.
state_t hsmp_scan(topology_t *tp);

state_t hsmp_close();

state_t hsmp_send(int socket, uint function, uint *args, uint *reps);

#endif
