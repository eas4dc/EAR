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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#define _GNU_SOURCE
#include <pthread.h>
#define SHOW_DEBUGS 1
#include <common/config.h>
#include <common/colors.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/system/execute.h>
#include <daemon/powercap/powercap_status_conf.h>
#include <common/system/monitor.h>
#include <management/cpufreq/frequency.h>

#define POWERCAP_MON 0



/* This version is a prototype and should be replaced by an INM plugin+GPU commands for powercap */

#define INM_ENABLE_CMD	"ipmitool raw 0x3a 0xc7 0x01"
#define INM_ENABLE_XCC_BRIGE "ipmitool raw 0x06 0x32 0 1"
#define INM_ENABLE_POLICY_CONTROL_CMD "ipmitool -b 0x00 -t 0x2c nm control enable global" 
#define INM_DISABLE_POLICY_CONTROL_CMD "ipmitool -b 0x00 -t 0x2c nm control disable global" 
#define INM_ENABLE_POWERCAP_POLICY_CMD "ipmitool -b 0x00 -t 0x2c nm policy add policy_id %u domain platform correction hard power %d  trig_lim 1000 stats 30 enable"
#define INM_ENABLE_POWERCAP_POLICY_CMD_SOFT "ipmitool -b 0x00 -t 0x2c nm policy add policy_id %u domain platform correction soft power %u trig_lim 1000 stats 5 enable"

#define INM_DISABLE_POWERCAP_POLICY_CMD "ipmitool -b 0x00 -t 0x2c nm policy remove policy_id %u"
#define INM_GET_POWERCAP_POLICY_CMD "NO_CMD"

/* To be used to reset the node configuration to default values in case of problems */
#define RESET_DEFAULT_CONF_POWERCAP "ipmitool -b 0 -t 0x2c raw 0x2E 0xDF 0x57 0x01 0x00 0x02"
static uint c_limit;
static uint policy_enabled=0;
static uint pc_on=0;
static ulong c_req_f;
int do_cmd(char *cmd)
{
  if (strcmp(cmd,"NO_CMD")==0) return 0;
  return 1;
}

state_t inm_disable_powercap_policy(uint pid)
{
  char cmd[1024];
  debug("INM:1-Disable policy");
  sprintf(cmd,INM_DISABLE_POWERCAP_POLICY_CMD,pid);
  if (do_cmd(cmd)){
  if (execute(cmd)!=EAR_SUCCESS){
    error("Error executing policy disable");
		return EAR_ERROR;
  }
  }
	return EAR_SUCCESS;
}
state_t inm_disable_powercap_policies()
{
	char cmd[1024];
  debug("INM:2-Disable policy control");
  sprintf(cmd,INM_DISABLE_POLICY_CONTROL_CMD);
  if (do_cmd(cmd)){
  if (execute(cmd)!=EAR_SUCCESS){
    debug("INM:Error disabling policy control");
		return EAR_ERROR;
  }
  }
	return EAR_SUCCESS;

}

state_t disable()
{
	state_t ret;
	/* Policies is still pending to manage */
	ret=inm_disable_powercap_policy(0);
	if (ret!=EAR_SUCCESS) return ret;
	return inm_disable_powercap_policies();
}

state_t inm_enable_powercap_policies()
{
	char cmd[1024];
  /* 3-Enable powercap policy control */
  sprintf(cmd,INM_ENABLE_POLICY_CONTROL_CMD);
  debug("INM:Enable INM policy control");
  if (do_cmd(cmd)){
  if (execute(cmd)!=EAR_SUCCESS){
    error("Error executing INM CMD Policy control");
    return EAR_ERROR;
  }
  }
	return EAR_SUCCESS;
}
state_t enable(suscription_t *sus)
{
	char cmd[1024];
	state_t ret;
  /* 1-Enable XCC-Bridge comm */
  debug("INM:Enable XCC-Bridge");
  sprintf(cmd,INM_ENABLE_XCC_BRIGE);
  if (do_cmd(cmd)){
  if (execute(cmd)!=EAR_SUCCESS){
    error("Error executing INM XCC-bridge");
    return EAR_ERROR;
  }
  }
  /* 2-Enable INM commands */
  debug("INM:Enable INM");
  sprintf(cmd,INM_ENABLE_CMD);
  if (do_cmd(cmd)){
  if (execute(cmd)!=EAR_SUCCESS){
    error("Error executing INM CMD enable");
    return EAR_ERROR;
  }
  }
  ret=inm_enable_powercap_policies();
	if (ret==EAR_SUCCESS) policy_enabled=1;
  return ret;
}

state_t set_powercap_value(uint pid,uint domain,uint limit,uint *cpu_util)
{
	char cmd[1024];
	if (!policy_enabled) return EAR_SUCCESS;
	debug("INM:inm_set_powercap_value policy %u limit %u",pid,limit);
	c_limit=limit;
	pc_on=1;
	sprintf(cmd,INM_ENABLE_POWERCAP_POLICY_CMD,pid,limit);
	debug(cmd);
	return execute(cmd);
}

state_t get_powercap_value(uint pid,uint *powercap)
{
	debug("INM. get_powercap_value");
	*powercap=c_limit;
	return EAR_SUCCESS;
}

uint is_powercap_policy_enabled(uint pid)
{
	return policy_enabled;
}

void print_powercap_value(int fd)
{
	dprintf(fd,"%u",c_limit);
}
void powercap_to_str(char *b)
{
	sprintf(b,"%u",c_limit);
}

void set_status(uint status)
{
	debug("INM. pending");
}
uint get_powercap_stragetgy()
{
	return PC_POWER;
}
void set_pc_mode(uint mode)
{
	debug("INM. pending");
}

void set_verb_channel(int fd)
{
	WARN_SET_FD(fd);
	VERB_SET_FD(fd);
	DEBUG_SET_FD(fd);
}
void set_app_req_freq(ulong *f)
{
	debug("INM:Requested application freq set to %lu",*f);
	c_req_f=*f;	
}

#define MIN_CPU_POWER_MARGIN 10
uint get_powercap_status(uint *in_target,uint *tbr)
{
	ulong c_freq;
	*in_target = 0;
	*tbr = 0;
	if (c_limit == PC_UNLIMITED) return 0;
	/* If we don't know the req_f we cannot release power */
	if (c_req_f == 0) return 0;
	c_freq=frequency_get_cpu_freq(0);
	if (c_freq != c_req_f) return 0;
	/* Power reading is pending */
	*in_target = 1;
	*tbr = 0;
	return 1;
}
	
