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
	verbosen(VCCONF, "Island[%u] \n", conf->id);
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
		    verbosen(VCCONF, "---->prefix: %s\tstart: %u\tend: %u eargm: %d\n",
                conf->ranges[i].prefix, conf->ranges[i].start, conf->ranges[i].end, conf->ranges[i].eargm_id);

        }else if (conf->ranges[i].sec_ip < 0){
		    verbosen(VCCONF, "---->prefix: %s\tstart: %u\tend: %u\tip: %s eargm: %d\n",
                conf->ranges[i].prefix, conf->ranges[i].start, conf->ranges[i].end, conf->db_ips[conf->ranges[i].db_ip], conf->ranges[i].eargm_id);
    
        }else{
		    verbosen(VCCONF, "---->prefix: %s\tstart: %u\tend: %u\tip: %s\tbackup: %s eargm: %d\n",
                conf->ranges[i].prefix, conf->ranges[i].start, conf->ranges[i].end, conf->db_ips[conf->ranges[i].db_ip], conf->backup_ips[conf->ranges[i].sec_ip], conf->ranges[i].eargm_id);
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
	verbosen(VCCONF, "\n>>>>> Path specifications <<<<<\n");

	verbosen(VCCONF, "\nDB file_pathname: %s\n--->EAR_TMP: %s\n--->EAR_ETC: %s\n",
			conf->DB_pathname, conf->install.dir_temp, conf->install.dir_conf);
	verbosen(VCCONF, "\nPlugins_path: %s\n",conf->install.dir_plug);
	verbosen(VCCONF, "\nDefault plugins: Energy %s power_models %s\n",conf->install.obj_ener,conf->install.obj_power_model);
    if (strlen(conf->net_ext) > 1)
    {
    	verbosen(VCCONF, "\nVerbose: %u\n--->Default_policy: %s\n--->Min_time_perf_acc: %u\n--->Network_extension: %s\n",
	    		conf->verbose, conf->power_policies[conf->default_policy].name, conf->min_time_perf_acc, conf->net_ext);
    }
    else
    {
    	verbosen(VCCONF, "\nVerbose: %u\n--->Default_policy: %s (id %d)\n--->Min_time_perf_acc: %u\n",
	    		conf->verbose, conf->power_policies[conf->default_policy].name, conf->default_policy, conf->min_time_perf_acc);
    }

	int i;
	verbosen(VCCONF, "\n>>>>> Policies configuration section <<<<<\n");
	for (i = 0; i < conf->num_policies; i++)
		print_policy_conf(&conf->power_policies[i]);

	verbosen(VCCONF, "\n>>>>> Authorization section <<<<<\n");
	verbosen(VCCONF, "\nUsers \n");
	for (i = 0; i < conf->num_priv_users; i++)
		verbosen(VCCONF, "--->user: %s\n", conf->priv_users[i]);
	verbosen(VCCONF, "\nGroups \n");
	for (i = 0; i < conf->num_priv_groups; i++)
		verbosen(VCCONF, "--->groups: %s\n", conf->priv_groups[i]);
	verbosen(VCCONF, "\nSccounts \n");
	for (i = 0; i < conf->num_acc; i++)
		verbosen(VCCONF, "--->acc: %s\n", conf->priv_acc[i]);


	verbosen(VCCONF, "\n>>>>> Specific node configurations section <<<<<\n");
	for (i = 0; i < conf->num_nodes; i++)
		print_node_conf(&conf->nodes[i]);

	verbosen(VCCONF, "\n>>>>> SQL DB server section <<<<<\n");
	print_database_conf(&conf->database);

	verbosen(VCCONF, "\n>>>>> EARDBD: DB  manager section <<<<<\n");
	print_db_manager(&conf->db_manager);

	verbosen(VCCONF, "\n>>>>> EARD: Node manager section <<<<<\n");
	print_eard_conf(&conf->eard);

	verbosen(VCCONF, "\n>>>>> EARGM: System power manager section <<<<<\n");
	print_eargm_conf(&conf->eargm);

	
    verbosen(VCCONF, "\n>>>>> TAGS <<<<<<\n");
    for (i = 0; i < conf->num_tags; i++)
		print_tags_conf(&conf->tags[i],i );

	verbosen(VCCONF, "\n>>>>> Computational nodes <<<<<\n");
	for (i = 0; i < conf->num_islands; i++)
		print_islands_conf(&conf->islands[i]);

  verbosen(VCCONF, "\n>>>>> Data Center Monitoring  section <<<<<<\n");
	for (i = 0; i < conf->num_edcmons_tags; i++){
		print_edcmon_tags_conf(&conf->edcmon_tags[i], i);
	}
	verbose(VCCONF," ");

	verbosen(VCCONF, "\n>>>>> Energy tags section <<<<<\n");
	for (i = 0; i < conf->num_etags; i++)
		print_energy_tag(&conf->e_tags[i]);

    verbosen(VCCONF, "\n>>>>> EAR library section <<<<<\n");
    print_earlib_conf(&conf->earlib);

	verbosen(VCCONF, "\n");
}
