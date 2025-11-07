/* *INDENT-OFF* */
/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <semaphore.h>
#include <stdio.h>

#include <common/config.h>
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/system/time.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/types/types.h>
#include <report/report.h>

static char csv_loop_log_file[1024];
static char csv_log_file[1024];

static ullong my_time = 0;

static uint must_report;
static sem_t *report_csv_sem_app;
static sem_t *report_csv_sem_loop;
static char sem_file_app_path[1024];
static char sem_file_loop_path[1024];
static uint current_ID  = 0;
static uint sem_created = 0;
static char nodename[128];

static void create_semaphore(uint ID, char *node)
{
    /* This sem avoid simultaneous access to files */
    xsnprintf(sem_file_app_path, sizeof(sem_file_app_path), "%s.sem_app", node);
    xsnprintf(sem_file_loop_path, sizeof(sem_file_loop_path), "%s.sem_loop", node);
    debug("Using sem_app %s", sem_file_app_path);
    debug("Using sem_loop %s", sem_file_loop_path);

    report_csv_sem_app = sem_open(sem_file_app_path, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (report_csv_sem_app == SEM_FAILED) {
        error("Creating sempahore %s (%s)", sem_file_app_path, strerror(errno));
    }
    report_csv_sem_loop = sem_open(sem_file_loop_path, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (report_csv_sem_loop == SEM_FAILED) {
        error("Creating sempahore %s (%s)", sem_file_loop_path, strerror(errno));
    }
#if 0
    if ((report_csv_sem_app == SEM_FAILED) || (report_csv_sem_loop == SEM_FAILED)){
      printf("CSV app (%s) or loop (%s) failed\n", sem_file_app_path, sem_file_loop_path);
    }else{
      printf("CSV app (%s) and loop (%s) success\n", sem_file_app_path, sem_file_loop_path);
    }
#endif
    current_ID  = ID;
    sem_created = 1;
}

static uint check_ID(uint ID)
{
    return (current_ID == ID);
}

state_t report_init(report_id_t *id, cluster_conf_t *cconf)
{
    debug("eard report_init");
    if (id->master_rank >= 0)
        must_report = 1;
    if (!must_report)
        return EAR_SUCCESS;

    gethostname(nodename, sizeof(nodename));
    strtok(nodename, ".");

    char *csv_log_file_env = ear_getenv(ENV_FLAG_PATH_USERDB);

    snprintf(csv_log_file, sizeof(csv_log_file) - 1, "%s_%s", (csv_log_file_env) ? csv_log_file_env : "ear", nodename);
    strncpy(csv_loop_log_file, csv_log_file, sizeof(csv_log_file) - 1);

    strncat(csv_log_file, "_apps.csv", sizeof(csv_log_file) - strlen(csv_log_file) - 1);
    strncat(csv_loop_log_file, "_loops.csv", sizeof(csv_loop_log_file) - strlen(csv_loop_log_file) - 1);

    my_time = timestamp_getconvert(TIME_SECS);

    /* We set to 0 to be sure the semaphore will be created even when the process is created with a fork */
    sem_created = 0;

    return EAR_SUCCESS;
}

state_t report_applications(report_id_t *id, application_t *apps, uint count)
{
    int i;
    if (!must_report)
        return EAR_SUCCESS;
    debug("csv report_applications");
    if ((apps == NULL) || (count == 0))
        return EAR_SUCCESS;

    if (!sem_created) {
        create_semaphore(create_ID(apps[0].job.id, apps[0].job.step_id), nodename);
    }
    sem_wait(report_csv_sem_app);
    for (i = 0; i < count; i++) {
        if (!check_ID(create_ID(apps[i].job.id, apps[i].job.step_id))) {
            continue;
        }
        append_application_text_file(csv_log_file, &apps[i], 1, 1, 0);
    }
    sem_post(report_csv_sem_app);
    return EAR_SUCCESS;
}

state_t report_misc(report_id_t *id, uint type, const char *data, uint count)
{
    if (type == WF_APPLICATION) {
        report_applications(id, (application_t *) data, count);
    }
    return EAR_SUCCESS;
}

state_t report_loops(report_id_t *id, loop_t *loops, uint count)
{
    int i;
    ullong currtime;
    if (!must_report)
        return EAR_SUCCESS;
    debug("csv report_loops");
    if ((loops == NULL) || (count == 0))
        return EAR_ERROR;
    ullong sec = timestamp_getconvert(TIME_SECS);
    currtime   = sec - my_time;

    if (!sem_created) {
        create_semaphore(create_ID(loops[0].jid, loops[0].step_id), nodename);
    }
    sem_wait(report_csv_sem_loop);
    for (i = 0; i < count; i++) {
        if (!check_ID(create_ID(loops[i].jid, loops[i].step_id))) {
            continue;
        }
        // TODO: we could return EAR_ERROR in case the below functions returns EAR_ERROR
        append_loop_text_file_no_job_with_ts(csv_loop_log_file, &loops[i], currtime, 1, 0, ' ');
    }
    sem_post(report_csv_sem_loop);
    return EAR_SUCCESS;
}

state_t report_dispose(report_id_t *id)
{
    if (sem_created) {
        sem_close(report_csv_sem_app);
        sem_close(report_csv_sem_loop);
    }
    current_ID  = 0;
    sem_created = 0;
    return EAR_SUCCESS;
}
