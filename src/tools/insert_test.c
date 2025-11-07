/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/output/verbose.h>
#include <common/sizes.h>
#include <common/string_enhanced.h>
#include <common/types/application.h>
#include <common/types/configuration/cluster_conf.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include <report/report.h>

void usage(char *name)
{
    printf("\n");
    printf("Usage: %s\t\n", name);
    printf("\tThis will insert 10 applications with job_id current timestamp, step_ids from 0 to 1, application_name \n"
           "\t'test_insert.current_time' and everything else to 0. The application will have a Signature, where the\n"
           "\tavg_f is 112 (for testing purposes)\n");
    printf("\n");
    exit(0);
}

int main(int argc, char *argv[])
{

    int i;
    int num_apps = 10;
    char test_name[256];
    time_t ttime = time(NULL);
    char ear_path[256];
    cluster_conf_t conf;
    state_t st;

    if (argc > 1) {
        usage(argv[0]);
    }

    // ear conf
    if (get_ear_conf_path(ear_path) == EAR_ERROR) {
        printf("Error getting ear.conf path\n");
        exit(0);
    }
    read_cluster_conf(ear_path, &conf);

    // db init
    st = report_load("/home/void/ear_install/lib/plugins", "psql.so");
    if (st != EAR_SUCCESS) {
        printf("Error loading report libraries\n");
        goto exit;
    }
    report_id_t id;
    st = report_init(&id, &conf);
    if (st != EAR_SUCCESS) {
        printf("Error initializing report libraries\n");
        goto exit;
    }

    sprintf(test_name, "test_insert.%ld", ttime);
    application_t *apps = calloc(num_apps, sizeof(application_t));

    for (i = 0; i < num_apps; i++) {
        apps[i].job.id      = ttime;
        apps[i].job.step_id = i;
        strcpy(apps[i].job.app_id, test_name);
        apps[i].is_mpi          = 1;
        apps[i].signature.avg_f = 112;
    }

    st = report_applications(&id, apps, num_apps);

    if (st != EAR_SUCCESS) {
        printf("Error while reporting applications\n");
    }

    free(apps);

exit:
    free_cluster_conf(&conf);

    return 0;
}
