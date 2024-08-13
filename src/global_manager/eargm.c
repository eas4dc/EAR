/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define PRINT_ONLY 1 //prints config and exits
#define _XOPEN_SOURCE 700 //to get rid of the warning

#define GM_DEBUG 0

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
//#define SHOW_DEBUGS 0
#include <report/report.h>

#include <common/states.h>
#include <common/config.h>
#include <common/colors.h>
#include <common/system/execute.h>
#include <common/output/verbose.h>
#include <common/database/db_helper.h>
#include <common/utils/sched_support.h>

#include <common/types/generic.h>
#include <common/types/gm_warning.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/types/configuration/cluster_conf_verbose.h>

#include <daemon/remote_api/eard_rapi_internals.h>
#include <daemon/remote_api/eard_rapi.h>

#include <global_manager/meta_eargm.h>
#include <global_manager/eargm_ext_rm.h>
#include <global_manager/cluster_energycap.h>
#include <global_manager/cluster_powercap.h>

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

uint eargm_id;
uint def_p;
uint use_aggregation;
uint units;
uint policy;
uint divisor = 1;
uint last_id=0;
uint process_created=0;
pthread_mutex_t plocks = PTHREAD_MUTEX_INITIALIZER;
uint default_state=1;
int64_t default_cluster_powercap;
uint  cluster_powercap_period;


int last_state=EARGM_NO_PROBLEM;
unsigned long last_avg_power=0,curr_avg_power=0;

uint t1_expired=0;
uint must_refill=0;
uint powercap_th_start=0;

pthread_t eargm_server_api_th;
pthread_t meta_eargm_th;
cluster_conf_t my_cluster_conf;
eargm_def_t *e_def;
char **nodes = NULL;
int num_eargm_nodes = 0;
char my_ear_conf_path[GENERIC_NAME] = { 0 };	
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
int64_t energy_budget;
uint aggregate_samples;
uint in_action=0;
double perc_energy,perc_time,perc_power;
double avg_power_t2,avg_power_t1;
static int fd_my_log=2;
char host[GENERIC_NAME];

bool use_first_eargm_if_failed = false;


void create_tmp(char *tmp_dir) {
        int ret;
        ret = mkdir(tmp_dir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if ((ret < 0) && (errno != EEXIST)) {
                error("ear tmp dir cannot be created (%s)", strerror(errno));
                _exit(0);
        }

        if (chmod(tmp_dir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH) < 0) {
                warning("ear_tmp permissions cannot be set (%s)", strerror(errno));
                _exit(0);
        }
}



uint reload_eargm_configuration(cluster_conf_t *current,cluster_conf_t *new)
{
    eargm_conf_t *cc,*newc;
    uint must_refil=0;

    cc=&current->eargm;
    newc=&new->eargm;

    /* EARGM specific conf */
    e_def = get_eargm_conf(new, host);
    if (e_def == NULL) {
        error("Could find node in EARGMs list in ear.conf, exiting\n");
        exit(1);
    }

    /** Atttributes with conflicts */
    if ((cc->t1!=newc->t1) || (cc->t2!=newc->t2) || (cc->energy!=newc->energy) ){	
        verbose(VGM,"New energy arguments: T1 %lu T2 %lu EnergyBudget %lu",newc->t1,newc->t2,newc->energy);
        must_refil=1;
    }
    if ((cc->power!=newc->power) || (cc->t1_power!=newc->t1_power)){
        verbose(VGM,"New powercap arguments: Powercap %lu Power cap period %lu",newc->power,newc->t1_power);
        if (cc->power>newc->power)    ear_cluster_reset_default_powercap(&my_cluster_conf);  
        if ((cc->power==0) && (newc->power>0)) powercap_th_start=1;

    }
    if (cc->use_log!=newc->use_log){
        verbose(VGM,"Log output cannot be dynamically changed, Stop and Start the service");
    }
    if (cc->port!=newc->port){
        verbose(VGM,"EARGM port cannot be dynamically changed, Stop and Start the service");
    }
    if (cc->units!=newc->units){
        verbose(VGM,"EARGM units cannot be dynamically changed, Stop and Start the service");
    }
    /** attributes that can be adapted */
    cc->t1=newc->t1;
    cc->t2=newc->t2;
    cc->mode=newc->mode;
    cc->energy=e_def->energy;
    cc->use_aggregation=newc->use_aggregation;
    cc->power=e_def->power;
    cc->t1_power=newc->t1_power;
    cc->verbose=newc->verbose;
    strcpy(cc->mail,newc->mail);
    memcpy(cc->defcon_limits,newc->defcon_limits,sizeof(uint)*3);
    cc->grace_periods=newc->grace_periods;
    /** Global variables */
    verb_level=cc->verbose;
    period_t1=cc->t1;
    period_t2=cc->t2;	
    energy_budget=e_def->energy;
    if (energy_budget == -1) {
        energy_budget = cluster_get_min_power(new, ENERGY_TYPE)*cc->t2;
        energy_budget /= divisor;
        e_def->energy = energy_budget;
    }
    default_cluster_powercap=cc->power;
    //default_cluster_power=default_cluster_power*divisor;
    cluster_powercap_period=cc->t1_power;
    use_aggregation=cc->use_aggregation;
    return must_refil;
}

void update_eargm_configuration(cluster_conf_t *conf)
{
    verb_level=conf->eargm.verbose;
    char host[GENERIC_NAME];

    /* EARGM specific conf */
    gethostname(host, sizeof(host));
    strtok(host, ".");
    int i, id;
    char *idx = ear_getenv("EARGMID");
    e_def = NULL;
    if (idx != NULL) {
        id = atoi(idx);
        for (i = 0; i < conf->eargm.num_eargms; i++)
        {
            if (conf->eargm.eargms[i].id == id)
            {
                e_def = &conf->eargm.eargms[i];
                break;
            }
        }
    }
    else {
        verbose(VGM, "Couldn't find EARGMID environment variable, using hostname\n");
        e_def = get_eargm_conf(conf, host);
    }
    if (e_def == NULL) {
        if (use_first_eargm_if_failed) {
            if (conf->eargm.num_eargms == 0) {
                error("At least one EARGM definition must be set in ear.conf for a local EARGM to be usabel. Exiting.\n");
                exit(1);
            }
            e_def = &conf->eargm.eargms[0];
            if (e_def->num_subs > 0) {
                free(e_def->subs);
                e_def->num_subs = 0;
            }
        } else {
            error("Couldn't find node in EARGMs list in ear.conf, exiting\n");
            exit(1);
        }
    }
    char *nnodes = ear_getenv("EARGM_NODES");
    if (nnodes != NULL)
    {
        str_cut_list(nnodes, &nodes, &num_eargm_nodes, ",");
    }
    /* Remove unnecessary islands from ear_conf */
    remove_extra_islands(conf, e_def);
    print_cluster_conf(conf);

    if (verbose_arg  >= 0) verb_level=verbose_arg;


    my_port = e_def->port;
    eargm_id = e_def->id;
    default_cluster_powercap = e_def->power;
    cluster_powercap_period = conf->eargm.t1_power;
    period_t1 = conf->eargm.t1;
    period_t2 = conf->eargm.t2;
    use_aggregation = conf->eargm.use_aggregation;
    energy_budget = e_def->energy;
    units = conf->eargm.units;
    policy = conf->eargm.policy;
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
    if (energy_budget == -1) { //EARGM energy automatically calculated
        energy_budget = cluster_get_min_power(conf, ENERGY_TYPE)*conf->eargm.t2;
        energy_budget /= divisor;
        e_def->energy = energy_budget;
        printf("units %d, energy_budget: %ld\n", divisor, energy_budget);
    }
    //default_cluster_power=default_cluster_power*divisor;
}

void reopen_log()
{
    log_close(fd_my_log);
    create_log(my_cluster_conf.install.dir_temp, "eargmd", fd_my_log);
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
    verbose(0, "Usage: %s [options]", app);
    verbose(0, "--help\t\t Displays this message");
    verbose(0, "--verbose [level]\t\t Sets the level of verbosity");
    verbose(0, "--conf-path [/path/to/ear.conf]\t\t Overwrites the ear.conf to be used by EARGM");
    verbose(0, "--nodes [host1,host2]\t\t Specifies which nodes the EARGM will monitor.");
    verbose(0, "--powercap [limit]\t\t Sets the total power budget for the EARGM to distribute");
    verbose(0, "--powercap-policy [pol]\t\t Selects the distribution policy [hard, soft, monitor]");
    verbose(0, "--powercap-period [seconds]\t\t Time between checks of power status");
    verbose(0, "--suspend-perc [90]\t\t Percentage of power that triggers the suspend_action");
    verbose(0, "--suspend-action [/path/to/action]\t\t Program to be executed when suspend_perc is reached (once)");
    verbose(0, "--resume-perc [60]\t\t Percentage of power that triggers the suspend_action");
    verbose(0, "--resume-action [/path/to/action]\t\t Program to be executed when suspend_perc is reached (once)");

#if 0
    verbose(0, "This program controls the energy consumed in a period T2 seconds defined in $EAR_ETC/ear/ear.conf file");
    verbose(0, "energy is checked every T1 seconds interval");
    verbose(0, "Global manager can be configured in active or passive mode. Automatic actions are taken in active mode (defined in ear.conf file)");
#endif

    exit(0);

}

typedef struct eargm_preconf {
    char *hosts;
    int32_t powercap_period;
    uint32_t powercap;
    int32_t powercap_policy;
    int32_t suspend_perc;
    char *suspend_action;
    int32_t resume_perc;
    char *resume_action;
} eargm_preconf_t;


void update_eargm_with_preconf(cluster_conf_t *conf, eargm_preconf_t *preconf)
{

    /* Set energy budget to 0 for this use case */
    energy_budget = 0;

    if (preconf->powercap_policy >= 0) conf->eargm.powercap_mode = preconf->powercap_policy;
    if (preconf->powercap > 0) default_cluster_powercap = preconf->powercap;
    if (preconf->powercap_period > 0) cluster_powercap_period = preconf->powercap_period;

    verbose(VGM, "Final powercap mode %lu. Final powercap %lu, final powercap period %u", 
            conf->eargm.powercap_mode, default_cluster_powercap, cluster_powercap_period);

    if (preconf->hosts != NULL) {
        char *expanded_hosts = NULL;
        expand_list_alloc(preconf->hosts, &expanded_hosts);
        str_cut_list(expanded_hosts, &nodes, &num_eargm_nodes, ",");
        verbose(VGM, "Set new nodelist to contact");
    }

    if (preconf->suspend_perc > 0) conf->eargm.defcon_power_limit = preconf->suspend_perc;
    if (preconf->resume_perc  > 0) conf->eargm.defcon_power_lower = preconf->resume_perc;

    verbose(VGM, "Final suspend percentage %ld%%. Final resume percentage %ld%%.", 
            conf->eargm.defcon_power_limit, conf->eargm.defcon_power_lower);

    if (preconf->suspend_action != NULL)
        strncpy(conf->eargm.powercap_limit_action, preconf->suspend_action, GENERIC_NAME);
    if (preconf->resume_action != NULL) 
        strncpy(conf->eargm.powercap_lower_action, preconf->resume_action, GENERIC_NAME);

}

bool parse_args(int32_t argc, char *argv[], eargm_preconf_t *preconf)
{
    bool read = false;
    int32_t c = 0;

    //set it to -1 so we know if it has been read or not
    preconf->powercap_policy = -1;

    static struct option long_options[] = {
        { "help",            no_argument,       0, 'h' },
        { "verbose",         required_argument, 0, 'v' },
        { "nodes",           required_argument, 0, 'n' },
        { "conf-path",       required_argument, 0, 'c' },
        { "powercap",        required_argument, 0, 'p' },
        { "powercap-period", required_argument, 0, 'D' },
        { "powercap-policy", required_argument, 0, 'P' },
        { "suspend-perc",    required_argument, 0, 's' },
        { "suspend-action",  required_argument, 0, 'S' },
        { "resume-perc",     required_argument, 0, 'r' },
        { "resume-action",   required_argument, 0, 'R' },
    };

    while (1) {
        c = getopt_long(argc, argv, "hn:v:c:p:P:D:s:S:r:R:", long_options, NULL);
        if (c == -1) break;
        switch(c)
        {
            case 'h':
                usage(argv[0]);
                break;
            case 'v':
                verbose_arg = atoi(optarg);
                break;
            case 'n':
                preconf->hosts = optarg;
                read = true;
                break;
            case 'c':
                strncpy(my_ear_conf_path, optarg, sizeof(my_ear_conf_path));
                read = true;
                break;
            case 'p':
                preconf->powercap = atoi(optarg);
                read = true;
                break;
            case 'P':
                if (!strcasecmp(optarg, "MONITORING")) {
                    preconf->powercap_policy = MONITOR;
                } else if (!strcasecmp(optarg, "HARD")) {
                    preconf->powercap_policy = HARD_POWERCAP;
                } else if (!strcasecmp(optarg, "SOFT")) {
                    preconf->powercap_policy = SOFT_POWERCAP;
                }
                read = true;
                break;
            case 'D':
                preconf->powercap_period = atoi(optarg);
                break;
            case 's':
                preconf->suspend_perc = atof(optarg);
                read = true;
                break;
            case 'S':
                preconf->suspend_action = optarg;
                read = true;
                break;
            case 'r':
                preconf->resume_perc = atof(optarg);
                read = true;
                break;
            case 'R':
                preconf->resume_action = optarg;
                read = true;
                break;
        }
    }

    return read;
}


int main(int argc, char *argv[])
{
    sigset_t set;
    int ret;
    ulong result;
    int resulti;
    gm_warning_t my_warning;
    unsigned int new_actions;
    risk_t current_risk;
    eargm_preconf_t preconf = { 0 };
    bool preconf_read = false;

    preconf_read = parse_args(argc, argv, &preconf);

#if RUN_AS_ROOT
    if (getuid() != 0)
    {
        fprintf(stderr, "EARGM error: It must be executed as root. Exiting...\n");
        exit(0);
    }
#endif

    gethostname(host, sizeof(host));
    strtok(host, ".");

    // We read the cluster configuration and sets default values in the shared memory
    if (strlen(my_ear_conf_path) < 6) {
        if (get_ear_conf_path(my_ear_conf_path)==EAR_ERROR){
            error("Error opening ear.conf file, not available at regular paths ($EAR_ETC/ear/ear.conf)");
            exit(0);
        }
    }
#if SYSLOG_MSG
    openlog("eargm",LOG_PID|LOG_PERROR,LOG_DAEMON);
#endif

    verbose(VGM,"Using %s as EARGM configuration file",my_ear_conf_path);
    if (read_cluster_conf(my_ear_conf_path,&my_cluster_conf)!=EAR_SUCCESS){
        error(" Error reading cluster configuration");
        exit(0);
    }
    else{
        //print_cluster_conf(&my_cluster_conf);
        print_eargm_conf(&my_cluster_conf.eargm);
    }

    create_tmp(my_cluster_conf.install.dir_temp);

    if (my_cluster_conf.eargm.use_log){
        create_log(my_cluster_conf.install.dir_temp, "eargmd", fd_my_log);
    }
    TIMESTAMP_SET_EN(my_cluster_conf.eargm.use_log);

    /* Mark that the port to be used by the EARGM is the one in the first definition if we are using
     * CLI arguments for a local EARGM. */
    use_first_eargm_if_failed = preconf_read;
    update_eargm_configuration(&my_cluster_conf);

    if (preconf_read) {
        update_eargm_with_preconf(&my_cluster_conf, &preconf);
        verbose(VGM, "Read optional arguments, reprinting the EARGM configuration");
        print_eargm_conf(&my_cluster_conf.eargm);
    }


    state_t report_ret = report_load(my_cluster_conf.install.dir_plug, my_cluster_conf.eargm.plugins);
    if (report_ret != EAR_SUCCESS) {
        error("Error loading report plugins");
    }
    report_ret = report_init(NULL, &my_cluster_conf);
    if (report_ret != EAR_SUCCESS) {
        error("Error initializing report plugins");
    }
    switch (policy){
        case MAXENERGY:
            verbose(VGM,"MAXENERGY policy configured with limit %lu %s",energy_budget,unit_name);
            break;
        default: break;
    }	

    if ((period_t1<=0) || (period_t2<=0) ) usage(argv[0]);
    cluster_powercap_init(&my_cluster_conf);

    aggregate_samples=period_t2/period_t1;
    if ((period_t2%period_t1)!=0){
        warning("warning period_t2 is not multiple of period_t1");
        aggregate_samples++;
    }

    energy_consumed=malloc(sizeof(ulong)*aggregate_samples);


#if PRINT_ONLY
    exit(0);
#endif

    catch_signals();

    /* This thread accepts external commands */
    if ((ret=pthread_create(&eargm_server_api_th, NULL, eargm_server_api, NULL))){
        errno=ret;
        error("error creating eargm_server for external api %s",strerror(errno));
    }

    if (e_def->num_subs > 0) { //if it has subnodes, we need a META-EARGM
        if ((ret = pthread_create(&meta_eargm_th, NULL, meta_eargm, (void *)&my_cluster_conf))) {
            errno=ret;
            verbose(VGM,"error creating eargm_server for external api %s",strerror(errno));
        }
    } else { verbose(VGM,"EARGM node %s is not META-EARGM", host); }


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
    verbose(VGM,"db_select_acum_energy inicial %lu%s",result,unit_energy);
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
        if (energy_budget != 0) {
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
                                ear_cluster_restore_conf(&my_cluster_conf);
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
                                ear_cluster_set_risk(&my_cluster_conf, current_risk, MAXENERGY);
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

                if (report_misc(NULL, EARGM_WARNINGS, (const char *)&my_warning, 1) != EAR_SUCCESS) {
                    verbose(VGM,"EARGM[%d]: report EARGM_RECORD FAILED", getpid());
                }

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
    }

#if SYSLOG_MSG
    closelog();
#endif

    return 0;
}




