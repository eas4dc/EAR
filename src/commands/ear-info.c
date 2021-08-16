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
#include <common/config.h>
#include <common/states.h>
#include <common/types/version.h>
#include <common/types/configuration/cluster_conf.h>

#define DEFAULT_EARL 1

int main(int argc,char *argv[])
{
	cluster_conf_t my_cluster;
	char ver[128];
	char ear_path[256];
	if (get_ear_conf_path(ear_path)==EAR_ERROR){
			printf("Error getting ear.conf path, load the ear module\n");
			exit(0);
	}
	read_cluster_conf(ear_path,&my_cluster);
	version_to_str(ver);
	printf("EAR version %s\n",ver);
	#if USE_GPUS
	printf("EAR installed with GPU support\n");
	#else
	printf("EAR installed without GPU support\n");
	#endif
	if (my_cluster.eard.force_frequencies){
		printf("EAR controls node CPU frequencies");	
	}
	printf("Default cluster policy is %s\n",my_cluster.power_policies[my_cluster.default_policy].name);
	printf("EAR optimization  by default set to %d\n",DEFAULT_EARL);		

	return 0;
}

