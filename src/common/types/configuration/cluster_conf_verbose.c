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
#include <stdlib.h>
#include <common/config.h>
#include <common/output/verbose.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/types/configuration/cluster_conf_db.h>
#include <common/types/configuration/cluster_conf_tag.h>
#include <common/types/configuration/cluster_conf_eard.h>
#include <common/types/configuration/cluster_conf_eargm.h>
#include <common/types/configuration/cluster_conf_eardbd.h>
#include <common/types/configuration/cluster_conf_generic.h>



void print_islands_conf(node_island_t *conf)
{
	int i, j;
	verbosen(VCCONF, "Islands configuration\n");
	verbosen(VCCONF, "--->id: %u (min_power %.0lf, max_power %.0lf,power_cap %.1lf power_cap_type=%s)\n", conf->id,conf->min_sig_power,conf->max_sig_power,conf->max_power_cap,conf->power_cap_type);
	verbosen(VCCONF, "--->       (power>%.0lf or temp>%lu are errors)\n", conf->max_error_power,conf->max_temp);
    if (conf->num_tags > 0)
    {
        verbosen(VCCONF, "--->       tags: ");
        for (i = 0; i < conf->num_tags; i++)
        {
            verbosen(VCCONF, "tag%d:%s  ", i, conf->tags[i]);
        }
        verbosen(VCCONF, "\n");
    }
	for (i = 0; i < conf->num_ranges; i++)
	{
       
        if (conf->ranges[i].db_ip < 0){
		    verbosen(VCCONF, "---->prefix: %s\tstart: %u\tend: %u\n",
                conf->ranges[i].prefix, conf->ranges[i].start, conf->ranges[i].end);

        }else if (conf->ranges[i].sec_ip < 0){
		    verbosen(VCCONF, "---->prefix: %s\tstart: %u\tend: %u\tip: %s\n",
                conf->ranges[i].prefix, conf->ranges[i].start, conf->ranges[i].end, conf->db_ips[conf->ranges[i].db_ip]);
    
        }else{
		    verbosen(VCCONF, "---->prefix: %s\tstart: %u\tend: %u\tip: %s\tbackup: %s\n",
                conf->ranges[i].prefix, conf->ranges[i].start, conf->ranges[i].end, conf->db_ips[conf->ranges[i].db_ip], conf->backup_ips[conf->ranges[i].sec_ip]);
		}
        if (conf->ranges[i].num_tags > 0) {
            verbosen(VCCONF, "\t----tags: ");
            for (j = 0; j < conf->ranges[i].num_tags; j++)
            {
                verbosen(VCCONF, "tag%d:%s  ", j, conf->specific_tags[conf->ranges[i].specific_tags[j]]);
            }
            verbosen(VCCONF, "\n");
        }

	}
}




void print_cluster_conf(cluster_conf_t *conf)
{
	verbosen(VCCONF, "\nDIRECTORIES\n--->DB_pathname: %s\n--->TMP_dir: %s\n--->ETC_dir: %s\n",
			conf->DB_pathname, conf->install.dir_temp, conf->install.dir_conf);
	verbosen(VCCONF, "\nPlugins_path: %s\n",conf->install.dir_plug);
	verbosen(VCCONF, "PLUGINS: Energy %s power_models %s\n",conf->install.obj_ener,conf->install.obj_power_model);
    if (strlen(conf->net_ext) > 1)
    {
    	verbosen(VCCONF, "\nGLOBALS\n--->Verbose: %u\n--->Default_policy: %s\n--->Min_time_perf_acc: %u\n--->Network_extension: %s\n",
	    		conf->verbose, conf->power_policies[conf->default_policy].name, conf->min_time_perf_acc, conf->net_ext);
    }
    else
    {
    	verbosen(VCCONF, "\nGLOBALS\n--->Verbose: %u\n--->Default_policy: %s\n--->Min_time_perf_acc: %u\n",
	    		conf->verbose, conf->power_policies[conf->default_policy].name, conf->min_time_perf_acc);
    }

	int i;
	verbosen(VCCONF, "\nAVAILABLE POLICIES\n");
	for (i = 0; i < conf->num_policies; i++)
		print_policy_conf(&conf->power_policies[i]);
	verbosen(VCCONF, "\nPRIVILEGED USERS\n");
	for (i = 0; i < conf->num_priv_users; i++)
		verbosen(VCCONF, "--->user: %s\n", conf->priv_users[i]);
	verbosen(VCCONF, "\nPRIVILEGED GROUPS\n");
	for (i = 0; i < conf->num_priv_groups; i++)
		verbosen(VCCONF, "--->groups: %s\n", conf->priv_groups[i]);
	verbosen(VCCONF, "\nPRIVILEGED ACCOUNTS\n");
	for (i = 0; i < conf->num_acc; i++)
		verbosen(VCCONF, "--->acc: %s\n", conf->priv_acc[i]);
	verbosen(VCCONF, "\nNODE CONFIGURATIONS\n");
	for (i = 0; i < conf->num_nodes; i++)
		print_node_conf(&conf->nodes[i]);
	verbosen(VCCONF, "\nDB MANAGER\n");
	print_db_manager(&conf->db_manager);
	verbosen(VCCONF, "\nEARD CONFIGURATION\n");
	print_eard_conf(&conf->eard);
	verbosen(VCCONF, "\nEARGM CONFIGURATION\n");
	print_eargm_conf(&conf->eargm);
	print_database_conf(&conf->database);

	verbosen(VCCONF, "\nISLES\n");
	for (i = 0; i < conf->num_islands; i++)
		print_islands_conf(&conf->islands[i]);
	
    verbosen(VCCONF, "\nGENERAL TAGS\n");
    for (i = 0; i < conf->num_tags; i++)
        print_tags_conf(&conf->tags[i]);

	verbosen(VCCONF, "\nENERGY TAGS\n");
	for (i = 0; i < conf->num_etags; i++)
		print_energy_tag(&conf->e_tags[i]);

    verbosen(VCCONF, "\nLIBRARY CONF\n");
    print_earlib_conf(&conf->earlib);

	verbosen(VCCONF, "\n");
}
