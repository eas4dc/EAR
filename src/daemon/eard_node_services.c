/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1
#define VEARD_LAPI_DEBUG 2

#define _GNU_SOURCE
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <linux/limits.h>

#include <common/includes.h>
#include <common/environment.h>
#include <common/system/fd_sets.h>
#include <common/system/poll.h>
#include <common/system/monitor.h>
#include <common/types/generic.h>
#include <common/utils/strscreen.h>
#include <common/types/pc_app_info.h>
#include <common/utils/serial_buffer.h>
#include <common/hardware/hardware_info.h>
#include <management/management.h>
#include <metrics/common/msr.h>
#include <metrics/gpu/gpu.h>
#include <metrics/cpufreq/cpufreq.h>
#include <metrics/imcfreq/imcfreq.h>
#include <metrics/energy/cpu.h>
#include <metrics/energy/energy_node.h>
#include <metrics/bandwidth/bandwidth.h>
#include <metrics/energy_cpu/energy_cpu.h>
#include <metrics/temperature/temperature.h>
#include <report/report.h>
#include <daemon/eard_node_services.h>
#include <daemon/local_api/eard_api.h>
#include <daemon/local_api/node_mgr.h>
#include <daemon/power_monitor.h>
#include <daemon/shared_configuration.h>
#include <daemon/remote_api/dynamic_configuration.h>
#include <daemon/log_eard.h>
#if USE_GPUS
#include <daemon/gpu/gpu_mgt.h>
#endif

#define MIN_INTERVAL_RT_ERROR   3600
#define NUM_PRIVILEGED_SERVICES 11
#define REQ_LOCAL_FD            0
#define ACK_LOCAL_FD            1
#define NUM_LOCAL_FDS           2


void set_default_management_init();

typedef struct local_connection{
	ulong jid,sid,lid,anonymous;
	int fds[NUM_LOCAL_FDS];
	int cancelled;
} local_connection_t;

uint privileged_services[NUM_PRIVILEGED_SERVICES] =
		{ 0, 0, 0, 0, 0, 0, 0, WRITE_APP_SIGNATURE, WRITE_EVENT, WRITE_LOOP_SIGNATURE, 0 };

void eard_exit(uint restart);

/* Defined in eard.c */
extern report_id_t      rid;
extern state_t          report_connection_status;
extern topology_t       node_desc;
extern ehandler_t       handler_energy;
extern int              fd_rapl[MAX_PACKAGES];
extern char             ear_tmp[MAX_PATH_SIZE];
extern char             nodename[MAX_PATH_SIZE];
extern int              eard_must_exit;
extern ulong            eard_max_freq;
extern pthread_barrier_t setup_barrier;
#if USE_GPUS
extern char             gpu_str[256];
#endif
extern int              num_uncore_counters; // Me extraña que esto funcione.
static ulong            node_energy_datasize;
static edata_t          node_energy_data;
static int              num_packs = 0;
static ullong          *values_rapl;
static imcfreq_t       *unc_data;
static uint             unc_count;
static int              global_req_fd;
static char             ear_commreq_global[MAX_PATH_SIZE * 2];
static ulong            energy_freq;
/* This var is used in all the eard services, no thread-safe */
static struct daemon_req req;
/* FDs for local connections */
static afd_set_t        rfds_basic;
static local_connection_t *eard_local_conn;
static uint             num_local_con=0;
// New grounds
static char             big_chunk1[16384];
static char             big_chunk2[16384];
static wide_buffer_t    wb;
       manages_info_t   man;
       metrics_info_t   met;
static metrics_read_t   met_read;
static metrics_t        uc; // Obsolete, use metrics_info_t and metrics_read_t
static metrics_t        ui; // Obsolete, use metrics_info_t and metrics_read_t
static metrics_t        ub; // Obsolete, use metrics_info_t and metrics_read_t
static metrics_t        ug; // Obsolete, use metrics_info_t and metrics_read_t

#if USE_GPUS
static gpu_t           *eard_read_gpu;
#endif

#define sf(function) \
	if (state_fail(s = function)) { \
		error("when calling " #function); \
	}

extern char **environ;

void services_init(topology_t *tp)
{
    state_t s;

    management_init(&man, tp, API_FREE);

    cpufreq_load(tp, NO_EARD);
    imcfreq_load(tp, NO_EARD);
     bwidth_load(tp, NO_EARD);
       temp_load(tp, NO_EARD);
	#if USE_GPUS
        gpu_load(NO_EARD);
    #endif

       temp_get_info(&met.tmp);

    cpufreq_get_api((uint *) &uc.api);
    imcfreq_get_api((uint *) &ui.api);
     bwidth_get_api((uint *) &ub.api);
    #if USE_GPUS
        gpu_get_api((uint *) &ug.api);
    #endif

    sf(    cpufreq_init(no_ctx));
    sf(    imcfreq_init(no_ctx));
    sf(     bwidth_init(no_ctx));
    sf(       temp_init(      ));
    #if USE_GPUS
    sf(        gpu_init(no_ctx));
    #endif

    sf(cpufreq_count_devices(no_ctx, &uc.devs_count));
    sf(imcfreq_count_devices(no_ctx, &ui.devs_count));
        bwidth_count_devices(no_ctx, &ub.devs_count);
    #if USE_GPUS
    gpu_get_devices((gpu_devs_t **) &ug.avail_list, &ug.devs_count);
    #endif

    sf(cpufreq_data_alloc((cpufreq_t **) &uc.current_list, empty));
    sf(imcfreq_data_alloc((imcfreq_t **) &ui.current_list, empty));
        bwidth_data_alloc((bwidth_t  **) &ub.current_list        );
          temp_data_alloc(&met_read.tmp);
    #if USE_GPUS
	       gpu_data_alloc(&eard_read_gpu);
    #endif

	serial_alloc(&wb, SIZE_8KB);
    // Resetting values
    set_default_management_init();
}

void services_print(topology_t *tp, cluster_conf_t *cluster_conf)
{
    static strscreen_t ss;
    static int         id_conf;
    static int         id_logo;
    static int         id_metrics;
    static int         id_manages;
    static int         id_topology;
    static char        buf_host[128];
    static char        buffer[4096];

    char *logo = ""
                 "        ___           ___           ___           ___     \n"
                 "       /\\  \\         /\\  \\         /\\  \\         /\\  \\    \n"
                 "      /::\\  \\       /::\\  \\       /::\\  \\       /::\\  \\   \n"
                 "     /:/\\:\\  \\     /:/\\:\\  \\     /:/\\:\\  \\     /:/\\:\\  \\  \n"
                 "    /::\\~\\:\\  \\   /::\\~\\:\\  \\   /::\\~\\:\\  \\   /:/  \\:\\__\\ \n"
                 "   /:/\\:\\ \\:\\__\\ /:/\\:\\ \\:\\__\\ /:/\\:\\ \\:\\__\\ /:/__/ \\:|__|\n"
                 "   \\:\\~\\:\\ \\/__/ \\/__\\:\\/:/  / \\/_|::\\/:/  / \\:\\  \\ /:/  /\n"
                 "    \\:\\ \\:\\__\\        \\::/  /     |:|::/  /   \\:\\  /:/  / \n"
                 "     \\:\\ \\/__/        /:/  /      |:|\\/__/     \\:\\/:/  /  \n"
                 "      \\:\\__\\         /:/  /       |:|  |        \\__/__/   \n"
                 "       \\/__/         \\/__/         \\|__|                    ";

    metrics_info_get(&met);
    management_info_get(&man);

    gethostname(buf_host, 128);
    sprintf(buffer                 , "component    : EARD (EAR Daemon/Node Manager)\n");
    sprintf(&buffer[strlen(buffer)], "version      : %u.%u\n", VERSION_MAJOR, VERSION_MINOR);
    sprintf(&buffer[strlen(buffer)], "hostname     : %s\n", buf_host);
    sprintf(&buffer[strlen(buffer)], "verbose level: %d\n", verb_level);
    sprintf(&buffer[strlen(buffer)], "tmp dir      : %s\n", cluster_conf->install.dir_temp);
    sprintf(&buffer[strlen(buffer)], "etc dir      : %s\n", cluster_conf->install.dir_conf);
    sprintf(&buffer[strlen(buffer)], "install dir  : %s\n", cluster_conf->install.dir_inst);
    sprintf(&buffer[strlen(buffer)], "db server IP : %s\n", cluster_conf->database.ip);
    sprintf(&buffer[strlen(buffer)], "db mirror IP : %s\n", cluster_conf->database.sec_ip);
    sprintf(&buffer[strlen(buffer)], "USE_GPUS     : %u\n", USE_GPUS);
    sprintf(&buffer[strlen(buffer)], "MAX_CPUS     : %u\n", MAX_CPUS_SUPPORTED);
    sprintf(&buffer[strlen(buffer)], "MAX_SOCKETS  : %u\n", MAX_SOCKETS_SUPPORTED);
    sprintf(&buffer[strlen(buffer)], "MAX_GPUS     : %u\n", MAX_GPUS_SUPPORTED);

    scprintf_init(&ss, 44, 140, -1, '#'); // 0 to 49, 0 to 179
    scprintf_divide(&ss, (int[]) {  1,  1 }, (int[]) {  20,  74 }, &id_topology, "topology"      );
    scprintf_divide(&ss, (int[]) {  1, 76 }, (int[]) {  12, 137 }, &id_logo,     "ear-logo"      );
    scprintf_divide(&ss, (int[]) { 14, 76 }, (int[]) {  33, 137 }, &id_conf,     "configuration" );
    scprintf_divide(&ss, (int[]) { 22,  1 }, (int[]) {  33,  74 }, &id_metrics,  "metrics"       );
    scprintf_divide(&ss, (int[]) { 35,  1 }, (int[]) {  42,  74 }, &id_manages,  "management"    );

    scsprintf(&ss, id_logo    , 0, 0, logo);
    scsprintf(&ss, id_conf    , 0, 1, buffer);
    scsprintf(&ss, id_topology, 0, 1, topology_tostr(tp, buffer, 1024));
    scsprintf(&ss, id_metrics , 0, 1, metrics_info_tostr(&met, buffer));
    scsprintf(&ss, id_manages , 0, 1, management_info_tostr(&man, buffer));

    dprintf(verb_channel, "%s", scprintf(&ss));
    dprintf(verb_channel, "# Log:                                                                                                                                    #\n");
}

void services_dispose()
{
}

void connect_service(struct daemon_req *new_req);

static int answer(int fd, int call, char *data, size_t size, state_t s, char *state_msg)
{
    if (state_fail(eard_rpc_answer(fd, call, s, data, size, state_msg))) {
        return 0;
    }
    return 1;
}

int services_misc(eard_head_t *head, int req_fd, int ack_fd)
{
    uint call = head->req_service;
    eard_state_t s ;

    verbose(VEARD_LAPI_DEBUG, "EARD misc");
    switch (call) {
    case CONNECT_EARD_NODE_SERVICES:
        connect_service(&req);
    break;
    case DISCONNECT_EARD_NODE_SERVICES:
        eard_close_comm(req_fd,ack_fd);
    break;
    case RPC_GET_STATE:
        s.pid = getpid();
        s.gpus_supported_count = MAX_GPUS_SUPPORTED;
        s.cpus_supported_count = MAX_CPUS_SUPPORTED;
        s.sock_supported_count = MAX_SOCKETS_SUPPORTED;
        s.gpus_support_enabled = USE_GPUS;
        version_set(&s.version, VERSION_MAJOR, VERSION_MINOR);
        return answer(ack_fd, call, (char *) &s, sizeof(eard_state_t), EAR_SUCCESS, NULL);
    break;
    default:
        return 0;
    }
    return 1;
}

int services_mgt_cpufreq(eard_head_t *head, int req_fd, int ack_fd)
{
    uint call = head->req_service;
    state_t s = EAR_SUCCESS;
    char *data = NULL;
    size_t size = 0;

    switch (call) {
    case RPC_MGT_CPUFREQ_GET_API:
        data = (char *) &man.cpu.api;
        size = sizeof(uint);
    break;
    case RPC_MGT_CPUFREQ_GET_NOMINAL:
        if (state_fail(s = mgt_cpufreq_get_nominal(no_ctx, (uint *) big_chunk1))) {
            serror("Could not get the current nominal frequency");
        }
        data = big_chunk1;
        size = sizeof(uint);
    break;
    case RPC_MGT_CPUFREQ_GET_AVAILABLE:
        serial_clean(&wb);
        //
        serial_add_elem(&wb, (char *) &man.cpu.list1_count, sizeof(uint));
        serial_add_elem(&wb, (char *) man.cpu.list1, sizeof(pstate_t)*man.cpu.list1_count);
        // Setting to send
        size = serial_size(&wb);
        data = serial_data(&wb);
    break;
    case RPC_MGT_CPUFREQ_GET_CURRENT:
        if (state_fail(s = mgt_cpufreq_get_current_list(no_ctx, man.cpu.list2))) {
            serror("Could not get the current management frequency");
        }
        data = (char *) man.cpu.list2;
        size = man.cpu.devs_count * sizeof(pstate_t);
    break;
    case RPC_MGT_CPUFREQ_GET_GOVERNOR:
        if (state_fail(s = mgt_cpufreq_get_governor(no_ctx, (uint *) big_chunk1))) {
            serror("Could not get the current governor");
        }
        data = big_chunk1;
        size = sizeof(uint);
    break;
    case RPC_MGT_CPUFREQ_SET_CURRENT_LIST:
        size =  man.cpu.devs_count * sizeof(uint);
        if (state_fail(s = eard_rpc_read_pending(req_fd, man.cpu.list3, head->size, size))) {
            serror(Rpcerr.pending);
        }
        if (state_ok(s)) {
            if (state_fail(s = mgt_cpufreq_set_current_list(no_ctx, (uint *) man.cpu.list3))) {
                serror("When setting a CPU frequency list");
            }
            size = 0;
        }
    break;
    case RPC_MGT_CPUFREQ_SET_CURRENT:
        size = sizeof(uint) + sizeof(int);
        if (state_fail(s = eard_rpc_read_pending(req_fd, big_chunk1, head->size, size))) {
            serror(Rpcerr.pending);
        }
        if (state_ok(s)) {
            uint idx = *((uint *) &big_chunk1[0]);
             int cpu = *((uint *) &big_chunk1[sizeof(int)]);
            if (state_fail(s = mgt_cpufreq_set_current(no_ctx, idx, cpu))) {
                serror("When setting a CPU frequency");
            }
            size = 0;
        }
    break;
    case RPC_MGT_CPUFREQ_SET_GOVERNOR:
        if (state_fail(s = eard_rpc_read_pending(req_fd, big_chunk1, head->size, sizeof(uint)))) {
            serror(Rpcerr.pending);
        }
        if (state_ok(s)) {
            if (state_fail(s = mgt_cpufreq_set_governor(no_ctx, (uint) big_chunk1[0]))) {
                serror(Rpcerr.answer);
            }
        }
    break;
    case RPC_MGT_CPUFREQ_SET_GOVERNOR_MASK:
        size = sizeof(cpu_set_t) + sizeof(uint);
        if (state_fail(s = eard_rpc_read_pending(req_fd, big_chunk1, head->size, size))) {
            serror(Rpcerr.pending);
        }
        if (state_ok(s)) {
            cpu_set_t mask = *((cpu_set_t *) &big_chunk1[0]);
            uint governor  = *((uint *) &big_chunk1[sizeof(cpu_set_t)]);
            if (state_fail(s = mgt_cpufreq_governor_set_mask(no_ctx, governor, mask))) {
                serror("When setting CPU governors");
            }
            size = 0;
        }
    break;
    case RPC_MGT_CPUFREQ_SET_GOVERNOR_LIST:
        size =  man.cpu.devs_count * sizeof(uint);
        if (state_fail(s = eard_rpc_read_pending(req_fd, man.cpu.list3, head->size, size))) {
            serror(Rpcerr.pending);
        }
        if (state_ok(s)) {
            if (state_fail(s = mgt_cpufreq_set_current_list(no_ctx, (uint *) man.cpu.list3))) {
                serror("When setting a governors list");
            }
            size = 0;
        }
    break;
    default:
        return 0;
    }

    // Always answering (if s != SUCCESS the error message is sent)
    if (state_fail(eard_rpc_answer(ack_fd, call, s, data, size, state_msg))) {
        return 0;
    }
    return 1;
}
int services_mgt_cpuprio(eard_head_t *head, int fd_req, int fd_ack)
{
    uint     rpc_id   = head->req_service;
    size_t   rpc_size = head->size;
    uint    *ibuffer  = (uint *) big_chunk1;
    state_t  ret_s    = EAR_SUCCESS;
    char    *ret_data = NULL;
    size_t   ret_size = 0;

    switch(rpc_id) {
    case RPC_MGT_PRIO_GET_STATIC_VALUES:
        serial_clean(&wb);
        serial_add_elem(&wb, (char *) &man.pri.api        , sizeof(uint));
        serial_add_elem(&wb, (char *) &man.pri.list1_count, sizeof(uint));
        serial_add_elem(&wb, (char *) &man.pri.list2_count, sizeof(uint));
        ret_size = serial_size(&wb);
        ret_data = serial_data(&wb);
    break;
    case RPC_MGT_PRIO_ENABLE:
        ret_s = mgt_cpuprio_enable();
    break;
    case RPC_MGT_PRIO_DISABLE:
        ret_s = mgt_cpuprio_disable();
    break;
    case RPC_MGT_PRIO_IS_ENABLED:
        ibuffer[0] = mgt_cpuprio_is_enabled();
        ret_data = (char *) ibuffer;
        ret_size = sizeof(int);
    break;
    case RPC_MGT_PRIO_GET_AVAILABLE:
        ret_s    = mgt_cpuprio_get_available_list((cpuprio_t *) man.pri.list1);
        ret_data = (char *) man.pri.list1;
        ret_size = sizeof(cpuprio_t) * man.pri.list1_count;
    break;
    case RPC_MGT_PRIO_SET_AVAILABLE:
        if (state_fail(ret_s = eard_rpc_read_pending(fd_req, man.pri.list1, rpc_size, 0))) {
            serror(Rpcerr.pending);
        } else {
            ret_s = mgt_cpuprio_set_available_list(man.pri.list1);
        }
    break;
    case RPC_MGT_PRIO_GET_CURRENT:
        ret_s    = mgt_cpuprio_get_current_list((uint *) man.pri.list2);
        ret_data = (char *) man.pri.list2;
        ret_size = sizeof(uint) * man.pri.list2_count;
    break;
    case RPC_MGT_PRIO_SET_CURRENT_LIST:
        if (state_fail(ret_s = eard_rpc_read_pending(fd_req, man.pri.list2, rpc_size, 0))) {
            serror(Rpcerr.pending);
        } else {
            ret_s = mgt_cpuprio_set_current_list(man.pri.list2);
        }
    break;
    case RPC_MGT_PRIO_SET_CURRENT:
        if (state_fail(ret_s = eard_rpc_read_pending(fd_req, big_chunk1, rpc_size, 0))) {
            serror(Rpcerr.pending);
        } else {
            ret_s = mgt_cpuprio_set_current(ibuffer[0], (int) ibuffer[1]);
        }
    break;
    default:
        return 0;
    }

    if (state_fail(eard_rpc_answer(fd_ack, rpc_id, ret_s, ret_data, ret_size, state_msg))) {
        return 0;
    }
    return 1;
}

int services_mgt_imcfreq(eard_head_t *head, int req_fd, int ack_fd)
{
	uint call = head->req_service;
	state_t s = EAR_SUCCESS;
	char *data = NULL;
	size_t size = 0;

	verbose(VEARD_LAPI_DEBUG, "EARD mgt imcfreq");
    switch (call) {
    case RPC_MGT_IMCFREQ_GET_API:
		data = (char *) &man.imc.api;
		size = sizeof(uint);
    break;
    case RPC_MGT_IMCFREQ_GET_AVAILABLE:
		serial_clean(&wb);
		//
		serial_add_elem(&wb, (char *) &man.imc.list1_count, sizeof(uint));
		serial_add_elem(&wb, (char *) man.imc.list1, sizeof(pstate_t)*man.imc.list1_count);
		// Setting to send
		size = serial_size(&wb);
		data = serial_data(&wb);
    break;
    case RPC_MGT_IMCFREQ_GET_CURRENT:
		if (state_fail(s = mgt_imcfreq_get_current_list(no_ctx, man.imc.list2))) {
			serror("Could not get IMC current frequency");
		}
		data = (char *) man.imc.list2;
		size = sizeof(pstate_t)*man.imc.devs_count;
    break;
    case RPC_MGT_IMCFREQ_SET_CURRENT:
		if (state_fail(s = eard_rpc_read_pending(req_fd, (char *) man.imc.list3, head->size, 0))) {
			serror(Rpcerr.pending);
		} else {
		    if (state_fail(s = mgt_imcfreq_set_current_list(no_ctx, man.imc.list3))) {
			    serror("Could not set IMC current frequency");
            }
        }
    break;
    case RPC_MGT_IMCFREQ_GET_CURRENT_MIN:
		serial_clean(&wb);
		//
		if (state_fail(s = mgt_imcfreq_get_current_ranged_list(no_ctx, (pstate_t *) big_chunk1, (pstate_t *) big_chunk2))) {
			serror("Could not get IMC current ranged frequency");
		} else {
			serial_add_elem(&wb, big_chunk1, sizeof(pstate_t)*man.imc.devs_count);
			serial_add_elem(&wb, big_chunk2, sizeof(pstate_t)*man.imc.devs_count);
			size = serial_size(&wb);
			data = serial_data(&wb);
		}
    break;
    case RPC_MGT_IMCFREQ_SET_CURRENT_MIN:
		serial_reset(&wb, head->size);
		if (state_fail(s = eard_rpc_read_pending(req_fd, serial_data(&wb), head->size, 0))) {
			serror(Rpcerr.pending);
		} else {
			serial_copy_elem(&wb, (char *) big_chunk1, NULL);
			serial_copy_elem(&wb, (char *) big_chunk2, NULL);
			s = mgt_imcfreq_set_current_ranged_list(no_ctx, (uint *) big_chunk1, (uint *) big_chunk2);
		}
    break;
    default:
        return 0;
    }
	// Always answering (if s != SUCCESS the error message is sent)
	if (state_fail(eard_rpc_answer(ack_fd, call, s, data, size, state_msg))) {
        serror("RPC answer failed");
		return 0;
	}
	return 1;
}

int services_mgt_gpu(eard_head_t *head, int fd_req, int fd_ack)
{
    uint     rpc_id   = head->req_service;
    state_t  ret_s    = EAR_SUCCESS;
    char    *ret_data = NULL;
    size_t   ret_size = 0;
    uint     u1;
    #if USE_GPUS
    size_t   rpc_size = head->size;
    ulong  **p1;
    uint    *p2;
    #endif

    switch (rpc_id) {
    case RPC_MGT_GPU_GET_API:
        ret_data = (char *) &man.gpu.api;
        ret_size = sizeof(uint);
    break;
    #if USE_GPUS
    case RPC_MGT_GPU_GET_DEVICES:
        serial_clean(&wb);
        serial_add_elem(&wb, (char *) &man.gpu.devs_count, sizeof(uint));
        serial_add_elem(&wb, (char *) man.gpu.list1, sizeof(gpu_devs_t)*man.gpu.devs_count);
        ret_size = serial_size(&wb);
        ret_data = serial_data(&wb);
    break;
    case RPC_MGT_GPU_GET_FREQ_LIMIT_DEFAULT:
        if (state_fail (ret_s = mgt_gpu_freq_limit_get_default(no_ctx, man.gpu.list2)) ){
            serror("Could not get default GPU frequency limit");
        }
        ret_size = sizeof(ulong)*man.gpu.devs_count;
        ret_data = (char *) man.gpu.list2;
    break;
    case RPC_MGT_GPU_GET_FREQ_LIMIT_MAX:
        if (state_fail (ret_s = mgt_gpu_freq_limit_get_max(no_ctx, man.gpu.list2)) ){
            serror("Could not get max GPU frequency limit");
        }
        ret_size = sizeof(ulong)*man.gpu.devs_count;
        ret_data = (char *) man.gpu.list2;
    break;
    case RPC_MGT_GPU_GET_FREQ_LIMIT_CURRENT:
        if (state_fail (ret_s = mgt_gpu_freq_limit_get_current(no_ctx, man.gpu.list2)) ){
            serror("Could not get current GPU frequency limit");
        }
        ret_size = sizeof(ulong)*man.gpu.devs_count;
        ret_data = (char *) man.gpu.list2;
    break;
    case RPC_MGT_GPU_GET_FREQS_AVAILABLE:
        // Get frequencies list (there is no memcpy by now, just a pointer)
        ret_s = mgt_gpu_freq_get_available(no_ctx, (const ulong ***) &p1, (const uint **) &p2);
        // Preparing serial buffer
        serial_clean(&wb);
        serial_add_elem(&wb, (char *) p2, sizeof(uint)*man.gpu.devs_count);
        for (u1 = 0; u1 < man.gpu.devs_count; ++u1) {
            serial_add_elem(&wb, (char *) p1[u1], sizeof(ulong)*p2[u1]);
        }
        ret_size = serial_size(&wb);
        ret_data = serial_data(&wb);
    break;
    case RPC_MGT_GPU_RESET_FREQ_LIMIT:
            ret_s = mgt_gpu_freq_limit_reset(no_ctx);
    break;
    case RPC_MGT_GPU_SET_FREQ_LIMIT:
        if (state_fail(ret_s = eard_rpc_read_pending(fd_req, (char *) man.gpu.list2, rpc_size, 0)) ){
            serror(Rpcerr.pending);
        }
        if (state_ok(ret_s)) {
            if (state_fail(ret_s = mgt_gpu_freq_limit_set(no_ctx, (ulong *) man.gpu.list2)) ){
                serror("Error when setting a GPU frequency limit");
            }
        }
    break;
    case RPC_MGT_GPU_GET_POWER_CAP_DEFAULT:
        if (state_fail (ret_s = mgt_gpu_power_cap_get_default(no_ctx, man.gpu.list2)) ){
            serror("Could not get the default GPU power limit");
        }
        ret_data = (char *) man.gpu.list2;
        ret_size = sizeof(ulong)*man.gpu.devs_count;
    break;
    case RPC_MGT_GPU_GET_POWER_CAP_MAX:
        if (state_fail (ret_s = mgt_gpu_power_cap_get_rank(no_ctx, man.gpu.list2, man.gpu.list3)) ){
            serror("Could not get the default GPU power limit");
        }
        serial_clean(&wb);
        serial_add_elem(&wb, (char *) man.gpu.list2, sizeof(ulong)*man.gpu.devs_count);
        serial_add_elem(&wb, (char *) man.gpu.list3, sizeof(ulong)*man.gpu.devs_count);
        ret_size = serial_size(&wb);
        ret_data = serial_data(&wb);
    break;
    case RPC_MGT_GPU_GET_POWER_CAP_CURRENT:
        if (state_fail (ret_s = mgt_gpu_power_cap_get_current(no_ctx, man.gpu.list2)) ){
            serror("Could not get current GPU power limit");
        }
        ret_data = (char *) man.gpu.list2;
        ret_size = sizeof(ulong)*man.gpu.devs_count;
    break;
    case RPC_MGT_GPU_RESET_POWER_CAP:
        ret_s = mgt_gpu_power_cap_reset(no_ctx);
    break;
    case RPC_MGT_GPU_SET_POWER_CAP:
        if (state_fail(ret_s = eard_rpc_read_pending(fd_req, (char *) man.gpu.list2, rpc_size, 0))) {
            serror(Rpcerr.pending);
        }
        if (state_ok(ret_s)) {
            if (state_fail(ret_s = mgt_gpu_power_cap_set(no_ctx, (ulong *) man.gpu.list2))) {
                serror("Error when setting a GPU power cap");
            }
        }
    break;
    #endif
    case RPC_MGT_GPU_IS_SUPPORTED:
        u1       = (uint) mgt_gpu_is_supported();
        ret_data = (char *) &u1;
        ret_size = sizeof(int);
        break;
    default:
        return 0;
    }

    if (state_fail(eard_rpc_answer(fd_ack, rpc_id, ret_s, ret_data, ret_size, state_msg))) {
        return 0;
    }
    return 1;
}

int services_met_cpufreq(eard_head_t *head, int req_fd, int ack_fd)
{
    uint call = head->req_service;
    state_t s = EAR_SUCCESS;
    char *data = NULL;
    size_t size = 0;

    verbose(VEARD_LAPI_DEBUG,"EARD met cpufreq");
    switch (call) {
    case RPC_MET_CPUFREQ_GET_API:
        data = (char *) &uc.api;
        size = sizeof(uint);
    break;
    case RPC_MET_CPUFREQ_GET_CURRENT:
        if(state_fail(s = cpufreq_read(no_ctx, (cpufreq_t *) uc.current_list))) {
            serror("Could not get cpu frequency (aperf)");
        }
        data = (char *) uc.current_list;
        size = sizeof(cpufreq_t) * uc.devs_count;
    break;
    default:
        return 0;
    }

    // Always answering (if s != SUCCESS the error message is sent)
    if (state_fail(eard_rpc_answer(ack_fd, call, s, data, size, state_msg))) {
        serror("RPC answer failed");
        return 0;
    }
    return 1;
}

int services_met_imcfreq(eard_head_t *head, int req_fd, int ack_fd)
{
    uint call = head->req_service;
    state_t s = EAR_SUCCESS;
    char *data = NULL;
    size_t size = 0;

    verbose(VEARD_LAPI_DEBUG, "EARD met_imcfreq");
    switch (call) {
    case RPC_MET_IMCFREQ_GET_API:
        data = (char *) &ui.api;
        size = sizeof(uint);
    break;
    case RPC_MET_IMCFREQ_COUNT_DEVICES:
        data = (char *) &ui.devs_count;
        size = sizeof(uint);
    break;
    case RPC_MET_IMCFREQ_GET_CURRENT:
        if(state_fail(s = imcfreq_read(no_ctx, (imcfreq_t *) ui.current_list))) {
            serror("Could not get imc frequency");
        }
        data = (char *) ui.current_list;
        size = sizeof(imcfreq_t) * ui.devs_count;
    break;
    default:
        return 0;
    }

    // Always answering (if s != SUCCESS the error message is sent)
    if (state_fail(eard_rpc_answer(ack_fd, call, s, data, size, state_msg))) {
        serror("RPC answer failed");
        return 0;
    }
    return 1;
}

int services_met_bwidth(eard_head_t *head, int req_fd, int ack_fd)
{
    uint call = head->req_service;
    state_t s = EAR_SUCCESS;
    char *data = NULL;
    size_t size = 0;

    verbose(VEARD_LAPI_DEBUG,"EARD met bandwith");
    switch (call) {
    case RPC_MET_BWIDTH_GET_API:
        data = (char *) &ub.api;
        size = sizeof(uint);
    break;
    case RPC_MET_BWIDTH_COUNT_DEVICES:
        data = (char *) &ub.devs_count;
        size = sizeof(uint);
    break;
    case RPC_MET_BWIDTH_READ:
        if(state_fail(s = bwidth_read(no_ctx, (bwidth_t *) ub.current_list))) {
            serror("Could not get mem frequency");
        }
        data = (char *) ub.current_list;
        size = sizeof(bwidth_t) * ub.devs_count;
    break;
    default:
        return 0;
    }

    // Always answering (if s != SUCCESS the error message is sent)
    if (state_fail(eard_rpc_answer(ack_fd, call, s, data, size, state_msg))) {
        serror("RPC answer failed");
        return 0;
    }
    return 1;
}

int services_met_temp(eard_head_t *head, int req_fd, int ack_fd)
{
    uint call = head->req_service;
    state_t s = EAR_SUCCESS;
    char *data = NULL;
    size_t size = 0;

    verbose(VEARD_LAPI_DEBUG,"EARD met bandwith");
    switch (call) {
        case RPC_MET_TEMP_GET_INFO:
            data = (char *) &met.tmp;
            size = sizeof(apinfo_t);
            break;
        case RPC_MET_TEMP_READ:
            if (state_fail(s = temp_read(met_read.tmp, NULL))) {
                serror("Could not get mem frequency");
            }
            debug("#### met_read %lld\n", met_read.tmp[0]);
            data = (char *) met_read.tmp;
            size = sizeof(llong) * met.tmp.devs_count;
            break;
        default:
            return 0;
    }
    // Always answering (if s != SUCCESS the error message is sent)
    if (state_fail(eard_rpc_answer(ack_fd, call, s, data, size, state_msg))) {
        serror("RPC answer failed");
        return 0;
    }
    return 1;
}

/* These functions links local fds with eard job context. The jobid.stepid must
 * be already created */
uint is_anonymous(ulong jid,ulong sid);

state_t init_local_connections(local_connection_t **my_con,int num_con)
{
  int i;
  local_connection_t *lcon;
  *my_con = calloc(sizeof(local_connection_t),num_con);
  lcon = *my_con;
  for (i=0;i<num_con;i++) lcon[i].jid = ULONG_MAX;
  num_local_con = num_con;
  return EAR_SUCCESS;
}

state_t add_local_fd(local_connection_t *my_con,int fd, int pos)
{
  if (pos >= NUM_LOCAL_FDS){
    return_msg(EAR_ERROR, "No more FDs in local connection");
  }
  my_con->fds[pos] = fd;
  return EAR_SUCCESS;
}

int is_new_job(local_connection_t *my_con,ulong jid,ulong sid)
{
  int i;
  i = 0;
  while((i < num_local_con) && !((my_con[i].jid == jid) && (my_con[i].sid == sid))) i++;
  if (i<num_local_con) return 1;
  return 0;
}

int add_new_local_connection(local_connection_t *my_con,ulong jid,ulong sid,ulong lid)
{
  int i=0;
  while ((my_con[i].jid != ULONG_MAX) && (i< num_local_con)) i++;
  if (i == num_local_con){
    error("Maximum number of local connections reached");
    return -1;
  }
  my_con[i].jid = jid;
  my_con[i].sid = sid;
  my_con[i].lid = lid;
  my_con[i].anonymous = is_anonymous(jid,sid);
  my_con[i].cancelled = 0;

  debug("New local connection: pos %d lid %lu", i, lid);

  return i;
}

int get_local_connection_ack(local_connection_t *my_con,int reqfd)
{
  int i=0;
  while ((my_con[i].fds[REQ_LOCAL_FD] != reqfd) && (i<num_local_con)){
      //debug("Check local conn req=%d in pos %d req=%d ack=%d",reqfd,i,my_con[i].fds[REQ_LOCAL_FD],my_con[i].fds[ACK_LOCAL_FD]);
      i++;
  }
  if (i == num_local_con){
      debug("Local connection not found %d", reqfd);
    return_msg(EAR_ERROR, "Local connection not found");
  }
  return my_con[i].fds[ACK_LOCAL_FD];
}

int get_local_connection_by_fd(local_connection_t *my_con,int reqfd,int ackfd)
{
  int i=0;
  while (!((my_con[i].fds[REQ_LOCAL_FD] == reqfd) && (my_con[i].fds[ACK_LOCAL_FD] == ackfd)) && (i<num_local_con)) i++;
  if (i == num_local_con){
    return_msg(EAR_ERROR, "Local connection not found");
  }
  return i;
}

int get_local_connection_by_id(local_connection_t *my_con, ulong jid, ulong sid)
{
  int i=0;
  while (!((my_con[i].jid == jid) && (my_con[i].sid== sid) && !my_con[i].cancelled) &&  (i<num_local_con)) i++;
	if (i == num_local_con) return -1;
	return i;
}

int get_local_connection_by_req(local_connection_t *my_con,int reqfd)
{
  int i=0;
  while ((my_con[i].fds[REQ_LOCAL_FD] != reqfd) && (i<num_local_con)) i++;
  if (i == num_local_con){
      return_msg(EAR_ERROR, "Local connection not found");
  }
  return i;
}

void clean_local_connection(local_connection_t *my_con)
{
  my_con->jid = ULONG_MAX;
  my_con->sid = 0;
  my_con->lid = 0;
  my_con->anonymous = 0;
  my_con->cancelled = 0;
  my_con->fds[REQ_LOCAL_FD] = -1;
  my_con->fds[ACK_LOCAL_FD] = -1;
}

uint is_privileged_service(uint service)
{
    int i = 0;
    while((i < NUM_PRIVILEGED_SERVICES) && (privileged_services[i] != service)) i++;
    if (i<NUM_PRIVILEGED_SERVICES) return 1;
    else return 0;
}

uint is_anonymous(ulong jid,ulong sid)
{
  return ((jid == 0) && (sid == 0));
}

int is_valid_sec_tag(ulong tag)
{
	#if 1
	debug("received tag: %lu / SEC_KEY %lu", tag, SEC_KEY);
	// You make define a constant at makefile time called SEC_KEY to use that option
	return ((ulong) SEC_KEY == tag);
	#endif
}

void clean_global_connector()
{
  close(global_req_fd);
  unlink(ear_commreq_global);
}

void create_global_connector(char *ear_tmp, char *nodename)
{
    debug("########################");
    // Creates a pipe to receive requests for applications. eard creates 1 pipe
    // (per node) and service to receive requests
    sprintf(ear_commreq_global, "%s/.ear_comm.globalreq", ear_tmp);
    verbose(VEARD_LAPI, "Global connector placed at %s", ear_commreq_global);
    // ear_comreq files will be used to send requests from the library to the eard
    if (mknod(ear_commreq_global, S_IFIFO | S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP | S_IROTH | S_IWOTH, 0) < 0) {
        if (errno != EEXIST) {
          error("Error creating ear global communicator for requests %s\n", strerror(errno));
        }
    }
    if (chmod(ear_commreq_global, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)<0){
        error("Error setting privileges for eard-earl connection");
    }
    global_req_fd = open(ear_commreq_global,O_RDWR);
    if (global_req_fd < 0){
        error("Error creating global communicator for connections");
        _exit(0);
    }
    AFD_SETT(global_req_fd, &rfds_basic, "global-req");
}

void connect_service(struct daemon_req *new_req)
{
  char ear_commack[MAX_PATH_SIZE * 2];
  char ear_commreq[MAX_PATH_SIZE * 2];
  application_t *new_app = &new_req->req_data.app;
  job_t *new_job = &new_app->job;
  unsigned long ack;
  int ear_req_fd,ear_ack_fd;
  int newc;
  ulong lid = new_req->con_id.lid;
  int pid = create_ID(new_job->id, new_job->step_id);
	uint ID = (uint)pid;
#if WF_SUPPORT
  uint AID = (uint)(new_job->local_id);
#endif

  // Let's check if there is another application
  verbose(VEARD_LAPI, "request for connection at service (%lu,%lu)",new_job->id,new_job->step_id);

  /* Creates 1 pipe (per node),  jobid and step id to send acks. pid is a combination of jobid and stepid */
  verbose(VEARD_LAPI, "New local connection job_id=%lu step_id=%lu local_id %lu\n", new_job->id, new_job->step_id,new_req->con_id.lid);
  /* This pipe is to send data, 1 per connection */

#if WF_SUPPORT
  sprintf(ear_commack, "%s/%u/%u/.ear_comm.ack_0.%d.%lu", ear_tmp, ID, AID, pid,lid);
  sprintf(ear_commreq, "%s/%u/%u/.ear_comm.req_0.%d.%lu", ear_tmp, ID, AID, pid,lid);
#else
  sprintf(ear_commack, "%s/%u/.ear_comm.ack_0.%d.%lu", ear_tmp, ID, pid,lid);
  sprintf(ear_commreq, "%s/%u/.ear_comm.req_0.%d.%lu", ear_tmp, ID, pid,lid);
#endif
  verbose(VEARD_LAPI,"Comm channels %s and %s",ear_commack,ear_commreq);
  /* We must create a new local connection, for now, 1 connection per job is allowed  */
  if ((newc = add_new_local_connection(eard_local_conn,new_job->id,new_job->step_id,new_req->con_id.lid)) <0){
      error("Error opening slot for new local connection");
  }
  debug("application %d.%lu asks for new connection ", pid,lid);
	verbose(VEARD_LAPI,"Opening %s",ear_commack);
  ear_ack_fd = open(ear_commack, O_WRONLY);
  if (ear_ack_fd < 0) {
        error("ERROR while opening ack_private pipe %s (%s)", ear_commack, strerror(errno));
  }
  /* We must associate the ack pipe with th local connection */
  if (add_local_fd(&eard_local_conn[newc],ear_ack_fd,ACK_LOCAL_FD) != EAR_SUCCESS){
        error("Opening adding local ack newc");
  }
  /* we send and ack to the earl */
  if (write(ear_ack_fd, &ack, sizeof(ack)) != sizeof(ack)) {
        warning("WARNING while sending ack to  %d", pid);
    eard_close_comm(-1,ear_ack_fd);
  }
  /* we open the local req pipe */
	verbose(VEARD_LAPI,"Opening %s", ear_commreq);
  ear_req_fd = open(ear_commreq, O_RDONLY);
  if (ear_req_fd < 0){
        error("ERROR while opening req_private pipe %s (%s)", ear_commreq, strerror(errno));
        eard_close_comm(-1,ear_ack_fd);
  }
  if (add_local_fd(&eard_local_conn[newc],ear_req_fd,REQ_LOCAL_FD) != EAR_SUCCESS){
        error("Opening adding local ack newc");
  }
  debug("New communication FDs created for application FD[%d]=%d,%d",newc,ear_ack_fd,ear_req_fd);
  AFD_SETT(ear_req_fd, &rfds_basic, "req%lu", new_req->con_id.lid);
  /* This function notifies the local connection */
  if (!eard_local_conn[newc].anonymous) powermon_mpi_init(&handler_energy, new_app);
  debug("Application %d connected with local API (anonymous %lu)",pid,eard_local_conn[newc].anonymous);
} 

void eard_close_comm(int req_fd,int ack_fd)
{
    int i, ret;
    int id;
    ulong jid, sid;
    ulong was_anonymous = 1;
    char ear_commack[MAX_PATH_SIZE * 2];
    char ear_commreq[MAX_PATH_SIZE * 2];

    /* This is a special case where one of the fds has been closed */
    if ((req_fd == -1) && (ack_fd == -1)) {
        for (i = 0; i < rfds_basic.fds_count; i++) {
            /* What about was_anonymous ? */
            if (AFD_ISSET(i, &rfds_basic) && !AFD_STAT(i, NULL)) {
                id = get_local_connection_by_req(eard_local_conn, i);
                close(i);
                close(eard_local_conn[id].fds[ACK_LOCAL_FD]);
                AFD_CLR(i, &rfds_basic);
                //AFD_CLR(eard_local_conn[id].fds[ACK_LOCAL_FD], &rfds_basic);
                clean_local_connection(&eard_local_conn[id]);
            }
        }
    } else if (req_fd == CLOSE_PIPE) {
        /* SIGPIPE received  in eard */
        verbose(VEARD_LAPI, "SIGPIPE received in EARD: PENDING");
    } else if (req_fd == CLOSE_EARD) {
        /* End eard */
        verbose(VEARD_LAPI, "Releasing all the files: PENDING");
    } else { /* this is the normal use case */
        id = get_local_connection_by_fd(eard_local_conn, req_fd, ack_fd);
        if (id < 0) {
            error("We cannot find (%d,%d) in the list of local connections", req_fd, ack_fd);
            return;
        }
        debug("Closing communication for (%lu,%lu)", eard_local_conn[id].jid, eard_local_conn[id].sid);
        jid = eard_local_conn[id].jid;
        sid = eard_local_conn[id].sid;
        ulong lid = eard_local_conn[id].lid;
        uint ID = create_ID(jid, sid);
        sprintf(ear_commack, "%s/%u/.ear_comm.ack_0.%u.%lu", ear_tmp, ID, ID, lid);
        sprintf(ear_commreq, "%s/%u/.ear_comm.req_0.%u.%lu", ear_tmp, ID, ID, lid);
        verbose(VEARD_LAPI, "Cleaning files for job %lu/%lu/%lu", jid, sid, lid);
        verbose(VEARD_LAPI, "Deleting %s", ear_commack);
        ret = unlink(ear_commack);
        if (ret != ENOENT) verbose(VEARD_LAPI, "Error deleting file %s", strerror(errno));
        verbose(VEARD_LAPI, "Deleting %s", ear_commreq);
        ret = unlink(ear_commreq);
        if (ret != ENOENT) verbose(VEARD_LAPI, "Error deleting file %s", strerror(errno));
        was_anonymous = eard_local_conn[id].anonymous;
        if (req_fd != -1) {
            close(req_fd);
            AFD_CLR(req_fd, &rfds_basic);
        }
        if (ack_fd != -1) {
            close(ack_fd);
            //AFD_CLR(ack_fd, &rfds_basic);
        }
        debug("application (%lu,%lu) disconnected (anonymous %lu)", jid, sid, was_anonymous);
        clean_local_connection(&eard_local_conn[id]);
        // This debug was set outside, but it requires a valid id to prevent segmentation faults.
    }
    if (!was_anonymous) {
        powermon_mpi_finalize(&handler_energy, jid, sid);
    }
}

int service_gpu(eard_head_t *head, int req_fd, int ack_fd)
{
	uint call = head->req_service;
	state_t s = EAR_SUCCESS;
	char *data = NULL;
	size_t size = 0;
    uint u1;

	switch (call) {
	case RPC_MET_GPU_GET_API:
		data = (char *) &ug.api;
		size = sizeof(uint);
	break;
	#if USE_GPUS
	case RPC_MET_GPU_GET_DEVICES:
        serial_clean(&wb);
        serial_add_elem(&wb, (char *) &ug.devs_count, sizeof(uint));
        serial_add_elem(&wb, (char *) ug.avail_list, sizeof(gpu_devs_t)*ug.devs_count);
        size = serial_size(&wb);
        data = serial_data(&wb);
	break;
	case RPC_MET_GPU_COUNT_DEVICES:
		data = (char *) &ug.devs_count;
		size = sizeof(uint);
	break;
	case RPC_MET_GPU_GET_METRICS:
		if (state_fail(gpu_read(no_ctx, eard_read_gpu)) ){
			serror("Could not get management GPU metrics");
		}
		data = (char *) eard_read_gpu;
		size = sizeof(gpu_t) * ug.devs_count;
	break;
	case RPC_MET_GPU_GET_METRICS_RAW:
		if (state_fail(gpu_read_raw(no_ctx, eard_read_gpu)) ){
			serror("Could not get management GPU metrics");
		}
		data = (char *) eard_read_gpu;
		size = sizeof(gpu_t) * ug.devs_count;
	break;
	#endif
    case RPC_MET_GPU_IS_SUPPORTED:
        u1   = (uint) gpu_is_supported();
        data = (char *) &u1;
        size = sizeof(uint);
    break;
	default:
		return 0;
	}
	// Always answering
	if (state_fail(eard_rpc_answer(ack_fd, call, s, data, size, state_msg)) ){
		return 0;
	}
	return 1;
}

int eard_gpu(eard_head_t *head, int req_fd, int ack_fd)
{
    unsigned long ack = 0;

	switch (head->req_service) {
    case GPU_SUPPORTED:
        #if USE_GPUS
        verbose(0, "GPU_SUPPORTED check");
        ack = 1;
        #else
        verbose(0, "GPU_NOT_SUPPORTED check");
        ack = 0;
        #endif
        if (write(ack_fd, &ack, sizeof(ack)) != sizeof(ack)) {
            // Do something with the error
        }
        break;
    default:
        return 0;
    }
    return 1;
}

int eard_node_energy(eard_head_t *head,int req_fd,int ack_fd)
{
    unsigned long ack;
    verbose(VEARD_LAPI_DEBUG, "EARD old node_energy");
    switch (head->req_service) {
    case READ_DC_ENERGY:
        energy_dc_read(&handler_energy, node_energy_data);
        if (write(ack_fd, node_energy_data, node_energy_datasize) != node_energy_datasize) {
            // Do something with the error
        }
        break;
    case ENERGY_FREQ:
        ack = (ulong) energy_freq;
        if (write(ack_fd, &ack, sizeof(unsigned long)) != sizeof(unsigned long)) {
            // Do something with the error
        }
        break;
    default:
        return 0;
    }
    return 1;
}

int eard_system(eard_head_t *head, int req_fd, int ack_fd)
{
    application_t  app;
    ear_event_t    event;
    loop_t         loop;
    int            con_id;
    state_t        s;
    char          *data = NULL;
    size_t         size = 0;
    uint           call = head->req_service;

    con_id = get_local_connection_by_fd(eard_local_conn,req_fd,ack_fd);
	verbose(VEARD_LAPI_DEBUG,"EARD old system");

    switch (head->req_service) {
        case RPC_WRITE_APPLICATION:
        case RPC_WRITE_WF_APPLICATION:
            // Initializing
            memset(&app, 0, sizeof(application_t));
            serial_reset(&wb, head->size);
            // Receiving
            if (state_fail(s = eard_rpc_read_pending(req_fd, serial_data(&wb), head->size, 0))) {
                serror(Rpcerr.pending);
            } else {
                application_deserialize(&wb, &app);

                if (!eard_local_conn[con_id].anonymous) {
									if (head->req_service == RPC_WRITE_APPLICATION){
                  	powermon_mpi_signature(&app);
									}
									if (head->req_service == RPC_WRITE_WF_APPLICATION){
										report_applications(&rid, &app, 1);
									}
                }
            }
            break;
        case RPC_WRITE_LOOP:
            // Initializing
            memset(&loop, 0, sizeof(loop_t));
            serial_reset(&wb, head->size);
            // Receiving
            if (state_fail(s = eard_rpc_read_pending(req_fd, serial_data(&wb), head->size, 0))) {
                serror(Rpcerr.pending);
            } else {
                loop_deserialize(&wb, &loop);

                powermon_loop_signature(eard_local_conn[con_id].jid, eard_local_conn[con_id].sid, &loop);
                // print_loop_fd(1,&loop);
                s = report_loops(&rid, &loop, 1);
            }
            break;
        case RPC_WRITE_EVENT:
            // Initializing
            memset(&event, 0, sizeof(ear_event_t));
            serial_reset(&wb, head->size);
            // Receiving
            if (state_fail(s = eard_rpc_read_pending(req_fd, serial_data(&wb), head->size, 0))) {
                serror(Rpcerr.pending);
            } else {
                event_deserialize(&wb, &event);
                s = report_events(&rid, &event, 1);
            }
            break;
        default:
            return 0;
    }
    // Always answering
    if (state_fail(eard_rpc_answer(ack_fd, call, s, data, size, state_msg)) ){
        return 0;
    }
    return 1;
}

int eard_rapl(eard_head_t *head,int req_fd,int ack_fd)
{
    unsigned long ack = 0;
    size_t size;

    verbose(VEARD_LAPI_DEBUG, "EARD old eard_rapl");
    size = sizeof(ullong) * RAPL_POWER_EVS * num_packs;
    switch (head->req_service) {
    case READ_RAPL:
        if (energy_cpu_read(no_ctx, values_rapl) < 0) {
            error("Error reading rapl msr counters");
        }
        if (write(ack_fd, values_rapl, size) != size) {
            // Do something with the error
        }
        break;
    case DATA_SIZE_RAPL:
        ack = size;
        if (write(ack_fd, &ack, sizeof(ulong)) != sizeof(ulong)) {
            // Do something with the error
        }
        break;
    default:
       return 0;
    }
    return 1;
}

state_t service_close(int req_fd, int ack_fd)
{
	ulong job_id, step_id;
	ulong was_anonymous;
	int id;
	
	debug("Trying to close FDs (req %d, ack %d)", req_fd, ack_fd);
	if (req_fd == -1 || ack_fd == -1) {
		return EAR_ERROR;
	}
	if ((id = get_local_connection_by_fd(eard_local_conn, req_fd, ack_fd)) < 0){
		error("FDs (%d, %d) not found in list of local connections", req_fd, ack_fd);
		return EAR_ERROR;
	}
	debug("Closing FDS (req %d, ack %d) communication (%lu, %lu)",
		  req_fd, ack_fd, eard_local_conn[id].jid, eard_local_conn[id].sid);
	was_anonymous = eard_local_conn[id].anonymous;
	job_id  = eard_local_conn[id].jid;
	step_id = eard_local_conn[id].sid;
	// Remove from fd_set and close
    AFD_CLR(req_fd, &rfds_basic);
    //AFD_CLR(ack_fd, &rfds_basic);

    clean_local_connection(&eard_local_conn[id]);
	close(req_fd);
	close(ack_fd);
	// End job
	if (!was_anonymous) {
		powermon_mpi_finalize(&handler_energy, job_id, step_id);
	}

	return EAR_SUCCESS;
}

state_t service_close_by_id(ulong jid,ulong sid)
{
	int i;
	do{
		i = get_local_connection_by_id(eard_local_conn, jid, sid);
		if (i >= 0){
			verbose(VEARD_LAPI,"Cancelling local connection %d",i);
			eard_local_conn[i].cancelled = 1;
		}
	}while (i>=0);
	return EAR_SUCCESS;
}

state_t service_listen(int req_fd, int ack_fd, struct daemon_req *req)
{ 
	size_t size;
	uint rpc;
	int i;

	if (req_fd != global_req_fd){
		i = get_local_connection_by_fd(eard_local_conn,req_fd,ack_fd);
		if ((i >= 0) && (i < num_local_con) && eard_local_conn[i].cancelled){ 
			verbose(VEARD_LAPI,"Local connection %d was cancelled",i);
			return EAR_ERROR;
		}
	}
  verbose(VEARD_LAPI, "New request. new connection %d", req_fd == global_req_fd);
	// Reading the header.
	if ((size = eards_read(req_fd, (char *) req, sizeof(eard_head_t), NO_WAIT_DATA)) != sizeof(eard_head_t)) {
		error("when reading the RPC header %ld: expected %lu bytes and received %lu", req->req_service, sizeof(eard_head_t), size);
		return EAR_ERROR;
	}
	// Reading the union part
	size = sizeof(union daemon_req_opt);
	rpc  = (req->req_service > NEW_API_SERVICES);
	verbose(VEARD_LAPI_DEBUG,"EARD new local call: %lu, new %u", req->req_service, rpc);
	// If is not RPC, complete the daemon_req.
	if (!rpc && (eards_read(req_fd, (char *) &req->req_data, size, NO_WAIT_DATA) != size)) {
		error("when reading union daemon_req: %s", strerror(errno));
		return EAR_ERROR;
	}
	// Checking if its privileged service
	if (!is_privileged_service(req->req_service)) {
		return EAR_SUCCESS;
	}
	if (!is_valid_sec_tag(req->sec)) {
		if (rpc) {
			// If is not valid, clean the rest of the requirement.
			if (state_fail(eard_rpc_clean(req_fd, req->size))) {
				serror("when cleaning RPC during service respond");
			}
			// And answer the error.
			if (state_fail(eard_rpc_answer(ack_fd, req->req_service, EAR_ERROR, NULL, 0, "Invalid security key"))) {
				serror("RPC answer during service respond");
			}
		}
		return EAR_ERROR;
	}
	return EAR_SUCCESS;
}

state_t service_answer(int req_fd, int ack_fd, uint call, state_t s, char *data, size_t size, char *error)
{ 
  if (state_fail(eard_rpc_answer(ack_fd, call, s, data, size, error))) {
    serror("Error during RPC answer");
    if (state_fail(service_close(req_fd, ack_fd))) {
      serror("Error during service close");
    }
  }
  return EAR_SUCCESS;
}

state_t service_answer_old(int req_fd, int ack_fd, char *data, size_t size)
{ 
  if (eards_write(ack_fd, data, size) != size) {
    error("Error during answer old: %s", strerror(errno));
    if (state_fail(service_close(req_fd, ack_fd))) {
      serror("Error during service close");
    }
  }
  return EAR_SUCCESS;
}

void service_select(int req_fd, int ack_fd)
{ 
	eard_head_t *local_req = (eard_head_t *) &req;
    
	// Hanging up
	if (state_fail(service_listen(req_fd, ack_fd, &req))) {
		verbose(VEARD_LAPI_DEBUG,"Closing local service");
		service_close(req_fd, ack_fd);
		return;
	}
	// New services
    if (services_misc(local_req, req_fd, ack_fd)) {
        return;
    }
    if(services_mgt_cpufreq(local_req, req_fd, ack_fd)) {
        return;
    }
    if(services_mgt_cpuprio(local_req, req_fd, ack_fd)) {
        return;
    }
	if(services_mgt_imcfreq(local_req, req_fd, ack_fd)) {
		return;
	}
    if(services_mgt_gpu(local_req, req_fd, ack_fd)) {
        return;
    }
    if (services_met_cpufreq(local_req, req_fd, ack_fd)) {
        return;
    }
    if (services_met_imcfreq(local_req, req_fd, ack_fd)) {
        return;
    }
    if (services_met_bwidth(local_req, req_fd, ack_fd)) {
        return;
    }
    if (services_met_temp(local_req, req_fd, ack_fd)) {
        return;
    }
    // Old services
    if (eard_rapl(local_req,req_fd,ack_fd)) {
        return;
    }
    if (eard_system(local_req,req_fd,ack_fd)) {
        return;
    }
    if (eard_node_energy(local_req,req_fd,ack_fd)) {
        return;
    }
    if (eard_gpu(local_req,req_fd,ack_fd)){
        return;
    }
    if (service_gpu(local_req, req_fd, ack_fd)){
        return;
    }
	
	error("Problem with request '%lu', answering error", local_req->req_service);
	eard_rpc_answer(ack_fd, local_req->req_service, EAR_ERROR, NULL, 0, "EARD RPC does not exist");
	// Closing, it is not necessary to control the error.
	service_close(req_fd,ack_fd);
}


state_t eard_local_api()
{
	ear_njob_t *eard_jobs_list;
	uint rapl_count;
	int numfds_ready;
	int ack_fd;
	state_t s;

	num_packs = node_desc.socket_count;

	verbose(VEARD_LAPI,"Creating node_mgr data");
    if (nodemgr_server_init(ear_tmp, &eard_jobs_list) != EAR_SUCCESS){
		verbose(VEARD_LAPI,"Error creating shared memory region for node_mgr");
		exit(1);
	}

	// IMC frequency metrics
	state_assert(s, imcfreq_count_devices(no_ctx, &unc_count), _exit(0));
	state_assert(s, imcfreq_data_alloc(&unc_data, NULL),       _exit(0));
	verbose(VEARD_LAPI,"CPUfreq and IMC freq data ok for node_services devices %u", unc_count);

	// Energy node metrics
    energy_datasize(&handler_energy,&node_energy_datasize);
    verbose(VEARD_LAPI,"EARD main thread: node_energy_datasize %lu",node_energy_datasize);
    node_energy_data=(edata_t)malloc(node_energy_datasize);
    energy_frequency(&handler_energy, &energy_freq);
    verbose(VCONF, "energy: power performance accuracy %lu usec", energy_freq);

	// RAPL metrics
	state_assert(s, energy_cpu_load(&node_desc, 0), _exit(0)); //no context and no eards
	state_assert(s, energy_cpu_init(no_ctx),        _exit(0));
	state_assert(s, energy_cpu_data_alloc(no_ctx, &values_rapl, &rapl_count), _exit(0));
    if (values_rapl==NULL) error("values_rapl returns NULL in eard initialization");
	verbose(VEARD_LAPI,"RAPL data allocated in node_serices");
	
    // HW initialized HERE...creating communication channels
	verbose(VEARD_LAPI, "Creating comm in %s for node %s", ear_tmp, nodename);

    AFD_ZERO(&rfds_basic);
    /* This function creates the pipe for new local connections */
    verbose(VEARD_LAPI,"Initializing local connections");
    if (init_local_connections(&eard_local_conn,node_desc.cpu_count) != EAR_SUCCESS){
        error("Error initializing local connections");
        _exit(0);
    }

    verbose(VEARD_LAPI,"Creating global connector");
    create_global_connector(ear_tmp, nodename);

	verbose(VEARD_LAPI, "Communicator for %s ON", nodename);

	// we wait until EAR daemon receives a request
	// We support requests realted to frequency and to uncore counters
	// rapl support is pending to be supported
	// EAR daemon main loop

    verbose(VEARD_LAPI, "EARD local API set up. Waiting for other threads...");
    pthread_barrier_wait(&setup_barrier);

	/*
	*
	*	MAIN LOOP 
	*
	*/
    while (((numfds_ready = aselectv(&rfds_basic, NULL)) >= 0) ||
           ((numfds_ready < 0) && (errno == EINTR)))
    {
        debug("select unblock with numfds_ready %d", numfds_ready);
        if (eard_must_exit) {
            verbose(0, "eard exiting");
            eard_exit(2);
        }
        if (numfds_ready >= 0) {
            if (numfds_ready > 0) {
                for (uint i = rfds_basic.fd_min; i <= rfds_basic.fd_max; i++) {
                    // Instruction manual for dummies:
                    //
                    // REQ is for request/demand, from libear to eard
                    // ACK is for answer/result, from eard to libear
                    //
                    // - When new connection, i is equal to global_req_fd,
                    //   the service_select detects that is global_req_fd
                    //   and then reads the petition which must be
                    //   CONNECT_EARD_NODE_SERVICES, and then the function
                    //   connect_services is called. This function, creates
                    //   two files in TMP (comm.req and comm.ack).
                    if (AFD_ISSET(i, &rfds_basic)) {
                        if (i == global_req_fd) {
                            /* New connection */
                            ack_fd = 0;
                        } else {
                            /* We need to find the local req and ack fds */
                            ack_fd = get_local_connection_ack(eard_local_conn, i);
                        }
                        if (ack_fd >= 0) {
                            service_select(i, ack_fd);
                        } else {
                            error("at eard_node_services: ack_fd is %d. This shouldn't happen.", ack_fd);
                            _exit(0);
                        }
                    }    // IF AFD_ISSET
                } //for
                // We have to check if there is something else
            } else {
                debug("select unblocks with numfds_ready  = 0");
            }
        } else {// Signal received
            debug("signal received");
        }
        // debug("eard waiting.....\n");
    }//while

	verbose(VEARD_LAPI,"Releasing node_mgr data ");
	nodemgr_server_end();

	return EAR_SUCCESS;
}
