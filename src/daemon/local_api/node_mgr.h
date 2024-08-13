/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _LIB_NODE_MGR_H
#define _LIB_NODE_MGR_H

#define _GNU_SOURCE
#include <sched.h>

#include <common/states.h>
#include <common/types/generic.h>

#define LOCK_NODE_MGR_LCK	".node_mgr_lck"
#define LOCK_NODE_MGR_INFO	".node_mgr_data"
#define NULL_NODE_MGR_INDEX 10000

typedef struct ear_njob {
	job_id     jid;
	job_id     sid;
	cpu_set_t  node_mask;
	time_t     creation_time;
#if WF_SUPPORT
	time_t     modification_time;
  uint       num_earl_apps;
#endif
} ear_njob_t;

/** Initializes the lock file and attaches the shared memory region.
 * Nodelist is internally allocated. Must be used by the EARL. */
state_t nodemgr_job_init(char *tmp, ear_njob_t **nodelist );

/** Created the lock and the shared region. To be used by EARD. */
state_t nodemgr_server_init(char *tmp, ear_njob_t **nodelist);

/** Computes the number of jobs attached. */
state_t nodemgr_get_num_jobs_attached(uint *num_jobs);

/** Attaches a new job . To be used by EARL. */
state_t nodemgr_attach_job(ear_njob_t *my_job, uint *my_index);

/** Finds for an already existing attached job. */
state_t nodemgr_find_job(ear_njob_t *my_job, uint *my_index);


/** Marks a job entry as free. To be used by EARL before job ends. */
state_t nodemgr_job_end(uint index);

/** Cleans a given job entry. Tobe used by EARD to guarantee the job is not there. */
state_t nodemgr_clean_job(job_id jid,job_id sid);

/** Removes the shared region. To be used by eard at eard exit */
state_t nodemgr_server_end();

state_t node_mgr_info_lock();
void node_mgr_info_unlock();

/** Checks and creates the nodemgr_lock file */
state_t nodemgr_lock_init(char *tmp);
#endif
