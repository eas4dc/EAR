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

#include <common/hardware/topology.h>
#include <common/states.h>
#include <fcntl.h>
#include <unistd.h>

#if defined __has_include
#if __has_include(<asm/amd_hsmp.h>)
#define HAS_AMD_HSMP_H 1
#include <asm/amd_hsmp.h>
#endif
#endif

// Mode
#define HSMP_RD O_RDONLY
#define HSMP_WR O_RDWR

#if !HAS_AMD_HSMP_H
enum hsmp_message_ids {
    HSMP_TEST = 1,                   /* 01h Increments input value by 1 */
    HSMP_GET_SMU_VER,                /* 02h SMU FW version */
    HSMP_GET_PROTO_VER,              /* 03h HSMP interface version */
    HSMP_GET_SOCKET_POWER,           /* 04h average package power consumption */
    HSMP_SET_SOCKET_POWER_LIMIT,     /* 05h Set the socket power limit */
    HSMP_GET_SOCKET_POWER_LIMIT,     /* 06h Get current socket power limit */
    HSMP_GET_SOCKET_POWER_LIMIT_MAX, /* 07h Get maximum socket power value */
    HSMP_SET_BOOST_LIMIT,            /* 08h Set a core maximum frequency limit (WR) */
    HSMP_SET_BOOST_LIMIT_SOCKET,     /* 09h Set socket maximum frequency level */
    HSMP_GET_BOOST_LIMIT,            /* 0Ah Get current frequency limit */
    HSMP_GET_PROC_HOT,               /* 0Bh Get PROCHOT status */
    HSMP_SET_XGMI_LINK_WIDTH,        /* 0Ch Set max and min width of xGMI Link */
    HSMP_SET_DF_PSTATE,              /* 0Dh Alter APEnable/Disable messages behavior (WR) */
    HSMP_SET_AUTO_DF_PSTATE,         /* 0Eh Enable DF P-State Performance Boost algorithm (WR) */
    HSMP_GET_FCLK_MCLK,              /* 0Fh Get FCLK and MEMCLK for current socket */
    HSMP_GET_CCLK_THROTTLE_LIMIT,    /* 10h Get CCLK frequency limit in socket */
    HSMP_GET_C0_PERCENT,             /* 11h Get average C0 residency in socket */
    HSMP_SET_NBIO_DPM_LEVEL,         /* 12h Set max/min LCLK DPM Level for a given NBIO */
    HSMP_GET_NBIO_DPM_LEVEL,         /* 13h Get LCLK DPM level min and max for a given NBIO */
    HSMP_GET_DDR_BANDWIDTH,          /* 14h Get theoretical maximum and current DDR Bandwidth in Gbps */
                                     /* Newer functions */
    HSMP_GET_TEMP_MONITOR,           /* 15h Get socket temperature */
    HSMP_GET_DIMM_TEMP_RANGE,        /* 16h Get per-DIMM temperature range and refresh rate */
    HSMP_GET_DIMM_POWER,             /* 17h Get per-DIMM power consumption */
    HSMP_GET_DIMM_THERMAL,           /* 18h Get per-DIMM thermal sensors */
    HSMP_GET_SOCKET_FREQ_LIMIT,      /* 19h Get current active frequency per socket */
    HSMP_GET_CCLK_CORE_LIMIT,        /* 1Ah Get CCLK frequency limit per core */
    HSMP_GET_RAILS_SVI,              /* 1Bh Get SVI-based Telemetry for all rails */
    HSMP_GET_SOCKET_FMAX_FMIN,       /* 1Ch Get Fmax and Fmin per socket */
    HSMP_GET_IOLINK_BANDWITH,        /* 1Dh Get current bandwidth on IO Link */
    HSMP_GET_XGMI_BANDWITH,          /* 1Eh Get current bandwidth on xGMI Link */
    HSMP_SET_GMI3_WIDTH,             /* 1Fh Set max and min GMI3 Link width */
    HSMP_SET_PCI_RATE,               /* 20h Control link rate on PCIe devices */
    HSMP_SET_POWER_MODE,             /* 21h Select power efficiency profile policy */
    HSMP_SET_PSTATE_MAX_MIN,         /* 22h Set the max and min DF P-State  */
    HSMP_GET_METRIC_TABLE_VER,       /* 23h Get metrics table version */
    HSMP_GET_METRIC_TABLE,           /* 24h Get metrics table */
    HSMP_GET_METRIC_TABLE_DRAM_ADDR, /* 25h Get metrics table dram address */
    HSMP_MSG_ID_MAX,
};
#endif

// It opens HSMP by various methods: contacting amd_hsmp driver, loading e_smi library
// or opening PCIs directly (in that order).
state_t hsmp_open(topology_t *tp, mode_t mode);

state_t hsmp_close();

state_t hsmp_send(int socket, uint function, uint *args, uint *reps);

#endif
