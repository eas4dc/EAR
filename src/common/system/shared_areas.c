/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <common/config.h>
#include <common/states.h>
#include <common/output/debug.h>
#include <common/system/shared_areas.h>
#include <common/system/file.h>
#include <common/system/user.h>



/** BASIC FUNCTIONS */


#define SEC_FLAGS O_NOFOLLOW | O_EXCL | O_CLOEXEC

/**
 * create_shared_area:
 * Creates a file on disk to be used as shared memory, initializes it with data,
 * and maps it into the process address space.
 *
 * @path: Full path of the file to create (including directory and filename).
 * @file_perms_bits: Permissions for the created file.
 * @data: Initial data to write to the shared area.
 * @area_size: Size of the shared area in bytes.
 * @shared_fd: Output pointer to store the file descriptor of the created file.
 * @must_clean: If true, the data area will be zeroed out before writing.
 * @user: If not NULL, the file ownership will be changed to this user.
 * @return: Pointer to the mapped shared memory area, or NULL on error.
 */
void *create_shared_area(char *path, mode_t file_perms_bits, char *data, int area_size, int *shared_fd, int must_clean,
                         char *user)
{
    // Basic argument validation
	if (!area_size || !data || !path)
	{
		debug("creating shared region, invalid arguments. Not created\n");
		return NULL;
	}

	int ret;
	void *my_shared_region = NULL;		
	int fd;
    char dir_buf[MAX_PATH_SIZE];
    char file_buf[MAX_PATH_SIZE];
    char *p;

    // Save current umask and set to 0 to ensure exact permissions requested

	mode_t my_mask;
	my_mask = umask(0);

    /* Split the path into directory (dir_buf) and filename (file_buf).
     * This implementation avoids modifying the original path string. */
    strncpy(dir_buf, path, sizeof(dir_buf) - 1);
    dir_buf[sizeof(dir_buf) - 1] = '\0';

    p = strrchr(dir_buf, '/');
    if (p) {
        if (p == dir_buf) { // Path is in root directory (e.g., "/file")
            strcpy(dir_buf, "/");
            strncpy(file_buf, path + 1, sizeof(file_buf) - 1);
        } else {
            *p = '\0'; // Truncate dir_buf to get the directory part
            // Recover filename from the original path after the last slash
            strncpy(file_buf, strrchr(path, '/') + 1, sizeof(file_buf) - 1);
        }
    } else {
        // No slash found: current directory
        strcpy(dir_buf, ".");
        strncpy(file_buf, path, sizeof(file_buf) - 1);
    }
    file_buf[sizeof(file_buf) - 1] = '\0';

    debug("creating file (%s) in dir (%s) for shared memory", file_buf, dir_buf);

    /* Open the parent directory first to use openat().
     * This is more secure against certain race conditions (TOCTOU). */
    int dir_fd = open(dir_buf, O_RDONLY | O_DIRECTORY);
    if (dir_fd < 0) {
        debug("error opening directory %s (%s)\n", dir_buf, strerror(errno));
        umask(my_mask);
        return NULL;
    }

    // Perform security checks on the parent directory
    struct stat dir_st;
    if (fstat(dir_fd, &dir_st) < 0) {
        debug("error stating directory %s (%s)\n", dir_buf, strerror(errno));
        close(dir_fd);
        umask(my_mask);
        return NULL;
    }

    /* Unlink any existing file to ensure we create a new one safely.
     * This avoids the "File exists" error when O_EXCL is used and debris remains. */
    unlinkat(dir_fd, file_buf, 0);

    /* Create the file using openat() relative to the directory file descriptor.
     * SEC_FLAGS include O_NOFOLLOW | O_EXCL to prevent following symlinks. */
    fd = openat(dir_fd, file_buf, O_CREAT | O_RDWR | O_TRUNC | SEC_FLAGS, file_perms_bits);
    close(dir_fd); // Directory is no longer needed

	if (fd < 0)
	{
        error("Error creating sharing memory (%s)\n", strerror(errno));
		umask(my_mask);
		return NULL;
	}

    // Change file ownership if a user was specified
    if (user != NULL) {
        /* ear_chown_fd sets both owner and group to the user's primary group.
         * Using the file descriptor (fchown variant) is safer. */
        if (state_fail(ear_chown_fd(fd, user))) {
            if (user_is_root()) {
                error("Failed to change owner of shared file to %s", user);
                close(fd);
                unlink(path);
                umask(my_mask);
                return NULL;
            } else {
                warning("Failed to change owner of shared file to %s (not root). Granting world access.", user);
                /* If we cannot change ownership (non-root), we must ensure the file
                 * is accessible by others. We add read/write for group and others. */
                file_perms_bits |= (S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
            }
        }
    }

    /* Ensure the requested permissions are set.
     * Use fchmod on the file descriptor for safety. */
    if (fchmod(fd, file_perms_bits) < 0) {
        error("Error setting permissions for %s (%s)", path, strerror(errno));
        close(fd);
        unlink(path);
        umask(my_mask);
        return NULL;
    }
	debug("shared file for mmap created with fd=%d", fd);

    // Restore the original umask
	umask(my_mask);


    // Optionally zero-out the data buffer before writing to the file
  if (must_clean) bzero(data, area_size);

    // Initialize the file with the provided data
  debug("writting default values");
  ret = write(fd, data, area_size);

  if (ret<0)
  {
        error("Error writing default values to shared memory (%s)\n", strerror(errno));
    close(fd);
    return NULL;
  }

    // Map the file into memory to allow shared access
    debug("Mapping shared memory");
	my_shared_region= mmap(NULL, area_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);                                     
	if (my_shared_region == MAP_FAILED || my_shared_region == NULL)
	{
        error("Error mapping shared memory (%s)\n", strerror(errno));
		close(fd);
		return NULL;
	}

	*shared_fd = fd;
	return my_shared_region;
}


void *attach_shared_area(char *path, int area_size, uint perm, int *shared_fd, int *s)
{
    void *my_shared_region = NULL;
		char buff[MAX_PATH_SIZE];
		int flags;
		int size;
		int fd;

		strncpy(buff, path, sizeof(buff) - 1);
    fd = open(buff, perm);
    *shared_fd = fd;

    if (fd < 0)
		{
#if SHOW_DEBUGS
			error("error attaching to shared memory at (%s): %s", buff, strerror(errno));
  		char test[2048];
  		sprintf(test,"ls -la %s", buff);
  		system(test);

#endif
				return NULL;
    }
		fsync(fd);
	debug("File for shared region %s opened", path);
	if (area_size==0){
		size=lseek(fd,0,SEEK_END);
		if (size<0){
			debug("Error computing shared memory size (%s) ",strerror(errno));
		}else area_size=size;
		if (s!=NULL) *s=size;
	}
	if (perm==O_RDWR){
		flags=PROT_READ|PROT_WRITE;
	}else{
		flags=PROT_READ;
	}
    my_shared_region= mmap(NULL, area_size,flags, MAP_SHARED, fd, 0);
    if ((my_shared_region == MAP_FAILED) || (my_shared_region == NULL)){
        debug("error attaching to sharing memory (%s)\n",strerror(errno));
        close(fd);
        return NULL;
    }
	debug("File for shared region %s mapped at %p", path, my_shared_region);
	*shared_fd=fd;
    return my_shared_region;
}

void dettach_shared_area(int fd)
{
	close(fd);
}
void dispose_shared_area(char *path,int fd)
{
	close(fd);
	unlink(path);
}

