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

#define _GNU_SOURCE             
#include <time.h>
#include <sched.h>
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
#include <pthread.h>
#include <common/config.h>
#include <common/states.h>
#include <common/system/poll.h>
#include <common/system/lock.h>
#include <common/types/generic.h>
#include <common/output/verbose.h>
#include <common/types/application.h>
#include <common/utils/serial_buffer.h>
#include <common/system/folder.h>
#include <daemon/local_api/eard_api.h>
#include <metrics/gpu/gpu.h>
#include <metrics/common/apis.h>
#include <metrics/cpufreq/cpufreq.h>
#include <metrics/imcfreq/imcfreq.h>
#include <management/gpu/gpu.h>

#define OUTTER_MAX_PIPE_TRIES  60
#define MAX_PIPE_TRIES         (MAX_SOCKET_COMM_TRIES*1000)
#define MAX_TRIES              1
#define USE_NON_BLOCKING_IO    1

#define VPROC_LAPI	3

/* Below macros define the indices to store/recover fds to/from the saved array used in eards_save_connection and eards_recover_connection. */
#define ear_fd_req_global_idx 0
#define ear_fd_req_idx 1
#define ear_fd_ack_idx 2

#define FDS_FILENAME ".eard_connection_info"

static int connecting = 0;

/* These pipes are used for local communication between ear library and eard */
static char ear_commreq_global[SZ_PATH]; 		/* This pipe connects with EARD, the same for all the jobs,
                                         * then a specific per job-step-localid is created */
static char ear_commreq[SZ_PATH];
static char ear_commack[SZ_PATH];
int ear_fd_req_global = -1;
int ear_fd_req = -1;
int ear_fd_ack = -1;

afd_set_t eard_api_client_fd;

uint8_t app_connected = 0;
static app_id_t local_con_info;
static int my_step_id;
static int my_job_id;
static char *ear_tmp;
// Metrics: Needed for old API where data size must be requested in advance
static ulong rapl_size;

extern pthread_mutex_t lock_rpc;
static serial_buffer_t b;

#if WF_SUPPORT
/* Marks whether this API created by itself the application directory. */
static uint app_directory_created;

static char app_directory_pathname[MAX_PATH_SIZE];
#endif

#define CLOSE_LOCAL_COMM()    \
  close(ear_fd_req_global); \
  ear_fd_req_global = -1;   \
  close(ear_fd_ack);        \
  ear_fd_ack = -1;          \
  close(ear_fd_req);        \
  ear_fd_req = -1;          \
  unlink(ear_commack);      \
  unlink(ear_commreq);


static state_t create_base_path(char *base_path, size_t base_path_sz, char *tmp_path, uint job_step_id, uint app_local_id);

#if WF_SUPPORT
static state_t create_app_directory(char *path);
#endif

/** Saves EARD connection info to be restored in the case the client lose this info. */
static state_t eards_save_connection(char *tmp, ulong job_id, ulong step_id, ulong local_id);

int eards_read(int fd,char *buff,int size, uint wait_mode)
{
#if USE_NON_BLOCKING_IO
	int ret; 
	uint to_recv, received = 0;
	uint must_abort = 0;
	to_recv = size;

	int out_tries = 0;
	int tries = 0;
	ullong limit_outer_tries;

	if (connecting) limit_outer_tries = OUTTER_MAX_PIPE_TRIES * 1000;
	else            limit_outer_tries = OUTTER_MAX_PIPE_TRIES;

	while (out_tries < limit_outer_tries)
	{
		tries = 0;
		do
		{
			ret = read(fd, buff+received, to_recv);
			if (ret > 0){
				received+=ret;
				to_recv-=ret;
			}else if (ret < 0)
			{
				if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) tries++;
				else
				{
					debug("Error receiving data from eard %s,%d",strerror(errno),errno);
					must_abort = 1;
				}
			} else if (ret == 0)
			{
				if (wait_mode == WAIT_DATA) tries++;
				else 												must_abort = 1;
			}
		} while(tries < MAX_PIPE_TRIES && to_recv > 0 && must_abort == 0);
		// Below code gives more tries. It is implemented with an outer loop due to the integer limit.
		if (tries == MAX_PIPE_TRIES && !must_abort)
		{
			out_tries++;
		} else
		{
			out_tries = limit_outer_tries;
		}
	}
	debug("limits reached %lu/%llu", tries, out_tries);

	if (tries >= MAX_PIPE_TRIES)
	{
		error("Error reading data, max number of tries reached. tries %d/%d received %u (last ret %d errno %d fd %d)",tries, MAX_PIPE_TRIES, received, ret,errno, fd);
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
	ullong limit_outer_tries;

	int out_tries = 0;
  if (connecting) limit_outer_tries = OUTTER_MAX_PIPE_TRIES * 1000;
  else            limit_outer_tries = OUTTER_MAX_PIPE_TRIES;

	while (out_tries < limit_outer_tries)
	{
		tries = 0;
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
		} while(tries < MAX_PIPE_TRIES && to_send > 0 && must_abort==0);

		// Below code gives more tries. It is implemented with an outer loop due to the integer limit.
		if (tries == MAX_PIPE_TRIES && !must_abort)
		{
			out_tries++;
		} else
		{
			out_tries = limit_outer_tries;
		}
	}
	debug("limits reached %lu/%llu", tries, out_tries);
  /* If there are bytes left to send, we return a 0 */
	if (ret<0) return ret;
	return sended;

#else
  return write(fd,buff,size);
#endif
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
		verbose(VPROC_LAPI, "eards: %s (returned %d expected %d)", msg,return_value,expected);
	}
	if (return_value < 0){ 
		app_connected=0;
		verbose(VPROC_LAPI, "eards: %s (%s)", msg,strerror(errno));
	}
	return (return_value != expected);
}

void eards_signal_handler(int s)
{
  debug("EARD has been disconnected");
  //mark_as_eard_disconnected(my_job_id,my_step_id,getpid());
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
    my_dummy_app.job.id       = 0;
    my_dummy_app.job.step_id  = 0;
#if WF_SUPPORT
    my_dummy_app.job.local_id = 0; // Create the folder if not exists
#endif
    return (state_t) eards_connect(&my_dummy_app, getpid());
}

int eards_connect(application_t *my_app, ulong lid)
{
    char nodename[256];
    struct daemon_req req;
    int connected = 0;
    int tries = 0;
    ulong ack;
    int my_id;
    int ret1;
    int i;

    if (app_connected) {
        return EAR_SUCCESS;
    }
    AFD_ZERO(&eard_api_client_fd);

    #if FAKE_EAR_NOT_INSTALLED
    return_print(EAR_ERROR, "FAKE_EAR_NOT_INSTALLED applied");
    #endif
    // These files connect EAR with EAR_COMM
		// TODO: Should we use get_ear_tmp()?
    ear_tmp=ear_getenv(ENV_PATH_TMP);
    if (ear_tmp == NULL) {
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
    my_job_id 	       = my_app->job.id;
    my_step_id	       = my_app->job.step_id;
    local_con_info.jid = my_job_id;
    local_con_info.sid = my_step_id;
    local_con_info.lid = lid;
    memcpy(&req.con_id, &local_con_info, sizeof(local_con_info));

    /* my_id must be extended to include the PID */
    my_id = create_ID(my_app->job.id, my_app->job.step_id);
    debug("Connecting with daemon job_id=%lu step_id%lu and PID %d",
          my_app->job.id, my_app->job.step_id, getpid());
    i = 0;

	/* There are three channels: One global for acceptance , and the ones for each connection */
    sprintf(ear_commreq_global,"%s/.ear_comm.globalreq",ear_tmp);
	verbose(VPROC_LAPI,"Opening Global comm pipe %s", ear_commreq_global);

#if WF_SUPPORT
	char base_path[MAX_PATH_SIZE];
	/* Create the common base path used inside this block ("tmp/id/local_id") */
	state_t ret_st = create_base_path(base_path, sizeof(base_path),
        ear_tmp, (uint) my_id, (uint) my_app->job.local_id);
	if (state_fail(ret_st)) {
        return_print(EAR_ERROR, "Error when building the EAR communicator directory path name.");
	}

	// Save the application base_path
	memcpy(app_directory_pathname, base_path, sizeof(app_directory_pathname));

	/* Create the directory of this application if it doesn't exist yet.
	 * The directory may not exist on anonymous connections. */
	if (state_fail(create_app_directory(base_path)))
	{
     return_print(EAR_ERROR, "Error when creating the EAR communicator directory.");
	}

	/* Pipe full paths. They're stored at base_path as well. */
    sprintf(ear_commreq,"%s/.ear_comm.req_%d.%d.%lu", base_path, i, my_id, lid);
    sprintf(ear_commack,"%s/.ear_comm.ack_%d.%d.%lu", base_path, i, my_id, lid);
#else
    sprintf(ear_commreq, "%s/%u/.ear_comm.req_%d.%d.%lu", ear_tmp, (uint) my_id, i, my_id, lid);
    sprintf(ear_commack, "%s/%u/.ear_comm.ack_%d.%d.%lu", ear_tmp, (uint) my_id, i, my_id, lid);
#endif
    debug("comreq_global %s comreq %s comack %s",ear_commreq_global,ear_commreq,ear_commack);
    debug("comreq_self  : %s", ear_commreq);
    debug("comack_self  : %s", ear_commack);

    /* Private pipes are created before the connection request is sent */
	verbose(VPROC_LAPI,"Creating %s",ear_commack);
    if (mknod(ear_commack, S_IFIFO | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, 0) < 0) {
        return_print(EAR_ERROR, "Error when creating ear communicator for ack: %s", strerror(errno));
    }
    chmod(ear_commack, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	verbose(VPROC_LAPI,"Creating %s",ear_commreq);
    if (mknod(ear_commreq, S_IFIFO | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, 0) < 0) {
        debug("Error when creating ear communicator for requests: %s", strerror(errno));
        return_print(EAR_ERROR, "Error when creating ear communicator for requests %s", strerror(errno));
    }
    chmod(ear_commreq, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	debug("Communication PIPEs created");
    /* Pipes are created, we give some opportunities to connect */
    do{
        //verbose(3,"Connecting with EARD using file %s\n",ear_commreq_global);
        debug("Connecting with EARD using file %s", ear_commreq_global);
        if ((ear_fd_req_global = open(ear_commreq_global,O_WRONLY|O_NONBLOCK))<0) tries++;
        else connected=1;
        if ((MAX_TRIES>1) && (!connected)) sleep(1);
    } while ((tries<MAX_TRIES) && !connected);

    if (!connected) {
        // Not possible to connect with ear_daemon
        debug("while opening the communicator for requests %s (%d attempts) (%s)", ear_commreq_global, tries, strerror(errno));
        return_print(EAR_ERROR, "while opening the communicator for requests %s (%d attempts) (%s)", ear_commreq_global, tries, strerror(errno));
    }
    verbose(VPROC_LAPI, "EARL: Communication FD created, connecting with EARD");

    #if USE_NON_BLOCKING_IO
    ret1 = fcntl(ear_fd_req_global, F_SETFL, O_NONBLOCK);
    if (ret1 < 0) {
       return_print(EAR_ERROR, "Setting O_NONBLOCK for eard global req communicator %s", strerror(errno));
    }
    #endif

    connecting = 1;
    /* We now connect with the EARD, the tag CONNECT_EARD_NODE_SERVICES is used as the first service requested (for backward compatibility ) */
    req.req_service = CONNECT_EARD_NODE_SERVICES;
    req.sec         = create_sec_tag();
		debug("sec: %lu", req.sec);

		if (ear_fd_req_global >= 0) {
        debug("Sending connection request to EARD\n");
        if (warning_api( eards_write(ear_fd_req_global, (char *)&req, sizeof(req)), sizeof(req), "writting req_service in ear_daemon_client_connect"))
        {
						connecting = 0;
            ear_fd_req_global = -1;
            return_print(EAR_ERROR, "Error sending connection request");
        }
    }

    /* At this point we have sent the data for connecting with EARD */
    if (ear_fd_req_global >= 0)
    {
        /* EARD sends an ack when ack pipe for specific service is created */
        verbose(VPROC_LAPI+1,"%s: Opening %s",nodename, ear_commack);
        ear_fd_ack = open(ear_commack,O_RDONLY);
        if (ear_fd_ack < 0){
            error("Error opening local ack communicator with EARD (%s)",strerror(errno));
						connecting = 0;
            close(ear_fd_req_global);
            ear_fd_req_global = -1;
            return EAR_ERROR;
        }
        verbose(VPROC_LAPI+1,"%s: %s open",nodename, ear_commack);
        #if USE_NON_BLOCKING_IO
        ret1 = fcntl(ear_fd_ack, F_SETFL, O_NONBLOCK);
        if (ret1<0){
						connecting = 0;
            return_print(EAR_ERROR,"Setting O_NONBLOCK for eard ack communicator %s",strerror(errno));
        }
        #endif

        debug( "ear_client: waiting for ear_daemon ok");
        if (warning_api(eards_read(ear_fd_ack, (char *) &ack,sizeof(ulong), WAIT_DATA),sizeof(ulong), "ERROR while reading ack ")) {
					  verbose(2,"EARL[%d] EARD connection failed because of timeout", getpid());
            CLOSE_LOCAL_COMM();
						connecting = 0;
            return EAR_ERROR;
        }
        /* ACK received */
        verbose(VPROC_LAPI+1,"ACK received from EARD %s. Opening REQ...", nodename);
				debug("ACK received: %lu", ack);

        verbose(VPROC_LAPI+1,"%s: Open %s",nodename, ear_commreq);
        if ((ear_fd_req = open(ear_commreq,O_WRONLY)) < 0) {
            error("Error opening local req communicator, closing the EARD connection");
            CLOSE_LOCAL_COMM();
						connecting = 0;
            return EAR_ERROR;
        }
        verbose(VPROC_LAPI+1, "%s: %s open", nodename, ear_commreq);
        #if USE_NON_BLOCKING_IO
        ret1 = fcntl(ear_fd_req, F_SETFL, O_NONBLOCK);
        if (ret1 < 0) {
            error("Setting O_NONBLOCK for eard req communicator %s",strerror(errno));
						connecting = 0;
            return EAR_ERROR;
        }
        #endif
        verbose(VPROC_LAPI, "EARD-EARL in %s connected (%s)", nodename, ear_commack);
        close(ear_fd_req_global);
    } //if (ear_fd_req_global >= 0)
    app_connected = 1;

    AFD_SETT(ear_fd_ack, &eard_api_client_fd, NULL);
    eards_signal_catcher();
    debug("Connected");

    // Initializing serial buffer with plenty space
    serial_alloc(&b, SIZE_8KB);
		connecting = 0;

#if WF_SUPPORT
		if (state_fail(eards_save_connection(ear_tmp, my_app->job.id, my_app->job.step_id, my_app->job.local_id))) {
			verbose(2, "%sWarning%s Connection info saving failed: %s",
							COL_YLW, COL_CLR, state_msg);
		}
#endif

    return EAR_SUCCESS;
}

void eards_connection_failure()
{
  app_connected = 0;
}

void eards_new_process_disconnect()
{
	// This function closes the EARD fd for new processes to connect again
	#warning "This part is pending to be tested"
	close(ear_fd_req);
  close(ear_fd_req_global); 
  close(ear_fd_ack);        
	
  ear_fd_req_global = -1;   
	ear_fd_req        = -1;
  ear_fd_ack        = -1;          

	app_connected = 0;
}

void eards_disconnect()
{
  struct daemon_req req;
  req.req_service = DISCONNECT_EARD_NODE_SERVICES;
  req.sec = create_sec_tag();

  debug("Disconnecting...");

  if (!app_connected)
		return;

  if (ear_fd_req >= 0) {
    warning_api(eards_write(ear_fd_req, (char *)&req,sizeof(req)), sizeof(req),
        "writting req in ear_daemon_client_disconnect");
  }

  CLOSE_LOCAL_COMM();
  AFD_ZERO(&eard_api_client_fd);

#if WF_SUPPORT
	if (app_directory_created)
		folder_remove(app_directory_pathname);
#endif

  app_connected = 0;
}

static state_t eards_save_connection(char *tmp, ulong job_id, ulong step_id, ulong local_id)
{
	int my_id = create_ID(job_id, step_id);

	// Create the base path for storing the connection info, i.e., tmp/id/local_id
	char base_path[SZ_PATH_INCOMPLETE];
	if (state_fail
	    (create_base_path
	     (base_path, sizeof(base_path), tmp, (uint) my_id, (uint) local_id))) {
		return_msg(EAR_ERROR, "Creating the base path.");
	}
	// Store the communication channels.
	int fds[3];
	fds[ear_fd_req_global_idx] = ear_fd_req_global;
	fds[ear_fd_req_idx] = ear_fd_req;
	fds[ear_fd_ack_idx] = ear_fd_ack;

	// Open the storage file
	char fds_file_pathname[SZ_PATH];
	snprintf(fds_file_pathname, sizeof(fds_file_pathname), "%s/%s",
		 base_path, FDS_FILENAME);

	int fd_flags = O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC;
	int fd_mode = S_IRUSR | S_IWUSR | S_IRGRP;
	int fds_file = open(fds_file_pathname, fd_flags, fd_mode);
	if (fds_file < 0) {
		verbose(2, "%sWarning%s FDs file couldn't be opened: %d (%s)",
			COL_YLW, COL_CLR, errno, strerror(errno));
		return_msg(EAR_ERROR, "FDs file could not be opened.");
	}
	// Write the communication info
	if (write(fds_file, fds, sizeof(fds)) < 0) {
		verbose(2, "%sWarning%s FDs file couldn't be written: %d (%s)",
			COL_YLW, COL_CLR, errno, strerror(errno));
		close(fds_file);
		return_msg(EAR_ERROR, "FDs file could not be written.");
	}

	close(fds_file);

	return EAR_SUCCESS;
}

state_t eards_recover_connection(char *tmp, ulong job_id, ulong step_id, ulong local_id, int rank_id)
{
	int my_id = create_ID(job_id, step_id);

	// Create the base path for recovering the connection info, i.e., tmp/id/local_id
	char base_path[SZ_PATH_INCOMPLETE];
	if (state_fail
	    (create_base_path
	     (base_path, sizeof(base_path), tmp, (uint) my_id,
	      (uint) local_id))) {
		return_msg(EAR_ERROR, "Creating the base path.");
	}
	// Open the storage file
	char fds_file_pathname[SZ_PATH];
	snprintf(fds_file_pathname, sizeof(fds_file_pathname), "%s/%s",
		 base_path, FDS_FILENAME);

	int fd_flags = O_RDONLY | O_CLOEXEC;
	int fds_file = open(fds_file_pathname, fd_flags, 0);
	if (fds_file < 0) {
		verbose(2, "%sWarning%s FDs file %s couldn't be opened: %d (%s)",
			COL_YLW, COL_CLR, fds_file_pathname, errno, strerror(errno));
		return_msg(EAR_ERROR, "FDs file could not be opened.");
	}
	// Read the communication info
	int fds[3];
	if (read(fds_file, fds, sizeof(fds)) < 0) {
		verbose(2, "%sWarning%s FDs file couldn't be read: %d (%s)",
			COL_YLW, COL_CLR, errno, strerror(errno));
		close(fds_file);
		return_msg(EAR_ERROR, "FDs file could not be read.");
	}
	// Store communication info
	ear_fd_req_global = fds[ear_fd_req_global_idx];
	ear_fd_req = fds[ear_fd_req_idx];
	ear_fd_ack = fds[ear_fd_ack_idx];
	
	debug("[%d] Pipes recovered: req_global %d req %d ack %d", getpid(), ear_fd_req_global, ear_fd_req, ear_fd_ack);

	app_connected = 1;

	close(fds_file);

	/* Pipe full paths. They're stored at base_path as well. */
  sprintf(ear_commreq,"%s/.ear_comm.req_0.%d.%lu", base_path, my_id, (ulong)rank_id);
  sprintf(ear_commack,"%s/.ear_comm.ack_0.%d.%lu", base_path, my_id, (ulong)rank_id);


	eards_disconnect();

	return EAR_SUCCESS;
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
    aselect(&eard_api_client_fd, 3000, NULL);
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

ulong eards_write_app_signature(application_t *app)
{
    #if WF_SUPPORT
    serial_clean(&b);
    application_serialize(&b, app);
    return eard_rpc(RPC_WRITE_APPLICATION, serial_data(&b), serial_size(&b), NULL, 0);
    #else
    init_service(WRITE_APP_SIGNATURE);
    memcpy(&req.req_data.app, app, sizeof(application_t));
    return sendack((char *) &req, sizeof(req), (char *) &ack, sizeof(ulong), "APP signature", 0);
    #endif
}

ulong eards_write_wf_app_signature(application_t *app)
{
    #if WF_SUPPORT
    serial_clean(&b);
    application_serialize(&b, app);
    return eard_rpc(RPC_WRITE_WF_APPLICATION, serial_data(&b), serial_size(&b), NULL, 0);
    #else
		/* This case should never happen */
    init_service(WRITE_WF_APP_SIGNATURE);
    memcpy(&req.req_data.app, app, sizeof(application_t));
    return sendack((char *) &req, sizeof(req), (char *) &ack, sizeof(ulong), "APP signature", 0);
    #endif
}


ulong eards_write_event(ear_event_t *event)
{
    #if WF_SUPPORT
    serial_clean(&b);
    event_serialize(&b, event);
    return eard_rpc(RPC_WRITE_EVENT, serial_data(&b), serial_size(&b), NULL, 0);
    #else
    init_service(WRITE_EVENT);
    memcpy(&req.req_data.event, event, sizeof(ear_event_t));
    return sendack((char *) &req, sizeof(req), (char *) &ack, sizeof(ulong), "EVENT write", 0);
    #endif
}

ulong eards_write_loop_signature(loop_t *loop)
{
    #if WF_SUPPORT
    serial_clean(&b);
    loop_serialize(&b, loop);
    return eard_rpc(RPC_WRITE_LOOP, serial_data(&b), serial_size(&b), NULL, 0);
    #else
    init_service(WRITE_LOOP_SIGNATURE);
    memcpy(&req.req_data.loop, loop, sizeof(loop_t));
    return sendack((char *) &req, sizeof(req), (char *) &ack, sizeof(ulong), "LOOP signature", 0);
    #endif
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

int eards_gpu_support()
{
  init_service(GPU_SUPPORTED);
  return (int) sendack((char *) &req, sizeof(req), (char *) &ack, sizeof(ack), "GPU support", 0);
}

state_t eard_new_application(application_t *app)
{
  
  return EAR_SUCCESS;
}

state_t eard_end_application(application_t *app)
{
  return EAR_SUCCESS;
}

state_t eards_get_state(eard_state_t *state)
{
    return eard_rpc(RPC_GET_STATE, NULL, 0, (char *) state, sizeof(eard_state_t));
}

ulong eards_get_data_size_rapl() // size in bytes
{
    if (!app_connected) return sizeof(ulong);
    init_service(DATA_SIZE_RAPL);
    rapl_size = sendack((char *) &req, sizeof(req), (char *) &ack, sizeof(ulong), "RAPL size", 0);
    return rapl_size;
}

int eards_read_rapl(unsigned long long *values)
{
    if (!app_connected) { values[0]=0; return EAR_SUCCESS; }
    init_service(READ_RAPL);
    return (int) sendack((char *) &req, sizeof(req), (char *) values, rapl_size, "RAPL read", 1);
}

static state_t create_base_path(char *base_path, size_t base_path_sz, char *tmp_path, uint job_step_id, uint app_local_id)
{
	int ret = snprintf(base_path, base_path_sz, "%s/%u/%u", tmp_path, job_step_id, app_local_id);
	if (ret < base_path_sz && ret >= 0) {
		return EAR_SUCCESS;
	} else {
		return EAR_ERROR;
	}
}

#if WF_SUPPORT
static state_t create_app_directory(char *path)
{
	int ret = mkdir(path, S_IRWXU);
	if (ret < 0 && errno != EEXIST) {
		debug("App directory could not be created: %s", strerror(errno));
		return EAR_ERROR;
	} else if (!ret) {
		debug("Directory created");
		app_directory_created = 1;
	}

	return EAR_SUCCESS;
}
#endif
