/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <common/config.h>
#include <common/system/file.h>
#include <common/utils/string.h>
#include <common/output/verbose.h>
#include <daemon/local_api/node_mgr.h>
#include <common/system/shared_areas.h>
#include <common/output/debug.h>

static char node_mgr_tmp[PATH_MAX];
static char node_mgr_info[PATH_MAX];
static char node_mgr_lock[PATH_MAX];
static int fd_node_mgr_lck = -1;
static int fd_node_mgr_info = -1;
static ear_njob_t *node_jobs_list = NULL;

#define MAX_LOCK_TRIES (MAX_SOCKET_COMM_TRIES * 10)
#define WF_SUPPORT_VERB 2

/** Initialices the lock file and attaches the shared memory region. nodelist is internally allocated. Must be used by earl */

state_t nodemgr_lock_init(char *tmp)
{
	mode_t oldm;

  if (fd_node_mgr_lck >= 0) close(fd_node_mgr_lck);
  strncpy(node_mgr_tmp,tmp, sizeof(node_mgr_tmp));
  xsnprintf(node_mgr_lock,sizeof(node_mgr_lock),"%s/%s",node_mgr_tmp,LOCK_NODE_MGR_LCK);

  if (ear_file_is_regular(node_mgr_lock)){
    fd_node_mgr_lck = ear_file_lock_create(node_mgr_lock);
    if (fd_node_mgr_lck < 0) return EAR_ERROR;
  }else{
    oldm = umask(0);
    fd_node_mgr_lck = ear_file_lock_create(node_mgr_lock);
    if (fd_node_mgr_lck >= 0) chmod(node_mgr_lock, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    umask(oldm);
    if (fd_node_mgr_lck < 0) return EAR_ERROR;
  }

  return EAR_SUCCESS;
}

state_t nodemgr_job_init(char *tmp, ear_njob_t **nodelist)
{
	if (tmp == NULL) return EAR_ERROR;
	if (nodelist == NULL ) return EAR_ERROR;

  if (state_fail(nodemgr_lock_init(tmp))){
    verbose(WF_SUPPORT_VERB,"EAR Error, nodemgr_lock cannot be initialized");
    return EAR_ERROR;
  }

	/* If we can open it, we try to access the shared data */
	if (!ear_file_lock_timeout(fd_node_mgr_lck, MAX_LOCK_TRIES)) return EAR_ERROR;

	/* The eard will create this area */
	xsnprintf(node_mgr_info,sizeof(node_mgr_info),"%s/%s",node_mgr_tmp,LOCK_NODE_MGR_INFO);
	//verbose(WF_SUPPORT_VERB, "NODE MGR region path %s\n", node_mgr_info);
	node_jobs_list = (ear_njob_t *)attach_shared_area(node_mgr_info,sizeof(ear_njob_t)*MAX_CPUS_SUPPORTED,O_RDWR,&fd_node_mgr_info,NULL);

	ear_file_unlock(fd_node_mgr_lck);
	*nodelist = node_jobs_list;
	if (node_jobs_list == NULL){ 
		debug("Error: attach shared area for job mgr returns NULL");
		return EAR_ERROR;
	}
	debug("NODE MGR Initialized ok");
	return EAR_SUCCESS;
}

/* Created the lock and the shared region. To be used by eard */
state_t nodemgr_server_init(char *tmp, ear_njob_t **nodelist)
{
  if (tmp == NULL) return EAR_ERROR;
	if (nodelist == NULL ) return EAR_ERROR;


  if (state_fail(nodemgr_lock_init(tmp))){
    verbose(WF_SUPPORT_VERB," EAR Error, nodemgr_lock cannot be initialized");
    return EAR_ERROR;
  }

	node_jobs_list = (ear_njob_t *)calloc(MAX_CPUS_SUPPORTED,sizeof(ear_njob_t));
	if (node_jobs_list == NULL){ 
		return EAR_ERROR;
	}
	
	for (uint i = 0 ; i < MAX_CPUS_SUPPORTED ; i++){
		node_jobs_list[i].jid = -1;
		node_jobs_list[i].sid = -1;
	}

  xsnprintf(node_mgr_info,sizeof(node_mgr_info),"%s/%s",node_mgr_tmp,LOCK_NODE_MGR_INFO);
  /* If we can open it, we try to access the shared data */
	if (!ear_file_lock_timeout(fd_node_mgr_lck, MAX_LOCK_TRIES)) return EAR_ERROR;

  /* The eard will create this area */
	debug("NODE MGR INFO %s\n", node_mgr_info);
	mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; // Read and write permission for the owner user. Read permission for group and others.
  node_jobs_list = (ear_njob_t *) create_shared_area(node_mgr_info, perms, (char *) node_jobs_list, sizeof(ear_njob_t) * MAX_CPUS_SUPPORTED, &fd_node_mgr_info, 0);
	
  ear_file_unlock(fd_node_mgr_lck);
	*nodelist = node_jobs_list;
	if (node_jobs_list == NULL){ 
		debug("Error, shared memory region for job mgr not created");
		return EAR_ERROR;
	}
  return EAR_SUCCESS;
}


/* Computes the number of jobs attached */
state_t nodemgr_get_num_jobs_attached(uint *num_jobs)
{
	int total_jobs = 0;
	if (fd_node_mgr_lck < 0) return EAR_ERROR;
	if (node_jobs_list == NULL) return EAR_ERROR;
	if (num_jobs == NULL) return EAR_ERROR;

	*num_jobs = 0;

	/* Get the lock */
	if (!ear_file_lock_timeout(fd_node_mgr_lck, MAX_LOCK_TRIES)) return EAR_ERROR;

	for (uint i = 0 ; i < MAX_CPUS_SUPPORTED ; i++){
#if WF_SUPPORT
		if (node_jobs_list[i].jid != -1)
		{
			total_jobs += ear_max(node_jobs_list[i].num_earl_apps, 1);
		}
#else
		if (node_jobs_list[i].jid != -1)
		{
			total_jobs++;
		}
#endif
	}

	ear_file_unlock(fd_node_mgr_lck);
	*num_jobs = total_jobs;	
	return EAR_SUCCESS;
}

/* Attaches a new job . To be used by earl */
state_t nodemgr_attach_job(ear_njob_t *my_job, uint *my_index)
{
	state_t s = EAR_ERROR;
  if (fd_node_mgr_lck < 0) return EAR_ERROR;

  uint pos;
  uint new;

  //verbose(WF_SUPPORT_VERB,"Attaching job to node_mgr region %lu.%lu", my_job->jid, my_job->sid);
  new = (nodemgr_find_job(my_job, &pos) == EAR_ERROR);

  if (new){
    //verbose(WF_SUPPORT_VERB,"Adding new job in NODE MGR region ");
	  /* Get the lock */
	  if (!ear_file_lock_timeout(fd_node_mgr_lck, MAX_LOCK_TRIES)) return EAR_ERROR;
  
	  for (uint i = 0 ; i < MAX_CPUS_SUPPORTED ; i++){
      if (node_jobs_list[i].jid == -1){
			  debug("Found free position at %d", i);
			  memcpy(&node_jobs_list[i],my_job, sizeof(ear_njob_t));
			  *my_index = i;
  
			  ear_file_unlock(fd_node_mgr_lck);
			  return EAR_SUCCESS;
		  }else {
			  debug("Looking at index %d", i);
		  }
    }
  
	  ear_file_unlock(fd_node_mgr_lck);
  }else{
    s = EAR_SUCCESS;
#if WF_SUPPORT
    //verbose(WF_SUPPORT_VERB,"Adding new app in NODE MGR region in pos %u", pos);
    if (!ear_file_lock_timeout(fd_node_mgr_lck, MAX_LOCK_TRIES)) return EAR_ERROR;
    node_jobs_list[pos].modification_time = my_job->modification_time;
    node_jobs_list[pos].num_earl_apps++;
    //verbose(WF_SUPPORT_VERB,"PENDING add the mask");
    ear_file_unlock(fd_node_mgr_lck);
#endif
  }
	return s;
}

/* Looks for an already existing attached job */
state_t nodemgr_find_job(ear_njob_t *my_job, uint *my_index)
{ 
  state_t s = EAR_ERROR;

  if (fd_node_mgr_lck < 0) return EAR_ERROR;
  
  /* Get the lock */
  if (!ear_file_lock_timeout(fd_node_mgr_lck, MAX_LOCK_TRIES)) return EAR_ERROR;
  
  for (uint i = 0 ; i < MAX_CPUS_SUPPORTED ; i++){
    if ((node_jobs_list[i].jid == my_job->jid) && (node_jobs_list[i].sid == my_job->sid)){
      *my_index = i;
      ear_file_unlock(fd_node_mgr_lck);
      return EAR_SUCCESS;
    }
  }
  
  ear_file_unlock(fd_node_mgr_lck);
  return s;
}


/* Marks a job entry as free. To be used by earl before job end */
state_t nodemgr_job_end(uint index)
{
	state_t s = EAR_SUCCESS;
  if (fd_node_mgr_lck < 0) return EAR_ERROR;
	if (index >= MAX_CPUS_SUPPORTED) return EAR_ERROR;

	/* Get the lock */
	if (!ear_file_lock_timeout(fd_node_mgr_lck, MAX_LOCK_TRIES)) return EAR_ERROR;

#if WF_SUPPORT
  node_jobs_list[index].num_earl_apps--;
  if (node_jobs_list[index].num_earl_apps == 0){
#endif
	  node_jobs_list[index].jid = -1;
	  node_jobs_list[index].sid = -1;
	  node_jobs_list[index].creation_time     = 0;
#if WF_SUPPORT
	  node_jobs_list[index].modification_time = 0;
  }
#endif

  ear_file_unlock(fd_node_mgr_lck);
  return s;
}

/* Cleans a given job entry. Tobe used by eard to guarantee the job is not there */
state_t nodemgr_clean_job(job_id jid,job_id sid)
{
	if (fd_node_mgr_lck < 0) return EAR_ERROR;

	for (uint i = 0;i< MAX_CPUS_SUPPORTED;i++){
		if ((node_jobs_list[i].jid == jid) && (node_jobs_list[i].sid == sid)){
			/* Get the lock */
			if (!ear_file_lock_timeout(fd_node_mgr_lck, MAX_LOCK_TRIES)) return EAR_ERROR;

				if ((node_jobs_list[i].jid == jid) && (node_jobs_list[i].sid == sid)){
					node_jobs_list[i].jid = -1;
					node_jobs_list[i].sid = -1;
					node_jobs_list[i].creation_time     = 0;
#if WF_SUPPORT
          node_jobs_list[i].modification_time = 0;
#endif
				}

		ear_file_unlock(fd_node_mgr_lck);
			return EAR_SUCCESS;
		}
	}
	return EAR_SUCCESS;
}

/* Removes the shared region. To be used by eard at eard exit */
state_t nodemgr_server_end()
{
  /* Get the lock */
	if (!ear_file_lock_timeout(fd_node_mgr_lck, MAX_LOCK_TRIES)) return EAR_ERROR;

	/* Release shared memory */
	dispose_shared_area(node_mgr_info, fd_node_mgr_info);

	/* Remove the lock */
	ear_file_unlock(fd_node_mgr_lck);
	ear_file_lock_clean(fd_node_mgr_lck,node_mgr_lock);
  return EAR_SUCCESS;

}

state_t node_mgr_info_lock()
{
	if (!ear_file_lock_timeout(fd_node_mgr_lck, MAX_LOCK_TRIES)) return EAR_ERROR;
	return EAR_SUCCESS;
}

void node_mgr_info_unlock()
{
	ear_file_unlock(fd_node_mgr_lck);
}
