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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

//#define SHOW_DEBUGS 1
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <common/types/generic.h>
#include <common/system/folder.h>
#include <common/output/debug.h>

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

state_t folder_remove(char *path)
{
  char job_path[MAX_PATH_SIZE];
  char file_path[MAX_PATH_SIZE];
  state_t s;
  folder_t job_folder;
  char *file;

  xsnprintf(job_path,sizeof(job_path),"%s",path);
  s = folder_open(&job_folder, job_path);
  if (state_fail(s)) {
    debug("Error opening job path %s",job_path);
    return EAR_ERROR;
  }
  debug("Cleaning job_path: %s", job_path);

  while ((file = folder_getnext(&job_folder, NULL, NULL)))
  {
    xsnprintf(file_path,sizeof(file_path),"%s/%s",job_path,file);
    debug("Deleting: %s",file_path);
    unlink(file_path);
  }
  folder_close(&job_folder);
  rmdir(job_path);
	return EAR_SUCCESS;

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
