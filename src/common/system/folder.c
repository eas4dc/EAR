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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <common/types/generic.h>
#include <common/system/folder.h>
#include <common/output/debug.h>
#include <common/system/file.h>

state_t folder_open(folder_t *folder, char *path)
{
	if ((folder->dir = opendir(path)) == NULL) {
		return EAR_OPEN_ERROR;
	}

	return EAR_SUCCESS;
}

state_t folder_exists(char *path)
{
	return ((opendir(path) != NULL ) ? EAR_SUCCESS: EAR_ERROR);
}

state_t folder_close(folder_t *folder)
{
  if (folder->dir == NULL) return EAR_SUCCESS;
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

char *folder_getnext_type(folder_t *folder, char *prefix, char *suffix, uint type)
{
	struct dirent *file;
	int lpre, lsuf, lfil;
	char *pfil;

	pfil = folder->file_name;
	lpre = (prefix != NULL) ? (int) strlen(prefix) : 0;
	lsuf = (suffix != NULL) ? (int) strlen(suffix) : 0;

	while ((file = readdir(folder->dir)) != NULL)
	{
    if (type != DT_UNKNOWN){
		  if (file->d_type != type) {
			  continue;
		  }
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

	//folder_close(folder);

	return NULL;
}

char *folder_getnextdir(folder_t *folder, char *prefix, char *suffix)
{
	return folder_getnext_type(folder, prefix, suffix, DT_DIR);
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

  while ((file = folder_getnext_type(&job_folder, NULL, NULL, DT_UNKNOWN)))
  {
	if (strcmp(file, ".") == 0) continue;
	if (strcmp(file, "..") == 0) continue;
    xsnprintf(file_path,sizeof(file_path),"%s/%s",job_path,file);
    debug("Deleting: %s",file_path);
    if (file_is_directory(file_path)){
	debug("file '%s' is a directory, removing", file_path);
	folder_remove(file_path);
    }else unlink(file_path);
  }
  //folder_close(&job_folder);
  debug("Deleting folder %s", job_path);
  rmdir(job_path);
	return EAR_SUCCESS;

}

state_t folder_rename(char *oldp, char *newp)
{
  if (rename(oldp, newp) >= 0) return EAR_SUCCESS;
  else                       return EAR_ERROR;
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
