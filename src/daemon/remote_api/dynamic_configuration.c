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

//#define SHOW_DEBUGS 1
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include <common/config.h>
#include <common/states.h>
#include <common/types/job.h>
#include <daemon/log_eard.h>
#include <common/system/symplug.h>
#include <common/system/execute.h>
#include <common/output/verbose.h>
#include <common/messaging/msg_conf.h>
#include <common/messaging/msg_internals.h>
#include <common/types/configuration/cluster_conf.h>

#include <management/cpufreq/frequency.h>

#include <daemon/remote_api/eard_rapi.h>
#include <daemon/remote_api/eard_server_api.h>
#include <daemon/remote_api/dyn_conf_theading.h>
#include <daemon/power_monitor.h>
#include <daemon/powercap/powercap_status_conf.h>
#include <daemon/powercap/powercap.h>
#include <daemon/shared_configuration.h>
#include <report/report.h>

//#define SLURM_FAKE_ERROR 1

extern pthread_barrier_t setup_barrier;

/* Defined in eard.c */
extern report_id_t rid;


extern int eard_must_exit;
static int my_ip;
extern int *ips;
extern int self_id;
extern int max_context_created;
extern volatile int init_ips_ready;
extern unsigned long eard_max_freq;
extern policy_conf_t default_policy_context;
extern cluster_conf_t my_cluster_conf;
extern my_node_conf_t *my_node_conf;
extern my_node_conf_t my_original_node_conf;
extern powermon_app_t *current_ear_app[MAX_NESTED_LEVELS];
extern topology_t node_desc;


int eards_remote_socket, eards_client;
int last_command = -1;
int last_dist = -1;
int last_command_time = -1;
int node_found = EAR_ERROR;
struct sockaddr_in eards_remote_client;
char *my_tmp;
static ulong *f_list;
static uint num_f;
pthread_t act_conn_th;


/* New to manage risk */
typedef struct eard_policy_symbols {
    state_t (*set_risk) (policy_conf_t *ref_pol,policy_conf_t *node_pol,ulong risk,ulong opt_target,ulong cfreq,ulong *nfreq,ulong *f_list,uint nump);
} eard_polsym_t;

#define MAX_HISTORIC_COMM 50
static request_t list_last_commands[MAX_HISTORIC_COMM];
static int current_pos_last_comm = 0;

static eard_polsym_t *polsyms_fun;
//static void    *polsyms_obj = NULL;
const int       polsyms_n = 1;
const char     *polsyms_nam[] = {"policy_set_risk"};
/* End new section */

static char *TH_NAME = "RemoteAPI";
static ehandler_t my_eh_rapi;

void print_f_list(uint p_states, ulong *freql) {
    int i;
    for (i = 0; i < p_states; i++) {
        verbose(VCONF + 2, "Freq %u= %lu", i, freql[i]);
    }
}

uint is_valid_freq(ulong f, uint p_states, ulong *f_list) {
    int i = 0;

    while (i < p_states) {
        if (f_list[i] == f) {
            return 1;
        }
        i++;
    }
    return 0;

}

ulong lower_valid_freq(ulong f, uint p_states, ulong *f_list) {
    uint i = 0, found = 0;
    while (!found && (i < p_states)) {
        if (f_list[i] < f) found = 1;
        else i++;
    }
    if (found) return f_list[i];
    else return f_list[default_policy_context.p_state];

}

static void DC_my_sigusr1(int signun) {
    verbose(VCONF, "thread %u receives sigusr1", (uint) pthread_self());
    if (signun == SIGSEGV){
      verbose(0, "Remote API server thread receives SIGSEGV. Stack....");
      print_stack(verb_channel);
    }
    close_server_socket(eards_remote_socket);
    pthread_exit(0);
}

static void DC_set_sigusr1() {
    sigset_t set;
    struct sigaction sa;
    sigfillset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGALRM);
    pthread_sigmask(SIG_SETMASK, &set, NULL); // unblocking SIGUSR1 for this thread
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = DC_my_sigusr1;
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) < 0)
        error(" doing sigaction of signal s=%d, %s\n", SIGUSR1, strerror(errno));
    if (sigaction(SIGSEGV, &sa, NULL) < 0)
        error(" doing sigaction of signal s=%d, %s\n", SIGSEGV, strerror(errno));

}

ulong max_dyn_freq() {
  int i;
  ulong max = 0;
  for (i=0; i <= max_context_created;i++){
    if (current_ear_app[i]->settings->max_freq > max) max = current_ear_app[i]->settings->max_freq;
  }
  return max;

}

int dynconf_inc_th(uint p_id, ulong th) {
  double dth;
  int i;
  settings_conf_t *dyn_conf;
  resched_t *resched_conf;
  verbose(VCONF,"Increasing th by  %lu",th);
  dth=(double)th/100.0;
  for (i=0; i <= max_context_created;i++){
    if (current_ear_app[i] != NULL){
      dyn_conf = current_ear_app[i]->settings;
      resched_conf = current_ear_app[i]->resched;
      if (dyn_conf->policy == p_id){
        if (((dyn_conf->settings[0] + dth) > 0 ) && ((dyn_conf->settings[0] + dth) <=1.0) ){
          dyn_conf->settings[0] = dyn_conf->settings[0] + dth;
          resched_conf->force_rescheduling = 1;
        }
      }
    }
  }

    powermon_inc_th(p_id, dth);
    return EAR_SUCCESS;

}


int dynconf_max_freq(ulong max_freq) 
{
  int i;
  settings_conf_t *dyn_conf;
  resched_t *resched_conf;
  ulong new_max = 0;

  if (is_valid_freq(max_freq, num_f, f_list)) {
    new_max = max_freq;
  }else{
    new_max = lower_valid_freq(max_freq, num_f, f_list);
  }
  verbose(VCONF, "Setting max freq to %lu/%lu", max_freq,new_max);
  if (new_max == 0) return EAR_ERROR;
  for (i=0; i <= max_context_created;i++){
    if (current_ear_app[i] != NULL){
      dyn_conf = current_ear_app[i]->settings;
      resched_conf = current_ear_app[i]->resched;

      dyn_conf->max_freq = new_max;
      resched_conf->force_rescheduling = 1;
    }
  }
  powermon_new_max_freq(max_freq);
	return EAR_SUCCESS;
}

int dynconf_def_freq(uint p_id, ulong def) {
  int i;
  settings_conf_t *dyn_conf;
  resched_t *resched_conf;
  ulong new_def;
  if (is_valid_freq(def, num_f, f_list)) {
    new_def = def;
  }else{
    new_def = lower_valid_freq(def, num_f, f_list);
  }
  verbose(VCONF, "Setting default freq for pid %u to %lu/%lu", p_id, def,new_def);
  for (i=0; i <= max_context_created;i++){
      if (current_ear_app[i] != NULL){
        dyn_conf = current_ear_app[i]->settings;
        resched_conf = current_ear_app[i]->resched;

        if (dyn_conf->policy == p_id) {
          dyn_conf->def_freq = new_def;
          dyn_conf->def_p_state = frequency_closest_pstate(dyn_conf->def_freq);
          resched_conf->force_rescheduling = 1;
        }
      }
  }
  powermon_new_def_freq(p_id, def);
  return EAR_SUCCESS;


}

int dynconf_set_freq(ulong freq) {
  int i;
  ulong new_freq;
  settings_conf_t *dyn_conf;
  resched_t *resched_conf;
  if (is_valid_freq(freq, num_f, f_list))
  {
    new_freq = freq;
  }else{
    new_freq = lower_valid_freq(freq, num_f, f_list);
  }

  verbose(VCONF, "Setting freq to %lu/%lu", freq,new_freq);
  for (i=0; i <= max_context_created;i++){
      if (current_ear_app[i] != NULL){
        dyn_conf = current_ear_app[i]->settings;
        resched_conf = current_ear_app[i]->resched;
        dyn_conf->max_freq = new_freq;
        dyn_conf->def_freq = new_freq;
        dyn_conf->def_p_state = frequency_closest_pstate(dyn_conf->def_freq);
        resched_conf->force_rescheduling = 1;
      }
  }
  powermon_set_freq(new_freq);
  return EAR_SUCCESS;


}

int dyncon_restore_conf() {
  int pid;
  int i;
  settings_conf_t *dyn_conf;
  resched_t *resched_conf;
  policy_conf_t *my_policy;
  debug("dyncon_restore_conf");
  /* We copy the original configuration */
  copy_my_node_conf(my_node_conf,&my_original_node_conf);
  print_my_node_conf(my_node_conf);
  for (i=0; i <= max_context_created;i++){
      if (current_ear_app[i] != NULL){
        dyn_conf = current_ear_app[i]->settings;
        resched_conf = current_ear_app[i]->resched;
        pid = dyn_conf->policy;
        my_policy = get_my_policy_conf(my_node_conf,pid);
        if (my_policy == NULL){
          error("Policy %d not detected for job %lu/%lu",pid,current_ear_app[i]->app.job.id,current_ear_app[i]->app.job.step_id);
          continue;
        }
        dyn_conf->max_freq = frequency_pstate_to_freq(my_node_conf->max_pstate);
        dyn_conf->def_freq = frequency_pstate_to_freq(my_policy->p_state);
        dyn_conf->def_p_state = my_policy->p_state;
        memcpy(dyn_conf->settings, my_policy->settings, sizeof(double)*MAX_POLICY_SETTINGS);
        resched_conf->force_rescheduling=1;
      }
  }
  return EAR_SUCCESS;

}

int dynconf_set_def_pstate(uint p_states,uint p_id)
{
  int i;
  settings_conf_t *dyn_conf;
  resched_t *resched_conf;

  if (p_states > frequency_get_num_pstates()) return EAR_ERROR;
  if (p_id > my_cluster_conf.num_policies) return EAR_ERROR;
  my_node_conf->policies[p_id].p_state = p_states;
  for (i=0;i<= max_context_created;i++){
      if (current_ear_app[i] != NULL){
        dyn_conf = current_ear_app[i]->settings;
        resched_conf = current_ear_app[i]->resched;
        if (dyn_conf->policy == p_id){
          dyn_conf->def_p_state = p_states;
          dyn_conf->def_freq = frequency_pstate_to_freq(p_states);
          resched_conf->force_rescheduling = 1;
        }
      }
  }
  return EAR_SUCCESS;


}
int dynconf_set_max_pstate(uint p_states)
{
  int i;
  settings_conf_t *dyn_conf;
  resched_t *resched_conf;
  ulong new_max_freq;
  if (p_states > frequency_get_num_pstates()) return EAR_ERROR;
  new_max_freq = frequency_pstate_to_freq(p_states);
  /* Update dynamic info */
  for (i=0;i<=max_context_created;i++){
      if (current_ear_app[i] != NULL){
        dyn_conf = current_ear_app[i]->settings;
        resched_conf = current_ear_app[i]->resched;
        dyn_conf->max_freq = new_max_freq;
        resched_conf->force_rescheduling = 1;
      }
  }
  powermon_new_max_freq(new_max_freq);
  return EAR_SUCCESS;

}


int dynconf_red_pstates(uint p_states) {
  int i;
  settings_conf_t *dyn_conf;
  resched_t *resched_conf;
  uint def_pstate, max_pstate;
  ulong new_def_freq, new_max_freq;
  int variation=(int)p_states;
  for (i=0;i<=max_context_created;i++){
      if (current_ear_app[i] != NULL){
        dyn_conf = current_ear_app[i]->settings;
        resched_conf = current_ear_app[i]->resched;
        def_pstate = frequency_closest_pstate(dyn_conf->def_freq);
        max_pstate = frequency_closest_pstate(dyn_conf->max_freq);
        /* Reducing means incresing in the vector of pstates */
        def_pstate = def_pstate + variation;
        max_pstate = max_pstate + variation;

        new_def_freq = frequency_pstate_to_freq(def_pstate);
        new_max_freq = frequency_pstate_to_freq(max_pstate);

        /* reducing the maximum freq in N p_states */
        dyn_conf->max_freq = new_max_freq;
        dyn_conf->def_freq = new_def_freq;
        resched_conf->force_rescheduling = 1;
    }
  }

  /* We must update my_node_info */

  for (i = 0; i < my_node_conf->num_policies; i++) {
    my_node_conf->policies[i].p_state = my_node_conf->policies[i].p_state + variation;
  }
  powermon_new_max_freq(new_max_freq);
  return EAR_SUCCESS;

}

int dynconf_set_th(ulong p_id, ulong th) {
  double dth;
  int i;
  settings_conf_t *dyn_conf;
  resched_t *resched_conf;

  dth=(double)th/100.0;
  if ((dth <0) || (dth > 1.0)) return EAR_ERROR;
  for (i=0;i<=max_context_created;i++){
    if (current_ear_app[i] != NULL){
      dyn_conf = current_ear_app[i]->settings;
      resched_conf = current_ear_app[i]->resched;
      if (dyn_conf->policy == p_id){
        /*currently the first setting is changed, we could pass the setting id by parameter later on*/
        dyn_conf->settings[0] = dth;
        resched_conf->force_rescheduling=1; 
      }
    }
  }
  powermon_set_th(p_id, dth);
  return EAR_SUCCESS;

}


int dyncon_set_policy(new_policy_cont_t *p)
{
    state_t s = EAR_SUCCESS;
    int pid;
    pid = policy_name_to_nodeid(p->name,my_node_conf);
    if (pid == EAR_ERROR){ 
        error("Policy %s not found",p->name);
        return EAR_ERROR;
    }
    debug("Policy %s found at id %d",p->name,pid);
    if (frequency_is_valid_frequency(p->def_freq)){
        memcpy(my_cluster_conf.power_policies[pid].settings,p->settings,sizeof(double)*MAX_POLICY_SETTINGS);
        my_cluster_conf.power_policies[pid].def_freq = p->def_freq;
        my_cluster_conf.power_policies[pid].p_state  = frequency_pstate_to_freq(p->def_freq);
    }else{
        error("Invalid frequency %lu for policy %s ",p->def_freq,p->name);
        s = EAR_ERROR;
    }
    return s;

}

/** This function returns the current application status */
void dyncon_get_app_status(int fd, request_t *command) 
{
    app_status_t *status;
    int local_status;
    local_status = powermon_get_num_applications(command->req == EAR_RC_APP_MASTER_STATUS);
    verbose(VRAPI,"We have %d local apps in app_status",local_status);

    int num_status = propagate_app_status(command, my_cluster_conf.eard.port, &status, local_status);
    unsigned long return_status = num_status;
    verbose(VRAPI,"return_app_tatus %lu status=%p",return_status,status);
    if (num_status < 1) {
        error("Panic propagate_app_status returns less than 1 status");
        return_status = 0;
        _write(fd, &return_status, sizeof(return_status));
        return;
    }
    powermon_get_app_status(&status[num_status - local_status], local_status, command->req == EAR_RC_APP_MASTER_STATUS);

    //if no job is present on the current node or we requested the master node and this one isn't, we free its data
		if (status[num_status - local_status].job_id == 0 )
    {
        verbose(VRAPI,"Only default context created, not app_status returned, removing %d items", local_status);
        num_status -= local_status;
        status = realloc(status, sizeof(app_status_t)*num_status);
    }
		#if 0
    if (status[num_status - 1].job_id < 0 || (command->req == EAR_RC_APP_MASTER_STATUS && status[num_status - 1].master_rank != 0))
    {
        num_status--;
        status = realloc(status, sizeof(app_status_t)*num_status);
    }
		#endif

    send_data(fd, sizeof(app_status_t) * num_status, (char *)status, EAR_TYPE_APP_STATUS);
    verbose(VRAPI,"Returning from dyncon_get_app_status %d app_status sent",num_status);

    free(status);
    debug("app_status released");
}

/* This function will propagate the status command and will return the list of node failures */
void dyncon_get_status(int fd, request_t *command) {
    status_t *status;
    int num_status = propagate_status(command, my_cluster_conf.eard.port, &status);
    unsigned long return_status = num_status;
    debug("return_status %lu status=%p",return_status,status);
    if (num_status < 1) {
        error("Panic propagate_status returns less than 1 status");
        return_status = 0;
        _write(fd, &return_status, sizeof(return_status));
        return;
    }
    powermon_get_status(&status[num_status - 1]);
    send_data(fd, sizeof(status_t) * num_status, (char *)status, EAR_TYPE_STATUS);
    debug("Returning from dyncon_get_status");
    free(status);
    debug("status released");
}

int dyncon_filter_opt(powercap_opt_t *opt)
{
    int i;
    for (i = 0; i < opt->num_greedy; i++)
    {
        debug("checking our ip (%d) vs list (%d)", my_ip, opt->greedy_nodes[i]);
        if (opt->greedy_nodes[i] == my_ip) return i;
        debug("dyn_conf: greedy node found in position %d", i);
    }
    return -1;
}
void dyncon_power_management(int fd, request_t *command)
{
    int event;
    unsigned long value;
    int id;
    switch (command->req){
        case EAR_RC_RED_POWER:
            /* command->my_req.pc.type==RELATIVE not implemented */
            event = RED_POWERCAP;
            value = command->my_req.pc.limit;
            if (command->my_req.pc.type == ABSOLUTE){
                verbose(VRAPI,"Reduce powercap default in %lu watts",command->my_req.pc.limit);
                powercap_reduce_def_power(command->my_req.pc.limit);
            }
            if (command->my_req.pc.type == RELATIVE){
                verbose(VRAPI,"Error, reduce relative powercap not implemented");
            }
            break;	
        case EAR_RC_SET_POWER:
            event = SET_POWERCAP;
            value = command->my_req.pc.limit;
            verbose(VRAPI,"We must set the power to %lu watts",command->my_req.pc.limit);
            powercap_set_powercap(command->my_req.pc.limit);
            break;
        case EAR_RC_INC_POWER:
            event = INC_POWERCAP;
            value = command->my_req.pc.limit;
            if (command->my_req.pc.type == ABSOLUTE){
                verbose(VRAPI,"Increase powercap default in %lu watts",command->my_req.pc.limit);
                powercap_increase_def_power(command->my_req.pc.limit);
            }
            if (command->my_req.pc.type == RELATIVE){
                verbose(VRAPI,"Error, increase relative powercap not implemented");
            }
            break;	
        case EAR_RC_DEF_POWERCAP:
            event = RESET_POWERCAP;
            powercap_reset_default_power();
            value = 0;
            break;
        case EAR_RC_SET_POWERCAP_OPT:
            verbose(VRAPI,"Set powercap options received");
            id = dyncon_filter_opt(&command->my_req.pc_opt);
            debug("returned filter with id %d", id);
            powercap_set_opt(&command->my_req.pc_opt, id);
            free(command->my_req.pc_opt.greedy_nodes);
            free(command->my_req.pc_opt.extra_power);
            event = -1;
            break;
        default:
            verbose(VRAPI,"power command received not supported");
            event = -1;
            break;
    }
    if (event > 0) //POWERCAP_OPT has its own event
        powermon_report_event((uint)event, value);
}

void update_current_settings(policy_conf_t *cpolicy_settings)
{
  int i;
  settings_conf_t *dyn_conf;
  resched_t *resched_conf;

  for (i=0;i<=max_context_created;i++){
    if (current_ear_app[i] != NULL){
      dyn_conf = current_ear_app[i]->settings;
      resched_conf = current_ear_app[i]->resched;
      if (dyn_conf->policy == cpolicy_settings->policy){
        verbose(VRAPI,"current policy options: def freq %lu setting[0]=%.2lf def_pstate %u",dyn_conf->def_freq,dyn_conf->settings[0],dyn_conf->def_p_state);
        dyn_conf->def_freq = frequency_pstate_to_freq(cpolicy_settings->p_state);
        memcpy(dyn_conf->settings,cpolicy_settings->settings,sizeof(double)*MAX_POLICY_SETTINGS);
        dyn_conf->def_p_state = cpolicy_settings->p_state;
        resched_conf->force_rescheduling = 1;
        verbose(VRAPI,"new policy options: def freq %lu setting[0]=%.2lf def_pstate %u",dyn_conf->def_freq,dyn_conf->settings[0],dyn_conf->def_p_state);
      }
    }
  }

}

void dyncon_release_idle_power(int fd, request_t *command)
{
    pc_release_data_t rel_data;
    int return_status;

    debug("propagating release_idle_power");
    return_status = propagate_release_idle(command, my_cluster_conf.eard.port, (pc_release_data_t *)&rel_data);

    if (return_status < 1) error("dyncon_release_idle power and return status < 1");

    debug("releasing idle power");
    powercap_release_idle_power(&rel_data);

    send_data(fd, sizeof(pc_release_data_t), (char *)&rel_data, EAR_TYPE_RELEASED);
    debug("returning from release_idle_power");

}


void process_pmgt_status(powercap_status_t *my_status, pmgt_status_t *p_status)
{
    switch(p_status->status)
    {
        case PC_STATUS_OK:
            if (p_status->extra <= 0) break;
            p_status->requested = 0;
        case PC_STATUS_GREEDY:
            /* Memory management */
            if (my_status->num_greedy < 1) {
                my_status->greedy_nodes=NULL;
                my_status->greedy_data=NULL;
            }
            my_status->num_greedy++; 
            if (my_status->num_greedy < 1) break;
            my_status->greedy_nodes = realloc(my_status->greedy_nodes, sizeof(int)*my_status->num_greedy);
            my_status->greedy_data  = realloc(my_status->greedy_data, sizeof(greedy_bytes_t)*my_status->num_greedy);
            /* Data management */
            my_status->greedy_nodes[my_status->num_greedy - 1] = my_ip; 
            my_status->greedy_data[my_status->num_greedy - 1].requested = p_status->requested;
            my_status->greedy_data[my_status->num_greedy - 1].extra_power = p_status->extra;
            my_status->greedy_data[my_status->num_greedy - 1].stress = p_status->stress;
            break;
        default:
            break;
    }
}

void dyncon_get_powerstatus(int fd, request_t *command)
{
    powercap_status_t *status;
    pmgt_status_t p_status;
    int return_status;
    char *status_data;
    return_status = propagate_powercap_status(command, my_cluster_conf.eard.port, (powercap_status_t **)&status_data);
    status = mem_alloc_powercap_status(status_data);
    free(status_data);

    if (return_status < 1) 
    {
        //error
        error("dyncon_get_powerstatus and return status < 1 ");
    }
    debug("return_status %d status=%p", return_status, status);

    powercap_get_status(&status[return_status - 1], &p_status, command->my_req.release_power);
    process_pmgt_status(&status[return_status - 1], &p_status);

    status_data = mem_alloc_char_powercap_status(status);
    send_data(fd, sizeof(powercap_status_t) * return_status + ((sizeof(uint)*2 + sizeof(int)) * (status->num_greedy)), status_data, EAR_TYPE_POWER_STATUS);

    free(status_data);
    debug("Returning from dyncon_get_powerstatus");
    free(status);
    debug("powerstatus released");
}

void dyncon_get_power(int fd, request_t *command)
{
    power_check_t *power;
    uint64_t curr_power = 0;
    int return_status;
    return_status = propagate_get_power(command, my_cluster_conf.eard.port, (power_check_t **)&power);

    if (return_status < 1) 
    {
        //error
        error("dyncon_get_power and return status < 1 ");
    }
    debug("return_status %d power=%lu", return_status, *power);

    //powermon_get_power retrieves the actual reading, whereas powermon_current_power returns
    //the corrected power reading (making sure it is within the preset values and not a nonsense number),
    //which is what we want in this case
    //powermon_get_power(&curr_power);
    curr_power = (uint64_t) powermon_current_power();
    power->power += curr_power;
    power->num_nodes++;
    debug("return_status %d power=%lu current_power=%lu", return_status, power->power, curr_power);

    send_data(fd, sizeof(power_check_t), (char *)power, EAR_TYPE_POWER_CHECK);

    debug("Returning from dyncon_get_powerstatus");
    free(power);
    debug("powerstatus released");
}

void dyncon_set_risk(int fd, request_t *command)
{
  int i;
  unsigned long new_max_freq,c_max,mfreq,node_max = 0;
  settings_conf_t *dyn_conf;
  resched_t *resched_conf;

	/* How that applies when having TAGS en policies ?: PENDING */

  c_max = frequency_pstate_to_freq(my_node_conf->max_pstate);
  mfreq = c_max;
  for (i=0;i<my_node_conf->num_policies;i++){
    if (polsyms_fun[i].set_risk != NULL){
      verbose(VRAPI,"Setting risk level for %s to %lu",my_node_conf->policies[i].name,(unsigned long)command->my_req.risk.level);
      polsyms_fun[i].set_risk(&my_original_node_conf.policies[i],&my_node_conf->policies[i],command->my_req.risk.level,command->my_req.risk.target,mfreq,&new_max_freq,f_list,num_f);
    }
    for (i=0;i<=max_context_created;i++){
      if (current_ear_app[i] != NULL){
        dyn_conf = current_ear_app[i]->settings;
        resched_conf = current_ear_app[i]->resched;
        if (dyn_conf->policy == i){
          verbose(VRAPI,"Current policy for job %lu/%lu is %d", current_ear_app[i]->app.job.id,current_ear_app[i]->app.job.step_id,i);
          update_current_settings(&my_node_conf->policies[i]);
          if (new_max_freq != dyn_conf->max_freq){
            dyn_conf->max_freq = new_max_freq;
            resched_conf->force_rescheduling = 1;
            if (node_max < new_max_freq) node_max = node_max;
          }
        }
      }
    }
  }
  my_node_conf->max_pstate = frequency_freq_to_pstate(node_max);
  powermon_new_max_freq(node_max);
  verbose(VRAPI,"New max frequency is %lu pstate=%lu ",node_max,my_node_conf->max_pstate);


}


void adap_new_job_req_to_app(application_t *req_app,new_job_req_t *new_job)
{
    char *sig_start;
    memcpy(req_app,new_job,sizeof(new_job_req_t));
    sig_start = (char *)req_app + sizeof(new_job_req_t);
    memset(sig_start,0,sizeof(application_t)-sizeof(new_job_req_t));
}

/* Functions to compute if replicated command */


static int is_new_command(request_t *comm)
{
		if (comm->req == EAR_RC_NEW_JOB || comm->req == EAR_RC_END_JOB || comm->req == EAR_RC_NEW_TASK || comm->req == EAR_RC_STATUS) return 1;
    // if ((comm->req == last_command) && (comm->time_code == last_command_time)) return 0;
    for (uint i=0; i < MAX_HISTORIC_COMM ; i++){
    //    if ((list_last_commands[i].req == comm->req ) && (list_last_commands[i].time_code == comm->time_code)) return 0;
        if (!memcmp(&list_last_commands[i], comm, sizeof(request_t))) return 0;
    }
    return 1;
}

static void new_command_received(request_t *comm)
{
    last_dist     = comm->node_dist;
    last_command  = comm->req;
    last_command_time = comm->time_code;
    memcpy(&list_last_commands[current_pos_last_comm], comm, sizeof(request_t));
    current_pos_last_comm = (current_pos_last_comm +1) % MAX_HISTORIC_COMM;
}

#if 0
static void log_msg_in_file(char *filename, char *txt)
{
    mode_t oldm = umask(0);
    int fd = open(filename,O_WRONLY|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (fd < 0) return;
    write(fd,txt,strlen(txt));
    close(fd);
    umask(oldm);
}
#endif

state_t process_remote_requests(int clientfd) {
    request_t command;
    uint req;
    long ack = EAR_SUCCESS;
	int num_contexts = 0;
    verbose(VRAPI, "connection received");
    memset(&command,0,sizeof(request_t));
    command.req = NO_COMMAND;
    req = (int) read_command(clientfd, &command);

    debug("Process remote request %d", req);

    if (req == EAR_SOCK_DISCONNECTED) return req;
    /* New job and end job are different */
    /* Is it necesary */
    if (!is_new_command(&command)) {
            verbose(VRAPI, "Recieved repeating command: %u", req);
            ack = EAR_IGNORE;
            send_answer(clientfd, &ack);
            return EAR_SUCCESS;
    }

    //new_job is a special case because ack means that shared files are created
    if (req != EAR_RC_NEW_JOB && req != EAR_RC_NEW_JOB_LIST) { 
        send_answer(clientfd, &ack); //send ack BEFORE processing the message to avoid delays
    }

    verbose(VRAPI,"New command accepted");
    new_command_received(&command);

    switch (req) {
        case EAR_RC_NEW_JOB:
        case EAR_RC_NEW_JOB_LIST:
            verbose(VRAPI, "*******************************************");
            verbose(VRAPI, "new_job command received %lu", command.my_req.new_job.job.id);
            application_t req_app;
			// check pending contexts by looking at their pids (if they no longer exists we finish the jobs)
			num_contexts = mark_contexts_to_finish_by_pid();
			//do the end_jobs for the marked jobs
			if (num_contexts) finish_pending_contexts(&my_eh_rapi);

			//begin the new_job
            adap_new_job_req_to_app(&req_app,&command.my_req.new_job);
            powermon_new_job(NULL, &my_eh_rapi, &req_app, 0, req == EAR_RC_NEW_JOB_LIST);
            send_answer(clientfd, &ack);
            break;
        case EAR_RC_END_JOB:
#if SLURM_FAKE_ERROR
			break;
#endif
        case EAR_RC_END_JOB_LIST:
            powermon_end_job(&my_eh_rapi, command.my_req.end_job.jid, command.my_req.end_job.sid, req == EAR_RC_END_JOB_LIST);
			// mark any pending context for that job if it's an SBATCH
		    num_contexts = mark_contexts_to_finish_by_jobid(command.my_req.end_job.jid, command.my_req.end_job.sid);
			// do the end_jobs for the marked jobs
			if (num_contexts) finish_pending_contexts(&my_eh_rapi);
#if SHOW_DEBUGS
			print_contexts_status();
#endif
            verbose(VRAPI, "end_job command received %lu", command.my_req.end_job.jid);
            verbose(VRAPI, "*******************************************");
            break;
        case EAR_RC_NEW_TASK:
            verbose(VRAPI, "NEW task received");
            powermon_new_task(&command.my_req.new_task);
            break;
        case EAR_RC_END_TASK:
            verbose(VRAPI, "END task received");
            //powermon_end_task(&command.my_req.end_task); //pending to implement
            break;
        case EAR_RC_MAX_FREQ:
            verbose(VRAPI, "max_freq command received %lu", command.my_req.ear_conf.max_freq);
            ack = dynconf_max_freq(command.my_req.ear_conf.max_freq);
            break;
        case EAR_RC_NEW_TH:
            verbose(VRAPI, "new_th command received %lu", command.my_req.ear_conf.th);
            ack = dynconf_set_th(command.my_req.ear_conf.p_id, command.my_req.ear_conf.th);
            break;
        case EAR_RC_INC_TH:
            verbose(VRAPI, "inc_th command received, %lu", command.my_req.ear_conf.th);
            ack = dynconf_inc_th(command.my_req.ear_conf.p_id, command.my_req.ear_conf.th);
            break;
        case EAR_RC_RED_PSTATE:
            verbose(VRAPI, "red_max_and_def_p_state command received");
            ack = dynconf_red_pstates(command.my_req.ear_conf.p_states);
            break;
        case EAR_RC_SET_FREQ:
            verbose(VRAPI, "set freq command received");
            ack = dynconf_set_freq(command.my_req.ear_conf.max_freq);
            break;
        case EAR_RC_DEF_FREQ:
            verbose(VRAPI, "set def freq command received");
            ack = dynconf_def_freq(command.my_req.ear_conf.p_id, command.my_req.ear_conf.max_freq);
            break;
        case EAR_RC_SET_DEF_PSTATE:
            verbose(VRAPI, "set def pstate command received");
            ack=dynconf_set_def_pstate(command.my_req.ear_conf.p_states,command.my_req.ear_conf.p_id);
            break;
        case EAR_RC_SET_MAX_PSTATE:
            verbose(VRAPI, "set max pstate command received");
            ack=dynconf_set_max_pstate(command.my_req.ear_conf.p_states);
            break;
        case EAR_RC_REST_CONF:
            verbose(VRAPI, "restore conf command received");
            ack = dyncon_restore_conf();
            break;
        case EAR_RC_SET_POLICY:
            verbose(VRAPI,"set policy received");
            ack = dyncon_set_policy(&command.my_req.pol_conf);
            break;
        case EAR_RC_PING:
            verbose(VRAPI + 1, "ping received");
            break;
        case EAR_RC_STATUS:
            verbose(VRAPI + 1, "Status received");
            dyncon_get_status(clientfd, &command);
            return EAR_SUCCESS;
            break;
        case EAR_RC_APP_NODE_STATUS:
        case EAR_RC_APP_MASTER_STATUS:
            verbose(VRAPI + 1, "App status received");
            dyncon_get_app_status(clientfd, &command);
            return EAR_SUCCESS;
            break;
        case EAR_RC_RED_POWER:
        case EAR_RC_SET_POWER:
        case EAR_RC_INC_POWER:
        case EAR_RC_SET_POWERCAP_OPT:
        case EAR_RC_DEF_POWERCAP:
            dyncon_power_management(clientfd, &command);
            break;
        case EAR_RC_GET_POWERCAP_STATUS:
            dyncon_get_powerstatus(clientfd, &command);
            return EAR_SUCCESS;
        case EAR_RC_RELEASE_IDLE:
            dyncon_release_idle_power(clientfd, &command);
            return EAR_SUCCESS;
        case EAR_RC_SET_RISK:
            verbose(VRAPI,"set risk command received");
            dyncon_set_risk(clientfd, &command);
            break;
        case EAR_RC_GET_POWER:
            dyncon_get_power(clientfd, &command);
            return EAR_SUCCESS;
        default:
            error("Invalid remote command %d\n",req);
            req = NO_COMMAND;
    }
    if (req != EAR_RC_PING && req != NO_COMMAND && node_found != EAR_ERROR)
    {
        verbose(VRAPI + 1, "command=%d propagated distance=%d", req, command.node_dist);
        propagate_req(&command, my_cluster_conf.eard.port);
    }
    return EAR_SUCCESS;
}

void policy_load()
{
    char basic_path[SZ_PATH];
    int i;
    polsyms_fun = (eard_polsym_t *)calloc(my_node_conf->num_policies,sizeof(eard_polsym_t));
    if (polsyms_fun == NULL){
        error("Allocating memory for policy functions in eard");
    }
    for (i=0; i<my_node_conf->num_policies;i++){
        sprintf(basic_path,"%s/eard_policies/%s_eard.so",my_cluster_conf.install.dir_plug,my_node_conf->policies[i].name);
        verbose(VRAPI,"Loading functions for policy %s",basic_path);
        symplug_open(basic_path, (void **)&polsyms_fun[i], polsyms_nam, polsyms_n);
    }
}


/*
 *	THREAD to process remote queries
 */

void *eard_dynamic_configuration(void *tmp)
{
    my_tmp = (char *) tmp;
    int ret;

    verbose(VRAPI, "[%ld] RemoteAPI thread UP", syscall(SYS_gettid));

    DC_set_sigusr1();

    if (pthread_setname_np(pthread_self(), TH_NAME)) {
        error("Setting name for %s thread %s", TH_NAME, strerror(errno));
    }
    debug("Initializing energy in main dyn_conf thread");
    if (init_power_monitoring(&my_eh_rapi, &node_desc) != EAR_SUCCESS) {
        error("Error initializing energy in %s thread", TH_NAME);
    }

    if (init_active_connections_list()!=EAR_SUCCESS){
        error("Error initializing remote connections data, remore requests, remote connections won't be accepted");
    }	
    if ((ret=pthread_create(&act_conn_th, NULL, process_remote_req_th, NULL))){
        error("error creating thread to process remore requests, remote connections won't be accepted");
    }

    num_f = frequency_get_num_pstates();
    f_list = frequency_get_freq_rank_list();
    print_f_list(num_f, f_list);
    verbose(VRAPI + 2, "We have %u valid p_states", num_f);
    verbose(VRAPI, "Init for ips");
    node_found = init_ips(&my_cluster_conf);
    if (node_found == EAR_ERROR) verbose(VRAPI, "Node not found in configuration file");
    verbose(VRAPI,"Init ips ready");


    verbose(VRAPI, "Creating socket for remote commands");
    eards_remote_socket = create_server_socket(my_cluster_conf.eard.port);
    if (eards_remote_socket < 0) {
        error("Error creating socket, exiting eard_dynamic_configuration thread\n");
        log_report_eard_init_error(&rid,RCONNECTOR_INIT_ERROR,eards_remote_socket);
        pthread_exit(0);
    }

    policy_load();

    /** IP init for powercap_status */
    if (init_ips_ready > 0) my_ip = ips[self_id];
    else my_ip = 0;

    verbose(VRAPI, "Dynamic configuration thread set up. Waiting for other threads...");
    pthread_barrier_wait(&setup_barrier);

    /*
     *	MAIN LOOP
     */
    do {
        verbose(VRAPI + 1, "waiting for remote commands");
        eards_client = wait_for_client(eards_remote_socket, &eards_remote_client);
        if (eards_client < 0) {
            error(" wait_for_client returns error\n");
            close_server_socket(eards_remote_socket);
            eards_remote_socket = create_server_socket(my_cluster_conf.eard.port);
            if (eards_remote_socket < 0) {
                eard_must_exit = 1;
                error("Panic, we cannot create socket for connection again,exiting");
            }
        } else {
            if (notify_new_connection(eards_client)!=EAR_SUCCESS){
                error("Notifying new remote connection for processing");
            }
        }
    } while (eard_must_exit == 0);
    warning("eard_dynamic_configuration exiting\n");
    close_ips();
    //ear_conf_shared_area_dispose(my_tmp);
    close_server_socket(eards_remote_socket);
    pthread_exit(0);

}

