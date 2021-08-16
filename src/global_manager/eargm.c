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
//#define SHOW_DEBUGS 0
#include <common/states.h>
#include <common/config.h>
#include <common/output/verbose.h>
#include <common/types/daemon_log.h>
#include <common/colors.h>
#include <common/database/db_helper.h>
#include <common/types/generic.h>
#include <common/system/execute.h>
#include <common/types/gm_warning.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/types/configuration/cluster_conf_verbose.h>
#include <global_manager/eargm_ext_rm.h>
#include <daemon/remote_api/eard_rapi.h>
#include <global_manager/cluster_energycap.h>
#if POWERCAP
#include <global_manager/cluster_powercap.h>
#endif

#if SYSLOG_MSG
#include <syslog.h>
#endif

/*
*	EAR Global Manager constants
*/

#define GRACE_PERIOD 100
#define DEFCON_L4	0
#define DEFCON_L3	1
#define DEFCON_L2	2
#define NUM_LEVELS  4
#define BASIC_U		1
#define KILO_U		1000
#define MEGA_U		1000000

#define min(a,b) (a<b?a:b)

ulong th_level[NUM_LEVELS]={10,10,5,0};
ulong pstate_level[NUM_LEVELS]={2,1,0,0};

uint def_p;
uint use_aggregation;
uint units;
uint policy;
uint divisor = 1;
uint last_id=0;
uint process_created=0;
pthread_mutex_t plocks = PTHREAD_MUTEX_INITIALIZER;
uint default_state=1;
uint  max_cluster_power;
uint  cluster_powercap_period;


int last_state=EARGM_NO_PROBLEM;
unsigned long last_avg_power=0,curr_avg_power=0;

uint t1_expired=0;
uint must_refill=0;
uint powercap_th_start=0;

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
uint current_sample=0,total_samples=0,last_level=EARGM_NO_PROBLEM,T1_stables=0;
uint last_risk_sent=EARGM_NO_PROBLEM;
ulong *energy_consumed;
ulong energy_budget;
uint aggregate_samples;
uint in_action=0;
double perc_energy,perc_time,perc_power;
double avg_power_t2,avg_power_t1;
static int fd_my_log=2;


uint reload_eargm_configuration(cluster_conf_t *current,cluster_conf_t *new)
{
	eargm_conf_t *cc,*newc;
	uint must_refil=0;
	cc=&current->eargm;
	newc=&new->eargm;

	/** Atttributes with conflicts */
	if ((cc->t1!=newc->t1) || (cc->t2!=newc->t2) || (cc->energy!=newc->energy) ){	
		verbose(1,"New energy arguments: T1 %lu T2 %lu EnergyBudget %lu",newc->t1,newc->t2,newc->energy);
		must_refil=1;
	}
	if ((cc->power!=newc->power) || (cc->t1_power!=newc->t1_power)){
		verbose(1,"New powercap arguments: Powercap %lu Power cap period %lu",newc->power,newc->t1_power);
		if (cc->power>newc->power)    cluster_reset_default_powercap_all_nodes(&my_cluster_conf);  
		if ((cc->power==0) && (newc->power>0)) powercap_th_start=1;

	}
	if (cc->use_log!=newc->use_log){
		verbose(1,"Log output cannot be dynamically changed, Stop and Start the service");
	}
	if (cc->port!=newc->port){
		verbose(1,"EARGM port cannot be dynamically changed, Stop and Start the service");
	}
	if (cc->units!=newc->units){
		verbose(1,"EARGM units cannot be dynamically changed, Stop and Start the service");
	}
	/** attributes that can be adapted */
	cc->t1=newc->t1;
	cc->t2=newc->t2;
	cc->mode=newc->mode;
	cc->energy=newc->energy;
	cc->use_aggregation=newc->use_aggregation;
	cc->power=newc->power;
	cc->t1_power=newc->t1_power;
	cc->verbose=newc->verbose;
	strcpy(cc->mail,newc->mail);
	memcpy(cc->defcon_limits,newc->defcon_limits,sizeof(uint)*3);
	cc->grace_periods=newc->grace_periods;
	/** Global variables */
	verb_level=cc->verbose;
	period_t1=cc->t1;
	period_t2=cc->t2;	
	energy_budget=cc->energy;
	#if POWERCAP
  max_cluster_power=cc->power;
	max_cluster_power=max_cluster_power*divisor;
  cluster_powercap_period=cc->t1_power;
  #endif
	use_aggregation=cc->use_aggregation;
	return must_refil;
}
void update_eargm_configuration(cluster_conf_t *conf)
{
	verb_level=conf->eargm.verbose;

	if (verbose_arg>0) verb_level=verbose_arg;

	period_t1=conf->eargm.t1;
	period_t2=conf->eargm.t2;
	energy_budget=conf->eargm.energy;
	#if POWERCAP
	max_cluster_power=conf->eargm.power;
  cluster_powercap_period=conf->eargm.t1_power;
	#endif
	my_port=conf->eargm.port;
	use_aggregation=conf->eargm.use_aggregation;
	units=conf->eargm.units;
	policy=conf->eargm.policy;
    switch(units)
    {
        case BASIC:divisor=BASIC_U;
					strcpy(unit_name,"Joules");
					strcpy(unit_energy,"J");
					strcpy(unit_power,"W");
					break;
        case KILO:divisor=KILO_U;
					strcpy(unit_name,"Kilo Joules");
					strcpy(unit_energy,"KJ");
					strcpy(unit_power,"KW");
					break;
        case MEGA:divisor=MEGA_U;	
					strcpy(unit_name,"Mega Joules");
					strcpy(unit_energy,"MJ");
					strcpy(unit_power,"MW");
					break;
        default:break;
    }
		max_cluster_power=max_cluster_power*divisor;
}
void reopen_log()
{
  close(fd_my_log);
  fd_my_log=create_log(my_cluster_conf.install.dir_temp,"eargmd");
  if (fd_my_log<0) fd_my_log=2;
  VERB_SET_FD(fd_my_log);
  ERROR_SET_FD(fd_my_log);
  WARN_SET_FD(fd_my_log);
  DEBUG_SET_FD(fd_my_log);

}

static void my_signals_function(int s)
{
	// uint ppolicy; // unused
	cluster_conf_t new_conf;	
	if (s==SIGALRM){
		alarm(period_t1);
		t1_expired=1;	
		return;
	}
	if (s == SIGUSR2){
		reopen_log();
		return;
	}
	if (s==SIGHUP){
		verbose(VCONF,"Reloading EAR configuration");
		// ppolicy=my_cluster_conf.eargm.policy; // unused
		// Reading the configuration
			
    	if (read_cluster_conf(my_ear_conf_path,&new_conf)!=EAR_SUCCESS){
        	error(" Error reading cluster configuration");
    	} else{
				print_eargm_conf(&new_conf.eargm);
				must_refill=reload_eargm_configuration(&my_cluster_conf,&new_conf);
    	}
		free_cluster_conf(&new_conf);
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
    sigaddset(&set,SIGUSR2);
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
    if (sigaction(SIGUSR2,&my_action,NULL)<0){
        error(" sigaction for SIGUSR2 (%s)",strerror(errno));
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
      verbose(VGM,"%sPercentage over energy budget %.2lf%% (total energy t2 %lu %s , energy limit %lu %s)%s",COL_BLU,perc_energy,e_t2,unit_energy,energy_budget,unit_energy,COL_CLR);
      if (perc_time<100.0){
        if (perc_energy>perc_time){
            warning("WARNING %.2lf%% of energy vs %.2lf%% of time!!",perc_energy,perc_time);
        }
      }
      perc=perc_energy;
      break;
      default:break;
  }
  if (perc<my_cluster_conf.eargm.defcon_limits[DEFCON_L4]) return EARGM_NO_PROBLEM;
  if ((perc>=my_cluster_conf.eargm.defcon_limits[DEFCON_L4]) && (perc<my_cluster_conf.eargm.defcon_limits[DEFCON_L3]))  return EARGM_WARNING1;
  if ((perc>=my_cluster_conf.eargm.defcon_limits[DEFCON_L3]) && (perc<my_cluster_conf.eargm.defcon_limits[DEFCON_L2]))  return EARGM_WARNING2;
  return EARGM_PANIC;

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
		verbose(VGM,"Initializing T2 period with %lu %s, each sample with %lu %s",energy,unit_energy,e_persample,unit_energy);
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
	char command_to_execute[512];
  char *my_command=my_cluster_conf.eargm.energycap_action;
	/* If action text is different from no_action, we must execute it */
  if (strcmp(my_command,"no_action")){
		/* Format for action is: command energy_T1 energy_T2 energy_limit T2 T1 units */
		sprintf(command_to_execute,"%s %lu %lu %lu %u %u %s",my_command,e_t1,e_t2,e_limit,t2,t1,units);
		verbose(0,"Executing energycap_action: %s",command_to_execute);
		execute_with_fork(command_to_execute);	
  }else{
    debug("eargm warning energycap_action not defined");
    return 0;
  }
  return 1;
}


int send_mail(uint level, double energy)
{
    char buff[400];
  char command[4400];
  char mail_filename[SZ_PATH];
  int fd;
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
					pthread_mutex_lock(&plocks);
          process_created--;
					pthread_mutex_unlock(&plocks);
        }else if (pid_process_created<0){
          warning("Waitpid returns <0 and process_created pendings to release");
					pthread_mutex_lock(&plocks);
          process_created=0;
					pthread_mutex_unlock(&plocks);
        }
      }while(process_created>0);

}


/*
*	ACTIONS for WARNING and PANIC LEVELS, USED in OLD version
*/

void report_status(gm_warning_t *my_warning)
{
    #if SYSLOG_MSG
    syslog(LOG_DAEMON|LOG_ERR,"Warning level %lu: p_state %lu energy/power perc %.3lf inc_th %.2lf ",my_warning->level,my_warning->new_p_state,my_warning->energy_percent,my_warning->inc_th);
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

void compute_efficiency_of_actions(unsigned long curr_avg_power,unsigned long last_avg_power,int in_action)
{
	float power_red,power_red_t2;
	uint estimated_t1_needed;
	uint required_saving=5;
	power_red=1.0-(curr_avg_power/last_avg_power);
	//verbose(0,"Power has been reduced by %f in t1",power_red);
	power_red_t2=power_red/aggregate_samples;	
	debug("Power has been reduced by %f in t2 and %f in t1",power_red_t2,power_red);
	switch(last_state){
		case EARGM_WARNING1:required_saving=curr_avg_power-my_cluster_conf.eargm.defcon_limits[DEFCON_L4];break;
		case EARGM_WARNING2:required_saving=curr_avg_power-my_cluster_conf.eargm.defcon_limits[DEFCON_L3];break;
		case EARGM_PANIC:required_saving=curr_avg_power-my_cluster_conf.eargm.defcon_limits[DEFCON_L2];break;
	}
	estimated_t1_needed=required_saving/power_red_t2;	
	debug("%u grace periods are needed to reduce the warning level,required saving %u",estimated_t1_needed,required_saving);
	#if 0
	if (estimated_t1_needed>in_action){
		verbose(0,"We will not reach our target with this number of grace periods");
	}
	#endif
	if (estimated_t1_needed<aggregate_samples){
		/* ACTION */	
	}else{
		/* ACTION */
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
	unsigned int new_actions;
	risk_t current_risk;
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
        //print_cluster_conf(&my_cluster_conf);
				print_eargm_conf(&my_cluster_conf.eargm);
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
		default: break;
	}	
	

		

	if ((period_t1<=0) || (period_t2<=0) || (energy_budget<=0)) usage(argv[0]);
	#if POWERCAP
	cluster_powercap_init(&my_cluster_conf);
	#endif

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
	
  #if 0
  set_default_powercap_all_nodes(&my_cluster_conf);  
  #endif
   
	
	time(&end_time);
	start_time=end_time-period_t2;
	if (db_select_acum_energy( start_time, end_time, divisor, use_aggregation,&last_id,&result)==EAR_ERROR){
		error("Asking for total energy system. Using aggregated %d",use_aggregation);
	}
	verbose(1,"db_select_acum_energy inicial %lu%s",result,unit_energy);
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
	
   			if (last_id==0){
					if (db_select_acum_energy( start_time, end_time, divisor, use_aggregation,&last_id,&result)==EAR_ERROR){
			    	error("Asking for total energy system. Using aggregated %d",use_aggregation);
  				}
 				}else{
	    		if ( db_select_acum_energy_idx(  divisor, use_aggregation,&last_id,&result)<0){
					error("Asing for last T1 energy period. Using aggregated %d.Last id =%u",use_aggregation,last_id);
					}
				}
				debug("Energy consumed in last T1 %lu %s",result,unit_energy);
	    	if (!result){ 
				verbose(VGM+1,"No results in that period of time found");
	    	}else{ 
					resulti=(int)result;
				if (resulti < 0) exit(1);
			}
			last_avg_power=curr_avg_power;
			curr_avg_power=(unsigned long)(result/period_t1);
			verbose(VGM,"%sEnergy consumed in last %u seconds %lu %s. Avg power %lu %s%s",COL_BLU,period_t1,result,unit_energy,curr_avg_power,unit_power,COL_CLR);
			
	
			new_energy_sample(result);
			energy_t1=result;
			total_energy_t2=compute_energy_t2();	
			perc_energy=((double)total_energy_t2/(double)energy_budget)*(double)100;

			uint current_level=defcon(total_energy_t2,energy_t1,total_nodes);
			set_gm_status(&my_warning,energy_t1,total_energy_t2,energy_budget,period_t1,period_t2,perc_energy,policy);
	
			if (!in_action){
			my_warning.level=current_level;
			my_warning.inc_th=0;my_warning.new_p_state=0;
			switch(current_level){
			case EARGM_NO_PROBLEM:
				verbose(VGM,"****************************************************************");
				verbose(VGM,"%s Safe area. energy budget %.2lf%%%s ",COL_GRE,perc_energy,COL_CLR);
				verbose(VGM,"****************************************************************");
				if ((my_cluster_conf.eargm.mode) && (last_level==EARGM_NO_PROBLEM) && (!default_state)){ 
					verbose(VGM,"Restoring default configuration");
					restore_conf_all_nodes(&my_cluster_conf);
					last_risk_sent=EARGM_NO_PROBLEM;
					default_state=1;
				}
				break;
			case EARGM_WARNING1:
				last_state=EARGM_WARNING1;
				in_action+=my_cluster_conf.eargm.grace_periods;
				verbose(VGM,"****************************************************************");
				verbose(VGM,"%sWARNING1... we are close to the maximum energy budget %.2lf%% %s",COL_RED,perc_energy,COL_CLR);
				verbose(VGM,"****************************************************************");
	
				if (my_cluster_conf.eargm.mode && last_risk_sent!=EARGM_WARNING1){ // my_cluster_conf.eargm.mode==1 is AUTOMATIC mode
					create_risk(&current_risk,EARGM_WARNING1);
					set_risk_all_nodes(current_risk,MAXENERGY,&my_cluster_conf);
				}
				new_actions=send_mail(EARGM_WARNING1,perc_energy);
				new_actions+=execute_action(energy_t1,total_energy_t2,energy_budget,period_t2,period_t1,unit_energy);
				pthread_mutex_lock(&plocks);
				process_created+=new_actions;
				pthread_mutex_unlock(&plocks);
				report_status(&my_warning);
				break;
			case EARGM_WARNING2:
				last_state=EARGM_WARNING2;
				in_action+=my_cluster_conf.eargm.grace_periods;
				verbose(VGM,"****************************************************************");
				verbose(VGM,"%sWARNING2... we are close to the maximum energy budget %.2lf%%%s ",COL_RED,perc_energy,COL_CLR);
				verbose(VGM,"****************************************************************");
				if (last_risk_sent!=EARGM_WARNING2){ // my_cluster_conf.eargm.mode==1 is AUTOMATIC mode
					manage_warning(&current_risk,EARGM_WARNING2,my_cluster_conf,0.05,my_cluster_conf.eargm.mode);
					#if 0
					create_risk(&current_risk,EARGM_WARNING2);
					set_risk_all_nodes(current_risk,MAXENERGY,my_cluster_conf);
					#endif
				}
				my_warning.energy_percent=perc_energy;
				new_actions=send_mail(EARGM_WARNING2,perc_energy);
				new_actions+=execute_action(energy_t1,total_energy_t2,energy_budget,period_t2,period_t1,unit_energy);
				pthread_mutex_lock(&plocks);
        process_created+=new_actions;
        pthread_mutex_unlock(&plocks);
				report_status(&my_warning);
				break;
			case EARGM_PANIC:
				last_state=EARGM_PANIC;
				in_action+=my_cluster_conf.eargm.grace_periods;
				verbose(VGM,"****************************************************************");
				verbose(VGM,"%sPANIC!... we are close or over the maximum energy budget %.2lf%%%s ",COL_RED,perc_energy,COL_CLR);
				verbose(VGM,"****************************************************************");
				if (last_risk_sent!=EARGM_PANIC){ // my_cluster_conf.eargm.mode==1 is AUTOMATIC mode
					manage_warning(&current_risk,EARGM_PANIC,my_cluster_conf,0.1,my_cluster_conf.eargm.mode);
					#if 0
					create_risk(&current_risk,EARGM_PANIC);	
					set_risk_all_nodes(current_risk,MAXENERGY,my_cluster_conf);
					#endif
				}
				new_actions=send_mail(EARGM_PANIC,perc_energy);
				new_actions+=execute_action(energy_t1,total_energy_t2,energy_budget,period_t2,period_t1,unit_energy);
				pthread_mutex_lock(&plocks);
        process_created+=new_actions;
        pthread_mutex_unlock(&plocks);
				report_status(&my_warning);
				break;
			}
			}else{ 
				/* We can check here the effect of actions */				
				//compute_efficiency_of_actions(curr_avg_power,last_avg_power,in_action);
				in_action--;
				set_gm_grace_period_values(&my_warning);
			}
			if (current_level!=last_level) T1_stables=0;
			else T1_stables++;
			#if USE_DB
			db_insert_gm_warning(&my_warning);
			#endif
			last_level = current_level;
		}// ALARM
		if (powercap_th_start){
			cluster_powercap_init(&my_cluster_conf);	
			powercap_th_start=0;
		}
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




