/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/resource.h>

#include <common/system/file.h>
#include <common/output/verbose.h>
#include <common/output/debug.h>

static struct flock lock;

int ear_file_lock(int fd)
{
	if (fd >= 0) {
		lock.l_type = F_WRLCK;
		return fcntl(fd, F_SETLKW, &lock);
	} else
		return -1;
}

int ear_file_trylock(int fd)
{
	if (fd >= 0) {
		lock.l_type = F_WRLCK;
		return fcntl(fd, F_SETLK, &lock);
	} else
		return -1;
}

int ear_file_lock_timeout(int fd, ulong timeout)
{
	uint tries = 0;
	while ((ear_file_trylock(fd) < 0) && (tries < timeout))
		tries++;
	if (tries >= timeout)
		return 0;
	return 1;
}

int ear_file_lock_create(char *lock_file_name)
{
	int fd = open(lock_file_name, O_WRONLY | O_CREAT | O_CLOEXEC, S_IWUSR);
	if (fd < 0)
		return fd;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;
	lock.l_pid = getpid();
	return fd;
}

int ear_file_lock_master_perm(char *lock_file_name, int flag, mode_t mode)
{
	int fd = open(lock_file_name, flag | O_CREAT | O_EXCL, mode);
	return fd;
}

int ear_file_lock_master(char *lock_file_name)
{
	return ear_file_lock_master_perm(lock_file_name, O_WRONLY, S_IWUSR);
}

void ear_file_lock_clean(int fd, char *lock_file_name)
{
	if (fd >= 0) {
		close(fd);
		unlink(lock_file_name);
	}
}

int ear_file_unlock(int fd)
{
	if (fd >= 0) {
		lock.l_type = F_UNLCK;
		return fcntl(fd, F_SETLKW, &lock);
	} else
		return -1;
}

int ear_file_unlock_master(int fd, char *lock_file_name)
{
	close(fd);
	if (unlink(lock_file_name) < 0) {
		return errno;
	}
	return 0;
}

int ear_file_exists(const char *path)
{
	struct stat path_stat;
	return (stat(path, &path_stat) == 0);
}

int ear_file_is_regular(const char *path)
{
	struct stat path_stat;
	if (stat(path, &path_stat) < 0)
		return 0;

	return S_ISREG(path_stat.st_mode);
}

int ear_file_is_directory(const char *path)
{
	struct stat buff;
	if (stat(path, &buff) < 0)
		return 0;
	return S_ISDIR(buff.st_mode);
}

ssize_t ear_file_size(char *path)
{
	int fd = open(path, O_RDONLY);
	ssize_t size;

	if (fd < 0) {
		state_return_msg((ssize_t) EAR_OPEN_ERROR, errno,
				 strerror(errno));
	}

	size = lseek(fd, 0, SEEK_END);
	close(fd);

	return size;
}

state_t ear_file_read(const char *path, char *buffer, size_t size)
{
	int fd = open(path, O_RDONLY);
	ssize_t r = 0;
	ssize_t totalr = 0;

	if (fd < 0) {
		state_return_msg(EAR_OPEN_ERROR, errno, strerror(errno));
	}

	while ((size > 0) && ((r = read(fd, &buffer[totalr], size)) >= 0)) {
		size = size - r;
		totalr += r;
	}

	close(fd);

	if (r < 0) {
		state_return_msg(EAR_READ_ERROR, errno, strerror(errno));
	}

	state_return(EAR_SUCCESS);
}

state_t ear_file_write(const char *path, const char *buffer, size_t size)
{
	int fd =
	    open(path, O_WRONLY | O_CREAT,
		 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	ssize_t w = 0;
	ssize_t totalw = 0;

	if (fd < 0) {
		state_return_msg(EAR_OPEN_ERROR, errno, strerror(errno));
	}

	while ((size > 0) && ((w = write(fd, &buffer[totalw], size)) >= 0)) {
		size = size - w;
		totalw += w;
	}

	close(fd);

	if (w < 0) {
		state_return_msg(EAR_SYSCALL_ERROR, errno, strerror(errno));
	}

	state_return(EAR_SUCCESS);
}

state_t ear_file_clean(const char *path)
{
	if (remove(path) != 0) {
		state_return_msg(EAR_ERROR, errno, strerror(errno));
	}
	state_return(EAR_SUCCESS);
}

void set_no_files_limit(ulong new_limit)
{
	/** Change the open file limit */
	struct rlimit rl;
	getrlimit(RLIMIT_NOFILE, &rl);
	debug("current limit %lu max %lu", rl.rlim_cur, rl.rlim_max);
	rl.rlim_cur = ear_min(rl.rlim_max, new_limit);
	debug("NEW NO file limit %lu", rl.rlim_cur);
	setrlimit(RLIMIT_NOFILE, &rl);
}

ulong get_no_files_limit()
{
	struct rlimit rl;
	getrlimit(RLIMIT_NOFILE, &rl);
	return rl.rlim_cur;
}

void set_stack_size_limit(ulong new_limit)
{

	/** Change the open file limit */
	struct rlimit rl;
	getrlimit(RLIMIT_STACK, &rl);
	debug("current limit %lu max %lu", rl.rlim_cur, rl.rlim_max);
	rl.rlim_cur = ear_min(rl.rlim_max, new_limit);
	debug("NEW STACK limit %lu", rl.rlim_cur);
	setrlimit(RLIMIT_STACK, &rl);
}

ulong get_stack_size_limit()
{
	struct rlimit rl;
	getrlimit(RLIMIT_STACK, &rl);
	return rl.rlim_cur;
}

state_t ear_fd_read(int fd, char *buffer, size_t size)
{
	ssize_t r = 0;
	ssize_t totalr = 0;

	if (fd < 0) {
		state_return_msg(EAR_OPEN_ERROR, errno, strerror(errno));
	}

	while ((size > 0) && ((r = read(fd, &buffer[totalr], size)) > 0)){
		size = size - r;
		totalr += r;
	}


	if (r < 0) {
		state_return_msg(EAR_READ_ERROR, errno, strerror(errno));
	}

	state_return(EAR_SUCCESS);
}

state_t ear_fd_write(int fd, const char *buffer, size_t size)
{
	ssize_t w = 0;
	ssize_t totalw = 0;

	if (fd < 0) {
		state_return_msg(EAR_OPEN_ERROR, errno, strerror(errno));
	}

	while ((size > 0) && ((w = write(fd, &buffer[totalw], size)) > 0)) {
		size = size - w;
		totalw += w;
	}


	if (w < 0) {
		state_return_msg(EAR_SYSCALL_ERROR, errno, strerror(errno));
	}

	state_return(EAR_SUCCESS);
}


