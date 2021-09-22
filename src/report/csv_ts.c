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
#include <common/config.h>
#include <common/states.h>
#include <common/types/types.h>
#include <common/system/time.h>
#include <common/output/verbose.h>

static char *csv_log_file_env;
static char csv_loop_log_file[1024];
static char csv_log_file[1024];

static ullong my_time = 0;

state_t report_init()
{
    debug("eard report_init");
    char nodename[128];
    gethostname(nodename, sizeof(nodename));
    strtok(nodename, ".");

    csv_log_file_env = getenv(VAR_OPT_USDB);
    /* Loop filename is automatically generated */
    if (csv_log_file_env != NULL){
        xsnprintf(csv_log_file,sizeof(csv_log_file),"%s.%s.time",csv_log_file_env,nodename);
    }else{
        xsnprintf(csv_log_file,sizeof(csv_log_file),"ear_app_log.%s.time",nodename);
    }
    xsnprintf(csv_loop_log_file,sizeof(csv_loop_log_file),"%s.loops.csv",csv_log_file);
    xstrncat(csv_log_file,".csv",sizeof(csv_log_file));
    my_time = timestamp_getconvert(TIME_SECS);
    return EAR_SUCCESS;
}

state_t report_applications(application_t *apps, uint count)
{
    int i;
    debug("csv report_applications");
    if ((apps == NULL) || (count == 0)) return EAR_SUCCESS;
    for (i=0;i<count;i++){
        append_application_text_file(csv_log_file,&apps[i],1);
    }
    return EAR_SUCCESS;
}

state_t report_loops(loop_t *loops, uint count)
{
    int i;
    ullong currtime;
    debug("csv report_loops");
    // TODO: we could return EAR_ERROR
    if ((loops == NULL) || (count == 0)) return EAR_SUCCESS;
    ullong sec = timestamp_getconvert(TIME_SECS);
    currtime = sec - my_time;
    for (i=0;i<count;i++){
        // TODO: we could return EAR_ERROR in case the below functions returns EAR_ERROR
        append_loop_text_file_no_job_with_ts(csv_loop_log_file,&loops[i], currtime);
    }
    return EAR_SUCCESS;
}
