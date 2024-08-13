/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <common/config.h>
#include <common/states.h>
#include <daemon/local_api/node_mgr.h>
#include <common/utils/sched_support.h>
#include <common/types/configuration/cluster_conf.h>
#include <daemon/shared_pmon.h>


int main(int argc, char *argv[])
{
	ear_njob_t *nlist;
	cluster_conf_t my_cluster;
	char ear_path[MAX_PATH_SIZE];
	char *tmp;
	uint earl_jobs_found = 0;
	uint eard_jobs_found = 0;

	if (get_ear_conf_path(ear_path)==EAR_ERROR){
            printf("Error getting ear.conf path\n");
            exit(0);
        }
    	read_cluster_conf(ear_path,&my_cluster);
	tmp = my_cluster.install.dir_temp;
	printf("Using tmp dir '%s'\n", tmp);


	printf("Testing jobs using EAR library \n");
	if (nodemgr_job_init(tmp, &nlist) != EAR_SUCCESS){
		printf("Error in nodemgr_job_init\n");
		exit(0);
	}

	for (uint j = 0;j < MAX_CPUS_SUPPORTED; j++){
		if (nlist[j].jid != -1){
			printf("Node:mgr[%u] jid %lu sid %lu creation_time %lu\n", j, nlist[j].jid, nlist[j].sid, nlist[j].creation_time);
			printf("Testing job %lu.%lu\n", nlist[j].jid, nlist[j].sid);
			earl_jobs_found++;
			if (!is_job_step_valid(nlist[j].jid, nlist[j].sid)){
				printf("Error, JOB %lu.%lu not longer valid\n", nlist[j].jid, nlist[j].sid);
			}
		}
	}

	printf("Jobs using EAR library tested, %u jobs found\n", earl_jobs_found);

	printf("Testing jobs in EARD start\n");

	char ejoblist_path[MAX_PATH_SIZE];
	char epmon_app_path[MAX_PATH_SIZE];

	if (get_joblist_path(tmp, ejoblist_path) != EAR_SUCCESS){
		printf("Error gathering eard job list path\n");
		exit(0);
	}
	printf("eard job list path : '%s'\n", ejoblist_path);
	int fd_ejoblist, ejob_list_size;
	uint *ejoblist_shmem = attach_joblist_shared_area(ejoblist_path, &fd_ejoblist, &ejob_list_size);
	if (ejoblist_shmem == NULL){
		printf("EARD power monitor list of jobs not found\n");
		exit(0);
	}
	uint max_jobs = ejob_list_size/sizeof(uint);
	printf("EARD power monitor list of jobs found, elements %u\n\n\n", max_jobs);

	for (uint i = 0; i < max_jobs; i++){
		if (ejoblist_shmem[i] != 0){
			printf("Job with ID %u found in pos %u\n", ejoblist_shmem[i], i);
			powermon_app_t *pmapp_shmem;
			int fd_pmapp;
			if (get_jobmon_path(tmp, ejoblist_shmem[i], epmon_app_path) != EAR_SUCCESS){
				printf("EARD pmapp info path for ID %u not found\n", ejoblist_shmem[i]);
				continue;
			}
			pmapp_shmem = attach_jobmon_shared_area(epmon_app_path, &fd_pmapp);
			if (pmapp_shmem == NULL){
				printf("EARD pmapp info for ID %u not found\n", ejoblist_shmem[i]);
				continue;
			}
			eard_jobs_found++;
			printf("PMAPP: ID.STEPID %lu.%lu is_job %u signature_reported %d CPU freq %lu\n", pmapp_shmem->app.job.id, pmapp_shmem->app.job.step_id, pmapp_shmem->is_job, pmapp_shmem->sig_reported, pmapp_shmem->current_freq);
			// if (argc > 1) pmapp_shmem->state = APP_FINISHED;
			dettach_jobmon_shared_area(fd_pmapp);

		}
	}
	dettach_joblib_shared_area(fd_ejoblist);


	printf("Jobs in EARD tested. %u jobs found\n", eard_jobs_found);

	return 0;	
	
}
