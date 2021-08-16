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

#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <sys/syscall.h>
#include <common/output/debug.h>
#include <metrics/common/perf.h>

// Future TO-DO:
// - Add a control and static manager for already open events (duplicates are
//   bad because there are limited counters in each CPU).
// - Convert to thead safe class (what happens when reading the same FD?).

state_t perf_open(perf_t *perf, perf_t *group, pid_t pid, uint type, ulong event)
{
	return perf_opex(perf, group, pid, type, event, 0);
}

state_t perf_opex(perf_t *perf, perf_t *group, pid_t pid, uint type, ulong event, uint options)
{
	int gp_flag =  0;	
	int gp_fd   = -1;

	//
	memset(perf, 0, sizeof(perf_t));

	if (group != NULL) {
		if (perf != group) {
			gp_fd = group->fd;
		} else {
			
		}
		perf->group = group;
		gp_flag = PERF_FORMAT_GROUP;
	}
	
	perf->attr.type           = type;
	perf->attr.size           = sizeof(struct perf_event_attr);
	perf->attr.config         = event;
//	perf->attr.config         = event | 0x200000;
//	perf->attr.config1        = 0x200000;
	perf->attr.exclusive      = options;
	perf->attr.disabled       = 1;
	perf->attr.exclude_kernel = 1;
	perf->attr.exclude_hv     = 1;
	perf->attr.read_format    = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING | gp_flag;

	perf->fd = syscall(__NR_perf_event_open, &perf->attr, pid, -1, gp_fd, 0);

	#ifdef SHOW_DEBUGS
	if (perf->fd == -1) {
		debug("event %x returned fd %d (no: %d, str: %s)", event, perf->fd, errno, strerror(errno));
	} else {
		debug("ok event %x, group %p and fd %d", event, group, perf->fd);
	}
	#endif
	if (perf->fd == -1) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	return EAR_SUCCESS;
}

state_t perf_close(perf_t *perf)
{
	if (perf->fd >= 0) {
		close(perf->fd);
	}
	memset(perf, 0, sizeof(perf_t));
	return EAR_SUCCESS;
}

state_t perf_reset(perf_t *perf)
{
	int gp_flag = 0;
	int ret;
	if (perf->group != NULL) {
		gp_flag = PERF_IOC_FLAG_GROUP;
	}
	ret = ioctl(perf->fd, PERF_EVENT_IOC_RESET, gp_flag);
	#ifdef SHOW_DEBUGS
	if (ret == -1) {
		debug("ioctl of fd %d returned %d (no: %d, s: %s)", perf->fd, ret, errno, strerror(errno));
	} else {
		debug("ok fd %d and group %p", perf->fd, perf->group);
	}
	#else
	(void) (ret);
	#endif
	if (perf->fd == -1) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	return EAR_SUCCESS;
}

state_t perf_start(perf_t *perf)
{
	int gp_flag = 0;
	int ret;
	if (perf->group != NULL) {
		gp_flag = PERF_IOC_FLAG_GROUP;
	}
	ret = ioctl(perf->fd, PERF_EVENT_IOC_ENABLE, gp_flag);
	#ifdef SHOW_DEBUGS
	if (ret == -1) {
		debug("ioctl of fd %d returned %d (no: %d, s: %s)", perf->fd, ret, errno, strerror(errno));
	} else {
		debug("ok fd %d and group %p", perf->fd, perf->group);
	}
	#else
	(void) (ret);
	#endif
	if (perf->fd == -1) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	return EAR_SUCCESS;
}

state_t perf_stop(perf_t *perf)
{
	int gp_flag = 0;
	int ret;
	if (perf->group != NULL) {
		gp_flag = PERF_IOC_FLAG_GROUP;
	}
	ret = ioctl(perf->fd, PERF_EVENT_IOC_DISABLE, gp_flag);
	#ifdef SHOW_DEBUGS
	if (ret == -1) {
		debug("ioctl of fd %d returned %d (no: %d, s: %s)", perf->fd, ret, errno, strerror(errno));
	} else {
		debug("ok fd %d and group %p", perf->fd, perf->group);
	}
	#else
	(void) (ret);
	#endif
	if (perf->fd == -1) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	return EAR_SUCCESS;
}

state_t perf_read(perf_t *perf, llong *value)
{
	struct read_format_s {
		ullong nrval;
		ullong time_enabled;
		ullong time_running;
		ullong values[8];
	} value_s;
	
	double time_act = 0.0;
	double time_run = 0.0;
	double time_mul = 0.0;
	double value_d;
	double total_d;
	int ret;
	int i;	

	memset(&value_s, 0, sizeof(struct read_format_s));
	ret = read(perf->fd, &value_s, sizeof(struct read_format_s));

	#ifdef SHOW_DEBUGS
	if (ret == -1) {
		debug("error when reading %d (no: %d)", ret, errno);
	}
	#endif
	if (ret == -1) {
		return_msg(EAR_ERROR, strerror(errno));
	}

	if (value_s.time_running > 0) {
		time_act = (double) value_s.time_enabled;
		time_run = (double) value_s.time_running;
		time_mul = time_act / time_run;
	}

	debug("time act/run/mul %lf/%lf/%lf (group %p)",
		time_act, time_run, time_mul, perf->group);

	if (perf->group == NULL)
	{
		value_d = (double) value_s.nrval;
		total_d = value_d * time_mul;
		*value  = (llong) total_d;
	}
	else for (i = 0; i < value_s.nrval && i < 8; ++i)
	{
		value_d  = (double) value_s.values[i];
		total_d  = value_d * time_mul;
		value[i] = (llong) total_d;
		debug("i=%d value %llu\n",i,value_s.values[i]);
	}

	return EAR_SUCCESS;
}
