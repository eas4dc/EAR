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

// #define SHOW_DEBUGS 1

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <common/system/file.h>
#include <common/output/verbose.h>
#include <common/output/debug.h>

static struct flock lock;

int file_lock(int fd)
{
	if (fd>=0){
		lock.l_type = F_WRLCK;
		return fcntl(fd, F_SETLKW, &lock);
	}else return -1;
}

int file_lock_timeout(int fd, uint timeout)
{
	uint tries = 0;
  while((file_lock(fd) < 0 ) && (tries < timeout)) tries++;
  if (tries >= timeout) return 0;
	return 1;	
}

int file_lock_create(char *lock_file_name)
{
	int fd = open(lock_file_name, O_WRONLY | O_CREAT, S_IWUSR);
	if (fd < 0) return fd;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;
	lock.l_pid = getpid();
	return fd;
}

int file_lock_master(char *lock_file_name)
{
	int fd=open(lock_file_name,O_WRONLY|O_CREAT|O_EXCL,S_IWUSR);
	return fd;
}

void file_lock_clean(int fd,char *lock_file_name)
{
	if (fd>=0){
		close(fd);
		unlink(lock_file_name);
	}
}

int file_unlock(int fd)
{
	if (fd>=0){
		lock.l_type = F_UNLCK;
		return fcntl(fd, F_SETLKW, &lock);
	}else return -1;
}
int file_unlock_master(int fd,char *lock_file_name)
{
	close(fd);
	if (unlink(lock_file_name) < 0){
		return errno;	
	}
	return 0;
}

int file_is_regular(const char *path)
{
	struct stat path_stat;
	stat(path, &path_stat);

	return S_ISREG(path_stat.st_mode);
}

int file_is_directory(const char *path)
{
	return 0;
}

ssize_t ear_file_size(char *path)
{
	int fd = open(path, O_RDONLY);
	ssize_t size;

	if (fd < 0){
		state_return_msg((ssize_t) EAR_OPEN_ERROR, errno, strerror(errno));
	}

	size = lseek(fd, 0, SEEK_END);
	close(fd);

	return size;
}

state_t ear_file_read(const char *path, char *buffer, size_t size)
{
	int fd = open(path, O_RDONLY);
	ssize_t r;
	ssize_t totalr = 0;

	if (fd < 0) {
		state_return_msg(EAR_OPEN_ERROR, errno, strerror(errno));
	}

	while ((size > 0) && ((r = read(fd, &buffer[totalr], size)) > 0)){
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
	int fd = open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	ssize_t w;
	ssize_t totalw = 0;

	if (fd < 0) {
		state_return_msg(EAR_OPEN_ERROR, errno, strerror(errno));
	}

	while ((size > 0) && ((w = write(fd, &buffer[totalw], size)) > 0)) {
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
