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

#ifndef _PWR_MGT_H
#define _PWR_MGT_H
#include <common/states.h>
#include <daemon/powercap/powercap_status_conf.h>

typedef void * pwr_mgt_t;

/* Initialization sequence
 *
 pmgt_init();
 pmgt_handler_alloc(&pmgr);
 pmgt_enable(pmgr);
 pmgt_set_pc_mode(pmgr,PC_MODE_TARGET);
 pmgt_set_powercap_value(pmgr,0,DOMAIN_NODE,powercap_init);
 pmgt_idle_to_run(pmgr);
 pmgt_new_job(pmgr);

 - Requested frqeuency needs to be set -
 
 pmgt_set_app_req_freq(pmgr,&finfo);
 */


state_t pmgt_init();
state_t pmgt_enable(pwr_mgt_t *phandler);
state_t pmgt_disable(pwr_mgt_t *phandler);
state_t pmgt_handler_alloc(pwr_mgt_t **phandler);
state_t pmgt_disable_policy(pwr_mgt_t *phandler,uint pid);
state_t pmgt_disable_policies(pwr_mgt_t *phandler);
state_t pmgt_set_powercap_value(pwr_mgt_t *phandler,uint pid,uint domain,ulong limit);

/* Sets in powercap the powercap per domain : 3 domains in CPU only systems and 4 in CPU+GPU */
state_t pmgt_get_powercap_value(pwr_mgt_t *phandler,uint pid,ulong *powercap);
uint pmgt_is_powercap_enabled(pwr_mgt_t *phandler,uint pid);
void pmgt_print_powercap_value(pwr_mgt_t *phandler,int fd);
void pmgt_powercap_to_str(pwr_mgt_t *phandler,char *b);
void pmgt_set_status(pwr_mgt_t *phandler,uint status);
uint pmgt_get_powercap_cpu_strategy(pwr_mgt_t *phandler);
uint pmgt_get_powercap_gpu_strategy(pwr_mgt_t *phandler);
void pmgt_set_pc_mode(pwr_mgt_t *phandler,uint mode);
/* Based on the current power consumption per domain and the status (idle/run) distributes the power accross domains */
void pmgt_set_power_per_domain(pwr_mgt_t *phandler,dom_power_t *pdomain,uint st);
/* Gets the requested frequency */
void pmgt_get_app_req_freq(uint domain, ulong *f, uint dom_size);
/* Sets the requested frequency */
void pmgt_set_app_req_freq(pwr_mgt_t *phandler,pc_app_info_t *pc_app);
/* Notifies the pwr_mgt a new job/end job event */
void pmgt_new_job(pwr_mgt_t *phandler);
void pmgt_end_job(pwr_mgt_t *phandler);
/* Notifies the pwr_mgt the node goes from idle to run */
void pmgt_idle_to_run(pwr_mgt_t *phandler);
/* Notifies the pwr_mgt the node goes from run to idle */
void pmgt_run_to_idle(pwr_mgt_t *phandler);
/* When the measured utilization changes, this funcion re-distributes the power based on that, only internally, it doesn't change the powercap allocated to each component */
void pmgt_powercap_node_reallocation();
/* Estimates the powercap status per domain (To be completed )*/
uint pmgt_powercap_status_per_domain();

/* Returns the current node status using its domains status */
void pmgt_get_status(pmgt_status_t *status);

#if SYN_TEST
void pmgt_set_burst(int b);
#endif

#endif
