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

int init_stall_metrics()
{
	return 0;
}

void reset_stall_metrics()
{

}

void start_stall_metrics()
{

}

void stop_stall_metrics(long long *stall_cycles)
{
	*stall_cycles = 0;
}

void get_stall_metrics(long long *total_stall_cycles)
{
	*total_stall_cycles = 0;
}