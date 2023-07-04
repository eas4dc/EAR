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

#include <stdio.h>
#include <stdlib.h>
#include <common/config.h>
#include <common/states.h>
#include <common/external/ear_external.h>
#include <common/types/configuration/cluster_conf.h>

enum actions {
    NO_ACTION,
    NEW_MASK,
};

int usage(char *app)
{
    printf("Usage: %s [options]\n", app);
    printf("At least one option is required\n");
    printf("  --jobid\t\tspecifies jobid\n");
    printf("  --stepid\t\tspecifies stepid\n");
    printf("  --newmask\t\tsends a new_mask rescheduling through shared memory\n");
    return 0;
}

void new_mask(int jobid, int stepid)
{
    char tmp[64];
    sprintf(tmp, "%d", jobid);
    setenv("SLURM_JOB_ID", tmp, 0);
    sprintf(tmp, "%d", stepid);
    setenv("SLURM_STEP_ID", tmp, 0);

    ear_mgt_t *mgt = NULL;
    if ((mgt = ear_connect()) == NULL) {
        printf("Error connecting with the shared region area\n");
        return;
    }

    mgt->new_mask = 1;

    if (state_fail(ear_disconnect())) {
        printf("Error deattaching from the shared region area\n"); 
    }
}

int main(int argc, char *argv[])
{

    if (getenv("EAR_TMP") == NULL) {
        printf("EAR_TMP must be set for the application to work\n");
        return 0;
    }

    char c;
    int option_idx = 0;
    static struct option long_options[] = {
        {"jobid",       required_argument, 0, 0},
        {"stepid",      required_argument, 0, 1},
        {"new-mask",    no_argument, 0, 2},
        {NULL,          0, 0, 0}
    };
    

    int jobid = 0, stepid = 0;
    enum actions action = NO_ACTION;
    while (1) {
		c = getopt_long(argc, argv, "j:s:", long_options, &option_idx);
        if (c == -1) break;
        switch (c) {
            case 0:
               jobid = atoi(optarg);
               break;
            case 1:
               stepid = atoi(optarg);
               break;
            case 2:
               action = NEW_MASK;
               break;
            default:
               break;
        }
    }

    switch(action) {
        case NO_ACTION:
            usage(argv[0]);
            break;
        case NEW_MASK:
            new_mask(jobid, stepid);
            break;
        default:
            break;
    }

    return 0;
}

