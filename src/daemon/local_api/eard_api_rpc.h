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

#ifndef DAEMON_LOCAL_RPC_H
#define DAEMON_LOCAL_RPC_H

#include <common/types.h>
#include <common/sizes.h>
#include <common/states.h>
#include <daemon/local_api/eard_api.h>
#include <daemon/local_api/eard_api_conf.h>

#define RPC_MGT_CPUFREQ_GET_API            1001
#define RPC_MGT_CPUFREQ_GET_AVAILABLE      1002
#define RPC_MGT_CPUFREQ_GET_CURRENT        1003
#define RPC_MGT_CPUFREQ_GET_NOMINAL        1004
#define RPC_MGT_CPUFREQ_GET_GOVERNOR       1005
#define RPC_MGT_CPUFREQ_SET_CURRENT_LIST   1006
#define RPC_MGT_CPUFREQ_SET_CURRENT        1007
#define RPC_MGT_CPUFREQ_SET_GOVERNOR       1008
#define RPC_MGT_IMCFREQ_GET_API            1021
#define RPC_MGT_IMCFREQ_GET_AVAILABLE      1022
#define RPC_MGT_IMCFREQ_GET_CURRENT        1023
#define RPC_MGT_IMCFREQ_SET_CURRENT        1024
#define RPC_MGT_IMCFREQ_GET_CURRENT_MIN    1025
#define RPC_MGT_IMCFREQ_SET_CURRENT_MIN    1026
#define RPC_MGT_GPU_GET_API                1041
#define RPC_MGT_GPU_COUNT_DEVICES          1042
#define RPC_MGT_GPU_GET_FREQ_LIMIT_DEFAULT 1043
#define RPC_MGT_GPU_GET_FREQ_LIMIT_MAX     1044
#define RPC_MGT_GPU_GET_FREQ_LIMIT_CURRENT 1045
#define RPC_MGT_GPU_GET_FREQS_AVAILABLE    1046
#define RPC_MGT_GPU_RESET_FREQ_LIMIT       1047
#define RPC_MGT_GPU_SET_FREQ_LIMIT         1048
#define RPC_MGT_GPU_GET_POWER_CAP_DEFAULT  1049
#define RPC_MGT_GPU_GET_POWER_CAP_MAX      1050
#define RPC_MGT_GPU_GET_POWER_CAP_CURRENT  1051
#define RPC_MGT_GPU_RESET_POWER_CAP        1052
#define RPC_MGT_GPU_SET_POWER_CAP          1053
#define RPC_MET_CPUFREQ_GET_API            1061
#define RPC_MET_CPUFREQ_GET_CURRENT        1062
#define RPC_MET_IMCFREQ_GET_API            1081
#define RPC_MET_IMCFREQ_COUNT_DEVICES      1082
#define RPC_MET_IMCFREQ_GET_CURRENT        1083
#define RPC_MET_GPU_GET_API                1101
#define RPC_MET_GPU_COUNT_DEVICES          1102
#define RPC_MET_GPU_GET_METRICS            1103
#define RPC_MET_GPU_GET_METRICS_RAW        1104
#define RPC_MET_BWIDTH_GET_API             1121
#define RPC_MET_BWIDTH_COUNT_DEVICES       1122
#define RPC_MET_BWIDTH_READ                1123

struct rpcerr_s {
	char *answer;
	char *pending;
} Rpcerr __attribute__((weak)) = {
	.answer = "Error during RPC answer",
	.pending = "Error during RPC pending data read",
};

state_t eard_rpc(uint call, char *data, size_t size, char *recv_data, size_t expc_size);

/** Used when you want the incoming data to be saved in the internal buffer. */
state_t eard_rpc_buffered(uint call, char *data, size_t size, char **buffer, size_t *recv_size);

state_t eard_rpc_answer(int fd, uint call, state_t s, char *data, size_t size, char *error);

state_t eard_rpc_read_pending(int fd, char *recv_data, size_t read_size, size_t expc_size);

state_t eard_rpc_clean(int fd, size_t size);

#endif //DAEMON_LOCAL_RPC_H

