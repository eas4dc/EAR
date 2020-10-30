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

#endif //EAR_PRIVATE_FOLDER_H
