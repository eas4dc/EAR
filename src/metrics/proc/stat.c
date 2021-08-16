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

//#define SHOW_DEBUGS 1

#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <metrics/proc/stat.h>

#define sassert(func) \
	if (state_fail(s = func)) { \
        error("returned %d, %s (%s:%d)", s, state_msg, __FILE__, __LINE__); \
		return 0; \
    }

typedef struct psctx_s {
	pid_t pid;
	int fd;
} psctx_t;

static uint mode;

state_t kernel_version(uint *kernel, uint *major, uint *minor, uint *patch)
{
	struct utsname buffer;
	uint ver[4];
	char *p;
	int i;

	if (uname(&buffer) != 0) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	p = buffer.release;
	i = 0;
	//
	while (*p) {
		if (isdigit(*p)) {
			ver[i] = (uint) strtol(p, &p, 10);
			i++;
		} else {
			p++;
		}
	}
	*kernel = ver[0];
	*major  = ver[1];
	*minor  = ver[2];
	*patch  = ver[3];

	return EAR_SUCCESS;
}

state_t proc_stat_load()
{
	uint kernel;
	uint major;
	uint minor;
	uint patch;
	state_t s;

	if (mode > 0) {
		return EAR_SUCCESS;
	}
	if (state_fail(s = kernel_version(&kernel, &major, &minor, &patch))) {
		return s;
	}
	debug("Detected kernel version %u.%u.%u.%u", kernel, major, minor, patch);
	// 2.6.24
	if ((kernel < 3) || (kernel == 3 && major < 3)) {
		mode = 1;
	}
	// 3.3
	if((kernel == 3 && major > 2)) {
		mode = 2;
	}
	// > 3.5
	if((kernel > 3 && major > 3)) {
		mode = 3;
	}
	if (mode == 0) {
		return_msg(EAR_ERROR, "Incompatble kernel version");
	}
	return EAR_SUCCESS;
}

static state_t get_context(ctx_t *c, psctx_t **p)
{
	if (c == NULL || c->context == NULL) {
		return_msg(EAR_ERROR, Generr.context_null);
	}
	*p = (psctx_t *) c->context;
	return EAR_SUCCESS;
}

state_t proc_stat_init(ctx_t *c, pid_t pid)
{
	char buffer[SZ_PATH];
	psctx_t *p;
	state_t s;

	//
	c->context = calloc(1, sizeof(psctx_t));
	if (state_fail(s = get_context(c, &p))) {
		return s;
	}
	//
	sprintf(buffer, "/proc/%d/stat", pid);
	if ((p->fd = open(buffer, O_RDONLY)) < 0) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	debug("Opening '%s' process file", buffer);
	p->pid = pid;

	return EAR_SUCCESS;
}

state_t proc_stat_dispose(ctx_t *c)
{
	psctx_t *p;
	state_t s;

	if (state_fail(s = get_context(c, &p))) {
		return s;
	}
	close(p->fd);
	free(c->context);
	c->context = NULL;
	return EAR_SUCCESS;
}

static state_t scan_buffer(char *tok, proc_stat_t *ps)
{
	int ret;
	debug("processing %lu bytes %s",strlen(tok),tok);
	if (mode == 1) {
	ret = sscanf(tok, " %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld" \
		"%ld %llu %lu %ld %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu" \
		"%d %d %u %u %llu %lu %ld",
		&ps->state, &ps->ppid, &ps->pgrp, &ps->session, &ps->tty_nr, &ps->tpgid,
		&ps->flags, &ps->minflt, &ps->cminflt, &ps->majflt, &ps->cmajflt, &ps->utime, &ps->stime,
		&ps->cutime, &ps->cstime, &ps->priority, &ps->nice, &ps->num_threads,
		&ps->itrealvalue, &ps->starttime, &ps->vsize, &ps->rss, &ps->rsslim,
		&ps->startcode, &ps->endcode, &ps->startstack, &ps->kstkesp, &ps->kstkeip,
		&ps->signal, &ps->blocked, &ps->sigignore, &ps->sigcatch, &ps->wchan,
		&ps->nswap, &ps->cnswap, &ps->exit_signal, &ps->processor, &ps->rt_priority,
		&ps->policy, &ps->delayacct_blkio_ticks, &ps->guest_time, &ps->cguest_time);
	}
	if (mode == 2) {
	ret = sscanf(tok, " %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld" \
		"%ld %llu %lu %ld %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu" \
		"%d %d %u %u %llu %lu %ld %lu %lu %lu",
		&ps->state, &ps->ppid, &ps->pgrp, &ps->session, &ps->tty_nr, &ps->tpgid,
		&ps->flags, &ps->minflt, &ps->cminflt, &ps->majflt, &ps->cmajflt, &ps->utime, &ps->stime,
		&ps->cutime, &ps->cstime, &ps->priority, &ps->nice, &ps->num_threads,
		&ps->itrealvalue, &ps->starttime, &ps->vsize, &ps->rss, &ps->rsslim,
		&ps->startcode, &ps->endcode, &ps->startstack, &ps->kstkesp, &ps->kstkeip,
		&ps->signal, &ps->blocked, &ps->sigignore, &ps->sigcatch, &ps->wchan,
		&ps->nswap, &ps->cnswap, &ps->exit_signal, &ps->processor, &ps->rt_priority,
		&ps->policy, &ps->delayacct_blkio_ticks, &ps->guest_time, &ps->cguest_time,
		&ps->start_data, &ps->end_data, &ps->start_brk);
	}
	if (mode == 3) {
	ret = sscanf(tok, " %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld" \
		"%ld %llu %lu %ld %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu" \
		"%d %d %u %u %llu %lu %ld %lu %lu %lu %lu %lu %lu %lu %d",
		&ps->state, &ps->ppid, &ps->pgrp, &ps->session, &ps->tty_nr, &ps->tpgid,
		&ps->flags, &ps->minflt, &ps->cminflt, &ps->majflt, &ps->cmajflt, &ps->utime, &ps->stime,
		&ps->cutime, &ps->cstime, &ps->priority, &ps->nice, &ps->num_threads,
		&ps->itrealvalue, &ps->starttime, &ps->vsize, &ps->rss, &ps->rsslim,
		&ps->startcode, &ps->endcode, &ps->startstack, &ps->kstkesp, &ps->kstkeip,
		&ps->signal, &ps->blocked, &ps->sigignore, &ps->sigcatch, &ps->wchan,
		&ps->nswap, &ps->cnswap, &ps->exit_signal, &ps->processor, &ps->rt_priority,
		&ps->policy, &ps->delayacct_blkio_ticks, &ps->guest_time, &ps->cguest_time,
		&ps->start_data, &ps->end_data, &ps->start_brk, &ps->arg_start, &ps->arg_end,
		&ps->env_start, &ps->env_end, &ps->exit_code);
	}
	debug("mode %d ret %d", mode, ret);
	(void) ret;
	
	return EAR_SUCCESS;
}

state_t proc_stat_read(ctx_t *c, proc_stat_t *ps)
{
	char buffer[SZ_BUFFER];
	ssize_t sread;
	size_t sbuff;
	off_t soff;
	psctx_t *p;
	state_t s;

	if (state_fail(s = get_context(c, &p))) {
		return s;
	}
	// Cleaning buffer
	memset(buffer, 0, SZ_BUFFER);
	// Reading
	sbuff = SZ_BUFFER;
	sread = 0;
	soff  = 0;
	do {
		sread  = pread(p->fd, buffer, sbuff, soff);
		sbuff -= sread;
		soff  += sread;
		debug("Read %ld bytes", sread);
	} while(sbuff > 0 && sread > 0 && buffer[soff-1] != '\n');
	debug("Read a total of %ld bytes", soff);
	//
	if (buffer[soff-1] != '\n') {
		debug("Buffer to short to read");
		return_msg(EAR_ERROR, "Buffer to short to read /proc/<pid>/stat");
	}
	buffer[soff-1] = '\0';
	// Destroying (name proc)
	char *tok = strtok(buffer, ")");
	sscanf(tok, "%d (%s", &ps->pid, ps->file);
	// Getting the rest of the file
	tok = strtok(NULL, ")");
	debug("READED %s", &tok[1]);
	if (state_fail(s = scan_buffer(&tok[1], ps))) {
		return s;
	}
	// proc_stat_data_print(ps);
	return EAR_SUCCESS;
}

state_t proc_stat_read_diff(ctx_t *c, proc_stat_t *ps2, proc_stat_t *ps1, proc_stat_t *psD)
{
	state_t s;
	if (state_fail(s = proc_stat_read(c, ps2))) {
		return s;
	}
	return proc_stat_data_diff(ps2, ps1, psD);
}

state_t proc_stat_read_copy(ctx_t *c, proc_stat_t *ps2, proc_stat_t *ps1, proc_stat_t *psD)
{
	state_t s;
	if (state_fail(s = proc_stat_read_diff(c, ps2, ps1, psD))) {
		return s;
	}
	return proc_stat_data_copy(ps1, ps2);
}

state_t proc_stat_read_now(ctx_t *c, proc_stat_t *ps1, proc_stat_t *psD)
{
	proc_stat_t ps2;
	return proc_stat_read_copy(c, &ps2, ps1, psD);
}

state_t proc_stat_data_diff(proc_stat_t *ps2, proc_stat_t *ps1, proc_stat_t *psD)
{
	proc_stat_data_copy(psD, ps2);

	psD->utime  = ps2->utime  - ps1->utime;
	psD->stime  = ps2->stime  - ps1->stime;
	psD->cutime = ps2->cutime - ps1->cutime;
	psD->cstime = ps2->cstime - ps1->cstime;

	return EAR_SUCCESS;
}

state_t proc_stat_data_copy(proc_stat_t *dest, proc_stat_t *src)
{
	memcpy(dest, src, sizeof(proc_stat_t));
	return EAR_SUCCESS;
}

state_t proc_stat_data_print(proc_stat_t *ps)
{
	printf("[ 1] pid:    %d\n", ps->pid);
	printf("[ 2] file:   %s\n", ps->file);
	printf("[ 3] state:  %c\n", ps->state);
	printf("[ 9] flags:  %u\n", ps->flags);
	printf("[14] utime:  %lu\n", ps->utime);
	printf("[15] stime:  %lu\n", ps->stime);
	printf("[16] cutime: %ld\n", ps->cutime);
	printf("[17] cstime: %ld\n", ps->cstime);
	printf("[19] nice:  %ld\n", ps->nice);
	printf("[24] rss:  %ld\n", ps->rss);
	printf("[28] startstack:  %lu\n", ps->startstack);
	printf("[31] signal:  %lu\n", ps->signal);
	printf("[39] processor:  %d\n", ps->processor);
	printf("[43] guest_time:  %lu\n", ps->guest_time);
	printf("[44] cguest_time: %ld\n", ps->cguest_time);
	return EAR_SUCCESS;
}

state_t proc_stat_data_to_str(proc_stat_t *ps,char *buffer,size_t len)
{
	snprintf(buffer,len,"(pid %d state %c utime %lu stime %lu cutime %ld cstime %ld)",ps->pid,ps->state,ps->utime,ps->stime,ps->cutime,ps->cstime);
	return EAR_SUCCESS;
}

#if 0
void fakefor()
{
	double a = 2.0;
	double b = 2.0;
	uint i = 0;

	for (; i < 1000000000; ++i) {
		a = a + (b * 3.0);
	}
}

int main(int argc, char *argv[])
{
	proc_stat_t ps1;
	proc_stat_t psD;
	state_t s;
	ctx_t c;

	sassert(proc_stat_load());
	sassert(proc_stat_init(&c, getpid()));
	sassert(proc_stat_read(&c, &ps1));

	//sleep(1);
	fakefor();
	sassert(proc_stat_read_now(&c, &ps1, &psD));

	//proc_stat_data_print(&psD);

	return 0;
}
#endif
