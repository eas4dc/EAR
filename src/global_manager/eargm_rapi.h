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
*    \file remote_daemon_client.h
*    \brief This file defines the client side of the remote EAR API
*
* 	 Note:Specific functions could be substituted by a generic function passing a local_config_t
*/

#ifndef _EARGM_API_H
#define _EARGM_API_H

/** Connects with the EARGM.
*	The sequence must be : connect +  command + disconnect
* 	@param the nodename to connect with
*/
int eargm_connect(char *nodename,uint use_port);

/** Notifies the EARGM a new job using N nodes will start. It is supposed to be used by the EAR slurm plugin
*/
int eargm_new_job(uint num_nodes);

/** Notifies the EARGM  a job using N nodes is finishing
*/
int eargm_end_job(uint num_nodes);

/** Disconnect from the previously connected EARGM
*/
int eargm_disconnect();

#endif
