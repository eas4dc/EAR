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

/**
*    \file remote_conf.h
*    \brief This file defines data types and constants shared by the client and server in the remote EAR API
*
*/



#ifndef REMOTE_CONF_H
#define REMOTE_CONF_H

#include <common/types/application.h>
#include <common/types/configuration/policy_conf.h>
#include <common/types/risk.h>
#if POWERCAP
#include <daemon/powercap_status_conf.h>
#endif

typedef struct new_job_req{
  job_t job;
  uint8_t is_mpi;
  uint8_t is_learning;
}new_job_req_t;


typedef struct end_job_req{
	job_id jid;
	job_id sid;
	//int status;
}end_job_req_t;

typedef struct new_conf{
	ulong max_freq;
	ulong min_freq;
	ulong th;
	uint p_states;
	uint p_id;
}new_conf_t;

typedef struct new_policy_cont{
	char name[POLICY_NAME_SIZE];
	ulong def_freq;
	double settings[MAX_POLICY_SETTINGS];
}new_policy_cont_t;

typedef struct power_limit{
	unsigned int type;
	unsigned long limit;
}power_limit_t;

typedef struct risk_dec{
	risk_t level;
	unsigned long target;
}risk_dec_t;

typedef union req_data{
		new_job_req_t 		new_job;
		end_job_req_t 		end_job;
		new_conf_t 				ear_conf;
		new_policy_cont_t	pol_conf;
	  power_limit_t     pc;
		risk_dec_t 			risk;
#if POWERCAP
		powercap_opt_t  pc_opt;
#endif
}req_data_t;



typedef struct request{
    uint        req;
    uint        node_dist;
    int        time_code;
    req_data_t  my_req;
}request_t;

typedef struct status_node_info{
	ulong avg_freq; // In KH
	ulong temp; // In degres, No creo que haya falta enviar un unsigned long long
	ulong power; // In Watts 
	ulong max_freq;// in KH
} status_node_info_t;

typedef struct app_info{
	uint job_id;
	uint step_id;
}app_info_t;

typedef struct request_header {
    int type;
    uint size;
} request_header_t;

typedef struct eard_policy_info{
    ulong freq; /* default freq in KH, divide by 1000000 to show Ghz */
    uint th;     /* th x 100, divide by 100 */
}eard_policy_info_t;



typedef struct status{
    unsigned int     ip;
    char    ok;
    status_node_info_t  node;
    app_info_t  app;
	unsigned int num_policies;
    eard_policy_info_t    policy_conf[TOTAL_POLICIES];
} status_t;

typedef struct performance{
	float cpi;
	float gbs;
	float gflops_watt;
}performance_t;




#define EAR_RC_NEW_JOB     0
#define EAR_RC_END_JOB     1
#define EAR_RC_MAX_FREQ    100
#define EAR_RC_NEW_TH	   101
#define EAR_RC_INC_TH	   102
#define EAR_RC_RED_PSTATE   103
#define EAR_RC_SET_FREQ 	104
#define EAR_RC_DEF_FREQ		105
#define EAR_RC_REST_CONF	106
#define EAR_RC_SET_POLICY 108
#define EAR_RC_SET_DEF_PSTATE 109
#define EAR_RC_SET_MAX_PSTATE 110
#define EAR_RC_PING		    500
#define EAR_RC_STATUS		600

/* New functions for power limits */
#define EAR_RC_RED_POWER 700
#define EAR_RC_SET_POWER 701 
#define EAR_RC_INC_POWER 702
#define EAR_RC_GET_POWER 703
#define EAR_RC_GET_POWERCAP_STATUS 	704
#define EAR_RC_SET_POWERCAP_OPT			705
#define EAR_RC_SET_RISK 800
#define EAR_RC_RELEASE_IDLE		801



/******************* IMPORTANT ***********************/
// UPDATE MAX_TYPE_VALUE FOR EACH NEW TYPE OR THE NEW TYPE WON'T BE ACCEPTED
#define EAR_TYPE_COMMAND        2000
#define EAR_TYPE_STATUS         2001
#define EAR_TYPE_POWER_STATUS   2002
#define EAR_TYPE_RELEASED       2003
///  |||||
///  vvvvv
/******************* IMPORTANT ***********************/
// UPDATE MAX_TYPE_VALUE FOR EACH NEW TYPE OR THE NEW TYPE WON'T BE ACCEPTED
#define MIN_TYPE_VALUE  EAR_TYPE_COMMAND
#define MAX_TYPE_VALUE  EAR_TYPE_RELEASED
/*****************************************************/

#define NO_COMMAND 100000

#define STATUS_BAD      0
#define STATUS_OK       1

#define ABSOLUTE 0
#define RELATIVE 1


#else
#endif
