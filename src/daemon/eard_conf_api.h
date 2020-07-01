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

#ifndef _EAR_DAEMON_COMMON_H
#define _EAR_DAEMON_COMMON_H

#include <common/types/generic.h>
#include <common/types/application.h>
#include <common/types/job.h>
#include <common/types/loop.h>


// Data type to send the requests
union daemon_req_opt {
    unsigned long req_value;
    application_t app;
	loop_t		  loop;
	ear_event_t event;
};

struct daemon_req {
    ulong req_service;
	ulong  sec;
    union daemon_req_opt req_data;
};

// Number of services supported
#define ear_daemon_client_requests 	1
#define freq_req 					0
#define uncore_req 					0
#define rapl_req 					0
#define system_req 					0
#define node_energy_req 			0

// Services related with frequency
#define SET_FREQ 				0
#define START_GET_FREQ 			1
#define END_GET_FREQ 			2
#define SET_TURBO 				3
#define DATA_SIZE_FREQ 			4
#define CONNECT_FREQ 			5
#define START_APP_COMP_FREQ 	6
#define END_APP_COMP_FREQ 		7

#define END_COMM 				1000

#define EAR_COM_OK     	 		0
#define EAR_COM_ERROR 			-1

// Services related with uncore counters
#define START_UNCORE 			100
#define RESET_UNCORE 			101
#define READ_UNCORE 			102
#define DATA_SIZE_UNCORE 		103
#define CONNECT_UNCORE 			104
#define STOP_UNCORE 			105

// Services related with rapl counters 
#define START_RAPL 				200
#define RESET_RAPL 				201
#define READ_RAPL 				202
#define DATA_SIZE_RAPL 			203
#define CONNECT_RAPL 			204

// Services to contact with the rest of the system, such as writting data
#define WRITE_APP_SIGNATURE 	300
#define CONNECT_SYSTEM 			301
#define WRITE_EVENT				302
#define WRITE_LOOP_SIGNATURE 	303

// Services related to node energy management
#define READ_DC_ENERGY 			400
#define DATA_SIZE_ENERGY_NODE 	401
#define CONNECT_ENERGY 			402
#define ENERGY_FREQ				403

#else
#endif
