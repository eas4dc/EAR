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

#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sched.h>
//#define SHOW_DEBUGS 1
#include <common/config.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/types/generic.h>
#include <common/types/configuration/cluster_conf.h>
#include <daemon/power_monitor.h>
#include <daemon/app_api/app_conf_api.h>
#include <metrics/energy/energy_node.h>
#include <management/gpu/gpu.h>
#include <management/cpufreq/frequency.h>

#define close_app_connection()

static char app_to_eard[MAX_PATH_SIZE];
static char *TH_NAME="No-EARL_API";

static int fd_app_to_eard=-1;
static unsigned long node_energy_datasize;

#define MAX_FDS 100
#define MAX_TRIES 100
typedef struct connect{
	int recv;
	int send;
	int pid;
}connect_t;
static int connections=0;
static connect_t apps_eard[MAX_FDS];
static fd_set rfds_basic;
static int numfds_req;

extern cluster_conf_t my_cluster_conf;
extern my_node_conf_t my_node_conf;
extern int eard_must_exit;
static ehandler_t my_eh_app_api;

/*********************************************************/
/***************** PRIVATE FUNCTIONS in this module ******/
/*********************************************************/

static void app_api_set_sigterm()
{
    sigset_t set;
    sigfillset(&set);
    pthread_sigmask(SIG_SETMASK,&set,NULL); 

}



static int send_app_answer(int fd_out,app_recv_t *data)
{
	if (write(fd_out,data,sizeof(app_recv_t))!=sizeof(app_recv_t))	return EAR_ERROR;
	return EAR_SUCCESS;
}



/*********************************************************/
/***************** FUNCTIONS USED BY EARD ************/
/*********************************************************/
int create_app_connection(char *root)
{
	mode_t old_mask;
	int i;
	debug("create_app_connection\n");
	for (i=0;i<MAX_FDS;i++){
		apps_eard[i].recv=-1;
		apps_eard[i].send=-1;
		apps_eard[i].pid=-1;
	}
    sprintf(app_to_eard,"%s/.app_to_eard",root);
	old_mask=umask(0);
	// app_to_eard is used to send requests from app to eard
    if (mknod(app_to_eard,S_IFIFO|S_IRUSR|S_IRGRP|S_IWUSR|S_IWGRP|S_IROTH|S_IWOTH,0)<0){
        if (errno!=EEXIST){
			return EAR_ERROR;
        }
    }
	umask(old_mask);
	fd_app_to_eard=open(app_to_eard,O_RDWR);
	if (fd_app_to_eard<0){
		return EAR_ERROR;
	}
	return EAR_SUCCESS;
}

int find_first_free()
{
	int i;
	i=0;
	while ((apps_eard[i].recv==-1) && (i<MAX_FDS)) i++;
	if (i==MAX_FDS) return -1;
	return i;	
}

void check_connection(char *root,int i);
void remove_connection(char *root,int i);

void accept_new_connection(char *root,int pid)
{
	int i,tries=0;	
	char eard_to_app[MAX_PATH_SIZE];
	char app_to_eard[MAX_PATH_SIZE];
	debug("APP_API:accepting connection from %d\n",pid);
	sprintf(app_to_eard,"%s/.app_to_eard.%d",root,pid);
	sprintf(eard_to_app,"%s/.eard_to_app.%d",root,pid);
	debug("APP_API: %s and %s used",app_to_eard,eard_to_app);
	if (connections<MAX_FDS) i=connections;
	else{
		i=find_first_free();
		if (i<0){ 
			/* At this point we must check if connections are still alive */
			for (i=0;i<MAX_FDS;i++){
				if (apps_eard[i].recv>=0){ check_connection(root,i);}
			}
			if ((i=find_first_free())<0) return;
		}
	}
	apps_eard[i].recv=open(app_to_eard,O_RDONLY|O_NONBLOCK);
	apps_eard[i].send=open(eard_to_app,O_WRONLY|O_NONBLOCK);	
	if (apps_eard[i].send<0){
		if (errno==ENXIO){
		while((tries<MAX_TRIES) && ((apps_eard[i].send=open(eard_to_app,O_WRONLY|O_NONBLOCK))<0) && (errno==ENXIO)) tries++;
		}
	}

	if ((apps_eard[i].recv>=0) && (apps_eard[i].send>=0)){
		debug("Connection established pos %d  fds=%d-%d\n",i,apps_eard[i].recv,apps_eard[i].send);
		FD_SET(apps_eard[i].recv,&rfds_basic);
		if (apps_eard[i].recv>=numfds_req) numfds_req=apps_eard[i].recv+1;
		apps_eard[i].pid=pid;
		connections++;
		debug("connections %d numfds_req %d",connections,numfds_req);
	}else{
		debug("Not connected %d-%d",apps_eard[i].recv,apps_eard[i].send);
		close(apps_eard[i].recv);
		close(apps_eard[i].send);
		apps_eard[i].recv=-1;
		apps_eard[i].send=-1;
	}
}

void check_connection(char *root,int i)
{
    char eard_to_app[MAX_PATH_SIZE];
	
    debug("checking connection from %d\n",apps_eard[i].pid);
    sprintf(eard_to_app,"%s/.eard_to_app.%d",root,apps_eard[i].pid);
	close(apps_eard[i].send);
    apps_eard[i].send=open(eard_to_app,O_WRONLY|O_NONBLOCK);
    if (apps_eard[i].send>=0){
        debug("Connection is still alive\n");
	}else{
		remove_connection(root,i);
	}
}
/* This function is similar to close but we must clean the environment */
void remove_connection(char *root,int i)
{
	int fd_in,max=-1,new_con=0,pid;
	char eard_to_app[MAX_PATH_SIZE];
	char app_to_eard[MAX_PATH_SIZE];
	pid=apps_eard[i].pid;
	debug("cleaning connection from %d\n",pid);
	sprintf(app_to_eard,"%s/.app_to_eard.%d",root,pid);
	sprintf(eard_to_app,"%s/.eard_to_app.%d",root,pid);
	fd_in=apps_eard[i].recv;
	close(fd_in);
	apps_eard[i].recv=-1;
	apps_eard[i].pid=-1;
	FD_CLR(fd_in,&rfds_basic);
    /* We reconfigure the arguments */
    for (i=0;i<MAX_FDS;i++){
        if (apps_eard[i].recv>=0){
            if (apps_eard[i].recv>max) max=apps_eard[i].recv;
            new_con=i;
        }
    }
    connections=new_con;
    numfds_req=max+1;
	unlink(app_to_eard);
	unlink(eard_to_app);
}
static void close_connection(int fd_in,int fd_out)
{
	int i,found=0;
	int max=-1,new_con=-1;
	debug("closing connections %d - %d\n",fd_in,fd_out);
	close(fd_in);
	close(fd_out);
	for (i=0;i<connections && !found;i++){
		if (apps_eard[i].recv==fd_in){
			found=1;
			debug("Cleaning position %d fds=%d-%d",i,apps_eard[i].recv,apps_eard[i].send);
			apps_eard[i].recv=-1;
			apps_eard[i].send=-1;
			apps_eard[i].pid=-1;
			FD_CLR(fd_in,&rfds_basic);
		}
	}
	/* We reconfigure the arguments */
	max=fd_app_to_eard;
	for (i=0;i<MAX_FDS;i++){
		if (apps_eard[i].recv>=0){
			if (apps_eard[i].recv>max) max=apps_eard[i].recv;		
			new_con=i;
		}
	}
	connections=new_con+1;
	numfds_req=max+1;
	debug("connections %d numfds_req %d\n",connections,numfds_req);
	
}

static uint read_app_command(int fd_in,app_send_t *app_req)
{
	int ret;
    int flags,orig_flags,tries;

	if ((ret=read(fd_in,app_req,sizeof(app_send_t)))!=sizeof(app_send_t)){
		if (ret<0){		
			error("Error reading NON-EARL application request\n");
			return DISCONNECT;
		}else{    
			/* If we have read something different, we read in non blocking mode */
			orig_flags = fcntl(fd_in, F_GETFD);
		    if (orig_flags<0){
        		error("Error getting flags for pipe %s",app_to_eard);
    		}
    		flags = orig_flags | O_NONBLOCK;
    		if (fcntl(fd_in, F_SETFD, flags)<0){
        		error("Error setting O_NONBLOCK flag for pipe %s",app_to_eard);
    		}
			/* It is a short type, it has no sense to execute a loop */
			int pending=sizeof(app_send_t)-ret,pos=ret;
			tries=0;
			while((pending >0) && (tries<MAX_TRIES)){
				ret=read(fd_in,(char *)app_req+pos,pending);
				if ((ret<0) && (errno!=EAGAIN)) pending=0;
				else if (ret>=0){
					pending-=ret;
					pos+=ret;
				}
				tries++;
			}	
			/* Set again original flags */
			fcntl(fd_in, F_SETFD, orig_flags);
			if (pending>0){	
				error("Error reading NON-EARL application request\n");
				return INVALID_COMMAND;
			}
		}
	}
	return app_req->req;
}


/** Releases resources to connect with applications */
static void dispose_app_connection()
{
	close(fd_app_to_eard);
	unlink(app_to_eard);
	#if 0
	close(fd_eard_to_app);
	unlink(eard_to_app);
	#endif
}

/*********************************************************/
/***************** FUNCTIONS EXPORTED BY EARD ************/
/*********************************************************/

/** Returns the energy in mJ and the time in ms  */
static void ear_energy(int fd_out)
{
	app_recv_t data;
	ulong energy_mj,time_ms;
	
	/* Execute specific request */

	debug("ear_energy command\n");
	energy_mj=0;time_ms=0;

	if (energy_dc_time_read(&my_eh_app_api, &energy_mj,&time_ms) != EAR_SUCCESS){
		error("Error reading energy in %s thread", TH_NAME);
	}


	/* Prepare the answer */
	data.ret=EAR_SUCCESS;
	data.my_data.my_energy.energy_mj=energy_mj;
	data.my_data.my_energy.time_ms=time_ms;

	send_app_answer(fd_out,&data);

	
	return;

}

static void ear_energy_debug(int fd_out)
{
        app_recv_t data;
        ulong energy_mj, time_ms;
        struct timespec tspec;

        memset(&data.my_data.my_energy,0,sizeof(data.my_data.my_energy));

        /* Execute specific request */
        data.ret=EAR_SUCCESS;

        if (clock_gettime(CLOCK_REALTIME, &tspec)) {
                data.ret=EAR_ERROR;
                data.my_data.my_energy.os_time_sec=0;
                data.my_data.my_energy.os_time_ms=0;
        }else{
                data.my_data.my_energy.os_time_sec=tspec.tv_sec;
                data.my_data.my_energy.os_time_ms=tspec.tv_nsec/1000000;
        }

	energy_mj=0;
	time_ms=0;

        if (energy_dc_time_read(&my_eh_app_api, &energy_mj, &time_ms) != EAR_SUCCESS) {
        	error("Error reading energy in %s thread", TH_NAME);
        }

        /* Prepare the answer */
        data.my_data.my_energy.energy_j  = (energy_mj / 1000);
        data.my_data.my_energy.energy_mj = (energy_mj);
        data.my_data.my_energy.time_sec  = (time_ms / 1000);
        data.my_data.my_energy.time_ms   = (time_ms);

        send_app_answer(fd_out,&data);


        return;

}

static void  ear_set_cpufreq(int fd_out, cpu_set_t *mask,unsigned long cpuf)
{
	app_recv_t data;
  /* Execute specific request */
  data.ret=EAR_SUCCESS;
	debug("APP_API: ear_set_cpufreq %lu",cpuf);	
	frequency_set_with_mask(mask,cpuf);	

	send_app_answer(fd_out,&data);
}

/* This funcion sets the frequency in one GPU */
static void ear_set_gpufreq(int fd_out,uint gpuid,ulong gpu_freq)
{
	app_recv_t data;
	state_t ret;
	ctx_t c;
	uint gnum;
	
	ulong * array;
	debug("APP_API: ear_set_gpufreq %lu",gpu_freq);	
	if ((ret = mgt_gpu_init(&c)) != EAR_SUCCESS){
		data.ret = ret;
		send_app_answer(fd_out,&data);
	}
	mgt_gpu_count(&c,&gnum);
	if (gpuid >= gnum){
		data.ret =EAR_ERROR;
		send_app_answer(fd_out,&data);
	}
	if ((ret = mgt_gpu_alloc_array(&c, &array, NULL)) != EAR_SUCCESS){
    data.ret = ret;
    send_app_answer(fd_out,&data);
  }
	if ((ret = mgt_gpu_freq_limit_get_current(&c,array)) != EAR_SUCCESS){
    data.ret = ret;
    send_app_answer(fd_out,&data);
  }
	array[gpuid] = gpu_freq;
	if ((ret = mgt_gpu_freq_limit_set(&c,array)) != EAR_SUCCESS){
    data.ret = ret;
    send_app_answer(fd_out,&data);
  }
	mgt_gpu_dispose(&c);
}

/* This function sets th frequency in all the GPUS */
static void ear_set_gpufreqlist(int fd_out,ulong *gpu_freq_list)
{
  app_recv_t data;
  state_t ret;
  ctx_t c;
  
	debug("ear_set_gpufreqlist %lu",gpu_freq_list[0]);
  if ((ret = mgt_gpu_init(&c)) != EAR_SUCCESS){
    data.ret = ret;
    send_app_answer(fd_out,&data);
  }
  if ((ret = mgt_gpu_freq_limit_set(&c,gpu_freq_list)) != EAR_SUCCESS){
    data.ret = ret;
    send_app_answer(fd_out,&data);
  }
  mgt_gpu_dispose(&c);

}




/***************************************************************/
/***************** THREAD IMPLEMENTING NON-EARL API ************/
/***************************************************************/

void app_api_process_connection(char *root)
{
	app_send_t req;
	debug("APP_API:process_connection\n");
	if (read(fd_app_to_eard,&req,sizeof(app_send_t))!=sizeof(app_send_t)) return;
	switch (req.req){
		case CONNECT:
			accept_new_connection(root,req.pid);
			break;
	}	
}

int get_fd_out(int fd_in)
{
	int i;
	for(i=0;i<connections;i++){
		if (apps_eard[i].recv==fd_in) return apps_eard[i].send;
	}
	return EAR_ERROR;
}

void app_api_process_request(int fd_in)
{
	int req;
	app_send_t app_req;
	int fd_out;
	debug("APP_API:process_request from %d\n",fd_in);
	fd_out=get_fd_out(fd_in);
	if (fd_out==EAR_ERROR){ 
		debug("fd_out not found for %d",fd_in);
		return;
	}
	req=read_app_command(fd_in,&app_req);
	switch (req){
	case DISCONNECT:
        close_connection(fd_in,fd_out);
		break;
	case ENERGY_TIME:
		ear_energy(fd_out);
		break;
  case ENERGY_TIME_DEBUG:
     	ear_energy_debug(fd_out);
     	break;
	case SELECT_CPU_FREQ:
			ear_set_cpufreq(fd_out,&app_req.send_data.cpu_freq.mask,app_req.send_data.cpu_freq.cpuf);
			break;
	case SET_GPUFREQ:
			ear_set_gpufreq(fd_out,app_req.send_data.gpu_freq.gpu_id,app_req.send_data.gpu_freq.gpu_freq);	
			break;
	case SET_GPUFREQ_LIST:
			ear_set_gpufreqlist(fd_out,app_req.send_data.gpu_freq_list.gpu_freq);	
			break;
	case INVALID_COMMAND:
		verbose(0,"PANIC, invalid command received and not recognized\n");
		break;
	default:
		verbose(0,"PANIC, non-earl command received and not recognized\n");
		break;
	}
	
}

void *eard_non_earl_api_service(void *noinfo)
{
	fd_set rfds;
	int numfds_ready;
	int max_fd,i;

	app_api_set_sigterm();

	if (pthread_setname_np(pthread_self(),TH_NAME)) {
		error("Setting name for %s thread %s",TH_NAME,strerror(errno));
	}

	/* Create connections */
	if (create_app_connection(my_cluster_conf.install.dir_temp)!= EAR_SUCCESS){
		error("Error creating files for non-EARL requests\n");
		pthread_exit(0);
	}

	if (energy_init(&my_cluster_conf, &my_eh_app_api) != EAR_SUCCESS){
		error("Error initializing energy_init in No-EARL_API thread");
		pthread_exit(0);
	}

	energy_datasize(&my_eh_app_api,&node_energy_datasize);
	if (node_energy_datasize!=sizeof(unsigned long)){
		error("Application api for energy readings not yet supported,exiting thread for app api");
  	energy_dispose(&my_eh_app_api);
  	pthread_exit(0);
	}

	FD_ZERO(&rfds);
	FD_SET(fd_app_to_eard, &rfds);
	/* fd_eard_to_app is created the last */
    max_fd=fd_app_to_eard;
    numfds_req=max_fd+1;
	rfds_basic=rfds;

	/* Wait for messages */
	verbose(0,"Waiting for non-earl requestst\n");
	while ((eard_must_exit==0) && (numfds_ready=select(numfds_req,&rfds,NULL,NULL,NULL))>=0){
		verbose(0,"New APP_API connections");
		if (numfds_ready>0){
			for (i=0;i<numfds_req;i++){
				if (FD_ISSET(i,&rfds)){
					if (i==fd_app_to_eard){
						app_api_process_connection(my_cluster_conf.install.dir_temp);
					}else{
						app_api_process_request(i);	
					}
				}
			}	
		}
		rfds=rfds_basic;	
	}
	/* Close and remove files , never reached if thread is killed */
	energy_dispose(&my_eh_app_api);
	dispose_app_connection();
	pthread_exit(0);
}


