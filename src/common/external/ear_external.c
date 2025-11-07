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

#include <common/external/ear_external.h>
#include <common/output/debug.h>
#include <common/system/shared_areas.h>
#include <common/types/application.h>
#include <common/types/types.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>

static int fd_resched = -1;
static char path[256];

int get_ear_external_path(char *tmp, uint ID, char *path)
{
    if ((tmp == NULL) || (path == NULL))
        return EAR_ERROR;
    sprintf(path, "%s/%u/.ear_resched_affinity", tmp, ID);
    return EAR_SUCCESS;
}

ear_mgt_t *create_ear_external_shared_area(char *path)
{
    debug("Creating third party shared area in path %s...", path);

    // Read and write permission for user and group. Read permission for others.
    mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

    ear_mgt_t mgt;
    return (ear_mgt_t *) create_shared_area(path, perms, (char *) &mgt, sizeof(ear_mgt_t), &fd_resched, 1);
}

ear_mgt_t *attach_ear_external_shared_area(char *path)
{
    debug("Attaching to third party shared area in path %s", path);
    return (ear_mgt_t *) attach_shared_area(path, sizeof(ear_mgt_t), O_RDWR, &fd_resched, NULL);
}

void dettach_ear_external_shared_area()
{
    dettach_shared_area(fd_resched);
}

void dipose_ear_external_shared_area(char *path)
{
    dispose_shared_area(path, fd_resched);
}

ear_mgt_t *ear_connect()
{

    // get the path
    char *tmp = getenv("EAR_TMP");
    char *jid = getenv("SLURM_JOB_ID");
    char *sid = getenv("SLURM_STEP_ID");
    if ((jid == NULL) || (sid == NULL))
        return NULL;

    int jobid  = atoi(jid);
    int stepid = atoi(sid);
    uint id    = create_ID(jobid, stepid); // jobid i stepid es poden treure de la variable d'entorn de slurm
    if (get_ear_external_path(tmp, id, path) != EAR_SUCCESS)
        return NULL;

    return attach_ear_external_shared_area(path);
}

state_t ear_disconnect()
{
    dettach_ear_external_shared_area();

    return EAR_SUCCESS;
}
