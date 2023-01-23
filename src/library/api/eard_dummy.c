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

#define _GNU_SOURCE
#include <sched.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
//#define SHOW_DEBUGS 1
#include <common/config.h>
#include <common/config/config_env.h>
#include <common/output/verbose.h>
#include <library/common/verbose_lib.h>
#include <library/common/global_comm.h>
#include <common/output/debug.h>
#include <common/states.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/types/configuration/cluster_conf_generic.h>
#include <daemon/local_api/node_mgr.h>


#include <daemon/shared_configuration.h>

#define DEFAULT_EAR_INSTALL_PATH "/usr/lib/ear"
#define DEFAULT_EAR_ETC_PATH "/etc"
#define DEFAULT_ENERGY_PLUGIN "energy_rapl.so"


static char path[GENERIC_NAME];
static cluster_conf_t dummy_cc;
extern masters_info_t masters_info;
static my_node_conf_t *dummy_node_conf;
static char nodename[MAX_PATH_SIZE];
static ear_njob_t *dummy_nodemgr;

#define check_null(p, str, arg) \
	if (p == NULL){ \
		verbose_master(2, str, arg);\
		return EAR_ERROR;\
	}

void create_dummy_path_lock(char *ear_tmp, uint ID, char *dummy_areas_path, uint size)
{
	snprintf(dummy_areas_path, size, "%s/.dummy_areas_lock.%u", ear_tmp, ID);
}

static void create_dummy_island(cluster_conf_t *dummycc)
{
	dummycc->num_islands = 1;
	dummycc->islands = calloc(1, sizeof(node_island_t));
	set_default_island_conf(&dummycc->islands[0], 0);
	dummycc->islands[0].num_ips = 1;
	dummycc->islands[0].db_ips = calloc(1, sizeof(char *));
	dummycc->islands[0].db_ips[0] = calloc(1, strlen(nodename)+1);
	strcpy(dummycc->islands[0].db_ips[0], nodename);
	generate_node_ranges(&dummycc->islands[0], nodename);
	dummycc->num_tags = 1;
	dummycc->tags = calloc(1, sizeof(tag_t));
	memset(dummycc->tags, 0, sizeof(tag_t));
	set_default_tag_values(dummycc->tags);
	strcpy(dummycc->tags[0].energy_model, "avx512_model.so");
	strcpy(dummycc->tags[0].energy_plugin, DEFAULT_ENERGY_PLUGIN);
	strcpy(dummycc->tags[0].powercap_plugin, "");
	strncpy(dummycc->tags[0].id,  "default", sizeof(dummycc->tags[0].id));
	dummycc->tags[0].is_default = 1;
		
}

static void create_dummy_node_conf(cluster_conf_t *dummycc, char *nodename)
{
  node_island_t *island = &dummycc->islands[dummycc->num_islands - 1];
  island->ranges = realloc(island->ranges, sizeof(node_range_t)*(island->num_ranges+1));
  if (island->ranges==NULL){
    error("NULL pointer in generate_node_ranges");
    return;
  }
  memset(&island->ranges[island->num_ranges], 0, sizeof(node_range_t));
  sprintf(island->ranges[island->num_ranges].prefix, "%s", nodename);
  island->ranges[island->num_ranges].start = island->ranges[island->num_ranges].end = -1;
  island->ranges[island->num_ranges].db_ip = island->ranges[island->num_ranges].sec_ip = 0;
  island->num_ranges++;
}

static void create_dummy_policy(cluster_conf_t *dummycc)
{
	dummycc->num_policies = 1;
	dummycc->default_policy = 0;
	dummycc->power_policies = calloc(1, sizeof(policy_conf_t));
	dummycc->power_policies[0].policy = 0;
	strcpy(dummycc->power_policies[0].name, "monitoring");
	dummycc->power_policies[0].p_state = 0;
	dummycc->power_policies[0].is_available = 1;
}

static void create_dummy_conf_paths(cluster_conf_t *dummycc, char *ear_tmp)
{
	char *install = getenv("EAR_INSTALL_PATH");
	char *etc = getenv("EAR_ETC");
	strcpy(dummycc->install.dir_temp, ear_tmp);
	if (install != NULL)	strcpy(dummycc->install.dir_inst, install);
	else                    strcpy(dummycc->install.dir_inst, DEFAULT_EAR_INSTALL_PATH); 
	if (etc != NULL)        strcpy(dummycc->install.dir_conf, etc);
	else 			strcpy(dummycc->install.dir_conf, DEFAULT_EAR_ETC_PATH);
	strcpy(dummycc->install.dir_plug, dummycc->install.dir_inst);
	strcat(dummycc->install.dir_plug, "/lib/plugins");
	strcpy(dummycc->install.obj_power_model, "cpu_power_model_dummy.so");
	strcpy(dummycc->install.obj_ener, DEFAULT_ENERGY_PLUGIN);
}
state_t eard_dummy_cluster_conf(char *ear_tmp, uint ID)
{
				char *dummy_ser_cc,  *local_dummy_ser_cc;
				size_t dummy_ser_cc_size;
        state_t cc_read_result, check_cc = 1;
				if (gethostname(nodename, sizeof(nodename)) < 0) {
        				verbose_master(0, "Error getting node name (%s)", strerror(errno));
        			}
        			strtok(nodename, ".");

				/* Init dummy_cc */

        #ifdef FAKE_EAR_NOT_INSTALLED 
        #if FAKE_EAR_NOT_INSTALLED
        check_cc = 0;
        #endif
        #endif

        if (check_cc){
          cc_read_result = get_ear_conf_path(path);
        }else{
          verbose_master(0," FAKE_EAR_NOT_INSTALLED when creating cluster conf");
          cc_read_result = EAR_ERROR;
        }

				if (cc_read_result != EAR_ERROR){
					verbose_master(2, "Reading cluster_conf");
					read_cluster_conf(path, &dummy_cc);
				}else{
					memset(&dummy_cc, 0 , sizeof(cluster_conf_t));
					set_ear_conf_default(&dummy_cc);
					create_dummy_conf_paths(&dummy_cc, ear_tmp);
					create_dummy_island(&dummy_cc);
					create_dummy_policy(&dummy_cc);
				}
				print_cluster_conf(&dummy_cc);
				
				verbose_master(2, "Looking for node conf. Nodename %s", nodename);
				dummy_node_conf = get_my_node_conf(&dummy_cc, nodename);
				if (dummy_node_conf == NULL){
          create_dummy_node_conf(&dummy_cc, nodename);
          dummy_node_conf = get_my_node_conf(&dummy_cc, nodename);
        }
				check_null(dummy_node_conf, "Invalid configuration file for node %s", nodename);
        if (dummy_node_conf != NULL){
          print_my_node_conf(dummy_node_conf);
        }


				/* serialization */
				serialize_cluster_conf(&dummy_cc, &dummy_ser_cc, &dummy_ser_cc_size);
				get_ser_cluster_conf_path(ear_tmp, path);
				local_dummy_ser_cc = create_ser_cluster_conf_shared_area(path, dummy_ser_cc, dummy_ser_cc_size);

				check_null(local_dummy_ser_cc, "Error serializing cluster_conf in path %s", " ");


				verbose_master(2, "Dummy cluster_conf done");

				return EAR_SUCCESS;
}

state_t eard_dummy_earl_settings(char *ear_tmp, uint ID)
{
				settings_conf_t * dummy_sett;
				resched_t       * dummy_resch;
  			policy_conf_t   * my_policy;
			ulong             freq_base;
			topology_freq_getbase(0, &freq_base);
			//verbose_master(1,"Using freq_base %lu", freq_base);

				verbose_master(2, "looking for policy %d", dummy_cc.default_policy);	
  			my_policy = get_my_policy_conf(dummy_node_conf,dummy_cc.default_policy);
				check_null(my_policy, "My policy returns NULL for policy %d", dummy_cc.default_policy);

				get_settings_conf_path(ear_tmp, ID, path);
				dummy_sett = create_settings_conf_shared_area(path);
				check_null(dummy_sett, "create_settings_conf_shared_area returns NULL for path %s", path);

				memset(dummy_sett, 0, sizeof(settings_conf_t));

  			dummy_sett->id             = ID;
				dummy_sett->user_type	  	 = AUTHORIZED;
				dummy_sett->lib_enabled = 1;
				dummy_sett->policy           = dummy_cc.default_policy;
				dummy_sett->def_freq         = freq_base;
  			dummy_sett->def_p_state      = my_policy->p_state;
  			dummy_sett->max_freq         = freq_base;
  			strncpy(dummy_sett->policy_name, my_policy->name, 64);
  			memcpy(dummy_sett->settings, my_policy->settings, sizeof(double)*MAX_POLICY_SETTINGS);
  			dummy_sett->min_sig_power    = dummy_node_conf->min_sig_power;
  			dummy_sett->max_sig_power    = dummy_node_conf->max_sig_power;
  			dummy_sett->max_power_cap    = dummy_node_conf->powercap;
  			dummy_sett->report_loops     = dummy_cc.database.report_loops;
  			dummy_sett->max_avx512_freq  = dummy_node_conf->max_avx512_freq;
  			dummy_sett->max_avx2_freq    = dummy_node_conf->max_avx2_freq;
				dummy_sett->pc_opt.current_pc     = POWER_CAP_UNLIMITED;
  			memcpy(&dummy_sett->installation , &dummy_cc.install,sizeof(conf_install_t));
  			copy_ear_lib_conf(&dummy_sett->lib_info , &dummy_cc.earlib);
	

				verbose_master(2, "Dummy earl settings done");

				get_resched_path(ear_tmp, ID, path);
				dummy_resch = create_resched_shared_area(path);

				check_null(dummy_resch, "Resched are returns NULL for path %s", path);

				dummy_resch->force_rescheduling = 0;

				verbose_master(2, "Dummy resched are done");

				return EAR_SUCCESS;

}

state_t eard_dummy_app_mgt(char *ear_tmp, uint ID)
{
				get_app_mgt_path(ear_tmp, ID, path);
				app_mgt_t *dummy = create_app_mgt_shared_area(path);
				check_null(dummy, "Error creating app_mgr in path %s ", path);

				verbose_master(2, "Dummy app_mgt area done");

				return EAR_SUCCESS;
}

state_t eard_dummy_pc_app_info(char *ear_tmp, uint ID)
{
				pc_app_info_t *dummy;

				get_pc_app_info_path(ear_tmp, ID, path);	
				dummy = create_pc_app_info_shared_area(path);
				check_null(dummy, "Error creating pc area in path %s", path);

				memset(dummy, 0 , sizeof(pc_app_info_t));
				dummy->cpu_mode = PC_POWER;
				#if USE_GPUS
				dummy->gpu_mode = PC_POWER;
				#endif

				verbose_master(2, "Dummy powercap are done");
	
				return EAR_SUCCESS;
}

state_t eard_dummy_nodemgr(char *ear_tmp, uint ID)
{
				nodemgr_server_init(ear_tmp, &dummy_nodemgr);
				check_null(dummy_nodemgr, "Error creating dummy node_mgr %s", " ");				

				verbose_master(2, "Dummy node mgr area done");
				return EAR_SUCCESS;
}

state_t eard_dummy_coefficients(char *ear_tmp, uint ID)
{
				coefficient_t *coeffs, *dummy_coeff;

				/* How many coefficients ? Depends on frequencies */
				int size = sizeof(coefficient_t);
				coeffs = calloc(1, size);
				check_null(coeffs, "Error allocating memory for coefficients %s", " ");

				get_coeffs_default_path(ear_tmp, path);
				dummy_coeff = create_coeffs_default_shared_area(path, coeffs, size);
				check_null(dummy_coeff, "Error creating coefficients for path %s", path);

				verbose_master(2, "Dummy coefficients area done");
				return EAR_SUCCESS;
}

state_t eard_dummy_frequency_list(char *ear_tmp, uint ID)
{
				ulong * frequencies, *dummy_frequencies, freq_base;
				int ps;
				ps = 1;
				int size = ps * sizeof(ulong);
				frequencies = (ulong *) malloc(size);
				check_null(frequencies, "Error allocating memory for frequencies %s", " ");	
				topology_freq_getbase(0, &freq_base);
				frequencies[0] = freq_base;
				//verbose_master(1,"Using freq_base %lu", freq_base);

				get_frequencies_path(ear_tmp, path);
				dummy_frequencies = create_frequencies_shared_area(path, frequencies, size);
				check_null(dummy_frequencies, "Error creating frequencies area at path %s", path);	

				return EAR_SUCCESS;
	
}


/* This function is called when metrics are not yet initialized */
state_t create_eard_dummy_shared_regions(char *ear_tmp, uint ID)
{
				verbose(2,"Creating dummy areas in %s and ID %u", ear_tmp, ID);
				eard_dummy_cluster_conf(ear_tmp, ID);
				eard_dummy_earl_settings(ear_tmp, ID);
				eard_dummy_nodemgr(ear_tmp, ID);
				eard_dummy_frequency_list(ear_tmp, ID);
				eard_dummy_app_mgt(ear_tmp, ID);
				eard_dummy_pc_app_info(ear_tmp, ID);
				eard_dummy_coefficients(ear_tmp, ID);

				verbose_master(2,"All the dummy regions ready");
				return EAR_SUCCESS;
}


