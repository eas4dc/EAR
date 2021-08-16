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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <common/system/file.h>
#include <common/output/verbose.h>

static struct flock lock;

int file_lock(int fd)
{
	if (fd>=0){
		lock.l_type = F_WRLCK;
		return fcntl(fd, F_SETLKW, &lock);
	}else return -1;
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

void file_unlock_master(int fd,char *lock_file_name)
{
	close(fd);
	unlink(lock_file_name);
}

int file_is_regular(const char *path)
{
	struct stat path_stat;
	int s = stat(path, &path_stat);
fprintf(stderr, "LOOKING AT %s => %d (%d)\n", path, S_ISREG(path_stat.st_mode), s);

	return S_ISREG(path_stat.st_mode);
}

int file_is_directory(const char *path)
{
	return 0;
}

ssize_t file_size(char *path)
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

state_t file_read(const char *path, char *buffer, size_t size)
{
	int fd = open(path, O_RDONLY);
	ssize_t r;

	if (fd < 0) {
		state_return_msg(EAR_OPEN_ERROR, errno, strerror(errno));
	}

	while ((r = read(fd, buffer, size)) > 0){
		size = size - r;
	}

	close(fd);

	if (r < 0) {
		state_return_msg(EAR_READ_ERROR, errno, strerror(errno));
	}

	state_return(EAR_SUCCESS);
}

state_t file_write(const char *path, const char *buffer, size_t size)
{
	int fd = open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	ssize_t w;

	if (fd < 0) {
		state_return_msg(EAR_OPEN_ERROR, errno, strerror(errno));
	}

	while ((w = write(fd, buffer, size)) > 0) {
		size = size - w;
	}

	close(fd);

	if (w < 0) {
		state_return_msg(EAR_SYSCALL_ERROR, errno, strerror(errno));
	}

	state_return(EAR_SUCCESS);
}

state_t file_clean(const char *path)
{
	if (remove(path) != 0) {
		state_return_msg(EAR_ERROR, errno, strerror(errno));
	}
	state_return(EAR_SUCCESS);
}
