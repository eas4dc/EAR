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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>



/* EAR includes */
#include <daemon/remote_api/eard_rapi.h>
#include <common/string_enhanced.h>


#define NUM_ARGS 3
#define NEW_JOB 0
#define END_JOB 1
new_job_req_t my_new_job;
end_job_req_t my_end_job; 
uint action = NEW_JOB;
void Usage(char *app)
{
	printf("Usage: %s port newjob|endjob [hostfile]\n",app);
	printf("\t if hostfile is null, local host is used\n");
	exit(1);
}

int fill_new_job_data(new_job_req_t *myjob)
{
	char *cjid = getenv(SCHED_JOB_ID);
	job_id jid;
  uid_t uid = geteuid();
  gid_t gid = getgid();
  struct passwd *upw = getpwuid(uid);
  struct group  *gpw = getgrgid(gid);

	if (cjid == NULL){
		printf("Error accessing %s\n", SCHED_JOB_ID);
		return -1;
	}
	jid = atoi(cjid);
  if (upw == NULL || gpw == NULL) {
		printf("Error converting EUID and group\n");
		return -1;
  }

	myjob->job.id = jid;
	myjob->job.step_id = 0;
	strncpy(myjob->job.user_id , upw->pw_name, sizeof(myjob->job.user_id));
	strncpy(myjob->job.group_id, gpw->gr_name, sizeof(myjob->job.group_id)); 
	strcpy(myjob->job.user_acc,"");
	strcpy(myjob->job.energy_tag,"");
	strcpy(myjob->job.policy,"");
	myjob->job.th = 0;
	myjob->is_mpi = 1;
	myjob->is_learning = 0;
	return 0;
}
int fill_end_job_data(end_job_req_t *myjob)
{
	char *cjid = getenv(SCHED_JOB_ID);
	uint jid;
	if (cjid == NULL){
		printf("Error accessing %s\n", SCHED_JOB_ID);
		return -1;
	}
	jid = atoi(cjid);
	myjob->jid = jid;
	myjob->sid = 0;
	return 0;
}

static int ear_read_word(int fd, char* word, uint max_size, char sep)
{
	uint pos = 0;
	int ret;
	ret = read(fd,&word[pos], 1);
	while((ret > 0) && (pos < max_size) && (word[pos] != sep)){
		pos++;
		ret = read(fd, &word[pos], 1);
	}
	word[pos] = '\0';
	return (pos);	
}

#define MAX_NODE_SIZE 128 
static void create_nodelist(char *hostfile, char ***list, uint *nodecnt)
{
	int fd;
	char **llist = *list;
	char *cnode;
  char node[MAX_NODE_SIZE];
	*nodecnt = 0;
	fd = open(hostfile, O_RDONLY);
	if (fd < 0){
		return;
	}
	llist = calloc(1, sizeof(char *));
	while(ear_read_word(fd,node,MAX_NODE_SIZE,'\n') > 0){
		cnode = malloc(strlen(node)+1);
		strcpy(cnode, node);
		llist[*nodecnt] = cnode;
		*nodecnt += 1;
		llist = realloc(llist, (*nodecnt+1)*sizeof(char *));
	}
	llist[*nodecnt+1] = NULL;	
	*list = llist;
}
void main(int argc, char *argv[])
{
	char nodename[256];
	uint eard_port;
	char *hostfile;
	char **ear_hostfile;
	uint num_nodes = 0;
	
	
	if (argc < NUM_ARGS) Usage(argv[0]);
	printf("Action %s port %s\n",argv[2],argv[1]);
	eard_port = atoi(argv[1]);
	if (!strcmp(argv[2],"newjob")) action = NEW_JOB;
	else 													action = END_JOB;
	hostfile = argv[3];
	if (hostfile == NULL){
  	if (gethostname(nodename, sizeof(nodename)) < 0) {
			printf("Error getting nodename %s\n",strerror(errno));
    	_exit(1);
  	}
	
  	strtok(nodename, ".");
		if (remote_connect(nodename, eard_port) < 0){
			printf("Error connecting with EARD in node %s port %u\n",nodename,eard_port);
			exit(1);
		}
	}else{
		create_nodelist(hostfile, &ear_hostfile, &num_nodes); 
		for (uint i = 0; i < num_nodes; i++) printf("Node %d: %s\n", i, ear_hostfile[i]);
	}
	if (action == NEW_JOB){
		if (fill_new_job_data(&my_new_job) < 0){
			printf("Error getting new job data\n");
			remote_disconnect();
			exit(1);
		}
		if (hostfile == NULL){	
			/* Single node */
			if(ear_node_new_job(&my_new_job) < 0){
				printf("Error notifying new job\n");
			}
		}else{
 			ear_nodelist_ear_node_new_job(&my_new_job, eard_port, NULL, ear_hostfile, num_nodes);
		}
	}else{
		if (fill_end_job_data(&my_end_job) < 0){
			printf("Error getting end job data\n");
			remote_disconnect();
			exit(1);
		}
		/* Single node */
		if (hostfile == NULL){
			if (ear_node_end_job(my_end_job.jid,my_end_job.sid) < 0){	
				printf("Error notifying end job\n");
			}
		}else{
 			ear_nodelist_ear_node_end_job(my_end_job.jid, my_end_job.sid, eard_port, NULL, ear_hostfile, num_nodes);
		}
	}
	if (hostfile == NULL){
		remote_disconnect();
	}
}

