/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*	
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*	
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING	
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

typedef union req_data{
		application_t 		new_job;
		end_job_req_t 		end_job;
		new_conf_t 				ear_conf;
		new_policy_cont_t	pol_conf;
	  unsigned long     pc;
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
#define EAR_RC_NEW_POWERCAP	700
#define EAR_RC_STATUS		600
#define NO_COMMAND 100000

#define STATUS_BAD      0
#define STATUS_OK       1


#else
#endif
