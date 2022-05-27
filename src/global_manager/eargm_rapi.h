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

#include <global_manager/meta_eargm.h>




/** Notifies the EARGM a new job using N nodes will start. It is supposed to be used by the EAR slurm plugin
*/
int eargm_new_job(uint num_nodes);

/** Notifies the EARGM  a job using N nodes is finishing
*/
int eargm_end_job(uint num_nodes);

/* Increases the EARGM's powercap allocation by increase*/
int eargm_inc_powercap(uint increase);

/* Reduces the EARGM's powercap allocation by decrease*/
int eargm_red_powercap(uint decrease);

/* Resets the EARGM's powercap to its default value (from ear.conf) */
int eargm_reset_powercap();

/** Disconnect from the previously connected EARGM
*/
int eargm_disconnect();

/** Given a fd, sends a request for an eargm_status and reads it. Returns the number of eargm_status retrieved */
int eargm_get_status(int fd, eargm_status_t **status);

/** Sends the appropriate message to every sub-eargm on the list */
int eargm_send_table_commands(eargm_table_t *egm_table);

int eargm_get_all_status(cluster_conf_t *conf, eargm_status_t **status, int eargm_idx);
#endif
