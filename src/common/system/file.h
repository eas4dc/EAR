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

#include <common/sizes.h>
#include <common/states.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PERMS000(T)    0
#define PERMS100(T)    S_IR##T
#define PERMS010(T)    S_IW##T
#define PERMS001(T)    S_IX##T
#define PERMS110(T)    (PERMS100(T)) | (PERMS010(T))
#define PERMS111(T)    (PERMS100(T)) | (PERMS010(T)) | (PERMS001(T))
#define PEXPA(...)     __VA_ARGS__
#define PERMS(u, g, o) PEXPA(PERMS##u)(USR) | PEXPA(PERMS##g)(GRP) | PEXPA(PERMS##o)(OTH)

#define F_WR           O_WRONLY
#define F_RD           O_RDONLY
#define F_CR           O_CREAT
#define F_TR           O_TRUNC
#define F_UR           S_IRUSR
#define F_UW           S_IWUSR
#define F_UX           S_IXUSR
#define F_GR           S_IRGRP
#define F_GW           S_IWGRP
#define F_GX           S_IXGRP
#define F_OR           S_IROTH
#define F_OW           S_IWOTH
#define F_OX           S_IXOTH

/** Locks the file for writting in a fixed part. */
int ear_file_lock(int fd);

/** Tries to acquire the lock through the file descriptor `fd`.
 * If the lock is hold by another process, the function returns immediately and errno can be read.
 * \return 0 The lock was acquired.
 * \return -1 Otherwise. */
int ear_file_trylock(int fd);

/** Tries to acquire the lock with file_trylock with a max a attemps = tiemoput, expressed as loop iterations, not time.
 * Returns true when the lock has been acquired. */
int ear_file_lock_timeout(int fd, ulong timeout);

/** Creates a file to be used  as lock. It doesn't locks the file */
int ear_file_lock_create(char *lock_file_name);

/** Opens a file with O_EXCL, only one process per node will do that */
int ear_file_lock_master(char *lock_file_name);
int ear_file_lock_master_perm(char *lock_file_name, int flag, mode_t mode);

/** Closes and removes the lock file */
void ear_file_lock_clean(int fd, char *lock_file_name);

/** Unlocks the file */
int ear_file_unlock(int fd);

/** Releases a lock file */
int ear_file_unlock_master(int fd, char *lock_file_name);

/** Returns whether the file pointed by \p path exsits. */
int ear_file_exists(const char *path);

/** */
int ear_file_is_regular(const char *path);

/** */
int ear_file_is_directory(const char *path);

/** */
ssize_t ear_file_size(char *path);

/** This function reads at most up to \p size bytes from the file pointed by \p
 * path, and stores them in the memory region pointed by \p buffer. It is up to
 * the caller to allocate at least \p size bytes at \p buffer. The \p wait_for_bytes
 * parameter change the mode of operation of this funciton.
 *
 * \param wait_for_bytes This boolean parameter is used to determine whether the
 * function must read exactly \p size bytes. If false, it will try to read at
 * most \p size bytes while the read(2) function still reads bytes.
 *
 * \return EAR_OPEN_ERROR open(3) function returned an error.
 * \return EAR_READ_ERROR read(2) function returned an error.
 * \return EAR_SUCCESS Otherwise.
 */
state_t ear_file_read(const char *path, char *buffer, size_t size, int wait_for_bytes);

/** */
state_t ear_file_write(const char *path, const char *buffer, size_t size);

/** */
state_t ear_file_clean(const char *path);

/** Changes the limit of files open , use ULONG_MAX to set the maximum */
void set_no_files_limit(ulong new_limit);

/** Returns the current limit of files open */
ulong get_no_files_limit();

/** Returns the current limit of stack size */
ulong get_stack_size_limit();

/** Changes the limit of stack size, use ULONG_MAX to set the maximum*/
void set_stack_size_limit(ulong new_limit);
state_t ear_fd_read(int fd, char *buffer, size_t size);
state_t ear_fd_write(int fd, const char *buffer, size_t size);

#endif
