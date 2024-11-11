/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>

// #define SHOW_DEBUGS 1

#include <common/config.h>
#include <common/states.h>
#include <common/types/types.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/system/time.h>
#include <common/system/file.h>
#include <common/output/verbose.h>
#include <common/output/debug.h>
#include <report/report.h>
#include <library/api/clasify.h>
#include <library/metrics/metrics.h>

#define DCGMI_LOGS_PATH "DCGMI_LOGS_PATH"
#define dcgmi_verbose(lvl, msg, ...) do { \
	verbose(lvl, "[dcgmi.so] " msg, ##__VA_ARGS__); \
} while (0);

#if 0
static char *csv_log_file_env_loops;
#endif
static char csv_loop_log_file[MAX_PATH_SIZE];
static char csv_log_file[MAX_PATH_SIZE];
static char path_base[1024];
static char path_dir_app[1024], path_dir_loops[1024];

static ullong my_time = 0;

static uint must_report;
static uint        dcgmi = DCGM_DEFAULT;

static char extra_header[8192];
static char extra_header_app[8192];
static char extra_metrics[8192];
static char app_header[8192];

static char nodename[128];

static uint sem_created;
static char sem_file_app_path[1024];
static char sem_file_loop_path[1024];
static sem_t *report_csv_sem_app;
static sem_t *report_csv_sem_loop;

static uint current_ID;

static int fd_flags = O_WRONLY | O_APPEND | O_CLOEXEC;

static void create_semaphore(uint ID, char *node);

static uint check_ID(uint ID);

state_t report_init(report_id_t *id, cluster_conf_t *cconf)
{
	if (id->master_rank >= 0) {
		/* Read the environment variable to see whether
		 * DCGMI metrics module is enabled. */

		char *use_dcgmi = ear_getenv(FLAG_GPU_DCGMI_ENABLED);
		if (use_dcgmi != NULL) {
			must_report = atoi(use_dcgmi);
		} else
			must_report = dcgmi;

		dcgmi_verbose(1, "DCGMI report: %s",
			      (must_report) ? "ON" : "OFF");

	} else {
		debug("Slaves don't report through this plug-in.");
		must_report = 0;
	}

	if (must_report) {
		debug("report_init");

		gethostname(nodename, sizeof(nodename));
		strtok(nodename, ".");

		char *csv_log_file_env = ear_getenv(DCGMI_LOGS_PATH);
		if (!csv_log_file_env) {
			csv_log_file_env = getcwd(path_base, sizeof(path_base));
			snprintf(csv_log_file, sizeof(csv_log_file) - 1,
			 "%s/dcgmi_app_%s.csv", csv_log_file_env, nodename);

			snprintf(csv_loop_log_file, sizeof(csv_loop_log_file) - 1,
			 "%s/dcgmi_loops_%s.csv", csv_log_file_env, nodename);
		}else{
			snprintf(csv_log_file, sizeof(csv_log_file) - 1,
			 "%s_dcgmi_app_%s.csv", csv_log_file_env, nodename);

			snprintf(csv_loop_log_file, sizeof(csv_loop_log_file) - 1,
			 "%s_dcgmi_loops_%s.csv", csv_log_file_env, nodename);
		}

		dcgmi_verbose(2, "DCGMI paths app`%s loops %s", csv_log_file, csv_loop_log_file);

#if 0
		/*
		 * Commented since I'm trying to mimic the code of csv_ts.c plug-in.
		 */
		/* Loop filename is automatically generated */
		if (csv_log_file_env) {
			int ret;
			debug("Using PATH: %s", csv_log_file_env);
			if (!file_is_directory(csv_log_file_env)) {
				if (file_is_regular(csv_log_file_env)) {
					error
					    ("DCGMI report plug-in: Invalid path. It must be a directory, not a regular file (%s).",
					     csv_log_file_env);
					must_report = 0;
					return EAR_ERROR;
				}

				ret =
				    mkdir(csv_log_file_env,
					  S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP
					  | S_IXGRP | S_IROTH | S_IXOTH);
				if (ret < 0) {
					error
					    ("DCGMI report plug-in: Creating output directory %s (%s).",
					     csv_log_file_env, strerror(errno));
					must_report = 0;
					return EAR_ERROR;
				}
			}

			if (file_is_directory(csv_log_file_env)
			    || (csv_log_file_env[strlen(csv_log_file_env) - 1]
				== '/')) {
				debug("%s is directory", csv_log_file_env);

				sprintf(path_dir_app, "%s/dcgmi_app_logs",
					csv_log_file_env);

				ret =
				    mkdir(path_dir_app,
					  S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP
					  | S_IXGRP | S_IROTH | S_IXOTH);
				if (ret < 0 && errno != EEXIST) {
					error
					    ("DCGMI report plug-in: Creating application output directory %s (%s).",
					     path_dir_app, strerror(errno));
					must_report = 0;
					return EAR_ERROR;
				}

				sprintf(path_dir_loops, "%s/dcgmi_loop_logs",
					csv_log_file_env);

				ret =
				    mkdir(path_dir_loops,
					  S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP
					  | S_IXGRP | S_IROTH | S_IXOTH);
				if (ret < 0 && errno != EEXIST) {
					error
					    ("DCGMI report plug-in: Creating loops output directory %s (%s).",
					     path_dir_loops, strerror(errno));
					must_report = 0;
					return EAR_ERROR;
				}

				xsnprintf(csv_log_file, sizeof(csv_log_file),
					  "%s/dcgmi.%d.csv", path_dir_app,
					  id->master_rank);
				xsnprintf(csv_loop_log_file,
					  sizeof(csv_loop_log_file),
					  "%s/dcgmi.loops.%d.csv",
					  path_dir_loops, id->master_rank);

				debug("csv_log_file %s", csv_log_file);
				debug("csv_loop_log_file %s",
				      csv_loop_log_file);
			}
		} else {
			must_report = 0;
			return EAR_ERROR;
		}
#endif

		my_time = timestamp_getconvert(TIME_SECS);

		debug("Done.");
	}

	/* We set to 0 to be sure the semaphore will be created
	 * even when the process is created with a fork */
	sem_created = 0;

	return EAR_SUCCESS;

}

// static uint file_app_created = 0;
static int fd_app = -1;
state_t report_applications(report_id_t *id, application_t *apps, uint count)
{
	if (!must_report)
		return EAR_SUCCESS;

	debug("dcgmi report_applications");

	if ((apps == NULL) || (count == 0))
		return EAR_SUCCESS;

	if (!sem_created) {
		create_semaphore(create_ID(apps[0].job.id, apps[0].job.step_id),
				 nodename);
	}

	int i;
	sig_ext_t *sigex;
	dcgmi_sig_t *dcgmis;

	uint num_gpus = MAX_GPUS_SUPPORTED;

	sem_wait(report_csv_sem_app);

	/*
	 * fd_app negative means we haven't opened the file.
	 * If file_is_regular we don't need to create the header.
	 * So we just create the header if we don't have the file opened and the
	 * file doesn't exist.
	 */
	if (fd_app < 0 && !file_is_regular(csv_log_file)) {
		debug("Creating application header");
#if DCGMI
		dcgmi_lib_dcgmi_sig_csv_header(extra_header_app,
					       sizeof(extra_header_app));
		strcat(extra_header_app, ";");
#endif
		strcpy(app_header, extra_header_app);

		/* Headers for phases */
		for (uint ph = 1; ph < EARL_MAX_PHASES; ph++) {
			strcat(app_header, phase_to_str(ph));
			strcat(app_header, ";");
		}

		create_app_header(app_header, csv_log_file, num_gpus, 1, 0);
		// file_app_created = 1;
		fd_app = open(csv_log_file, fd_flags);
		debug("app csv file: %d", fd_app);
		if (fd_app < 0) {
			debug("Error opening csv %s file: %s", csv_log_file, strerror(errno));
		}
	}

	/* Open the file if we didn't it yet. */
	if (fd_app < 0) {
		fd_app = open(csv_log_file, fd_flags);
		if (fd_app < 0) {
			debug("Error opening csv %s file: %s", csv_log_file, strerror(errno));
		}
	}

	for (i = 0; i < count; i++) {
		if (!check_ID(create_ID(apps[i].job.id, apps[i].job.step_id))) {
			continue;
		}
		debug("Reporting app %u", i);
		if (fd_app >= 0) {
#if DCGMI
			sigex = (sig_ext_t *) apps[i].signature.sig_ext;
			dcgmis = (dcgmi_sig_t *) & sigex->dcgmis;

			debug("Set count: %u", dcgmis->set_cnt);
			if (dcgmis->set_cnt && num_gpus) {
				dcgmi_lib_dcgmi_sig_to_csv(dcgmis,
							   extra_metrics,
							   sizeof
							   (extra_metrics));
				dprintf(fd_app, "%s;", extra_metrics);
			}
#endif

			if (apps[i].signature.sig_ext) {
				debug("Adding phases");
				sigex = (sig_ext_t *) apps[i].signature.sig_ext;
				for (uint ph = 1; ph < EARL_MAX_PHASES; ph++) {
					dprintf(fd_app, "%llu;",
						sigex->earl_phase[ph].elapsed);
				}
			}
		}
		debug("Reporting signature");
		append_application_text_file(csv_log_file, &apps[i], 1, 0, 0);
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

// static uint file_loop_created = 0;
static int fd_loops = -1;

state_t report_loops(report_id_t *id, loop_t *loops, uint count)
{
	if (!must_report) {
		debug("Report plug-in disabled.");
		return EAR_SUCCESS;
	}

	debug("dcgmi report_loops");

	int i;
	ullong currtime;
	sig_ext_t *sigex;
	dcgmi_sig_t *dcgmis;

	if ((loops == NULL) || (count == 0))
		return EAR_ERROR;

	uint num_gpus = MAX_GPUS_SUPPORTED;

	if (!sem_created) {
		create_semaphore(create_ID(loops[0].jid, loops[0].step_id),
				 nodename);
	}
	sem_wait(report_csv_sem_loop);

	/* The same logic as with report_applications. */
	if (fd_loops < 0 && !file_is_regular(csv_loop_log_file)) {
		debug("dcgmi creating loop header");

#if DCGMI
		dcgmi_lib_dcgmi_sig_csv_header(extra_header,
					       sizeof(extra_header));
		strcat(extra_header, ";");
#endif

		create_loop_header(extra_header, csv_loop_log_file, 1, num_gpus,
				   0);
		debug("Header file created");
		fd_loops = open(csv_loop_log_file, fd_flags);
	}

	if (fd_loops < 0)
		fd_loops = open(csv_loop_log_file, fd_flags);

	ullong sec = timestamp_getconvert(TIME_SECS);
	currtime = sec - my_time;
	for (i = 0; i < count; i++) {
		if (!check_ID(create_ID(loops[i].jid, loops[i].step_id))) {
			continue;
		}
		// TODO: we could return EAR_ERROR in case the below functions returns EAR_ERROR
		if (fd_loops >= 0) {
#if DCGMI
			sigex = (sig_ext_t *) loops[i].signature.sig_ext;
			dcgmis = (dcgmi_sig_t *) & sigex->dcgmis;
			if (num_gpus) {
				dcgmi_lib_dcgmi_sig_to_csv(dcgmis,
							   extra_metrics,
							   sizeof
							   (extra_metrics));
				dprintf(fd_loops, "%s;", extra_metrics);
			}
#endif
		}
		append_loop_text_file_no_job_with_ts(csv_loop_log_file,
						     &loops[i], currtime, 0, 0,
						     ' ');
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
	current_ID = 0;
	sem_created = 0;
	if (fd_app) {
		close(fd_app);
		fd_app = -1;
	}
	if (fd_loops) {
		close(fd_loops);
		fd_loops = -1;
	}
	return EAR_SUCCESS;
}

static void create_semaphore(uint ID, char *node)
{
	/* This sem avoids simultaneous access to files */
	xsnprintf(sem_file_app_path, sizeof(sem_file_app_path), "%s.sem_app",
		  node);
	xsnprintf(sem_file_loop_path, sizeof(sem_file_loop_path), "%s.sem_loop",
		  node);

	debug("Using sem_app %s", sem_file_app_path);
	debug("Using sem_loop %s", sem_file_loop_path);

	report_csv_sem_app =
	    sem_open(sem_file_app_path, O_CREAT, S_IRUSR | S_IWUSR, 1);
	if (report_csv_sem_app == SEM_FAILED) {
		error("Creating sempahore %s (%s)", sem_file_app_path,
		      strerror(errno));
	}

	report_csv_sem_loop =
	    sem_open(sem_file_loop_path, O_CREAT, S_IRUSR | S_IWUSR, 1);
	if (report_csv_sem_loop == SEM_FAILED) {
		error("Creating sempahore %s (%s)", sem_file_loop_path,
		      strerror(errno));
	}
#if 0
	if ((report_csv_sem_app == SEM_FAILED)
	    || (report_csv_sem_loop == SEM_FAILED)) {
		printf("CSV app (%s) or loop (%s) failed\n", sem_file_app_path,
		       sem_file_loop_path);
	} else {
		printf("CSV app (%s) and loop (%s) success\n",
		       sem_file_app_path, sem_file_loop_path);
	}
#endif

	current_ID = ID;
	sem_created = 1;
}

static uint check_ID(uint ID)
{
	return (current_ID == ID);
}
