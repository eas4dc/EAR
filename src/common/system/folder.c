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

#include <stdio.h>
#include <string.h>
#include <common/system/folder.h>

state_t folder_open(folder_t *folder, char *path)
{
	if ((folder->dir = opendir(path)) == NULL) {
		return EAR_OPEN_ERROR;
	}

	return EAR_SUCCESS;
}

state_t folder_close(folder_t *folder)
{
	closedir(folder->dir);
	folder->dir = NULL;
	return EAR_SUCCESS;
}

char *folder_getnext(folder_t *folder, char *prefix, char *suffix)
{
	struct dirent *file;
	int lpre, lsuf, lfil;
	char *pfil;

	pfil = folder->file_name;
	lpre = (prefix != NULL) ? (int) strlen(prefix) : 0;
	lsuf = (suffix != NULL) ? (int) strlen(suffix) : 0;

	while ((file = readdir(folder->dir)) != NULL)
	{
		if (file->d_type == DT_DIR) {
			continue;
		}

		if (prefix != NULL)
		{
			if (strstr(file->d_name, prefix) != NULL) {
				strcpy(pfil, &file->d_name[lpre]);
			} else {
				continue;
			}
		} else {
			strcpy(pfil, &file->d_name[0]);
		}

		if ((lfil = (int) strlen(pfil)) <= lsuf) {
			continue;
		}

		if (suffix != NULL)
		{
			if (strcmp(&pfil[lfil - lsuf], suffix) == 0) {
				pfil[lfil - lsuf] = '\0';
				return pfil;
			} else {
				continue;
			}
		} else {
			return pfil;
		}
	}

	folder_close(folder);

	return NULL;
}

/*
folder_t f;

int main(int argc, char *argv[])
{
	state_t s;
	char *pre = NULL;
	char *suf = NULL;
	char *file;

	s = folder_open(&f, argv[1]);

	if (state_fail(s)) {
		printf("error\n");
	}

	if (argc >= 3) { pre = argv[2]; }
	if (argc >= 4) { suf = argv[3]; }

	while ((file = folder_getnext(&f, pre, suf)))
	{
		printf("FILE '%s'\n", file);
	}

	return 0;
}
 */