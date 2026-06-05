/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1
#include <common/output/debug.h>
#include <common/system/file.h>
#include <common/system/folder.h>
#include <common/types/generic.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

state_t folder_open(folder_t *folder, char *path)
{
    if ((folder->dir = opendir(path)) == NULL) {
        return EAR_OPEN_ERROR;
    }

    return EAR_SUCCESS;
}

state_t folder_exists(char *path)
{
    return ((opendir(path) != NULL) ? EAR_SUCCESS : EAR_ERROR);
}

state_t folder_close(folder_t *folder)
{
    if (folder->dir == NULL)
        return EAR_SUCCESS;
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

    while ((file = readdir(folder->dir)) != NULL) {
        if (file->d_type == DT_DIR) {
            continue;
        }

        if (prefix != NULL) {
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

        if (suffix != NULL) {
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

    while ((file = readdir(folder->dir)) != NULL) {
        if (type != DT_UNKNOWN) {
            if (file->d_type != type) {
                continue;
            }
        }
        if (prefix != NULL) {
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
        if (suffix != NULL) {
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
    // folder_close(folder);

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

    xsnprintf(job_path, sizeof(job_path), "%s", path);
    s = folder_open(&job_folder, job_path);
    if (state_fail(s)) {
        debug("Error opening job path %s", job_path);
        return EAR_ERROR;
    }
    debug("Cleaning job_path: %s", job_path);

    while ((file = folder_getnext_type(&job_folder, NULL, NULL, DT_UNKNOWN))) {
        if (strcmp(file, ".") == 0)
            continue;
        if (strcmp(file, "..") == 0)
            continue;
        xsnprintf(file_path, sizeof(file_path), "%s/%s", job_path, file);
        debug("Deleting: %s", file_path);
        if (ear_file_is_directory(file_path)) {
            debug("file '%s' is a directory, removing", file_path);
            folder_remove(file_path);
        } else
            unlink(file_path);
    }
    // Closing folder to avoid fd to remain open
    folder_close(&job_folder);
    debug("Deleting folder %s", job_path);
    rmdir(job_path);
    return EAR_SUCCESS;
}

state_t folder_rename(char *oldp, char *newp)
{
    if (rename(oldp, newp) >= 0)
        return EAR_SUCCESS;
    else
        return EAR_ERROR;
}

/**
 * ear_create_tmp:
 * Creates a directory and all its parent components, similar to 'mkdir -p'.
 * It also enforces specific permissions (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
 * for each created directory component.
 *
 * @path: The absolute or relative path of the directory to create.
 * @return: EAR_SUCCESS on success, EAR_ERROR on failure.
 */
state_t ear_create_tmp(char *path, char *ear_owner)
{
    char tmp[MAX_PATH_SIZE];
    char *p = NULL;
    size_t len;
    struct stat st;

    // Validate input path
    if (!path || !strlen(path) || !ear_owner) {
        return EAR_ERROR;
    }

    // Clone the path to a local buffer for manipulation
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);

    // Remove trailing slash to avoid empty component in the loop
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }

    /* Iterate over the string, component by component.
     * We start from tmp + 1 to skip the root '/' if the path is absolute. */
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0; // Temporarily truncate the string at the current slash

            // Check if the current path component exists
            if (stat(tmp, &st) != 0) {
                // Component does not exist, try to create it
                if (mkdir(tmp, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0) {
                    // Check if it was created by another process in the meantime
                    if (errno != EEXIST) {
                        error("Error creating directory %s: %s", tmp, strerror(errno));
                        return EAR_ERROR;
                    }
                } else {
                    /* Successfully created. Enforce permissions and ownership */
                    if (chmod(tmp, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0) {
                        debug("Warning: could not chmod %s: %s", tmp, strerror(errno));
                    }
                    ear_chown_path(tmp, ear_owner);
                }
            } else {
                // Component exists, verify it's a directory
                if (!S_ISDIR(st.st_mode)) {
                    error("Error: %s is not a directory", tmp);
                    return EAR_ERROR;
                }
            }

            *p = '/'; // Restore the slash and continue
        }
    }

    // Final check/creation for the last component of the path
    if (stat(tmp, &st) != 0) {
        if (mkdir(tmp, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0) {
            if (errno != EEXIST) {
                error("Error creating directory %s: %s", tmp, strerror(errno));
                return EAR_ERROR;
            }
        } else {
            /* Successfully created.
             * Set permissions to 1777 (sticky bit) to allow shared usage (e.g. by erun)
             * while preventing users from deleting each other's files. */
            // S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO = 01777
            if (chmod(tmp, S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
                debug("Warning: could not chmod %s: %s", tmp, strerror(errno));
            }
            ear_chown_path(tmp, ear_owner);
        }
    } else {
        // Ensure the final component is indeed a directory
        if (!S_ISDIR(st.st_mode)) {
            error("Error: %s is not a directory", tmp);
            return EAR_ERROR;
        }
        /* Enforce ownership even if it exists */
        /* Also ensure permissions are correct (1777) for shared access */
        chmod(tmp, S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO);
        ear_chown_path(tmp, ear_owner);
    }

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
