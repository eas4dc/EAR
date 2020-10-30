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

#ifndef _FILE_H
#define _FILE_H

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <common/sizes.h>
#include <common/states.h>

#define FC_MAX_OBJECTS	10
#define F_WR 			O_WRONLY
#define F_RD			O_RDONLY
#define F_CR 			O_CREAT
#define F_TR			O_TRUNC
#define F_UR 			S_IRUSR
#define F_UW 			S_IWUSR
#define F_UX 			S_IXUSR
#define F_GR 			S_IRGRP
#define F_GW 			S_IWGRP
#define F_GX 			S_IXGRP
#define F_OR 			S_IROTH
#define F_OW 			S_IWOTH
#define F_OX 			S_IXOTH

/** Locks the file for writting in a fixe dpart */
int file_lock(int fd);

/** Creates a file to be used  as lock. It doesn't locks the file */
int file_lock_create(char *lock_file_name);

/** Opens a file with O_EXCL, only one process per node will do that */
int file_lock_master(char *lock_file_name);

/** Closes and removes the lock file */
void file_lock_clean(int fd,char *lock_file_name);

/** Unlocks the file */
int file_unlock(int fd);

/** Releases a lock file */
void file_unlock_master(int fd,char *lock_file_name);

/** */
int file_is_regular(const char *path);

/** */
int file_is_directory(const char *path);

/** */
ssize_t file_size(char *path);

/** */
state_t file_read(const char *path, char *buffer, size_t size);

/** */
state_t file_write(const char *path, const char *buffer, size_t size);

/** */
state_t file_clean(const char *path);

#endif