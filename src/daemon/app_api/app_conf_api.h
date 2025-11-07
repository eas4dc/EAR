/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _APP_CONF_API_H
#define _APP_CONF_API_H
#define _GNU_SOURCE
#include <sched.h>
#include <sys/types.h>

#include <common/config.h>

#define ENERGY_TIME       1000
#define ENERGY_TIME_DEBUG 1001
#define SELECT_CPU_FREQ   1002
#define SET_GPUFREQ       1003
#define SET_GPUFREQ_LIST  1004
#define CONNECT           2000
#define DISCONNECT        2001
#define INVALID_COMMAND   1

typedef struct energy {
    ulong energy_j;
    ulong energy_mj;
    ulong time_sec;
    ulong time_ms;
    ulong os_time_sec;
    ulong os_time_ms;
} energy_t;

typedef union app_recv_opt {
    energy_t my_energy;
} app_recv_opt_t;

typedef struct cpu_freq_req {
    cpu_set_t mask;
    unsigned long cpuf;
} cpu_freq_req_t;

typedef struct gpu_freq_req {
    uint gpu_id;
    ulong gpu_freq;
} gpu_freq_req_t;

typedef struct gpu_freq_list_req {
    ulong gpu_freq[MAX_GPUS_SUPPORTED];
} gpu_freq_list_req_t;

typedef union app_send_data {
    cpu_freq_req_t cpu_freq;
    gpu_freq_req_t gpu_freq;
    gpu_freq_list_req_t gpu_freq_list;
} app_send_data_t;

typedef struct app_send {
    uint req;
    int pid;
    app_send_data_t send_data;
} app_send_t;

typedef struct app_recv {
    int ret;
    app_recv_opt_t my_data;
} app_recv_t;

#else
#endif
