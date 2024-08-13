/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _FILE_H
#define _FILE_H

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <common/sizes.h>
#include <common/states.h>

#define PERMS000(T) 0
#define PERMS100(T) S_IR ## T
#define PERMS010(T) S_IW ## T
#define PERMS001(T) S_IX ## T
#define PERMS110(T) (PERMS100(T)) | (PERMS010(T))
#define PERMS111(T) (PERMS100(T)) | (PERMS010(T)) | (PERMS001(T))
#define PEXPA(...) __VA_ARGS__
#define PERMS(u, g, o) PEXPA(PERMS ## u)(USR) | PEXPA(PERMS ## g)(GRP) | PEXPA(PERMS ## o)(OTH)

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

/* Tries to acquire the lock with file_lock with a max a attemps = tiemoput, expressed as loop iterations, not time . Returns true when the lock has been acquired*/
int file_lock_timeout(int fd, ulong timeout);

/** Creates a file to be used  as lock. It doesn't locks the file */
int file_lock_create(char *lock_file_name);

/** Opens a file with O_EXCL, only one process per node will do that */
int file_lock_master(char *lock_file_name);
int file_lock_master_perm(char *lock_file_name, int flag, mode_t mode);

/** Closes and removes the lock file */
void file_lock_clean(int fd,char *lock_file_name);

/** Unlocks the file */
int file_unlock(int fd);

/** Releases a lock file */
int file_unlock_master(int fd,char *lock_file_name);

/** */
int file_is_regular(const char *path);

/** */
int file_is_directory(const char *path);

/** */
ssize_t ear_file_size(char *path);

/** */
state_t ear_file_read(const char *path, char *buffer, size_t size);

/** */
state_t ear_file_write(const char *path, const char *buffer, size_t size);

/** */
state_t ear_file_clean(const char *path);


/* Changes the limit of files open , use ULONG_MAX to set the maximum*/
void   set_no_files_limit(ulong new_limit);


/* Returns the current limit of files open */
ulong  get_no_files_limit();


/* Returns the current limit of stack size */
ulong get_stack_size_limit();


/* Changes the limit of stack size, use ULONG_MAX to set the maximum*/
void  set_stack_size_limit(ulong new_limit);

#endif
