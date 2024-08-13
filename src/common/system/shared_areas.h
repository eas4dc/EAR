/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _SHARED_AREAS_H
#define _SHARED_AREAS_H

#include <sys/stat.h>


/** Creates a shared memory region based on files and mmap.
 *
 * \param[in] path The full file name to store data.
 * \param[in] file_perms_bits The file permission bits of the to-be created file.
 * \param[in] data The data to be stored in shared memory.
 * \param[in] area_size The size in bytes of \ref data.
 * \param[in] must_clean Specifies whether erasing \ref data before storing it.
 * \param[out] shared_fd The file descriptor of the created file.
 *
 * \return NULL on error.
 * \return the shared region pointer otherwise. */
void *create_shared_area(char *path, mode_t file_perms_bits, char *data, int area_size, int *shared_fd, int must_clean);


void * attach_shared_area(char *path,int area_size,uint perm,int *shared_fd,int *s);
void dettach_shared_area(int fd);
void dispose_shared_area(char *path,int fd);
#endif
