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
#include <common/utils/string.h>
#include <common/config/config_env.h>
#include <common/config/config_def.h>
#include <common/states.h>
#include <common/types/version.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/system/user.h>

#define DEFAULT_EARL 0

void Usage(char *cprog)
{
  printf("Usage: %s [options]\n"
         "\t--node-conf[=nodename]\n"
         "\t--help\n", cprog);
  exit(0);
}

void print_value(char *format, char *env, int def)
{
  char buff[64];
  sprintf(buff,"%d",def);
  printf("%40.40s %s\n",format,(ear_getenv(env) == NULL)? buff: ear_getenv(env));
}

void print_value_float(char *format, char *env, float def)
{
  char buff[64];
  sprintf(buff,"%.2f",def);
  printf("%40.40s %s\n",format,(ear_getenv(env) == NULL)? buff: ear_getenv(env));
}

void print_value_str(char *format, char *env, char * def)
{
  printf("%40.40s %s\n",format,(ear_getenv(env) == NULL)? def: ear_getenv(env));
}


int main(int argc, char *argv[])
{
	cluster_conf_t my_cluster;
	char ver[128];
	char ear_path[256];
  char *nodename = NULL;

  int option_idx = 0;

  static struct option long_options[] = {
    {"node-conf", required_argument, 0, 'n'},
    {"help",      no_argument,       0, 'h'},
  };

  int opt = getopt_long(argc, argv, "n:h", long_options, &option_idx);
  if (opt != -1)
  {
    switch(opt)
    {
        case 'n': 
          if (optarg)
          {
            nodename = optarg; 
            printf("nodeconf with %s\n", nodename);
          }
          break;
        default: /* ? */
          Usage(argv[0]);
    }
  }



	if (get_ear_conf_path(ear_path)==EAR_ERROR){
			printf("Error getting ear.conf path, load the ear module\n");
			exit(0);
	}
	read_cluster_conf(ear_path,&my_cluster);
	version_to_str(ver);
	printf("EAR version %s\n",ver);
  printf("Max CPUS supported set to %d\n", MAX_CPUS_SUPPORTED);
  printf("Max sockets supported set to %d\n", MAX_SOCKETS_SUPPORTED);
	#if USE_GPUS
	printf("EAR installed with GPU support MAX_GPUS %d\n",MAX_GPUS_SUPPORTED);
	#else
	printf("EAR installed without GPU support\n");
	#endif
	if (my_cluster.eard.force_frequencies){
		printf("EAR controls node CPU frequencies");	
	}
	printf("Default cluster policy is %s\n",my_cluster.power_policies[my_cluster.default_policy].name);
	printf("EAR optimization  by default set to %d\n",DEFAULT_EARL);		


  if (!is_privileged_command(&my_cluster)) return 0;

  my_node_conf_t *my_node_conf = NULL;
  if (nodename != NULL){
    strtok(nodename, ".");
    my_node_conf = get_my_node_conf(&my_cluster, nodename); 
    if (my_node_conf == NULL){
      printf("Error, Node %s not found\n", nodename);
      exit(0);
    }
    printf("\n\nInformation for node %s..................\n", nodename);
    report_my_node_conf(my_node_conf);
    printf("............................................\n");
  }

  printf("\n\nEnvironment configuration section..............\n");
  print_value("eUFS ", FLAG_SET_IMCFREQ, EAR_eUFS);
  print_value_float("eUFS limit", FLAG_IMC_TH, EAR_IMC_TH); 
  print_value("Load balanced enabled",FLAG_LOAD_BALANCE,EAR_USE_LB);
  print_value_float("Load Balance th", FLAG_LOAD_BALANCE_TH, EAR_LB_TH); 
  print_value("Use turbo for critical path",FLAG_TURBO_CP,EAR_USE_TURBO_CP);
  print_value("Use turbo", FLAG_TRY_TURBO, USE_TURBO);
  print_value("Exclusive mode", FLAG_EXCLUSIVE_MODE, 0);
  print_value("Use EARL phases", FLAG_EARL_PHASES, EAR_USE_PHASES);
  print_value("Use energy models", FLAG_USE_ENERGY_MODELS, 1);
  if (my_node_conf)
  {
    print_value("Minimum CPU freq/pstate", FLAG_MIN_CPUFREQ,my_node_conf->cpu_max_pstate); 
  }
  print_value("Max IMC frequency (0 = not defined)", FLAG_MAX_IMCFREQ, 0); 
  print_value("Min IMC frequency (0 = not defined)", FLAG_MIN_IMCFREQ, 0); 
  print_value("GPU frequency/pstate (0 = max GPU freq)", FLAG_GPU_DEF_FREQ, 0);
  print_value("MPI optimization", FLAG_MPI_OPT,0); 
  print_value("MPI statistics", FLAG_GET_MPI_STATS, 0);
  print_value_str("App. Tracer", FLAG_TRACE_PLUGIN, "no trace");
  print_value_str("App. Extra report plugins", FLAG_REPORT_ADD, "no extra plugins");
  print_value("App. reporting loops to EARD", FLAG_REPORT_LOOPS, 1);

  printf("............................................\n");

  printf("\n\nHACK section............................\n");
  char *installp = ear_getenv(ENV_PATH_EAR);
  char *hack = ear_getenv(HACK_EARL_INSTALL_PATH);
  char path_files[256];
  char install[256];
  print_value_str("Install path", HACK_EARL_INSTALL_PATH, installp);
  if (hack == NULL) strcpy(install, installp);
  else              strcpy(install, hack); 

  if (my_node_conf)
  {
    xsnprintf(path_files, sizeof(path_files), "%s/lib/plugins/policies/%s.so",install, my_node_conf->policies[my_cluster.default_policy].name);
  }
  print_value_str("Energy optimization policy", HACK_POWER_POLICY, path_files);
  #if USE_GPUS
  #ifdef GPU_OPT 
  if (my_node_conf)
  {
    xsnprintf(path_files, sizeof(path_files), "%s/lib/plugins/policies/gpu_%s.so",install, my_node_conf->policies[my_cluster.default_policy].name);
  }
  #else
  xsnprintf(path_files, sizeof(path_files), "%s/lib/plugins/policies/gpu_monitoring.so",install);
  #endif
  print_value_str("GPU power policy", HACK_GPU_POWER_POLICY, path_files);
  #endif
  if (my_node_conf)
  {
    xsnprintf(path_files, sizeof(path_files), "%s/lib/plugins/models/%s.so",install, my_node_conf->energy_model);
  }
  print_value_str("CPU power  model", HACK_POWER_MODEL, path_files);
  xsnprintf(path_files, sizeof(path_files), "%s/lib/plugins/models/%s",install, "cpu_power_model_default.so");
  print_value_str("CPU shared power  model", HACK_CPU_POWER_MODEL, path_files);


  printf("............................................\n");

  if (nodename)
  {
    char buffer[256];
    printf("Validating status of EARD in node  (econtrol --status=%s)\n", nodename);
    sprintf(buffer,"econtrol --status=%s", nodename);
    system(buffer);
    printf("Validating status of power status in node  (econtrol --status=%s --type=power)\n", nodename);
    sprintf(buffer,"econtrol --status=%s --type=power", nodename);
    system(buffer);
  }


  return 0;
}

