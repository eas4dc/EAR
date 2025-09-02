/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef MANAGEMENT_CPUPOW_H
#define MANAGEMENT_CPUPOW_H

#include <common/types.h>
#include <common/states.h>
#include <common/hardware/topology.h>
#include <metrics/common/apis.h>

// Domain/device types
#define CPUPOW_DOMAINS  4
#define CPUPOW_SOCKET   0 //PKG (#devs = #sockets)
#define CPUPOW_DRAM     1 //    (#devs = #sockets)
#define CPUPOW_CORE     2 //PP0 (#devs = #cores), remember: cores != threads
#define CPUPOW_UNCORE   3 //PP1 (#devs = ¿#sockets?)
// Flag to use in powercap_set
#define POWERCAP_DISABLE     UINT_MAX     //Disables the power cap in the device
#define POWERCAP_DO_NOTHING (UINT_MAX-1)  //Don't change anything in the device
// Flags to use in powercap_reset()
#define RESET_DEFAULT 0 // Reset to previous values
#define RESET_TDP     1 // Reset to TDP

// API building scheme
#define CPUPOW_F_LOAD(name)                void name (topology_t *tpo, mgt_cpupow_ops_t *ops)
#define CPUPOW_F_GET_INFO(name)            void name (apinfo_t *info)
#define CPUPOW_F_COUNT_DEVICES(name)       int name (int domain)
#define CPUPOW_F_POWERCAP_IS_CAPABLE(name) int name (int domain)
#define CPUPOW_F_POWERCAP_IS_ENABLED(name) state_t name (int domain, uint *enabled)
#define CPUPOW_F_POWERCAP_GET(name)        state_t name (int domain, uint *watts)
#define CPUPOW_F_POWERCAP_SET(name)        state_t name (int domain, uint *watts)
#define CPUPOW_F_POWERCAP_RESET(name)      state_t name (int domain)
#define CPUPOW_F_TDP_GET(name)             state_t name (int domain, uint *watts)

#define CPUPOW_DEFINES(name) \
CPUPOW_F_LOAD                (mgt_cpupow_ ##name ##_load); \
CPUPOW_F_GET_INFO            (mgt_cpupow_ ##name ##_get_info); \
CPUPOW_F_COUNT_DEVICES       (mgt_cpupow_ ##name ##_count_devices); \
CPUPOW_F_POWERCAP_IS_CAPABLE (mgt_cpupow_ ##name ##_powercap_is_capable); \
CPUPOW_F_POWERCAP_IS_ENABLED (mgt_cpupow_ ##name ##_powercap_is_enabled); \
CPUPOW_F_POWERCAP_GET        (mgt_cpupow_ ##name ##_powercap_get); \
CPUPOW_F_POWERCAP_SET        (mgt_cpupow_ ##name ##_powercap_set); \
CPUPOW_F_POWERCAP_RESET      (mgt_cpupow_ ##name ##_powercap_reset); \
CPUPOW_F_TDP_GET             (mgt_cpupow_ ##name ##_tdp_get);

typedef struct mgt_cpupow_ops_s {
    CPUPOW_F_GET_INFO            ((*get_info));
    CPUPOW_F_COUNT_DEVICES       ((*count_devices));
    CPUPOW_F_POWERCAP_IS_CAPABLE ((*powercap_is_capable));
    CPUPOW_F_POWERCAP_IS_ENABLED ((*powercap_is_enabled));
    CPUPOW_F_POWERCAP_GET        ((*powercap_get));
    CPUPOW_F_POWERCAP_SET        ((*powercap_set));
    CPUPOW_F_POWERCAP_RESET      ((*powercap_reset));
    CPUPOW_F_TDP_GET             ((*tdp_get));
} mgt_cpupow_ops_t;

void mgt_cpupow_load(topology_t *tp, int eard);

#define mgt_cpupow_init()

void mgt_cpupow_get_info(apinfo_t *info);

int mgt_cpupow_count_devices(int domain);

int mgt_cpupow_powercap_is_capable(int domain);

state_t mgt_cpupow_powercap_is_enabled(int domain, uint *enabled);

state_t mgt_cpupow_powercap_get(int domain, uint *watts);

state_t mgt_cpupow_powercap_set(int domain, uint *watts);

// Reset the whole domain to its starting state.
state_t mgt_cpupow_powercap_reset(int domain, int reset_mode);

state_t mgt_cpupow_tdp_get(int domain, uint *watts);

void mgt_cpupow_tdp_print(int domain, uint *watts, char *buffer, int length);

void mgt_cpupow_tdp_tostr(int domain, uint *watts, char *buffer, int length);

void mgt_cpupow_data_alloc(int domain, uint **list);

#endif