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
