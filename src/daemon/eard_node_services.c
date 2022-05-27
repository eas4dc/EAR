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

//#define SHOW_DEBUGS 1
#define VEARD_LAPI_DEBUG 2
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <linux/limits.h>

#include <common/includes.h>
#include <common/environment.h>
#include <common/system/fd_sets.h>
#include <common/system/monitor.h>
#include <common/types/generic.h>
#include <common/types/pc_app_info.h>
#include <common/utils/serial_buffer.h>
#include <common/hardware/hardware_info.h>
#include <management/cpufreq/frequency.h>
#include <management/imcfreq/imcfreq.h>
#include <metrics/common/msr.h>
#include <metrics/gpu/gpu.h>
#include <metrics/energy/cpu.h>
#include <metrics/energy/energy_node.h>
#include <metrics/bandwidth/bandwidth.h>
#include <metrics/cpufreq/cpufreq.h>
#include <metrics/imcfreq/imcfreq.h>
#include <report/report.h>
#include <daemon/eard_node_services.h>
#include <daemon/local_api/eard_api_conf.h>
#include <daemon/local_api/eard_api_rpc.h>
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

typedef struct local_connection{
	ulong jid,sid,lid,anonymous;
	int fds[NUM_LOCAL_FDS];
	int cancelled;
} local_connection_t;

uint privileged_services[NUM_PRIVILEGED_SERVICES] =
		{ 0, 0, 0, 0, 0,
		  START_RAPL, RESET_RAPL, WRITE_APP_SIGNATURE, WRITE_EVENT, WRITE_LOOP_SIGNATURE,
		  GPU_SET_FREQ };

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
#if USE_GPUS
extern uint             eard_gpu_model;
extern gpu_t           *eard_read_gpu;
extern uint             eard_num_gpus;
extern char             gpu_str[256];
static ctx_t            eard_main_gpu_ctx;
static uint             eard_gpu_initialized=0;
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
static int              RAPL_counting = 0;
/* This var is used in all the eard services, no thread-safe */
static struct daemon_req req;
/* FDs for local connections */
int                     numfds_req = 0;
static fd_set           rfds;
static fd_set           rfds_basic;
int                     total_local_fd=0;
static local_connection_t *eard_local_conn;
static uint             num_local_con=0;

typedef struct apis_svc_s {
    ctx_t c;
    uint  api;
    uint  working;
    uint  granularity;
    uint  devs_count;
    uint  avail_count;
    void *current_list;
    void *avail_list;
    void *set_list;
} apis_svc_t;

static char          big_chunk1[16384];
static char          big_chunk2[16384];
static wide_buffer_t wb;
static metrics_t     mc;
static metrics_t     mi;
static metrics_t     uc;
static metrics_t     ui;
static metrics_t     ub;

#define sf(function) \
	if (state_fail(s = function)) { \
		error("when calling " #function); \
	}

#define dcase_init(id) \
	switch (call) { \
	case id: \
	//printf("Called " #id "\n");

#define dcase(id) \
	break; \
	case id: \
	//printf("Called " #id "\n");

#define dcase_fini() \
	break; \
	default: return 0; \
	}

void services_init(topology_t *tp, my_node_conf_t *conf)
{
	state_t s;

	// DONE
	mgt_imcfreq_load(tp, NO_EARD, conf);
	mgt_cpufreq_load(tp, NO_EARD);
        cpufreq_load(tp, NO_EARD);
	// DONE
        imcfreq_load(tp, NO_EARD);
         bwidth_load(tp, NO_EARD);

	mgt_cpufreq_get_api((uint *) &mc.api);
	mgt_imcfreq_get_api((uint *) &mi.api);
        cpufreq_get_api((uint *) &uc.api);
        imcfreq_get_api((uint *) &ui.api);
         bwidth_get_api((uint *) &ub.api);

	// DONE
	sf(mgt_cpufreq_init(no_ctx));
	sf(mgt_imcfreq_init(no_ctx));
    sf(    cpufreq_init(no_ctx));
	// DONE
    sf(    imcfreq_init(no_ctx));
    sf(     bwidth_init(no_ctx));

	sf(mgt_cpufreq_count_devices(no_ctx, &mc.devs_count));
	sf(mgt_imcfreq_count_devices(no_ctx, &mi.devs_count));
    sf(    cpufreq_count_devices(no_ctx, &uc.devs_count));
    sf(    imcfreq_count_devices(no_ctx, &ui.devs_count));
    sf(     bwidth_count_devices(no_ctx, &ub.devs_count));

	sf(mgt_cpufreq_get_available_list(no_ctx, (const pstate_t **) &mc.avail_list, &mc.avail_count));
	sf(mgt_imcfreq_get_available_list(no_ctx, (const pstate_t **) &mi.avail_list, &mi.avail_count));

    sf(mgt_cpufreq_data_alloc((pstate_t  **) &mc.current_list, (uint **) &mc.set_list));
    sf(mgt_cpufreq_data_alloc((pstate_t  **) &mi.current_list, (uint **) &mi.set_list));
    sf(    cpufreq_data_alloc((cpufreq_t **) &uc.current_list, empty));
    sf(    imcfreq_data_alloc((imcfreq_t **) &ui.current_list, empty));
    sf(     bwidth_data_alloc((bwidth_t  **) &ub.current_list));

	serial_alloc(&wb, SIZE_8KB);
}

void services_dispose()
{

}

void connect_service(struct daemon_req *new_req);

int services_misc(eard_head_t *head, int req_fd, int ack_fd)
{
    uint call = head->req_service;

		verbose(VEARD_LAPI_DEBUG, "EARD misc");
    dcase_init(CONNECT_EARD_NODE_SERVICES);
        connect_service(&req);
    dcase(DISCONNECT_EARD_NODE_SERVICES);
        eard_close_comm(req_fd,ack_fd);
    dcase_fini();

    return 1;
}

int services_mgt_cpufreq(eard_head_t *head, int req_fd, int ack_fd)
{
    uint call = head->req_service;
    state_t s = EAR_SUCCESS;
    char *data = NULL;
    size_t size = 0;

		verbose(VEARD_LAPI_DEBUG, "EARD mgt_cpufreq");
    dcase_init(RPC_MGT_CPUFREQ_GET_API);
        data = (char *) &mc.api;
        size = sizeof(uint);
    dcase(RPC_MGT_CPUFREQ_GET_NOMINAL);
        if (state_fail(s = mgt_cpufreq_get_nominal(no_ctx, (uint *) big_chunk1))) {
            serror("Could not get the current nominal frequency");
        }
        data = big_chunk1;
        size = sizeof(uint);
    dcase(RPC_MGT_CPUFREQ_GET_AVAILABLE);
        serial_clean(&wb);
        //
        serial_add_elem(&wb, (char *) &mc.avail_count, sizeof(uint));
        serial_add_elem(&wb, (char *) mc.avail_list, sizeof(pstate_t)*mc.avail_count);
        // Setting to send
        size = serial_size(&wb);
        data = serial_data(&wb);
    dcase(RPC_MGT_CPUFREQ_GET_CURRENT);
        if (state_fail(s = mgt_cpufreq_get_current_list(no_ctx, mc.current_list))) {
            serror("Could not get the current management frequency");
        }
        data = (char *) mc.current_list;
        size = mc.devs_count * sizeof(pstate_t);
    dcase(RPC_MGT_CPUFREQ_GET_GOVERNOR);
        if (state_fail(s = mgt_cpufreq_get_governor(no_ctx, (uint *) big_chunk1))) {
            serror("Could not get the current governor");
        }
        data = big_chunk1;
        size = sizeof(uint);
    dcase(RPC_MGT_CPUFREQ_SET_CURRENT_LIST);
        size =  mc.devs_count * sizeof(uint);
        if (state_fail(s = eard_rpc_read_pending(req_fd, mc.set_list, head->size, size))) {
            serror(Rpcerr.pending);
        }
        if (state_ok(s)) {
            if (state_fail(s = mgt_cpufreq_set_current_list(no_ctx, (uint *) mc.set_list))) {
                serror("When setting a CPU frequency list");
            }
            size = 0;
        }
    dcase(RPC_MGT_CPUFREQ_SET_CURRENT);
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
    dcase(RPC_MGT_CPUFREQ_SET_GOVERNOR);
        if (state_fail(s = eard_rpc_read_pending(req_fd, big_chunk1, head->size, sizeof(uint)))) {
            serror(Rpcerr.pending);
        }
        if (state_ok(s)) {
            if (state_fail(s = mgt_cpufreq_set_governor(no_ctx, (uint) big_chunk1[0]))) {
                serror(Rpcerr.answer);
            }
        }
    dcase_fini();

    // Always answering (if s != SUCCESS the error message is sent)
    if (state_fail(eard_rpc_answer(ack_fd, call, s, data, size, state_msg))) {
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
	dcase_init(RPC_MGT_IMCFREQ_GET_API);
		data = (char *) &mi.api;
		size = sizeof(uint);
	dcase(RPC_MGT_IMCFREQ_GET_AVAILABLE);
		serial_clean(&wb);
		//
		serial_add_elem(&wb, (char *) &mi.avail_count, sizeof(uint));
		serial_add_elem(&wb, (char *) mi.avail_list, sizeof(pstate_t)*mi.avail_count);
		// Setting to send
		size = serial_size(&wb);
		data = serial_data(&wb);
	dcase(RPC_MGT_IMCFREQ_GET_CURRENT);
		if (state_fail(s = mgt_imcfreq_get_current_list(no_ctx, mi.current_list))) {
			serror("Could not get IMC current frequency");
		}
		data = (char *) mi.current_list;
		size = sizeof(pstate_t)*mi.devs_count;
	dcase(RPC_MGT_IMCFREQ_SET_CURRENT);
		if (state_fail(s = eard_rpc_read_pending(req_fd, (char *) mi.set_list, head->size, 0))) {
			serror(Rpcerr.pending);
		} else {
		    if (state_fail(s = mgt_imcfreq_set_current_list(no_ctx, mi.set_list))) {
			    serror("Could not set IMC current frequency");
            }
        }
	dcase(RPC_MGT_IMCFREQ_GET_CURRENT_MIN);
		serial_clean(&wb);
		//
		if (state_fail(s = mgt_imcfreq_get_current_ranged_list(no_ctx, (pstate_t *) big_chunk1, (pstate_t *) big_chunk2))) {
			serror("Could not get IMC current ranged frequency");
		} else {
			serial_add_elem(&wb, big_chunk1, sizeof(pstate_t)*mi.devs_count);
			serial_add_elem(&wb, big_chunk2, sizeof(pstate_t)*mi.devs_count);
			size = serial_size(&wb);
			data = serial_data(&wb);
		}
	dcase(RPC_MGT_IMCFREQ_SET_CURRENT_MIN);
		serial_reset(&wb, head->size);
		if (state_fail(s = eard_rpc_read_pending(req_fd, serial_data(&wb), head->size, 0))) {
			serror(Rpcerr.pending);
		} else {
			serial_copy_elem(&wb, (char *) big_chunk1, NULL);
			serial_copy_elem(&wb, (char *) big_chunk2, NULL);
			s = mgt_imcfreq_set_current_ranged_list(no_ctx, (uint *) big_chunk1, (uint *) big_chunk2);
		}
	dcase_fini();
	// Always answering (if s != SUCCESS the error message is sent)
	if (state_fail(eard_rpc_answer(ack_fd, call, s, data, size, state_msg))) {
        serror("RPC answer failed");
		return 0;
	}
	return 1;
}

int services_mgt_gpu(eard_head_t *head, int req_fd, int ack_fd)
{
    uint call = head->req_service;
    state_t s = EAR_SUCCESS;
    char *data = NULL;
    size_t size = 0;

		verbose(VEARD_LAPI_DEBUG, "EARD mgt_cpu");
    // Always answering (if s != SUCCESS the error message is sent)
    if (state_fail(eard_rpc_answer(ack_fd, call, s, data, size, state_msg))) {
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
    dcase_init(RPC_MET_CPUFREQ_GET_API);
        data = (char *) &uc.api;
        size = sizeof(uint);
    dcase(RPC_MET_CPUFREQ_GET_CURRENT);
        if(state_fail(s = cpufreq_read(no_ctx, (cpufreq_t *) uc.current_list))) {
            serror("Could not get cpu frequency (aperf)");
        }
        data = (char *) uc.current_list;
        size = sizeof(cpufreq_t) * uc.devs_count;
    dcase_fini();

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
    dcase_init(RPC_MET_IMCFREQ_GET_API);
        data = (char *) &ui.api;
        size = sizeof(uint);
    dcase(RPC_MET_IMCFREQ_COUNT_DEVICES);
        data = (char *) &ui.devs_count;
        size = sizeof(uint);
    dcase(RPC_MET_IMCFREQ_GET_CURRENT);
        if(state_fail(s = imcfreq_read(no_ctx, (imcfreq_t *) ui.current_list))) {
            serror("Could not get imc frequency");
        }
        data = (char *) ui.current_list;
        size = sizeof(imcfreq_t) * ui.devs_count;
    dcase_fini();

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
    dcase_init(RPC_MET_BWIDTH_GET_API);
        data = (char *) &ub.api;
        size = sizeof(uint);
    dcase(RPC_MET_BWIDTH_COUNT_DEVICES);
        data = (char *) &ub.devs_count;
        size = sizeof(uint);
    dcase(RPC_MET_BWIDTH_READ);
        if(state_fail(s = bwidth_read(no_ctx, (bwidth_t *) ub.current_list))) {
            serror("Could not get imc frequency");
        }
        data = (char *) ub.current_list;
        size = sizeof(bwidth_t) * ub.devs_count;
    dcase_fini();

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
    return_msg(EAR_ERROR, "Local connection not found");
  }
  //debug("ACK for %d found at pos %d = %d",reqfd,i,my_con[i].fds[ACK_LOCAL_FD]);
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
  // Creates a pipe to receive requests for applications. eard creates 1 pipe (per node) and service to receive requests
  sprintf(ear_commreq_global, "%s/.ear_comm.globalreq", ear_tmp);
	verbose(VEARD_LAPI,"Global connector placed at %s",ear_commreq_global);
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
  new_fd(&rfds_basic,global_req_fd,&numfds_req,&total_local_fd);
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
  // Let's check if there is another application
  verbose(VEARD_LAPI, "request for connection at service (%lu,%lu)",new_job->id,new_job->step_id);

  /* Creates 1 pipe (per node),  jobid and step id to send acks. pid is a combination of jobid and stepid */
  verbose(VEARD + 1, "New local connection job_id=%lu step_id=%lu local_id %lu\n", new_job->id, new_job->step_id,new_req->con_id.lid);
  /* This pipe is to send data, 1 per connection */
  sprintf(ear_commack, "%s/%u/.ear_comm.ack_0.%d.%lu", ear_tmp, ID, pid,lid);
  sprintf(ear_commreq, "%s/%u/.ear_comm.req_0.%d.%lu", ear_tmp, ID, pid,lid);
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
  //new_fd(&rfds_basic,ear_ack_fd,&numfds_req,&total_local_fd);
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
  new_fd(&rfds_basic,ear_req_fd,&numfds_req,&total_local_fd);
  /* This function notifies the local connection */
  if (!eard_local_conn[newc].anonymous) powermon_mpi_init(&handler_energy, new_app);
  debug("Application %d connected with local API (anonymous %lu)",pid,eard_local_conn[newc].anonymous);
} 

void eard_close_comm(int req_fd,int ack_fd)
{
  int i, ret;
  int id;
  ulong jid,sid;
  ulong was_anonymous = 1;
  char ear_commack[MAX_PATH_SIZE * 2];
  char ear_commreq[MAX_PATH_SIZE * 2];

  /* This is a special case where one of the fds has been closed */
  if ((req_fd == -1) && (ack_fd == -1)){
    for (i=0;i<numfds_req;i++){
			/* What about was_anonymous ? */
      if (check_fd(&rfds_basic,i) == EAR_ERROR){
        id = get_local_connection_by_req(eard_local_conn,i);
        close(i);
        close(eard_local_conn[id].fds[ACK_LOCAL_FD]);
        clean_fd(&rfds_basic,i,&numfds_req,&total_local_fd);
        clean_fd(&rfds_basic,eard_local_conn[id].fds[ACK_LOCAL_FD],&numfds_req,&total_local_fd);
        clean_local_connection(&eard_local_conn[id]);
      }
    }
  }else if (req_fd == CLOSE_PIPE){
		/* SIGPIPE received  in eard */
		verbose(VEARD_LAPI,"SIGPIPE received in EARD: PENDING");
	}else if (req_fd == CLOSE_EARD){
		/* End eard */
		verbose(VEARD_LAPI,"Releasing all the files: PENDING");
	}else{ /* this is the normal use case */
    id = get_local_connection_by_fd(eard_local_conn,req_fd,ack_fd);
    if (id < 0){
      error("We cannot find (%d,%d) in the list of local connections",req_fd,ack_fd);
      return;
    }
    debug("Closing communication for (%lu,%lu)",eard_local_conn[id].jid,eard_local_conn[id].sid);
    jid = eard_local_conn[id].jid;
    sid = eard_local_conn[id].sid;
		ulong lid = eard_local_conn[id].lid;
		uint ID = create_ID(jid, sid);
  	sprintf(ear_commack, "%s/%u/.ear_comm.ack_0.%u.%lu", ear_tmp, ID, ID,lid);
  	sprintf(ear_commreq, "%s/%u/.ear_comm.req_0.%u.%lu", ear_tmp, ID, ID,lid);
		verbose(VEARD_LAPI,"Cleaning files for job %lu/%lu/%lu",jid,sid,lid);
		verbose(VEARD_LAPI,"Deleting %s",ear_commack);
		ret = unlink(ear_commack);
		if (ret != ENOENT) verbose(VEARD_LAPI,"Error deleting file %s",strerror(errno));
		verbose(VEARD_LAPI,"Deleting %s",ear_commreq);
		ret = unlink(ear_commreq);
		if (ret != ENOENT) verbose(VEARD_LAPI,"Error deleting file %s",strerror(errno));
    was_anonymous = eard_local_conn[id].anonymous;
    if (req_fd != -1){
      close(req_fd);
      clean_fd(&rfds_basic,req_fd,&numfds_req,&total_local_fd);
    }
    if (ack_fd != -1){
      close(ack_fd);
      clean_fd(&rfds_basic,ack_fd,&numfds_req,&total_local_fd);
    }
    debug("application (%lu,%lu) disconnected (anonymous %lu)", jid,sid,was_anonymous);
    clean_local_connection(&eard_local_conn[id]);
    // This debug was set outside, but it requires a valid id to prevent segmentation faults.
  }
  if (!was_anonymous) {
    powermon_mpi_finalize(&handler_energy,jid,sid);
  }
}

int eard_gpu(eard_head_t *head, int req_fd, int ack_fd)
{
    unsigned long ack = 0;
#if USE_GPUS
    gpu_info_t *info;
	gpu_info_t fake;

	verbose(VEARD_LAPI_DEBUG,"EARD old eard_gpu");
	switch (head->req_service) {
		case GPU_MODEL:
			debug("GPU_MODEL %d",eard_gpu_model);
			write(ack_fd,&eard_gpu_model,sizeof(eard_gpu_model));
		break;
		case GPU_DEV_COUNT:
			debug("GPU_DEV_COUNT %d",eard_num_gpus);
			write(ack_fd,&eard_num_gpus,sizeof(eard_num_gpus));
			break;
		case GPU_GET_INFO:
			debug("GPU_GET_INFO %d", eard_num_gpus);
			if (eard_gpu_initialized) {
				gpu_get_info(&eard_main_gpu_ctx, (const gpu_info_t **) &info, NULL);
			} else {
				memset(&fake, 0, sizeof(gpu_info_t));
				info = &fake;
			}
			write(ack_fd, info, sizeof(gpu_info_t)*eard_num_gpus);
			break;
		case GPU_DATA_READ:
			if (eard_gpu_initialized){
				gpu_read(&eard_main_gpu_ctx,eard_read_gpu);
			}else{
				memset(eard_read_gpu,0,sizeof(gpu_t));
			}
			debug("Sending %lu bytes en gpu_data_read",sizeof(gpu_t)*eard_num_gpus);
			gpu_data_tostr(eard_read_gpu,gpu_str,sizeof(gpu_str));
			debug("gpu_data_read info: %s",gpu_str);
			write(ack_fd,eard_read_gpu,sizeof(gpu_t)*eard_num_gpus);
			break;
		case GPU_SET_FREQ:
			if (eard_gpu_initialized){
				debug("Setting the GPU freq");
				gpu_mgr_set_freq(req.req_data.gpu_freq.num_dev,req.req_data.gpu_freq.gpu_freqs);
			}else{
				error("GPU not initialized and GPU_SET_FREQ requested");
			}
			ack=EAR_SUCCESS;
			write(ack_fd,&ack,sizeof(ack));
			break;
	  default:
			error("Invalid GPU command");
      return 0;
  }
#else
    switch (head->req_service) {
        case GPU_MODEL:
        case GPU_DEV_COUNT:
        case GPU_DATA_READ:
        case GPU_SET_FREQ:
        case GPU_GET_INFO:
            ack=EAR_ERROR;
            write(ack_fd,&ack,sizeof(ack));
            error("GPUS not supported");
            break;
        default: break;
    }
    return 0;
#endif
    return 1;
}

int eard_node_energy(eard_head_t *head,int req_fd,int ack_fd)
{
  unsigned long ack;
	verbose(VEARD_LAPI_DEBUG, "EARD old node_energy");
  switch (head->req_service) {
    case READ_DC_ENERGY:
      energy_dc_read(&handler_energy, node_energy_data);
      write(ack_fd, node_energy_data, node_energy_datasize);
      break;
    case DATA_SIZE_ENERGY_NODE:
      energy_datasize(&handler_energy, &ack);
      write(ack_fd, &ack, sizeof(unsigned long));
      break;
    case ENERGY_FREQ:
      ack = (ulong) energy_freq;
      write(ack_fd, &ack, sizeof(unsigned long));
      break;
/* Add new_services for RAPL here. Use req_fd to read pending data and ack_fd to send data*/
    default:
      return 0;
  }
  return 1;
}

int eard_system(eard_head_t *head,int req_fd,int ack_fd)
{
  unsigned long ack;
  int con_id;
  con_id = get_local_connection_by_fd(eard_local_conn,req_fd,ack_fd);

	verbose(VEARD_LAPI_DEBUG,"EARD old system");
  switch (head->req_service) {
    case WRITE_APP_SIGNATURE:
			verbose(VEARD_LAPI,"App signature received: Anonymous %lu", eard_local_conn[con_id].anonymous);
      if (!eard_local_conn[con_id].anonymous) powermon_mpi_signature(&req.req_data.app);
      ack = EAR_COM_OK;
      if (write(ack_fd, &ack, sizeof(ulong)) != sizeof(ulong)) {
        error("ERROR while writing system service ack, closing connection...");
        eard_close_comm(req_fd,ack_fd);
      }

      break;
    case WRITE_EVENT:
      ack = EAR_COM_OK;
			report_connection_status = report_events(&rid, &req.req_data.event, 1);
      if (report_connection_status == EAR_SUCCESS) ack = EAR_COM_OK;
      else ack = EAR_COM_ERROR;

      write(ack_fd, &ack, sizeof(ulong));
      break;
    case WRITE_LOOP_SIGNATURE:
      ack = EAR_COM_OK;
      powermon_loop_signature(eard_local_conn[con_id].jid,eard_local_conn[con_id].sid,&req.req_data.loop);
      // print_loop_fd(1,&req.req_data.loop);
			report_connection_status = report_loops(&rid, &req.req_data.loop, 1);

      if (report_connection_status == EAR_SUCCESS) ack = EAR_COM_OK;
      else ack = EAR_COM_ERROR;

      write(ack_fd, &ack, sizeof(ulong));
      break;
/* Add new_services for RAPL here. Use req_fd to read pending data and ack_fd to send data*/
    default:
      return 0;
  }
  return 1;
}

int eard_rapl(eard_head_t *head,int req_fd,int ack_fd)
{
  unsigned long ack = 0;

  verbose(VEARD_LAPI_DEBUG, "EARD old eard_rapl");
  switch (head->req_service) {
    case START_RAPL:
      error("starting rapl msr not supported");
      RAPL_counting = 1;
      write(ack_fd, &ack, sizeof(ack));
      break;
    case RESET_RAPL:
      error("Reset RAPL counters not provided");
      write(ack_fd, &ack, sizeof(ack));
      break;
    case READ_RAPL:
      if (read_rapl_msr(fd_rapl,values_rapl)<0) error("Error reading rapl msr counters");
      write(ack_fd, values_rapl, sizeof(unsigned long long) * RAPL_POWER_EVS*num_packs);
      break;
    case DATA_SIZE_RAPL:
      ack = sizeof(unsigned long long) * RAPL_POWER_EVS*num_packs;
      write(ack_fd, &ack, sizeof(unsigned long));
      break;
    /* Add new_services for RAPL here. Use req_fd to read pending data and ack_fd to send data*/
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
	clean_fd(&rfds_basic, req_fd, &numfds_req, &total_local_fd);
	clean_fd(&rfds_basic, ack_fd, &numfds_req, &total_local_fd);
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
		error("when reading the RPC header: expected %lu bytes and received %lu", sizeof(eard_head_t), size);
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
	if(services_mgt_imcfreq(local_req, req_fd, ack_fd)) {
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
	
	error("Problem with request '%lu', answering error", local_req->req_service);
	eard_rpc_answer(ack_fd, local_req->req_service, EAR_ERROR, NULL, 0, "EARD RPC does not exist");
	// Closing, it is not necessary to control the error.
	service_close(req_fd,ack_fd);
}


state_t eard_local_api()
{
	ear_njob_t *eard_jobs_list;
	struct timeval *my_to;
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
  values_rapl=(unsigned long long*)calloc(num_packs*RAPL_POWER_EVS,sizeof(unsigned long long));
  if (values_rapl==NULL) error("values_rapl returns NULL in eard initialization");

	verbose(VEARD_LAPI,"RAPL data allocated in node_serices");

	// HW initialized HERE...creating communication channels
	verbose(VEARD_LAPI, "Creating comm in %s for node %s", ear_tmp, nodename);

	#if USE_GPUS
	/* GPU Initialization for app requests */
	uint ret = gpu_init(&eard_main_gpu_ctx);
	if (ret != EAR_SUCCESS){
		error("EARD error when initializing GPU context for EARD API queries");
		eard_gpu_initialized = 0;	
	}else{
		eard_gpu_initialized = 1;
	}
	gpu_data_alloc(&eard_read_gpu);
	gpu_count(&eard_main_gpu_ctx,&eard_num_gpus);
	debug("gpu_count %u",eard_num_gpus);
	eard_gpu_model=gpu_model();
	debug("gpu_model %u",eard_gpu_model);
	#endif

  clean_fd_set(&rfds_basic,&total_local_fd);
  /* This function creates the pipe for new local connections */
  verbose(VEARD_LAPI,"Initializing local connections");
  if (init_local_connections(&eard_local_conn,node_desc.cpu_count) != EAR_SUCCESS){
    error("Error initializing local connections");
    _exit(0);
  }
	verbose(VEARD_LAPI,"Creating global connector");
  create_global_connector(ear_tmp, nodename);
  rfds = rfds_basic;

	verbose(VEARD_LAPI, "Communicator for %s ON", nodename);
	// we wait until EAR daemon receives a request
	// We support requests realted to frequency and to uncore counters
	// rapl support is pending to be supported
	// EAR daemon main loop
	my_to = NULL;
	/*
	*
	*	MAIN LOOP 
	*
	*/
  while (((numfds_ready = select(numfds_req, &rfds, NULL, NULL, my_to)) >= 0) ||
       ((numfds_ready < 0) && (errno == EINTR)))
  {
		debug("select unblock with numfds_ready %d", numfds_ready);
    if (eard_must_exit) {
      verbose(0, "eard exiting");
      eard_exit(2);
    }
    if (numfds_ready >= 0) {
      if (numfds_ready > 0) {
        for (uint i = 0; i < numfds_req; i++) {
          if (FD_ISSET(i, &rfds)) {
            if (i == global_req_fd){
              /* New connection */
            }else{
              /* We need to find the local req and ack fds */
              ack_fd = get_local_connection_ack(eard_local_conn,i);
            }
						debug("Reading from %d ack %d",i, ack_fd);
            if (ack_fd >=0) service_select(i,ack_fd);
            else _exit(0);
          }    // IF FD_ISSET
        } //for
        // We have to check if there is something else
      }else{
				debug("select unblocks with numfds_ready  = 0");
			}
    } else {// Signal received
      my_to = NULL;
      debug("signal received");
    }
    rfds = rfds_basic;
		debug("Waiting for %d fds. global req fd %d", numfds_req, global_req_fd);
    //debug("eard waiting.....\n");
  }//while
	verbose(VEARD_LAPI,"Releasing node_mgr data ");
	nodemgr_server_end();

	return EAR_SUCCESS;
}
