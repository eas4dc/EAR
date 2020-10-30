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
#include <string.h>
#include <common/config.h>
#include <common/output/verbose.h>
#include <daemon/app_mgt.h>

void app_mgt_new_job(app_mgt_t *a)
{
	if (a==NULL) return;
	memset(a,0,sizeof(app_mgt_t));
}
void app_mgt_end_job(app_mgt_t *a)
{
	if (a==NULL) return;
	memset(a,0,sizeof(app_mgt_t));
}
void print_app_mgt_data(app_mgt_t *a)
{
	if (a==NULL) return;

	verbose(1,"App_info:master_rank %u ppn %u nodes %u total_processes %u max_ppn %u",a->master_rank,a->ppn,a->nodes,a->total_processes,a->max_ppn);
}
uint is_app_master(app_mgt_t *a)
{
	if (a==NULL) return 0;
	return (a->master_rank==0);	
}



