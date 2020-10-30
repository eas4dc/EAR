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

#ifndef _APP_CONF_API_H
#define _APP_CONF_API_H
#include <common/config.h>

#define ENERGY_TIME		1000
#define ENERGY_TIME_DEBUG               1001
#define CONNECT		2000
#define DISCONNECT 	2001
#define INVALID_COMMAND 1

typedef struct energy{
        ulong energy_j;
        ulong energy_mj;
        ulong time_sec;
        ulong time_ms;
        ulong os_time_sec;
        ulong os_time_ms;
}energy_t;

union app_recv_opt {
	energy_t my_energy;
};

typedef struct app_send{
	uint req;
	int pid;
}app_send_t;


typedef struct app_recv{
	int 				ret;
	union app_recv_opt 	my_data;
}app_recv_t;

#else
#endif
