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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <common/config.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/environment.h>
#include <common/types/generic.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/environment_common.h>
#include <common/types/configuration/policy_conf.h>
#include <common/types/configuration/cluster_conf.h>

extern char *conf_ear_tmp;
char *conf_ear_user_db_pathname=NULL;
char *conf_ear_gui_pathname=NULL;
char *conf_ear_coeff_db_pathname=NULL;
char *conf_ear_app_name=NULL;
int conf_ear_learning_phase=DEFAULT_LEARNING_PHASE;
int conf_ear_power_policy=-1;
double conf_ear_power_policy_th=0.0;
int conf_ear_reset_freq=DEFAULT_RESET_FREQ;
unsigned long conf_ear_p_state=DEFAULT_MAX_P_STATE;
unsigned long conf_ear_def_freq=0;
double conf_ear_performance_accuracy=DEFAULT_PERFORMANCE_ACURACY;
int conf_ear_use_turbo=USE_TURBO;
int conf_ear_local_id=-1;
int conf_ear_num_nodes=0;
int conf_ear_total_processes=1;
int conf_ear_dynais_levels=DEFAULT_DYNAIS_LEVELS;
int conf_ear_dynais_window_size=DEFAULT_DYNAIS_WINDOW_SIZE;


#define USE_EAR_CONF 1

#ifdef USE_EAR_CONF
char my_ear_conf_path[GENERIC_NAME];
cluster_conf_t my_cluster_conf;
my_node_conf_t *my_node_conf=NULL;
#endif


# define __USE_GNU
#include <dlfcn.h>
# undef  __USE_GNU
static int (*my_omp_get_max_threads)(void) = NULL;


void set_ear_total_processes(int procs)
{
	conf_ear_total_processes=procs;
}
int get_ear_total_processes()
{
	return conf_ear_total_processes;
}


char *getenv_ear_user_db_pathname()
{
	char *my_user_db_pathname = getenv("EAR_USER_DB_PATHNAME");

	if (my_user_db_pathname != NULL && strcmp(my_user_db_pathname, "") != 0)
	{
		conf_ear_user_db_pathname=malloc(strlen(my_user_db_pathname) + 1);
		strcpy(conf_ear_user_db_pathname,my_user_db_pathname);
	}

	return conf_ear_user_db_pathname;
}

char *getenv_ear_gui_pathname()
{
	char *my_gui_path;
	my_gui_path=getenv("EAR_GUI_PATH");
	if (my_gui_path==NULL){
		my_gui_path=getenv("HOME");	
	}
	conf_ear_gui_pathname=malloc(strlen(my_gui_path)+1);
	strcpy(conf_ear_gui_pathname,my_gui_path);
	return conf_ear_gui_pathname;
}
char *getenv_ear_coeff_db_pathname()
{
	char *my_coeff_db;
	char *install_path;
	my_coeff_db=getenv("EAR_COEFF_DB_PATHNAME");
	if (my_coeff_db==NULL){
		install_path=getenv("EAR_INSTALL_PATH");
		if (install_path==NULL) return NULL;
		my_coeff_db=malloc(strlen(install_path)+strlen(DEFAULT_COEFF_PATHNAME)+2);
		sprintf(my_coeff_db,"%s/%s",install_path,DEFAULT_COEFF_PATHNAME);	
	}
	conf_ear_coeff_db_pathname=malloc(strlen(my_coeff_db)+1);
	strcpy(conf_ear_coeff_db_pathname,my_coeff_db);
	return conf_ear_coeff_db_pathname;
}
char *getenv_ear_app_name()
{
	char *my_name;
	my_name=getenv("EAR_APP_NAME");
	if (my_name!=NULL){
		if (strcmp(my_name,"")==0) return NULL;
		conf_ear_app_name=malloc(strlen(my_name)+1);
		strcpy(conf_ear_app_name,my_name);	
	}
	return conf_ear_app_name;
}
void set_ear_app_name(char *name)
{
	if (conf_ear_app_name!=NULL) free(conf_ear_app_name);
	conf_ear_app_name=malloc(strlen(name)+1);
	strcpy(conf_ear_app_name,name);
}
int getenv_ear_learning_phase()
{
	char *learning=getenv("EAR_LEARNING_PHASE");
	int learning_value;
	if (learning!=NULL){
		learning_value=atoi(learning);
		if (learning_value>0) conf_ear_learning_phase=1;
	}
	return conf_ear_learning_phase;
}
int getenv_ear_power_policy()
{
	char *my_policy;
	conf_ear_power_policy=DEFAULT_POWER_POLICY;
	my_policy=getenv("EAR_POWER_POLICY");
	if (my_policy!=NULL){
		conf_ear_power_policy=policy_name_to_nodeid(my_policy, my_node_conf);
		if (conf_ear_power_policy==EAR_ERROR)	conf_ear_power_policy=DEFAULT_POWER_POLICY;
	}	
	return conf_ear_power_policy;
}
// power_policy must be read before, but we check it
double getenv_ear_power_policy_th()
{
	char *my_th;
	if (conf_ear_power_policy<0) getenv_ear_power_policy();
	switch (conf_ear_power_policy){	
		case MIN_ENERGY_TO_SOLUTION:
			my_th=getenv("EAR_PERFORMANCE_PENALTY");
			if (my_th!=NULL){ 
				conf_ear_power_policy_th=strtod(my_th,NULL);
				if ((conf_ear_power_policy_th>1.0) || (conf_ear_power_policy_th<0)) conf_ear_power_policy_th=DEFAULT_MIN_ENERGY_TH;
			}else conf_ear_power_policy_th=DEFAULT_MIN_ENERGY_TH;
			break;
		case MIN_TIME_TO_SOLUTION:
			my_th=getenv("EAR_MIN_PERFORMANCE_EFFICIENCY_GAIN");
			if (my_th!=NULL){
				conf_ear_power_policy_th=strtod(my_th,NULL);
				if ((conf_ear_power_policy_th<DEFAULT_MIN_TIME_TH) || (conf_ear_power_policy_th>1.0)) conf_ear_power_policy_th=DEFAULT_MIN_TIME_TH;
			}else conf_ear_power_policy_th=DEFAULT_MIN_TIME_TH;
			break;
		case MONITORING_ONLY:
			conf_ear_power_policy_th=0.0;
			break;
	}
	return conf_ear_power_policy_th;
}
int getenv_ear_reset_freq()
{
	char *my_reset=getenv("EAR_RESET_FREQ");	
	if (my_reset!=NULL){
		if (strcmp(my_reset,"1")==0) conf_ear_reset_freq=1;
	}
	return conf_ear_reset_freq;
}

unsigned long getenv_ear_p_state()
{
	char *my_pstate = getenv("EAR_P_STATE");
	int max_p_state;

	// p_state depends on policy it must be called before getenv_ear_p_state, but we check it
	if (conf_ear_power_policy < 0) getenv_ear_power_policy();

	switch(conf_ear_power_policy)
	{
		case MIN_ENERGY_TO_SOLUTION:
			conf_ear_p_state = DEFAULT_MAX_P_STATE;
			break;
		case MIN_TIME_TO_SOLUTION:
			my_pstate = getenv("EAR_MIN_P_STATE");

			if (my_pstate == NULL) {
				conf_ear_p_state = DEFAULT_MIN_P_STATE;
			} else {
				conf_ear_p_state = atoi(my_pstate);
			}
			break;
		case MONITORING_ONLY:
			my_pstate = getenv("EAR_P_STATE");
			
			if (my_pstate != NULL)
			{
				conf_ear_p_state=atoi(my_pstate);

				// p_state[0] is turbo, p_state[1] is nominal, p_state[max_p_states-1] is the lower freq
				if (!conf_ear_use_turbo) {
					max_p_state=DEFAULT_MAX_P_STATE;
				}

				// Invalid value
				if (conf_ear_p_state < max_p_state) {
					conf_ear_p_state = DEFAULT_MAX_P_STATE;
				}
			} else {
				conf_ear_p_state=DEFAULT_MAX_P_STATE;
			}
			break;
	}
	return conf_ear_p_state;
}

double getenv_ear_performance_accuracy()
{
	char *my_perf_acu=getenv("EAR_PERFORMANCE_ACCURACY");
	if (my_perf_acu!=NULL){
		conf_ear_performance_accuracy=atof(my_perf_acu);
		if (conf_ear_performance_accuracy<0) conf_ear_performance_accuracy=DEFAULT_PERFORMANCE_ACURACY;
	}
	return conf_ear_performance_accuracy;
}

int getenv_ear_local_id()
{
	//char *my_local_id;
	//my_local_id=getenv("SLURM_LOCALID");
	//if (my_local_id!=NULL){ 
	//	conf_ear_local_id=atoi(my_local_id);
	//}
	// if not defined, it is computed later based on the number of total processes and EAR_NUM_NODES
	return conf_ear_local_id;
}

int getenv_ear_num_nodes()
{
	char *my_num_nodes = getenv("EAR_NUM_NODES");

	if (my_num_nodes == NULL)
	{ 
		my_num_nodes = getenv("SLURM_STEP_NUM_NODES");

		if (my_num_nodes == NULL) conf_ear_num_nodes=1;
		else conf_ear_num_nodes = atoi(my_num_nodes);

	} else conf_ear_num_nodes = atoi(my_num_nodes);

	return conf_ear_num_nodes;
}
void set_ear_num_nodes(int num_nodes)
{
	conf_ear_num_nodes=num_nodes;
}




int getenv_ear_dynais_levels()
{
	char *dynais_levels=getenv("EAR_DYNAIS_LEVELS");
	int dyn_level;
	if (dynais_levels!=NULL){
		dyn_level=atoi(dynais_levels);
		if (dyn_level>0) conf_ear_dynais_levels=dyn_level;
	}
	return conf_ear_dynais_levels;
}
int getenv_ear_dynais_window_size()
{
        char *dynais_size=getenv("EAR_DYNAIS_WINDOW_SIZE");
        int dyn_size;
        if (dynais_size!=NULL){
                dyn_size=atoi(dynais_size);
                if (dyn_size>0) conf_ear_dynais_window_size=dyn_size;
        }
        return conf_ear_dynais_window_size;

}
int get_ear_dynais_levels()
{
	return conf_ear_dynais_levels;
}

void set_ear_dynais_levels(int levels)
{
	conf_ear_dynais_levels=levels;
}
int get_ear_dynais_window_size()
{
	return conf_ear_dynais_window_size;
}
void set_ear_dynais_window_size(int size)
{
	conf_ear_dynais_window_size=size;
}
void set_ear_learning(int learning)
{
	conf_ear_learning_phase=learning;
}


// get_ functions must be used after getenv_

char * get_ear_user_db_pathname()
{
	return conf_ear_user_db_pathname;
}
char * get_ear_gui_pathname()
{
	return conf_ear_gui_pathname;
}
char * get_ear_coeff_db_pathname()
{
	return conf_ear_coeff_db_pathname;
}

void set_ear_coeff_db_pathname(char *path)
{
	if (path!=NULL){
		if (conf_ear_coeff_db_pathname!=NULL) free(conf_ear_coeff_db_pathname);
		conf_ear_coeff_db_pathname=malloc(strlen(path)+1);
		strcpy(conf_ear_coeff_db_pathname,path);
	}
}
char * get_ear_app_name()
{
	return conf_ear_app_name;
}
int get_ear_learning_phase()
{
	return conf_ear_learning_phase;
}
int get_ear_power_policy()
{
	return conf_ear_power_policy;
}

void set_ear_power_policy(int pid)
{
	conf_ear_power_policy=pid;
}
double get_ear_power_policy_th()
{
	return conf_ear_power_policy_th;
}

void set_ear_power_policy_th(double th)
{
	conf_ear_power_policy_th=th;
}
int get_ear_reset_freq()
{
	return conf_ear_reset_freq;
}
unsigned long get_ear_p_state()
{
	return conf_ear_p_state;
}

void set_ear_p_state(ulong pstate)
{
	conf_ear_p_state=pstate;
}
double get_ear_performance_accuracy()
{
	return conf_ear_performance_accuracy;
}
int get_ear_local_id()
{
	if (conf_ear_local_id<0) getenv_ear_local_id();
	return conf_ear_local_id;
}
int get_ear_num_nodes()
{
	if (conf_ear_num_nodes==0) getenv_ear_num_nodes();
	return conf_ear_num_nodes;
}

// This function reads and process environment variable It must be called before using get_ functions
void ear_lib_environment()
{
	// That part will be initialized from cluster_conf
	getenv_ear_performance_accuracy();
	getenv_ear_dynais_levels();
	// This part will be moved to cluster conf
	getenv_ear_dynais_window_size();
	getenv_ear_coeff_db_pathname();

	getenv_ear_learning_phase();
	getenv_ear_verbose();
	getenv_ear_user_db_pathname();
	getenv_ear_verbose();
	// from cluster_conf
	getenv_ear_tmp();
	// Not needed
	//getenv_ear_db_pathname();
	getenv_ear_user_db_pathname();
	getenv_ear_gui_pathname();
	getenv_ear_app_name();
	getenv_ear_learning_phase();
	// We will assume policy info is ok and set by slurm plug
	getenv_ear_power_policy();
	getenv_ear_power_policy_th();
	getenv_ear_reset_freq();
	getenv_ear_p_state();
	getenv_ear_num_nodes();
	getenv_ear_local_id();

}
// This function writes ear variables in $EAR_TMP/environment.txt file
void ear_print_lib_environment()
{
#if EAR_DEBUG
	char *tmp;
	char environ[256];
	char var[256];
	int fd;
	tmp=get_ear_tmp();
	sprintf(environ,"%s/ear_lib_environment.txt",tmp);
	fd=open(environ,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if (fd<0){
		verbose(0, "EAR error reporting environment variables %s", strerror(errno));
		return;
	}
	sprintf(var,"EAR_TMP=%s\n",get_ear_tmp());
	write(fd,var,strlen(var));
	sprintf(var,"EAR_VERBOSE=%d\n",get_ear_verbose());
	write(fd,var,strlen(var));
	sprintf(var,"EAR_DB_PATHNAME=%s\n",get_ear_db_pathname());
	write(fd,var,strlen(var));
	sprintf(var,"EAR_USER_DB_PATHNAME=%s\n",get_ear_user_db_pathname());
	write(fd,var,strlen(var));
	sprintf(var,"EAR_COEFF_DB_PATHNAME=%s\n",get_ear_coeff_db_pathname());
	write(fd,var,strlen(var));
	sprintf(var,"EAR_GUI_PATH=%s\n",get_ear_gui_pathname());
	write(fd,var,strlen(var));
	sprintf(var,"EAR_APP_NAME=%s\n",get_ear_app_name());
	write(fd,var,strlen(var));
	sprintf(var,"EAR_LEARNING_PHASE=%d\n",get_ear_learning_phase());
	write(fd,var,strlen(var));
	sprintf(var,"EAR_POWER_POLICY=%d\n",get_ear_power_policy());
	write(fd,var,strlen(var));
	sprintf(var,"EAR_POLICY_TH=%lf\n",get_ear_power_policy_th());
	write(fd,var,strlen(var));
	sprintf(var,"EAR_RESET_FREQ=%d\n",get_ear_reset_freq());
	write(fd,var,strlen(var));
	sprintf(var,"EAR_P_STATE=%lu\n",get_ear_p_state());
	write(fd,var,strlen(var));
	sprintf(var,"EAR_NUM_NODES=%d\n",get_ear_num_nodes());
	write(fd,var,strlen(var));
	sprintf(var,"EAR_PERFORMANCE_ACURACY=%lf\n",get_ear_performance_accuracy());
	write(fd,var,strlen(var));
	sprintf(var,"EAR_DYNAIS_LEVELS=%d\n",get_ear_dynais_levels());
	write(fd,var,strlen(var));
	sprintf(var,"EAR_DYNAIS_WINDOW_SIZE=%d\n",get_ear_dynais_window_size());
	write(fd,var,strlen(var));

	close(fd);	
#endif
}


int check_threads()
{
        my_omp_get_max_threads = (int(*)(void)) dlsym (RTLD_DEFAULT, "mkl_get_max_threads");
        if (my_omp_get_max_threads==NULL){
							debug("mkl_get_num_threads symbol not found");
                my_omp_get_max_threads = (int(*)(void)) dlsym (RTLD_DEFAULT, "omp_get_max_threads");
                if (my_omp_get_max_threads==NULL){ 
									debug("omp_get_max_threads symbol not found");
									return 0;
								}
                else{ 
									debug("omp_get_max_threads symbol found");
									return 1;
								}
        }else{
					debug("mkl_get_num_threads symbol found");
				}
        return 1;
}



int get_num_threads()
{
    int num_th;
    if (my_omp_get_max_threads!=NULL){
    	num_th=my_omp_get_max_threads();
			debug("num_threads_detected %d",num_th);
			return num_th;
    }   
    return 1;
} 

int get_total_resources()
{
	int procs_per_node;

	check_threads();

	procs_per_node = get_ear_total_processes()/get_ear_num_nodes();

	// TODO: When an executable has the OpenMP symbols, the OMP_NUM_THREADS
	// variable is taking into account. But maybe the executable is not using
	// any OMP thread.
	return procs_per_node*get_num_threads();
}

state_t read_config_env(char *var, const char* sched_env_var){
    var = getenv(sched_env_var);
    if (var != NULL){
        return EAR_SUCCESS;
    }
    return EAR_UNDEFINED;
}

state_t read_config(uint *var, const char *config_var){
    *var = (uint)atoi(config_var);
    return EAR_SUCCESS;
}
