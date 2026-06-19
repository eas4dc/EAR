/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <limits.h>
#include <semaphore.h>
#include <stdio.h>

#include <common/config.h>
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/system/file.h>
#include <common/system/time.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/types/types.h>
#include <report/report.h>

static char csv_loop_log_file[1024];
static char csv_log_file[1024];

static ullong start_time = 0;

static uint must_report;
static sem_t *report_csv_sem_app;
static sem_t *report_csv_sem_loop;
/*  According to sem_overview(7):
 *  named semaphore is identified by a name of the form
    /somename; that is, a null-terminated string of up to
    NAME_MAX-4 (i.e., 251) characters consisting of an initial
    slash, followed by one or more characters, none of which
    are slashes.  Two processes can operate on the same named
    semaphore by passing the same name to sem_open(3).
 */
static char sem_file_app_path[NAME_MAX - 4];
static char sem_file_loop_path[NAME_MAX - 4];
static uint current_ID  = 0;
static uint sem_created = 0;
static char nodename[128];

static int fd_flags = O_WRONLY | O_APPEND | O_CLOEXEC | O_NOFOLLOW;
static mode_t mode  = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

static int fd_apps  = -1;
static int fd_loops = -1;

static char ear_owner[GENERIC_NAME];

static int create_or_open_csv_file(char *csv_file, void (*header_creation_function)(char *, size_t));
static void create_app_csv_header(char *app_header_buff, size_t app_header_size);
static void create_loop_csv_header(char *loop_header_buff, size_t loop_header_size);

static uint check_ID(uint ID);
static void create_semaphore(uint ID, char *node);

state_t report_init(report_id_t *id, cluster_conf_t *cconf)
{
    if (id->master_rank >= 0)
        must_report = 1;

    if (!must_report)
        return EAR_SUCCESS;

    gethostname(nodename, sizeof(nodename));
    strtok(nodename, ".");

    char *csv_log_file_env = ear_getenv(ENV_FLAG_PATH_USERDB);

    snprintf(csv_log_file, sizeof(csv_log_file), "%s_%s", (csv_log_file_env) ? csv_log_file_env : "ear", nodename);

    strncpy(csv_loop_log_file, csv_log_file, sizeof(csv_log_file) - 1);
    csv_loop_log_file[sizeof(csv_log_file) - 1] = '\0';

    strncat(csv_log_file, "_apps.csv", sizeof(csv_log_file) - strlen(csv_log_file) - 1);
    strncat(csv_loop_log_file, "_loops.csv", sizeof(csv_loop_log_file) - strlen(csv_loop_log_file) - 1);

    debug("csv_log_file: '%s'. csv_loop_log_file: %s", csv_log_file, csv_loop_log_file);

    start_time = timestamp_getconvert(TIME_SECS);

    /* We set to 0 to be sure the semaphore will be created
     * even when the process is created with a fork. */
    sem_created = 0;

    strncpy(ear_owner, cconf->ear_owner, sizeof(ear_owner) - 1);
    ear_owner[sizeof(ear_owner) - 1] = '\0';

    return EAR_SUCCESS;
}

state_t report_applications(report_id_t *id, application_t *apps, uint count)
{
    if (!must_report)
        return EAR_SUCCESS;

    if ((apps == NULL) || (count == 0))
        return EAR_SUCCESS;

    if (!sem_created) {
        create_semaphore(create_ID(apps[0].job.id, apps[0].job.step_id), nodename);
    }

    sem_wait(report_csv_sem_app);

    if (fd_apps < 0) {
        fd_apps = create_or_open_csv_file(csv_log_file, create_app_csv_header);
        if (fd_apps < 0) {
            return EAR_ERROR;
        }
    }

    for (int i = 0; i < count; i++) {
        if (!check_ID(create_ID(apps[i].job.id, apps[i].job.step_id))) {
            continue;
        }
        debug("Reporting application %d", i);

        print_application_fd(fd_apps, &apps[i], 1, 1, 0);
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
    if (!must_report)
        return EAR_SUCCESS;

    if ((loops == NULL) || (count == 0))
        return EAR_ERROR;

    if (!sem_created) {
        create_semaphore(create_ID(loops[0].jid, loops[0].step_id), nodename);
    }

    sem_wait(report_csv_sem_loop);

    if (fd_loops < 0) {
        fd_loops = create_or_open_csv_file(csv_loop_log_file, create_loop_csv_header);
        if (fd_loops < 0) {
            return EAR_ERROR;
        }
    }

    ullong sec      = timestamp_getconvert(TIME_SECS);
    ullong currtime = sec - start_time;

    for (int i = 0; i < count; i++) {
        if (!check_ID(create_ID(loops[i].jid, loops[i].step_id))) {
            continue;
        }
        loop_print_fd(fd_loops, &loops[i], 1, currtime, 0, ' ');
    }
    sem_post(report_csv_sem_loop);
    return EAR_SUCCESS;
}

state_t report_dispose(report_id_t *id)
{
    if (sem_created) {
        sem_close(report_csv_sem_app);
        sem_close(report_csv_sem_loop);

        sem_unlink(sem_file_app_path);
        sem_unlink(sem_file_loop_path);
    }
    current_ID  = 0;
    sem_created = 0;
    return EAR_SUCCESS;
}

static int create_or_open_csv_file(char *csv_file, void (*header_creation_function)(char *, size_t))
{
    mode_t old_mask = umask(0);
    /* First we just let to create the file in order to know later
     * whether we need to print the csv header. */
    int fd = open(csv_file, fd_flags | O_CREAT | O_EXCL, mode);
    if (fd >= 0) {
        /* The file was created */

        /* If we are root, change the ownership to the ear_owner */
        if (getuid() == 0) {
            ear_chown_fd(fd, ear_owner);
        }

        /* Print the header. */
        char header_str[8192];
        header_creation_function(header_str, sizeof header_str);
        dprintf(fd, "%s\n", header_str);

    } else if (errno == EEXIST) {
        /* The file already exists, therefore we open it again and we don't print the header. */
        fd = open(csv_file, fd_flags, mode);
        if (fd < 0) {
            error("Opening file %s: (%d) %s", csv_file, errno, strerror(errno));
        }
    } else {
        /* Another error ocurred while opening the file. */
        error("Opening file %s: (%d) %s", csv_file, errno, strerror(errno));
    }
    umask(old_mask);
    return fd;
}

static void create_app_csv_header(char *app_header_buff, size_t app_header_size)
{
    if (!app_header_buff) {
        return;
    }
    memset(app_header_buff, 0, app_header_size);
    application_create_header_str(app_header_buff, app_header_size, NULL, MAX_GPUS_SUPPORTED, 1, 0);
}

static void create_loop_csv_header(char *loop_header_buff, size_t loop_header_size)
{
    if (!loop_header_buff) {
        return;
    }
    memset(loop_header_buff, 0, loop_header_size);
    loop_create_header_str(loop_header_buff, loop_header_size, NULL, 1, MAX_GPUS_SUPPORTED, 0);
}

static uint check_ID(uint ID)
{
    return (current_ID == ID);
}

static void create_semaphore(uint ID, char *node)
{
    /* This sem avoid simultaneous access to files */
    xsnprintf(sem_file_app_path, sizeof(sem_file_app_path), "/%s.%u.sem_app", node, ID);
    xsnprintf(sem_file_loop_path, sizeof(sem_file_loop_path), "/%s.%u.sem_loop", node, ID);
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

    current_ID  = ID;
    sem_created = 1;
}

#if TESTS
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(int argc, char **argv)
{
    int app_fd = create_or_open_csv_file("app_csv_test.csv", create_app_csv_header);
    struct stat statbuf;
    if (fstat(app_fd, &statbuf)) {
        return EXIT_FAILURE;
    }
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    assert(statbuf.st_mode & (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));
    return EXIT_SUCCESS;
}
#endif
