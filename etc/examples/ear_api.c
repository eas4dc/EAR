/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#include <ear.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int main(int argc,char *argv[])
{
 	unsigned long e_mj=0,t_ms=0,e_mj_init,t_ms_init,e_mj_end,t_ms_end=0;
 	unsigned long ej,emj,ts,tms,os,oms;
 	unsigned long ej_e,emj_e,ts_e,tms_e,os_e,oms_e;
	int i=0;
 	struct tm *tstamp,*tstamp2,*tstamp3,*tstamp4;
	char s[128],s2[128],s3[128],s4[128];
	char node_name[256];


	printf("This code has to be executed in a node with EARD running, please, execute with sbatch, srun or equivalent\n");
	gethostname(node_name, sizeof(node_name));
	printf("Running in %s\n", node_name);
	/* Connecting with ear */
	if (ear_connect() != EAR_SUCCESS)
 	{
 		printf("error connecting eard\n");
		exit(1);
 	}
	/* Reading energy */
	if (ear_energy(&e_mj_init,&t_ms_init)!=EAR_SUCCESS)
 	{
 		printf("Error in ear_energy\n");
 	}
	while(i<5)
 	{
 		sleep(5);
		/* READING ENERGY */
		if (ear_energy(&e_mj_end,&t_ms_end)!=EAR_SUCCESS)
 		{
 			printf("Error in ear_energy\n");
 		}
		else
 		{
 			ts=t_ms_init/1000;
 			ts_e=t_ms_end/1000;
 			tstamp=localtime((time_t *)&ts);
 			strftime(s, sizeof(s), "%c", tstamp);
 			tstamp2=localtime((time_t *)&ts_e);
 			strftime(s2, sizeof(s), "%c", tstamp2);
 			printf("Start time %s End time %s\n",s,s2);
 			ear_energy_diff(e_mj_init,e_mj_end, &e_mj, t_ms_init,t_ms_end,&t_ms); 
			printf("Time consumed %lu (ms), energy consumed %lu(mJ), \
 			Avg power %lf(W)\n",t_ms,e_mj,(double)e_mj/(double)t_ms);
 			e_mj_init=e_mj_end;
 			t_ms_init=t_ms_end;
 		}
 		i++;
 	}
 	ear_disconnect();
}

