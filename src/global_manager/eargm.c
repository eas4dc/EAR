/**************************************************************
*   Energy Aware Runtime (EAR)
*   This program is part of the Energy Aware Runtime (EAR).
*
*   EAR provides a dynamic, transparent and ligth-weigth solution for
*   Energy management.
*
*       It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*   BSC Contact     mailto:ear-support@bsc.es
*   Lenovo contact  mailto:hpchelp@lenovo.com
*
*   EAR is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Lesser General Public
*   License as published by the Free Software Foundation; either
*   version 2.1 of the License, or (at your option) any later version.
*   
*   EAR is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Lesser General Public License for more details.
*   
*   You should have received a copy of the GNU Lesser General Public
*   License along with EAR; if not, write to the Free Software
*   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*   The GNU LEsser General Public License is contained in the file COPYING  
*/

#define _XOPEN_SOURCE 700 //to get rid of the warning

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <common/states.h>
#include <common/config.h>
//#define SHOW_DEBUGS 0
#include <common/output/verbose.h>
#include <common/types/daemon_log.h>
#include <common/database/db_helper.h>
#include <common/types/generic.h>
#include <common/system/execute.h>
#include <common/types/gm_warning.h>
#include <common/types/configuration/cluster_conf.h>
#include <global_manager/eargm_ext_rm.h>
#include <daemon/eard_rapi.h>

#if SYSLOG_MSG
#include <syslog.h>
#endif

/*
*	EAR Global Manager constants
*/

#define GRACE_PERIOD 100
#define NO_PROBLEM 	3
#define WARNING_3	2
#define WARNING_2	1
#define PANIC		0
#define NUM_LEVELS  4
#define DEFCON_L4	0
#define DEFCON_L3	1
#define DEFCON_L2	2
#define BASIC_U		1
#define KILO_U		1000
#define MEGA_U		1000000

ulong th_level[NUM_LEVELS]={10,10,5,0};
ulong pstate_level[NUM_LEVELS]={2,1,0,0};

uint def_p;
uint use_aggregation;
uint units;
uint policy;
uint divisor = 1;
uint last_id=0;
uint process_created=0;
static uint default_state=1;

uint t1_expired=0;
uint must_refill=0;

pthread_t eargm_server_api_th;
cluster_conf_t my_cluster_conf;
char my_ear_conf_path[GENERIC_NAME];	
uint total_nodes=0;
char  unit_name[128],unit_energy[128],unit_power[128];

/* 
* EAR Global Manager global data
*/
int verbose_arg=-1;
uint period_t1,period_t2;
ulong total_energy_t2,energy_t1;
uint my_port;
uint current_sample=0,total_samples=0,last_level=NO_PROBLEM,T1_stables=0;
ulong *energy_consumed;
ulong energy_budget;
ulong power_budget;
uint aggregate_samples;
uint in_action=0;
double perc_energy,perc_time,perc_power;
double avg_power_t2,avg_power_t1;
static int fd_my_log=2;
double curr_th;
ulong curr_max;



void update_eargm_configuration(cluster_conf_t *conf)
{
	verb_level=conf->eargm.verbose;

	if (verbose_arg>0) verb_level=verbose_arg;

	period_t1=conf->eargm.t1;
	period_t2=conf->eargm.t2;
	energy_budget=conf->eargm.energy;
	power_budget=conf->eargm.energy;
	my_port=conf->eargm.port;
	use_aggregation=conf->eargm.use_aggregation;
	units=conf->eargm.units;
	policy=conf->eargm.policy;
    switch(units)
    {
        case BASIC:divisor=BASIC_U;
			switch (policy){
				case MAXENERGY:strcpy(unit_name,"Joules");break;
				case MAXPOWER:strcpy(unit_name,"Watts");break;
			}
			strcpy(unit_energy,"J");
			strcpy(unit_power,"W");
			
			break;
        case KILO:divisor=KILO_U;
			switch (policy){
				case MAXENERGY:strcpy(unit_name,"Kilo Joules");break;
				case MAXPOWER:strcpy(unit_name,"Kilo Watts");break;
			}
			strcpy(unit_energy,"KJ");
			strcpy(unit_power,"KW");
			break;
        case MEGA:divisor=MEGA_U;	
			switch (policy){
				case MAXENERGY:strcpy(unit_name,"Mega Joules");break;
				case MAXPOWER:strcpy(unit_name,"Mega Watts");break;
			}
			strcpy(unit_energy,"MJ");
			strcpy(unit_power,"MW");
			break;
        default:break;
    }
		def_p=conf->default_policy;
		if ((strcmp(conf->power_policies[def_p].name,"min_time")) && (conf->eargm.mode)){
			verbose(0,"Warning, default_policy is not min_time. AUtomatic mode only supported when min_time is default policy. Setting it to manual");
			conf->eargm.mode=0;	
		}
		curr_th=conf->power_policies[def_p].settings[0];
		curr_max=conf->eard.max_pstate;
		verbose(1,"Default th for policy min_time %lf, max_pstate %lu",curr_th,curr_max);
}


static void my_signals_function(int s)
{
	uint ppolicy;
	if (s==SIGALRM){
		alarm(period_t1);
		t1_expired=1;	
		return;
	}
	if (s==SIGHUP){
		verbose(VCONF,"Reloading EAR configuration");
		ppolicy=my_cluster_conf.eargm.policy;
		free_cluster_conf(&my_cluster_conf);
		// Reading the configuration
			
    	if (read_cluster_conf(my_ear_conf_path,&my_cluster_conf)!=EAR_SUCCESS){
        	error(" Error reading cluster configuration");
    	}
    	else{
        	print_cluster_conf(&my_cluster_conf);
			update_eargm_configuration(&my_cluster_conf);
			must_refill=1;
			if (ppolicy!=policy){
				error(" Error policy can not be change on the fly, stop & start EARGM");
				policy=ppolicy;
			}
			if (policy==MAXENERGY){ 
				verbose(VCONF,"Using new energy limit %lu",energy_budget);
			}
			if (policy==MAXPOWER){ 
				verbose(VCONF,"Using new power limit %lu",power_budget);
			}
    	}
	}else{
		verbose(VGM,"Exiting");
	    #if SYSLOG_MSG
	    closelog();
	    #endif

		exit(0);
	}
}

static void catch_signals()
{
    sigset_t set;
    struct sigaction my_action;

    sigemptyset(&set);
    sigaddset(&set,SIGHUP);
    sigaddset(&set,SIGTERM);
    sigaddset(&set,SIGINT);
    sigaddset(&set,SIGALRM);
    if (sigprocmask(SIG_SETMASK,&set,NULL)<0){
        error("Setting signal mask (%s)",strerror(errno));
        exit(1);
    }

    my_action.sa_handler=my_signals_function;
    sigemptyset(&my_action.sa_mask);
    my_action.sa_flags=0;
    if (sigaction(SIGHUP,&my_action,NULL)<0){
        error(" sigaction for SIGINT (%s)",strerror(errno));
        exit(1);
    }
    if (sigaction(SIGTERM,&my_action,NULL)<0){
        error("sigaction for SIGINT (%s)",strerror(errno));
        exit(1);
    }
    if (sigaction(SIGINT,&my_action,NULL)<0){
        error(" sigaction for SIGINT (%s)",strerror(errno));
        exit(1);
    }
    if (sigaction(SIGALRM,&my_action,NULL)<0){
        error(" sigaction for SIGALRM (%s)",strerror(errno));
        exit(1);
    }

}

void new_energy_sample(ulong result)
{
	energy_consumed[current_sample]=result;
	current_sample=(current_sample+1)%aggregate_samples;
	if (total_samples<aggregate_samples) total_samples++;
}

ulong compute_energy_t2()
{
	ulong energy=0;
	uint i;
	int limit=aggregate_samples;
	if (total_samples<aggregate_samples) limit=total_samples;
	for (i=0;i<limit;i++){
		energy+=energy_consumed[i];
	}
	return energy;
}



uint defcon(ulong e_t2,ulong e_t1,ulong load)
{
  double perc;
  switch (policy){
    case MAXENERGY:
      perc_energy=((double)e_t2/(double)energy_budget)*(double)100;
      perc_time=((double)total_samples/(double)aggregate_samples)*(double)100;
      verbose(VGM,"Percentage over energy budget %.2lf%% (total energy t2 %lu , energy limit %lu)",perc_energy,e_t2,energy_budget);
      if (perc_time<100.0){
        if (perc_energy>perc_time){
            warning("WARNING %.2lf%% of energy vs %.2lf%% of time!!",perc_energy,perc_time);
        }
      }
      perc=perc_energy;
      break;
    case MAXPOWER:;
      avg_power_t1=(e_t1/period_t1);
      avg_power_t2=(e_t2/period_t2);
      verbose(VGM,"Avg. Power for T1 %lf\nAvg. Power for T2 %lf",avg_power_t1,avg_power_t2);
      perc_power=(double)avg_power_t1/(double)power_budget;
      perc=perc_power;
      break;
  }
  if (perc<my_cluster_conf.eargm.defcon_limits[DEFCON_L4]) return NO_PROBLEM;
  if ((perc>=my_cluster_conf.eargm.defcon_limits[DEFCON_L4]) && (perc<my_cluster_conf.eargm.defcon_limits[DEFCON_L3]))  return WARNING_3;
  if ((perc>=my_cluster_conf.eargm.defcon_limits[DEFCON_L3]) && (perc<my_cluster_conf.eargm.defcon_limits[DEFCON_L2]))  return WARNING_2;
  return PANIC;

}

void fill_periods(ulong energy)
{
	int i;
	ulong e_persample;
	e_persample=energy/aggregate_samples;
	for (i=0;i<aggregate_samples;i++){
		energy_consumed[i]=e_persample;
	}
	total_samples=aggregate_samples;
	current_sample=0;
	switch (policy)
	{
	case MAXENERGY:
		verbose(VGM,"Initializing T2 period with %lu %s, each sample with %lu %s",energy,unit_name,e_persample,unit_name);
		break;
	case MAXPOWER:
		verbose(VGM,"AVG power in last %d seconds %lu %s",period_t1,e_persample/period_t1,unit_name);
		break;
	}
}

void process_status(pid_t pid_process_created,int process_created_status)
{
  int st;
  if (WIFEXITED(process_created_status))
  {
    st=WEXITSTATUS(process_created_status);
    if (st==0) return;
    error("Process %d terminates with exit status %d",pid_process_created,st);
  }else{
    error("Process %d terminates with signal %d",pid_process_created,WTERMSIG(process_created_status));
  }
}


int execute_action(ulong e_t1,ulong e_t2,ulong e_limit,uint t2,uint t1,char *units)
{
  int ret;
	char command_to_execute[512];
  char *my_command=getenv("EAR_WARNING_COMMAND");
  if (my_command!=NULL){
		sprintf(command_to_execute,"%s %lu %lu %lu %u %u %s",my_command,e_t1,e_t2,e_limit,t2,t1,units);
		verbose(0,"Executing %s",command_to_execute);
		execute_with_fork(command_to_execute);	
  }else{
    debug("eargm warning command not defined");
    return 0;
  }
  return 1;
}


int send_mail(uint level, double energy)
{
	
	char buff[128];
  char command[1024];
  char mail_filename[SZ_PATH];
  int fd,ret;
  if (strcmp(my_cluster_conf.eargm.mail,"nomail")){ 
		sprintf(buff,"Detected WARNING level %u, %lfi %% of energy from the total energy limit\n",level,energy);
		sprintf(mail_filename,"%s/warning_mail.txt",my_cluster_conf.install.dir_temp);
		fd=open(mail_filename,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  	if (fd<0){
      error("Warning mail file cannot be created at %s (%s)",mail_filename,strerror(errno));
			return 0;
  	}   
  	write(fd,buff,strlen(buff));
  	close(fd);
  	sprintf(command,"mailx -s \"Energy limit warning\" %s < %s",my_cluster_conf.eargm.mail,mail_filename);
		execute_with_fork(command);
		return 1;
	}
	return 0;
}

void check_pending_processes()
{
      pid_t pid_process_created;
      int process_created_status;
      do{
        /* Processing processes created */
        if ((pid_process_created=waitpid(-1,&process_created_status,WNOHANG))>0){
          process_status(pid_process_created,process_created_status);
          process_created--;
        }else if (pid_process_created<0){
          warning("Waitpid returns <0 and process_created pendings to release");
          process_created=0;
        }
      }while(pid_process_created>0);

}


/*
*	ACTIONS for WARNING and PANIC LEVELS
*/

double adapt_th(uint status)
{
	ulong def_th;
	def_th=(ulong)(my_cluster_conf.power_policies[def_p].settings[0]*100);
	switch (status){
	case WARNING_3:
	case WARNING_2:
	case PANIC:
		default_state=0;
		def_th=def_th+th_level[status];
		break;
	}
	verbose(1,"Setting th in all nodes to %lu",def_th);
	if (def_th!=curr_th){ 
		set_th_all_nodes(def_th,def_p,my_cluster_conf);
	}
	curr_th=def_th;
	return def_th;
}
unsigned long adapt_pstate(uint status)
{
	unsigned int def,max;
	int variation;
	def=my_cluster_conf.power_policies[def_p].p_state;
	max=my_cluster_conf.eard.max_pstate;
	switch (status){
	case WARNING_3:
	case WARNING_2:
	case PANIC:
		default_state=0;
		def=def+pstate_level[status];
		max=max+pstate_level[status];
		break;
	}
	variation=max-curr_max;
	verbose(1,"Setting def pstate %u and max_pstate %u in all nodes, variation %d",def,max, variation);
	if (curr_max!=max){
		//set_max_pstate_all_nodes(max,my_cluster_conf);
		// set_def_pstate_all_nodes(def,def_p,my_cluster_conf);
		red_def_max_pstate_all_nodes((uint)variation,my_cluster_conf);
	}
	curr_max=max;
	return max;
}

void report_status(gm_warning_t *my_warning)
{
    #if SYSLOG_MSG
    syslog(LOG_DAEMON|LOG_ERR,"Warning level %lu: p_state %lu energy/power perc %.3lf inc_th %.2lf \n",my_warning->level,my_warning->new_p_state,my_warning->energy_percent,my_warning->inc_th);
	#endif
}

void set_gm_grace_period_values(gm_warning_t *my_warning)
{
	if (my_warning==NULL) return;
	my_warning->level=GRACE_PERIOD;
	my_warning->new_p_state=GRACE_PERIOD;
}
void set_gm_status(gm_warning_t *my_warning,ulong et1,ulong et2,ulong ebudget,uint t1,uint t2,double perc,uint policy)
{
	if (my_warning==NULL) return;
	my_warning->energy_percent=perc;
	my_warning->new_p_state=0;
	my_warning->inc_th=0.0;
  my_warning->energy_t1=et1;
  my_warning->energy_t2=et2;
  my_warning->energy_limit=ebudget;
  my_warning->energy_p1=t1;
  my_warning->energy_p2=t2;
	switch(policy){
		case MAXENERGY:strcpy(my_warning->policy,"EnergyBudget");break;
		case MAXPOWER:strcpy(my_warning->policy,"PowerBudget");break;
		default:strcpy(my_warning->policy,"Error");
	}
}


/*
*
*	EAR GLOBAL MANAGER
*
*
*/
void usage(char *app)
{
    verbose(0, "Usage: %s [-h]|[--help]|verbose_level]", app);
	verbose(0, "This program controls the energy consumed in a period T2 seconds defined in $EAR_ETC/ear/ear.conf file");
	verbose(0, "energy is checked every T1 seconds interval");
	verbose(0, "Global manager can be configured in active or passive mode. Automatic actions are taken in active mode (defined in ear.conf file)");
    exit(0);
}


void parse_args(char *argv[])
{
	if (strcmp(argv[1],"-h")==0 || strcmp(argv[1],"--help")==0){
		usage(argv[0]);
		exit(0);
	}
	verbose_arg=atoi(argv[1]);	
}

#define GM_DEBUG 1

int main(int argc,char *argv[])
{
	sigset_t set;
	int ret;
	ulong result;
	int resulti;
	gm_warning_t my_warning;
    if (argc > 2) usage(argv[0]);
	if (argc==2) parse_args(argv);
    // We read the cluster configuration and sets default values in the shared memory
    if (get_ear_conf_path(my_ear_conf_path)==EAR_ERROR){
        error("Error opening ear.conf file, not available at regular paths ($EAR_ETC/ear/ear.conf)");
        exit(0);
    }
    #if SYSLOG_MSG
    openlog("eargm",LOG_PID|LOG_PERROR,LOG_DAEMON);
    #endif

	verbose(VGM,"Using %s as EARGM configuration file",my_ear_conf_path);
    if (read_cluster_conf(my_ear_conf_path,&my_cluster_conf)!=EAR_SUCCESS){
        error(" Error reading cluster configuration");
    }
    else{
        print_cluster_conf(&my_cluster_conf);
    }
	if (my_cluster_conf.eargm.use_log){
		fd_my_log=create_log(my_cluster_conf.install.dir_temp,"eargmd");
	
	}
  VERB_SET_FD(fd_my_log);
  ERROR_SET_FD(fd_my_log);
	WARN_SET_FD(fd_my_log);
	DEBUG_SET_FD(fd_my_log);
	TIMESTAMP_SET_EN(my_cluster_conf.eargm.use_log);

    update_eargm_configuration(&my_cluster_conf);


	switch (policy){
		case MAXENERGY:
			verbose(VGM,"MAXENERGY policy configured with limit %lu %s",energy_budget,unit_name);
			break;
		case MAXPOWER:
			verbose(VGM,"MAXPOWER policy configured with limit %lu %s",power_budget,unit_name);
			break;
	}	
	
		

	if ((period_t1<=0) || (period_t2<=0) || (energy_budget<=0)) usage(argv[0]);

	aggregate_samples=period_t2/period_t1;
	if ((period_t2%period_t1)!=0){
		warning("warning period_t2 is not multiple of period_t1");
		aggregate_samples++;
	}

	energy_consumed=malloc(sizeof(ulong)*aggregate_samples);



	catch_signals();

	/* This thread accepts external commands */
    if ((ret=pthread_create(&eargm_server_api_th, NULL, eargm_server_api, NULL))){
        errno=ret;
		error("error creating eargm_server for external api %s",strerror(errno));
    }

	
    time_t start_time, end_time;
	double perc_energy;
   	
	
	sigfillset(&set);
	sigdelset(&set,SIGALRM);
	sigdelset(&set,SIGHUP);
	sigdelset(&set,SIGTERM);
	sigdelset(&set,SIGINT);

    #if USE_DB
    verbose(VGM+1,"Connecting with EAR DB");
    init_db_helper(&my_cluster_conf.database);
    #endif
	
    
	
	time(&end_time);
	start_time=end_time-period_t2;
	if (db_select_acum_energy( start_time, end_time, divisor, use_aggregation,&last_id,&result)<0){
		error("Asking for total energy system. Using aggregated %d",use_aggregation);
	}
	debug("db_select_acum_energy inicial %lu",result);
	fill_periods(result);
	/*
	*
	*	MAIN LOOP
	*			
	*/
	if (my_cluster_conf.eargm.mode==0){ 
		my_warning.inc_th=0; 
		my_warning.new_p_state=0;
	}
	alarm(period_t1);
	while(1){
		// Waiting a for period_t1
		sigsuspend(&set);
	 	if (process_created){
			check_pending_processes();
    }   
		// ALARM RECEIVED
		if (t1_expired){
			t1_expired=0;

			// Compute the period
			time(&end_time);
			start_time=end_time-period_t1;
	
    
	    	if ( db_select_acum_energy_idx(  divisor, use_aggregation,&last_id,&result)<0){
				error("Asing for last T1 energy period. Using aggregated %d.Last id =%u",use_aggregation,last_id);
				}
				debug("Energy consumed in last T1 %lu",result);
	    	if (!result){ 
				verbose(VGM+1,"No results in that period of time found");
	    	}else{ 
					resulti=(int)result;
				if (resulti < 0) exit(1);
			}
			verbose(VGM,"Energy consumed in last %u seconds %lu %s. Avg power %lu %s",period_t1,result,unit_energy,(unsigned long)(result/period_t1),unit_power);
			
	
			new_energy_sample(result);
			energy_t1=result;
			total_energy_t2=compute_energy_t2();	
			perc_energy=((double)total_energy_t2/(double)energy_budget)*(double)100;

			uint current_level=defcon(total_energy_t2,energy_t1,total_nodes);
			set_gm_status(&my_warning,energy_t1,total_energy_t2,energy_budget,period_t1,period_t2,perc_energy,policy);
	
			if (!in_action){
			my_warning.level=current_level;
			switch(current_level){
			case NO_PROBLEM:
				verbose(VGM," Safe area. energy budget %.2lf%% ",perc_energy);
				if ((my_cluster_conf.eargm.mode) && (last_level==NO_PROBLEM) && (!default_state)){ 
					verbose(VGM,"Restoring default configuration");
					restore_conf_all_nodes(my_cluster_conf);
					default_state=1;
				}
				break;
			case WARNING_3:
				in_action+=my_cluster_conf.eargm.grace_periods;
				verbose(VGM,"****************************************************************");
				verbose(VGM,"WARNING1... we are close to the maximum energy budget %.2lf%% ",perc_energy);
				verbose(VGM,"****************************************************************");
	
				if (my_cluster_conf.eargm.mode){ // my_cluster_conf.eargm.mode==1 is AUTOMATIC mode
					my_warning.inc_th=adapt_th(WARNING_3);            
					my_warning.new_p_state=adapt_pstate(WARNING_3);
				}
				process_created+=send_mail(WARNING_3,perc_energy);
				process_created+=execute_action(energy_t1,total_energy_t2,energy_budget,period_t2,period_t1,unit_energy);
				report_status(&my_warning);
				break;
			case WARNING_2:
				in_action+=my_cluster_conf.eargm.grace_periods;
				verbose(VGM,"****************************************************************");
				verbose(VGM,"WARNING2... we are close to the maximum energy budget %.2lf%% ",perc_energy);
				verbose(VGM,"****************************************************************");
				if (my_cluster_conf.eargm.mode){ // my_cluster_conf.eargm.mode==1 is AUTOMATIC mode
					my_warning.inc_th=adapt_th(WARNING_2);            
					my_warning.new_p_state=adapt_pstate(WARNING_2);
				}
				process_created+=send_mail(WARNING_2,perc_energy);
				process_created+=execute_action(energy_t1,total_energy_t2,energy_budget,period_t2,period_t1,unit_energy);
				report_status(&my_warning);
				break;
			case PANIC:
				in_action+=my_cluster_conf.eargm.grace_periods;
				verbose(VGM,"****************************************************************");
				verbose(VGM,"PANIC!... we are close or over the maximum energy budget %.2lf%% ",perc_energy);
				verbose(VGM,"****************************************************************");
				if (my_cluster_conf.eargm.mode){ // my_cluster_conf.eargm.mode==1 is AUTOMATIC mode
					my_warning.inc_th=adapt_th(PANIC);            
					my_warning.new_p_state=adapt_pstate(PANIC);
				}
				process_created+=send_mail(PANIC,perc_energy);
				process_created+=execute_action(energy_t1,total_energy_t2,energy_budget,period_t2,period_t1,unit_energy);
				report_status(&my_warning);
				break;
			}
			}else{ 
				in_action--;
				set_gm_grace_period_values(&my_warning);
			}
			if (current_level!=last_level) T1_stables=0;
			else T1_stables++;
			last_level=current_level;
		}// ALARM
		#if USE_DB
		db_insert_gm_warning(&my_warning);
		#endif
		if (must_refill){
			must_refill=0;
    		aggregate_samples=period_t2/period_t1;
    				if ((period_t2%period_t1)!=0){
        				verbose(VGM+1,"warning period_t2 is not multiple of period_t1");
        				aggregate_samples++;
    		}
			if (energy_consumed!=NULL) free(energy_consumed);
    		energy_consumed=malloc(sizeof(ulong)*aggregate_samples);
		    time(&end_time);
    		start_time=end_time-period_t2;
			last_id=0;
    		if (db_select_acum_energy( start_time, end_time, divisor, use_aggregation,&last_id,&result)<0){
				error("Asking for total energy system. Using aggregated %d",use_aggregation);
			}
    		fill_periods(result);
		}
	}

    #if SYSLOG_MSG
    closelog();
    #endif


    
		return 0;
}




