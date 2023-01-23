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
#include <fcntl.h>
#include <unistd.h>

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


static char *csv_log_file_env;
static char *csv_log_file_env_loops;
static char csv_loop_log_file[1024];
static char csv_log_file[1024];
static char heroes_ids[1024];
static char path_base[1024];
static char path_dir_app[1024], path_dir_loops[1024];

static ullong my_time = 0;

static uint must_report;

/*
 * HEROES_JOBID
 * HEROES_USERID
 * HEROES_PROJECTID
 * HEROES_ORGID
 * HEROES_WFID
 */

#define NULL_WRF_ID "NO_WRF_ID"
#define NULL_HER_ID "NO_HER_ID"
#define NULL_USR_ID "NO_USR_ID"
#define NULL_PRJ_ID "NO_PRJ_ID"
#define NULL_ORG_ID "NO_ORG_ID"

/*
 * HEROES_ORGANIZATION_ID
 * HEROES_ORGANIZATION_NAME
 * HEROES_USER_ID
 * HEROES_TEMPLATE_WORKFLOW_ID
 * HEROES_INSTANCE_WORKFLOW_ID
 * HEROES_USER_WORKDIR
*/

#define HEROES_WRF_ID "HEROES_INSTANCE_WORKFLOW_ID"
#define HEROES_HER_ID "HEROES_TEMPLATE_WORKFLOW_ID"
#define HEROES_USR_ID "HEROES_USER_ID"
#define HEROES_PRJ_ID "HEROES_ORGANIZATION_NAME"
#define HEROES_ORG_ID "HEROES_ORGANIZATION_ID"


#define HEROES_LOGS_PATH "HEROES_LOGS_PATH"



static char * extra_header="HEROES_INSTANCE_WORKFLOW_ID;HEROES_TEMPLATE_WORKFLOW_ID;HEROES_USER_ID;HEROES_ORGANIZATION_NAME;HEROES_ORGANIZATION_ID;";
static char * extra_header_app="HEROES_INSTANCE_WORKFLOW_ID;HEROES_TEMPLATE_WORKFLOW_ID;HEROES_USER_ID;HEROES_ORGANIZATION_NAME;HEROES_ORGANIZATION_ID;MAX_POWER_W;";
static char app_header[2048];

static char *heroes_wrf_id = NULL;
static char *heroes_her_id = NULL;
static char *heroes_usr_id = NULL;
static char *heroes_prj_id = NULL;
static char *heroes_org_id = NULL;

state_t report_init(report_id_t *id,cluster_conf_t *cconf)
{
    debug("heroes report_init");
    char nodename[128];
    char *default_heroes_log_path = ".";
    int ret;
		if (id->master_rank >= 0 ) must_report = 1;
		if (!must_report) return EAR_SUCCESS;
    gethostname(nodename, sizeof(nodename));
    strtok(nodename, ".");

    csv_log_file_env = getenv(HEROES_LOGS_PATH);
    if (csv_log_file_env == NULL){ 
      csv_log_file_env = getcwd(path_base, sizeof(path_base));
    }
    
    /* Loop filename is automatically generated */
    if (csv_log_file_env != NULL){
	      debug("Using PATH :%s", csv_log_file_env);
				if (file_is_directory(csv_log_file_env) || (csv_log_file_env[strlen(csv_log_file_env) - 1] == '/')){ 
					debug("%s is directory", csv_log_file_env);
          sprintf(path_dir_app,"%s/heroes_app_logs", csv_log_file_env);
          ret = mkdir(path_dir_app, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
          if ((ret < 0 ) && (errno != EEXIST)){
            error("Creating heroes app dir (%s) (%s)",path_dir_app, strerror(errno));
            must_report = 0;
            return EAR_ERROR;
          }
          sprintf(path_dir_loops,"%s/heroes_loop_logs", csv_log_file_env);
          ret = mkdir(path_dir_loops, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
          if ((ret < 0 ) && (errno != EEXIST)){
            error("Creating heroes loops dir (%s) (%s)",path_dir_loops, strerror(errno));
            must_report = 0;
            return EAR_ERROR;
          }
					//xsnprintf(csv_log_file,sizeof(csv_log_file),"%s/%s.heroes",csv_log_file_env,nodename);
					xsnprintf(csv_log_file,sizeof(csv_log_file),"%s/heroes.%d.csv",path_dir_app, id->master_rank);
          xsnprintf(csv_loop_log_file,sizeof(csv_loop_log_file),"%s/heroes.loops.%d.csv",path_dir_loops,id->master_rank);
          debug("csv_log_file %s" , csv_log_file);
          debug("csv_loop_log_file %s", csv_loop_log_file);
				} 
    }else{
      must_report = 0;
      return EAR_ERROR;
    }
    my_time = timestamp_getconvert(TIME_SECS);

		/* HEROES specific */
		heroes_wrf_id = getenv(HEROES_WRF_ID);
		heroes_her_id = getenv(HEROES_HER_ID);
		heroes_usr_id = getenv(HEROES_USR_ID);
		heroes_prj_id = getenv(HEROES_PRJ_ID);
		heroes_org_id = getenv(HEROES_ORG_ID);

		if (heroes_wrf_id == NULL) heroes_wrf_id = NULL_WRF_ID;
		if (heroes_her_id == NULL) heroes_her_id = NULL_HER_ID;
		if (heroes_usr_id == NULL) heroes_usr_id = NULL_USR_ID;
		if (heroes_prj_id == NULL) heroes_prj_id = NULL_PRJ_ID;
		if (heroes_org_id == NULL) heroes_org_id = NULL_ORG_ID;

		snprintf(heroes_ids, sizeof(heroes_ids), "%s;%s;%s;%s;%s;", heroes_wrf_id, heroes_her_id, heroes_usr_id, heroes_prj_id, heroes_org_id);

		debug("heroes report init done");

    return EAR_SUCCESS;
}

static uint file_app_created = 0;
static int fd_app = -1;
state_t report_applications(report_id_t *id,application_t *apps, uint count)
{
    int i;
    debug("heroes report_applications");
    sig_ext_t *sigex;
		if (!must_report) return EAR_SUCCESS;
    if ((apps == NULL) || (count == 0)) return EAR_SUCCESS;
    if (!file_app_created && !file_is_regular(csv_log_file)){
			debug("heroes creating app header");
      uint num_gpus = 0;
#if USE_GPUS
      //for (uint l = 0; l < count; l++) num_gpus = ear_max(num_gpus, apps[l].signature.gpu_sig.num_gpus);
      num_gpus = MAX_GPUS_SUPPORTED;
			debug("Using %d GPUS", num_gpus);
#endif

      strcpy(app_header, extra_header_app);
      /* Headers for phases */
      for (uint ph = 1; ph < EARL_MAX_PHASES; ph++){
        strcat(app_header, phase_to_str(ph));
        strcat(app_header, ";");
      }

      create_app_header(app_header, csv_log_file, num_gpus, 1, 1);
      file_app_created = 1;
			fd_app = open(csv_log_file, O_WRONLY|O_APPEND);
    }
		if (!file_app_created) fd_app = open(csv_log_file, O_WRONLY|O_APPEND);
		

    for (i=0;i<count;i++){
        if (fd_app >= 0){
          dprintf(fd_app, "%s%.2lf;", heroes_ids,apps[i].power_sig.max_DC_power);
          if (apps[i].signature.sig_ext){
            sigex = (sig_ext_t *) apps[i].signature.sig_ext;
            for (uint ph = 1; ph < EARL_MAX_PHASES; ph++){
              dprintf(fd_app, "%llu;", sigex->earl_phase[ph].elapsed);
            }
          }
        }
        append_application_text_file(csv_log_file,&apps[i],1, 0, 1);
    }
    return EAR_SUCCESS;
}
static uint file_loop_created = 0;
static int fd_loops = -1;
state_t report_loops(report_id_t *id,loop_t *loops, uint count)
{
    int i;
    ullong currtime;
    debug("heroes report_loops");
		if (!must_report) return EAR_SUCCESS;
    // TODO: we could return EAR_ERROR
    if ((loops == NULL) || (count == 0)) return EAR_SUCCESS;

		debug("header created %u , %s is regular %d", file_loop_created, csv_loop_log_file, file_is_regular(csv_loop_log_file));
    if (!file_loop_created && !file_is_regular(csv_loop_log_file)){
			debug("heroes creating loop header: GPUS on=%d", USE_GPUS);
    	uint num_gpus = 0;
#if USE_GPUS
    	//for (uint l = 0; l < count; l++) num_gpus = ear_max(num_gpus, loops[l].signature.gpu_sig.num_gpus);
      num_gpus = MAX_GPUS_SUPPORTED;
			debug("Using %d GPUS", num_gpus);
#endif
      create_loop_header(extra_header, csv_loop_log_file, 1, num_gpus, 1);
			file_loop_created = 1;
			fd_loops = open(csv_loop_log_file, O_WRONLY|O_APPEND);
    }
		if (!file_loop_created) fd_loops = open(csv_loop_log_file, O_WRONLY|O_APPEND);

    ullong sec = timestamp_getconvert(TIME_SECS);
    currtime = sec - my_time;
    for (i=0;i<count;i++){
        // TODO: we could return EAR_ERROR in case the below functions returns EAR_ERROR
				if (fd_loops >= 0){
					dprintf(fd_loops, "%s", heroes_ids);
				}
        append_loop_text_file_no_job_with_ts(csv_loop_log_file,&loops[i], currtime, 0, 1, ',');
    }
    return EAR_SUCCESS;
}
