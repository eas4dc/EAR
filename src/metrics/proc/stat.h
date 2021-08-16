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

#ifndef METRICS_PROC_STAT_H
#define METRICS_PROC_STAT_H

#include <common/types.h>
#include <common/states.h>
#include <common/plugins.h>

typedef struct proc_stat_s {
	pid_t pid;
	char file[SZ_FILENAME];
	char state;
	int ppid;
	int pgrp;
	int session;
	int tty_nr;
	int tpgid;
	uint flags;
	ulong minflt;
	ulong cminflt;
	ulong majflt;
	ulong cmajflt;
	ulong utime;
	ulong stime;
	long cutime;
	long cstime;
	long priority;
	long nice;
	long num_threads;
	long itrealvalue;
	ullong starttime;
	ulong vsize;
	long rss;
	ulong rsslim;
	ulong startcode;
	ulong endcode;
	ulong startstack;
	ulong kstkesp;
	ulong kstkeip;
	ulong signal;
	ulong blocked;
	ulong sigignore;
	ulong sigcatch;
	ulong wchan;
	ulong nswap;
	ulong cnswap;
	int exit_signal;
	int processor;
	uint rt_priority;
	uint policy;
	ullong delayacct_blkio_ticks;
	ulong guest_time;
	long cguest_time;
	ulong start_data;
	ulong end_data;
	ulong start_brk;
	ulong arg_start;
	ulong arg_end;
	ulong env_start;
	ulong env_end;
	int exit_code;
} proc_stat_t;

state_t proc_stat_load();

state_t proc_stat_init(ctx_t *c, pid_t pid);

state_t proc_stat_dispose(ctx_t *c);

state_t proc_stat_read(ctx_t *c, proc_stat_t *ps);

state_t proc_stat_read_diff(ctx_t *c, proc_stat_t *ps2, proc_stat_t *ps1, proc_stat_t *psD);

state_t proc_stat_read_copy(ctx_t *c, proc_stat_t *ps2, proc_stat_t *ps1, proc_stat_t *psD);

state_t proc_stat_read_now(ctx_t *c, proc_stat_t *ps1, proc_stat_t *psD);

state_t proc_stat_data_diff(proc_stat_t *ps2, proc_stat_t *ps1, proc_stat_t *psD);

state_t proc_stat_data_copy(proc_stat_t *dest, proc_stat_t *src);

state_t proc_stat_data_print(proc_stat_t *ps);

state_t proc_stat_data_to_str(proc_stat_t *ps,char *buffer,size_t len);


#endif //METRICS_PROC_STAT_H
