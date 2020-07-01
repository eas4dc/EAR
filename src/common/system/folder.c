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