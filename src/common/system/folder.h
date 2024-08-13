/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_PRIVATE_FOLDER_H
#define EAR_PRIVATE_FOLDER_H

#include <dirent.h>
#include <sys/types.h>
#include <common/sizes.h>
#include <common/states.h>

typedef struct folder {
	char file_name[SZ_NAME_LARGE];
	DIR *dir;
} folder_t;

state_t folder_open(folder_t *folder, char *path);

state_t folder_close(folder_t *folder);

char *folder_getnext(folder_t *folder, char *prefix, char *suffix);
char *folder_getnextdir(folder_t *folder, char *prefix, char *suffix);

/* Looks for files/folders for a given type. If type is DT_UNKNOWN all types are considered */
char *folder_getnext_type(folder_t *folder, char *prefix, char *suffix, uint type);

state_t folder_remove(char *path);

state_t folder_rename(char *oldp, char *newp);

#endif //EAR_PRIVATE_FOLDER_H
