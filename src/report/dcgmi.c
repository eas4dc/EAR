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
#if 0
static char *csv_log_file_env_loops;
#endif
static char csv_loop_log_file[1024];
static char csv_log_file[1024];
static char dcgmi_metrics[1024];
static char path_base[1024];
static char path_dir_app[1024], path_dir_loops[1024];

static ullong my_time = 0;

static uint must_report;




static char extra_header[8192];
static char extra_header_app[8192];
static char extra_metrics[8192];
static char app_header[8192];

#define DCGMI_LOGS_PATH "DCGMI_LOGS_PATH"


state_t report_init(report_id_t *id,cluster_conf_t *cconf)
{
    debug("dcgmi report_init");
    char nodename[128];
#if 0
    char *default_dcgmi_log_path = ".";
#endif
    int ret;
		if (id->master_rank >= 0 ) must_report = 1;
		if (!must_report) return EAR_SUCCESS;
    gethostname(nodename, sizeof(nodename));
    strtok(nodename, ".");

    csv_log_file_env = ear_getenv(DCGMI_LOGS_PATH);
    if (csv_log_file_env == NULL){ 
      csv_log_file_env = getcwd(path_base, sizeof(path_base));
    }
    
    /* Loop filename is automatically generated */
    if (csv_log_file_env != NULL){
       if (!file_is_directory(csv_log_file_env)){
          if (file_is_regular(csv_log_file_env)){
            error("Invalid dcgmi path.It  must be a directory, not a regular file (%s)", csv_log_file_env);
            must_report = 0;
            return EAR_ERROR;
          }
          ret = mkdir(csv_log_file_env, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
          if (ret < 0){
            error("Creating dcgmi app dir (%s) (%s)",csv_log_file_env, strerror(errno));
            must_report = 0;
            return EAR_ERROR;
          }
        }

	      debug("Using PATH :%s", csv_log_file_env);
				if (file_is_directory(csv_log_file_env) || (csv_log_file_env[strlen(csv_log_file_env) - 1] == '/')){ 
					debug("%s is directory", csv_log_file_env);
          sprintf(path_dir_app,"%s/dcgmi_app_logs", csv_log_file_env);
          ret = mkdir(path_dir_app, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
          if ((ret < 0 ) && (errno != EEXIST)){
            error("Creating dcgmi app dir (%s) (%s)",path_dir_app, strerror(errno));
            must_report = 0;
            return EAR_ERROR;
          }
          sprintf(path_dir_loops,"%s/dcgmi_loop_logs", csv_log_file_env);
          ret = mkdir(path_dir_loops, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
          if ((ret < 0 ) && (errno != EEXIST)){
            error("Creating dcgmi loops dir (%s) (%s)",path_dir_loops, strerror(errno));
            must_report = 0;
            return EAR_ERROR;
          }
					//xsnprintf(csv_log_file,sizeof(csv_log_file),"%s/%s.dcgmi",csv_log_file_env,nodename);
					xsnprintf(csv_log_file,sizeof(csv_log_file),"%s/dcgmi.%d.csv",path_dir_app, id->master_rank);
          xsnprintf(csv_loop_log_file,sizeof(csv_loop_log_file),"%s/dcgmi.loops.%d.csv",path_dir_loops,id->master_rank);
          debug("csv_log_file %s" , csv_log_file);
          debug("csv_loop_log_file %s", csv_loop_log_file);
				} 
    }else{
      must_report = 0;
      return EAR_ERROR;
    }
    my_time = timestamp_getconvert(TIME_SECS);



		debug("dcgmi report init done");

    return EAR_SUCCESS;
}

static uint file_app_created = 0;
static int fd_app = -1;
state_t report_applications(report_id_t *id,application_t *apps, uint count)
{
    int i;
    uint num_gpus = 0;
    debug("dcgmi report_applications");
    sig_ext_t *sigex;
    dcgmi_sig_t *dcgmis;
		if (!must_report) return EAR_SUCCESS;
    if ((apps == NULL) || (count == 0)) return EAR_SUCCESS;
    num_gpus = MAX_GPUS_SUPPORTED;

    if (!file_app_created && !file_is_regular(csv_log_file)){
			debug("dcgmi creating app header");
#if USE_GPUS
      //for (uint l = 0; l < count; l++) num_gpus = ear_max(num_gpus, apps[l].signature.gpu_sig.num_gpus);
			debug("Using %d GPUS", num_gpus);
#endif
      #if DCGMI
      sigex = (sig_ext_t *)apps[0].signature.sig_ext;
      dcgmis = (dcgmi_sig_t *)&sigex->dcgmis; 
      char dcgmi_ev_name[128];
      strcpy(extra_header_app,"DCGMI_NUM_EVENTS;");
      for (uint set = 0; set < dcgmis->dcgmi_sets; set++){
				uint *ev_list = dcgmis->dcgmi_events[set];
        for (uint ev = 0; ev < dcgmis->dcgmi_num_ev; ev++){
          //strcpy(dcgmi_ev_name , gpu_dcgmi_event_to_name(dcgmis->dcgmi_events[set][ev]);
          for (uint gpus = 0; gpus < num_gpus; gpus++){
          	sprintf(dcgmi_ev_name, "GPU%u_%u", gpus, ev_list[ev]);
         	 	strcat(dcgmi_ev_name, ";");
          	strcat(extra_header_app,dcgmi_ev_name);
					}
        }
      }
      #endif
    
      strcpy(app_header, extra_header_app);
      /* Headers for phases */
      for (uint ph = 1; ph < EARL_MAX_PHASES; ph++){
        strcat(app_header, phase_to_str(ph));
        strcat(app_header, ";");
      }

      create_app_header(app_header, csv_log_file, num_gpus, 1, 0);
      file_app_created = 1;
			fd_app = open(csv_log_file, O_WRONLY|O_APPEND);
    }
		if (!file_app_created) fd_app = open(csv_log_file, O_WRONLY|O_APPEND);
		

    for (i=0;i<count;i++){
		debug("Reporting app %u", i);
        if (fd_app >= 0){
          #if DCGMI
          sigex = (sig_ext_t *)apps[i].signature.sig_ext;
          dcgmis = (dcgmi_sig_t *)&sigex->dcgmis;
          if (dcgmis->dcgmi_sets && num_gpus){
            char dcgmi_metrics[128];
            sprintf(extra_metrics,"%u;", dcgmis->dcgmi_sets*dcgmis->dcgmi_num_ev);
            for (uint set = 0; set < dcgmis->dcgmi_sets; set++){
              for (uint ev = 0; ev < dcgmis->dcgmi_num_ev; ev++){
                for (uint gpu = 0; gpu < num_gpus; gpu++){
                  if (dcgmis->dcgmi_gpus > gpu){  
										dcgmi_t *dcgmis_data = dcgmis->dcgmi_metrics[set];	
										sprintf(dcgmi_metrics, "%f;", dcgmis_data[gpu].values[ev]);
									}else sprintf(dcgmi_metrics, "%f;",0.0);
                  strcat(extra_metrics, dcgmi_metrics);
                }
              }
            }
            dprintf(fd_app,"%s", extra_metrics);
          }
          #endif

          if (apps[i].signature.sig_ext){
						debug("Adding phases");
            sigex = (sig_ext_t *) apps[i].signature.sig_ext;
            for (uint ph = 1; ph < EARL_MAX_PHASES; ph++){
              dprintf(fd_app, "%llu;", sigex->earl_phase[ph].elapsed);
            }
          }
        }
				debug("Reporting signature");
        append_application_text_file(csv_log_file,&apps[i],1, 0, 0);
    }
    return EAR_SUCCESS;
}
static uint file_loop_created = 0;
static int fd_loops = -1;
state_t report_loops(report_id_t *id,loop_t *loops, uint count)
{
    int i;
    ullong currtime;
    sig_ext_t *sigex;
    dcgmi_sig_t *dcgmis;
    uint num_gpus = 0;
    debug("dcgmi report_loops");
		if (!must_report) return EAR_SUCCESS;
    // TODO: we could return EAR_ERROR
    if ((loops == NULL) || (count == 0)) return EAR_SUCCESS;
    num_gpus = MAX_GPUS_SUPPORTED;

		debug("header created %u , %s is regular %d", file_loop_created, csv_loop_log_file, file_is_regular(csv_loop_log_file));
    if (!file_loop_created && !file_is_regular(csv_loop_log_file)){
			debug("dcgmi creating loop header: GPUS on=%d", USE_GPUS);
#if USE_GPUS
    	//for (uint l = 0; l < count; l++) num_gpus = ear_max(num_gpus, loops[l].signature.gpu_sig.num_gpus);
			debug("Using %d GPUS", num_gpus);
#endif
      #if DCGMI
      sigex = (sig_ext_t *)loops->signature.sig_ext;
      dcgmis = (dcgmi_sig_t *)&sigex->dcgmis;
      if (dcgmis->dcgmi_sets){
        char dcgmi_ev_name[128];
				uint *evlist;
        strcpy(extra_header,"DCGMI_NUM_EVENTS;");
        for (uint set = 0; set < dcgmis->dcgmi_sets; set++){
					evlist = dcgmis->dcgmi_events[set];
          for (uint ev = 0; ev < dcgmis->dcgmi_num_ev; ev++){
						for (uint gpu = 0; gpu < num_gpus; gpu++){
            	//strcpy(dcgmi_ev_name , gpu_dcgmi_event_to_name(dcgmis->dcgmi_events[set][ev]);
            	debug("Adding set %u ev %u gpu %u", set, ev, gpu);
            	sprintf(dcgmi_ev_name, "GPU%u_%u", gpu, evlist[ev]);
            	strcat(dcgmi_ev_name, ";");
            	strcat(extra_header,dcgmi_ev_name);
							debug("header size %u", strlen(extra_header));
						}
          }
        }
      }
			debug("Header create");
      #endif

      create_loop_header(extra_header, csv_loop_log_file, 1, num_gpus, 0);
			debug("Header file created");
			file_loop_created = 1;
			fd_loops = open(csv_loop_log_file, O_WRONLY|O_APPEND);
    }
		if (!file_loop_created) fd_loops = open(csv_loop_log_file, O_WRONLY|O_APPEND);

    ullong sec = timestamp_getconvert(TIME_SECS);
    currtime = sec - my_time;
    for (i=0;i<count;i++){
        // TODO: we could return EAR_ERROR in case the below functions returns EAR_ERROR
				if (fd_loops >= 0){
        #if DCGMI
        sigex = (sig_ext_t *)loops[i].signature.sig_ext;
        dcgmis = (dcgmi_sig_t *)&sigex->dcgmis;
        if (dcgmis->dcgmi_sets && num_gpus){
          char dcgmi_metrics[128];
          sprintf(extra_metrics,"%u;", dcgmis->dcgmi_sets*dcgmis->dcgmi_num_ev);
          for (uint set = 0; set < dcgmis->dcgmi_sets; set++){
            for (uint ev = 0; ev < dcgmis->dcgmi_num_ev; ev++){
              for (uint gpu = 0; gpu < num_gpus; gpu++){
							debug("Generating data for set %u ev %u gpu %u", set, ev, gpu);
							if (dcgmis->dcgmi_gpus > gpu){ 
								dcgmi_t *dcgmis_data = dcgmis->dcgmi_metrics[set];	
								sprintf(dcgmi_metrics, "%f;", dcgmis_data[gpu].values[ev]); 
							}else{ 
								sprintf(dcgmi_metrics, "%f;", 0.0);
							}
              strcat(extra_metrics, dcgmi_metrics);
              }
            }
          }
          dprintf(fd_loops,"%s", extra_metrics);
        }
        #endif
				}
        append_loop_text_file_no_job_with_ts(csv_loop_log_file,&loops[i], currtime, 0, 0, ' ');
    }
    return EAR_SUCCESS;
}
