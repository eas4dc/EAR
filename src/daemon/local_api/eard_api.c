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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#define _GNU_SOURCE             
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sched.h>
#include <time.h>
#include <pthread.h>

//#define SHOW_DEBUGS 1
#include <common/config.h>
#include <common/states.h>
#include <common/system/lock.h>
#include <common/output/verbose.h>
#include <common/types/generic.h>
#include <common/system/poll.h>
#include <metrics/common/apis.h>
#include <common/types/application.h>
#include <daemon/local_api/eard_api.h>

#define MAX_PIPE_TRIES       (MAX_SOCKET_COMM_TRIES*10)
#define MAX_TRIES            1
#define USE_NON_BLOCKING_IO  1

/* These pipes are used for local communication between ear library and eard */
char ear_commreq_global[SZ_PATH]; 		/* This pipe connects with EARD, the same for all the jobs,
                                         * then a specific per job-step-localid is created */
char ear_commreq[SZ_PATH];
char ear_commack[SZ_PATH];
int ear_fd_req_global = -1;
int ear_fd_req = -1;
int ear_fd_ack = -1;

afd_set_t eard_api_client_fd;

static uint8_t app_connected = 0;
static app_id_t local_con_info;
static int my_step_id;
static int my_job_id;
static char *ear_tmp;
// Metrics: Needed for old API where data size must be requested in advance
static ulong energy_size;
static ulong rapl_size;

extern pthread_mutex_t lock_rpc;

#define CLOSE_LOCAL_COMM()    \
  close(ear_fd_req_global); \
  ear_fd_req_global = -1;   \
  close(ear_fd_ack);        \
  ear_fd_ack = -1;          \
  unlink(ear_commack);      \
  unlink(ear_commreq);

int eards_read(int fd,char *buff,int size, uint wait_mode)
{
#if USE_NON_BLOCKING_IO
  int ret; 
  int tries=0;
  uint to_recv,received=0;
  uint must_abort=0;
  to_recv=size;
  do
  {
    ret = read(fd, buff+received,to_recv);
    if (ret > 0){
      received+=ret;
      to_recv-=ret;
    }else if (ret < 0){
      if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) tries++;
      else{
      	debug("Error receiving data from eard %s,%d",strerror(errno),errno);
        must_abort = 1;
      }
    }else if (ret == 0){
			if (wait_mode == WAIT_DATA) tries++;
			else 												must_abort = 1;
    }
  }while((tries<MAX_PIPE_TRIES) && (to_recv>0) && (must_abort==0));
	if (tries >= MAX_PIPE_TRIES) {
		error("Error reading data, max number of tries reached. tries %d received %u (last ret %d errno %d)",tries, received, ret,errno);
	}
	if (ret<0) return ret;
	return received;

#else
  return read(fd,buff,size);
#endif
}

int eards_write(int fd,char *buff,int size)
{
#if USE_NON_BLOCKING_IO
  int ret;
  int tries=0;
  uint to_send,sended=0;
  uint must_abort=0;
  to_send=size;
  do
  {
    ret=write(fd, buff+sended,size);
    if (ret>0){
      sended+=ret;
      to_send-=ret;
    }else if (ret<0){
      if ((errno==EWOULDBLOCK) || (errno==EAGAIN)) tries++;
      else{
        must_abort=1;
        debug("Error sending command to eard %s,%d",strerror(errno),errno);
      }
    }else if (ret==0){
      must_abort=1;
    }
  }while((tries<MAX_PIPE_TRIES) && (to_send>0) && (must_abort==0));
  /* If there are bytes left to send, we return a 0 */
	if (ret<0) return ret;
	return sended;

#else
  return write(fd,buff,size);
#endif
}

void eards_signal_handler(int s)
{
	debug("EARD has been disconnected");
	mark_as_eard_disconnected(my_job_id,my_step_id,getpid());
	app_connected=0;
}

void eards_signal_catcher()
{
    struct sigaction action;
    sigset_t earl_mask;
    
    sigfillset(&earl_mask);
    sigdelset(&earl_mask,SIGPIPE);
    sigprocmask(SIG_UNBLOCK,&earl_mask,NULL);
    sigemptyset(&action.sa_mask);
    action.sa_handler = eards_signal_handler;
    action.sa_flags = 0;
    
    if (sigaction(SIGPIPE, &action, NULL) < 0) {
    	error( "sigaction error on signal s=%d (%s)", SIGPIPE, strerror(errno));
    }
}    


// If this approach is selected, we will substitute the function call by the key
ulong create_sec_tag()
{
		// You make define a constant at makefile time called SEC_KEY to use that option
    #if SEC_KEY
    return (ulong)SEC_KEY;
    #else
    return (ulong)0;
    #endif

}

uint warning_api(int return_value, int expected, char *msg)
{
	if (return_value != expected){ 
		app_connected=0;
		debug("eards: %s (returned %d expected %d)", msg,return_value,expected);
	}
	if (return_value < 0){ 
		app_connected=0;
		debug("eards: %s (%s)", msg,strerror(errno));
	}
	return (return_value != expected);
}

int eards_connected()
{
  #if FAKE_EAR_NOT_INSTALLED
  return 0;
  #endif
	return app_connected;
}

/* 
 * This is an anonymoous connection
 */
state_t eards_connection()
{
  application_t my_dummy_app;
  my_dummy_app.job.id = 0;
  my_dummy_app.job.step_id = 0;
  return eards_connect(&my_dummy_app,getpid());
}

int eards_connect(application_t *my_app, ulong lid)
{
  char nodename[256];
  ulong ack;
  struct daemon_req req;
  int tries=0,connected=0;
  int i;
  int my_id;
  int ret1;
  if (app_connected) return EAR_SUCCESS;

  AFD_ZERO(&eard_api_client_fd);

  #if FAKE_EAR_NOT_INSTALLED
  return_print(EAR_ERROR, "FAKE_EAR_NOT_INSTALLED applied");
  #endif

  // These files connect EAR with EAR_COMM
  ear_tmp=ear_getenv(ENV_PATH_TMP);

  if (ear_tmp == NULL)
  {
    ear_tmp=ear_getenv("TMP");

    if (ear_tmp == NULL) {
      ear_tmp=ear_getenv("HOME");
    }
  }

  if (gethostname(nodename,sizeof(nodename)) < 0){
    error("ERROR while getting the node name (%s)", strerror(errno));
    return EAR_ERROR;
  }


  copy_application(&req.req_data.app,my_app);

  // We create a single ID
  my_job_id 	= my_app->job.id;
  my_step_id	= my_app->job.step_id;
  local_con_info.jid = my_job_id;
  local_con_info.sid = my_step_id;
  local_con_info.lid = lid;
  memcpy(&req.con_id,&local_con_info,sizeof(local_con_info));
  /* my_id must be extended to include the PID */
  my_id = create_ID(my_app->job.id,my_app->job.step_id);
  debug("Connecting with daemon job_id=%lu step_id%lu pid %d\n",my_app->job.id,my_app->job.step_id,getpid());
  i = 0;
	/* There are three channels: One global for acceptance , and the ones for each connection */
  sprintf(ear_commreq_global,"%s/.ear_comm.globalreq",ear_tmp);
  sprintf(ear_commreq,"%s/%u/.ear_comm.req_%d.%d.%lu",ear_tmp,(uint)my_id, i, my_id,lid);
  sprintf(ear_commack,"%s/%u/.ear_comm.ack_%d.%d.%lu",ear_tmp,(uint)my_id, i, my_id,lid);
  debug("comreq_global %s comreq %s comack %s",ear_commreq_global,ear_commreq,ear_commack);
  /* Private pipes are created before the connection request is sent */
	//verbose(2,"Creating %s",ear_commack);
  if (mknod(ear_commack, S_IFIFO | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, 0) < 0) {
     return_print(EAR_ERROR, "Error when creating ear communicator for ack %s", strerror(errno));
  }
  chmod(ear_commack, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	//verbose(2,"Creating %s",ear_commreq);
  if (mknod(ear_commreq, S_IFIFO | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, 0) < 0) {
     return_print(EAR_ERROR, "Error when creating ear communicator for requests %s", strerror(errno));
  }
  chmod(ear_commreq, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	debug("Communication pipes create");

  /* Pipes are created */
  /* We give some opportunities to connect */
  do{
        //verbose(3,"Connecting with EARD using file %s\n",ear_commreq_global);
        if ((ear_fd_req_global = open(ear_commreq_global,O_WRONLY|O_NONBLOCK))<0) tries++;
        else connected=1;
        if ((MAX_TRIES>1) && (!connected)) sleep(1);
  }while ((tries<MAX_TRIES) && !connected);
  if (!connected) {
    // Not possible to connect with ear_daemon
    return_print(EAR_ERROR, "while opening the communicator for requests %s (%d attempts) (%s)",
      ear_commreq_global, tries, strerror(errno));
  }
	debug("Connected with EARD");
  #if USE_NON_BLOCKING_IO
  ret1=fcntl(ear_fd_req_global, F_SETFL, O_NONBLOCK);
  if (ret1<0){
       return_print(EAR_ERROR, "Setting O_NONBLOCK for eard global req communicator %s",strerror(errno));
  }
  #endif

  /* We now connect with the EARD, the tag CONNECT_EARD_NODE_SERVICES is used as the first service requested (for backward compatibility ) */
  req.req_service = CONNECT_EARD_NODE_SERVICES;
  req.sec					= create_sec_tag();
  if (ear_fd_req_global >= 0)
  {
      debug("Sending connection request to EARD\n");
      if (warning_api(eards_write(ear_fd_req_global, (char *)&req, sizeof(req)), sizeof(req),
          "writting req_service in ear_daemon_client_connect"))
      {
        ear_fd_req_global = -1;
        return_print(EAR_ERROR, "Error sending connection request");
      }
    }
    /* At this point we have sent the data for connecting with EARD */
    if (ear_fd_req_global >= 0) {
      /* EARD sends an ack when ack pipe for specific service is created */
			verbose(3,"%s: Opening %s",nodename, ear_commack);
      ear_fd_ack = open(ear_commack,O_RDONLY);
      if (ear_fd_ack < 0){
        error("Error opening local ack communicator with EARD (%s)",strerror(errno));
        close(ear_fd_req_global);
        ear_fd_req_global = -1;
        return EAR_ERROR; 
      }
			verbose(3,"%s: %s open",nodename, ear_commack);
      #if USE_NON_BLOCKING_IO
      ret1 = fcntl(ear_fd_ack, F_SETFL, O_NONBLOCK);
      if (ret1<0){
        return_print(EAR_ERROR,"Setting O_NONBLOCK for eard ack communicator %s",strerror(errno));
      }
      #endif

      debug( "ear_client: waiting for ear_daemon ok");
      if (warning_api(eards_read(ear_fd_ack, (char *) &ack,sizeof(ulong), WAIT_DATA),sizeof(ulong),
        "ERROR while reading ack ")) {
        CLOSE_LOCAL_COMM();
        return EAR_ERROR;
      }
      /* ACK received */
      verbose(3,"ACK received from EARD %s,opening REQ", nodename);
			verbose(3,"%s: Open %s",nodename, ear_commreq);
      if ((ear_fd_req = open(ear_commreq,O_WRONLY)) < 0){
        error("Error opening local req communicator, closing the EARD connection");
        CLOSE_LOCAL_COMM();
        return EAR_ERROR;
      }
			verbose(3, "%s: %s open", nodename, ear_commreq);
      #if USE_NON_BLOCKING_IO
      ret1 = fcntl(ear_fd_req, F_SETFL, O_NONBLOCK);
      if (ret1<0){
        error("Setting O_NONBLOCK for eard req communicator %s",strerror(errno));
        return EAR_ERROR;
      }
      #endif
      verbose(3, "EARD-EARL in %s connected", nodename);
      close(ear_fd_req_global);
  }
  app_connected = 1;

  AFD_SETT(ear_fd_ack, &eard_api_client_fd, NULL);

  eards_signal_catcher();
  debug("Connected");
  return EAR_SUCCESS;
}

void eards_disconnect()
{
  struct daemon_req req;
  req.req_service = DISCONNECT_EARD_NODE_SERVICES;
  req.sec=create_sec_tag();

  debug( "Disconnecting");
  if (!app_connected) return;
  if (ear_fd_req >= 0) {
    warning_api(eards_write(ear_fd_req, (char *)&req,sizeof(req)), sizeof(req),
        "witting req in ear_daemon_client_disconnect");
  }


  CLOSE_LOCAL_COMM();

  AFD_ZERO(&eard_api_client_fd);

  app_connected=0;

}

//////////////// SYSTEM REQUESTS
ulong sendack(char *send_buf, size_t send_size, char *ack_buf, size_t ack_size, const char *description, int retsuc)
{
    state_t s;
		ulong *ack_long;
		debug("local api init : %s. send size %lu. FD %d", description, send_size, ear_fd_req);
    if (ear_fd_req < 0) {
        return (ulong) EAR_SUCCESS;
    }
    if (state_fail(s = ear_trylock(&lock_rpc))) {
				debug("local api, cannot get the lock");
        return s;
    }
    if (eards_write(ear_fd_req, send_buf, send_size) != send_size) {
				debug("local api end error write: %s. recv size %lu. FD %d", description, ack_size, ear_fd_ack);
				app_connected = 0;
        return_unlock((ulong) EAR_ERROR, &lock_rpc);
    }


    #if MIX_RPC
    int ack_ready;
    ack_ready = aselect(&eard_api_client_fd, 3000, NULL);
    AFD_ZERO(&eard_api_client_fd);
    AFD_SETT(ear_fd_ack, &eard_api_client_fd, NULL);
    #endif


    if (eards_read(ear_fd_ack, ack_buf, ack_size, WAIT_DATA) != ack_size) {
				debug("local api end error read: %s. recv size %lu. FD %d", description, ack_size, ear_fd_ack);
				app_connected = 0;
        return_unlock((ulong) EAR_ERROR, &lock_rpc);
    }
		debug("local api end : %s. recv size %lu. FD %d", description, ack_size, ear_fd_ack);
    if (!retsuc) {
				ack_long = (ulong *)ack_buf;
				debug("local api returns %lu from eard", *ack_long);
        return_unlock(*ack_long, &lock_rpc);
    }
    return_unlock((ulong) EAR_SUCCESS, &lock_rpc);
}

#define init_service(service)               \
    struct daemon_req req;                  \
    ulong ack = EAR_SUCCESS;                \
    (void) (ack);                           \
    if (!app_connected) return EAR_SUCCESS; \
    req.req_service = service;              \
    req.sec=create_sec_tag();               \
    memset(&req.req_data, 0, sizeof(union daemon_req_opt));

ulong eards_write_app_signature(application_t *app_signature)
{
    init_service(WRITE_APP_SIGNATURE);
    memcpy(&req.req_data.app, app_signature, sizeof(application_t));
    return sendack((char *) &req, sizeof(req), (char *) &ack, sizeof(ulong), "APP signature", 0);
}

ulong eards_write_event(ear_event_t *event)
{
    init_service(WRITE_EVENT);
    memcpy(&req.req_data.event, event, sizeof(ear_event_t));
    return sendack((char *) &req, sizeof(req), (char *) &ack, sizeof(ulong), "EVENT write", 0);
}

ulong eards_write_loop_signature(loop_t *loop_signature)
{
    init_service(WRITE_LOOP_SIGNATURE);
    memcpy(&req.req_data.loop, loop_signature, sizeof(loop_t));
    return sendack((char *) &req, sizeof(req), (char *) &ack, sizeof(ulong), "LOOP signature", 0);
}

ulong eards_get_data_size_rapl() // size in bytes
{
    if (!app_connected) return sizeof(ulong);
    init_service(DATA_SIZE_RAPL);
    rapl_size = sendack((char *) &req, sizeof(req), (char *) &ack, sizeof(ulong), "RAPL size", 0);
    return rapl_size;
}

int eards_reset_rapl()
{
    init_service(RESET_RAPL);
    return (int) sendack((char *) &req, sizeof(req), (char *) &ack, sizeof(ulong), "RAPL reset", 0);
}

int eards_start_rapl()
{
    init_service(START_RAPL);
    return (int) sendack((char *) &req, sizeof(req), (char *) &ack, sizeof(ulong), "RAPL start", 1);
}

int eards_read_rapl(unsigned long long *values)
{
    if (!app_connected) { values[0]=0; return EAR_SUCCESS; }
    init_service(READ_RAPL);
    return (int) sendack((char *) &req, sizeof(req), (char *) values, rapl_size, "RAPL read", 1);
}

ulong eards_node_energy_data_size()
{
    if (!app_connected) return sizeof(ulong);
    init_service(DATA_SIZE_ENERGY_NODE);
    energy_size = sendack((char *) &req, sizeof(req), (char *) &ack, sizeof(ulong), "energy data size", 0);
    return energy_size;
}

int eards_node_dc_energy(void *energy,ulong datasize)
{
    memset(energy,0,datasize);
    init_service(READ_DC_ENERGY);
    return (int) sendack((char *) &req, sizeof(req), (char *) energy, datasize, "energy read", 1);
}

ulong eards_node_energy_frequency()
{
    if (!app_connected) return 10000000;
    init_service(ENERGY_FREQ);
    return sendack((char *) &req, sizeof(req), (char *) &ack, sizeof(ulong), "energy freq", 0);
}

int eards_gpu_model(uint *gpu_model)
{
    *gpu_model = API_DUMMY;
    init_service(GPU_MODEL);
    return (int) sendack((char *) &req, sizeof(req), (char *) gpu_model, sizeof(uint), "GPU model", 1);
}

int eards_gpu_dev_count(uint *gpu_dev_count)
{
    *gpu_dev_count = 0;
    init_service(GPU_DEV_COUNT);
    return (int) sendack((char *) &req, sizeof(req), (char *) gpu_dev_count, sizeof(uint), "GPU count devices", 1);
}

int eards_gpu_data_read(gpu_t *gpu_info,uint num_dev)
{
    memset(gpu_info,0,sizeof(gpu_t)*num_dev);
    init_service(GPU_DATA_READ);
    return (int) sendack((char *) &req, sizeof(req), (char *) gpu_info, sizeof(gpu_t)*num_dev, "GPU data read", 1);
}

int eards_gpu_get_info(gpu_devs_t *info, uint num_dev)
{
    memset(info,0,sizeof(gpu_devs_t)*num_dev);
    init_service(GPU_GET_INFO);
    return (int) sendack((char *) &req, sizeof(req), (char *) info, sizeof(gpu_devs_t)*num_dev, "GPU info", 1);
}

int eards_gpu_set_freq(uint num_dev,ulong *freqs)
{
    init_service(GPU_SET_FREQ);
    int i;

    if (num_dev>MAX_GPUS_SUPPORTED){
        error("Using more GPUS than supported: Requested %u Max %u", num_dev, MAX_GPUS_SUPPORTED);
        num_dev=MAX_GPUS_SUPPORTED;
    }
    for (i = 0; i < num_dev; i++) {
        debug("GPU %d: Freq requested %lu", i, freqs[i]);
    }
    req.req_data.gpu_freq.num_dev=num_dev;
    memcpy(req.req_data.gpu_freq.gpu_freqs, freqs, sizeof(ulong)*num_dev);

    return (int) sendack((char *) &req, sizeof(req), (char *) &ack, sizeof(ack), "GPU set freq", 1);
}

int eards_gpu_support()
{
  init_service(GPU_SUPPORTED);
  return (int) sendack((char *) &req, sizeof(req), (char *) &ack, sizeof(ack), "GPU support", 0);
}

