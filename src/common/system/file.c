/**************************************************************
*   Energy Aware Runtime (EAR)
*   This program is part of the Energy Aware Runtime (EAR).
*
*   EAR provides a dynamic, transparent and ligth-weigth solution for
*   Energy management.
*
*       It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*   BSC Contact     mailto:ear-support@bsc.es
*   Lenovo contact  mailto:hpchelp@lenovo.com
*
*   EAR is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Lesser General Public
*   License as published by the Free Software Foundation; either
*   version 2.1 of the License, or (at your option) any later version.
*
*   EAR is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Lesser General Public License for more details.
*
*   You should have received a copy of the GNU Lesser General Public
*   License along with EAR; if not, write to the Free Software
*   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*   The GNU LEsser General Public License is contained in the file COPYING
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
	stat(path, &path_stat);
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
		state_return_msg(EAR_WRITE_ERROR, errno, strerror(errno));
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
