/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/config.h>
#include <common/output/verbose.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/types/configuration/cluster_conf_db.h>
#include <common/types/configuration/cluster_conf_eard.h>
#include <common/types/configuration/cluster_conf_eardbd.h>
#include <common/types/configuration/cluster_conf_eargm.h>
#include <common/types/configuration/cluster_conf_generic.h>
#include <common/types/configuration/cluster_conf_tag.h>
#include <stdio.h>
#include <stdlib.h>

#include <common/utils/sched_support.h>

void print_islands_conf(node_island_t *conf)
{
    int i, j;
    verbosen(VCCONF, "Island[%u] \n", conf->id);
    verbosen(VCCONF, "--->id: %u (min_power %.0lf, max_power %.0lf,power_cap %.1lf power_cap_type=%s)\n", conf->id,
             conf->min_sig_power, conf->max_sig_power, conf->max_power_cap, conf->power_cap_type);
    verbosen(VCCONF, "--->       (power>%.0lf or temp>%lu are errors)\n", conf->max_error_power, conf->max_temp);
    if (conf->num_tags > 0) {
        verbosen(VCCONF, "--->       tags: ");
        for (i = 0; i < conf->num_tags; i++) {
            verbosen(VCCONF, "tag%d:%s  ", i, conf->tags[i]);
        }
        verbosen(VCCONF, "\n");
    }
    for (i = 0; i < conf->num_ranges; i++) {
        char buffer[2048] = {0};
        size_t final_size = 0;
        int32_t cut_nodes = concat_range("", &conf->ranges[i].r_def, 2048, &final_size, buffer);
        if (cut_nodes > 0) {
            buffer[final_size]     = '.';
            buffer[final_size - 1] = '.';
            buffer[final_size - 2] = '.';
        }

        if (conf->ranges[i].db_ip < 0) {
            verbosen(VCCONF, "---->nodes: %s\t eargm: %d\n", buffer, conf->ranges[i].eargm_id);

        } else if (conf->ranges[i].sec_ip < 0) {
            verbosen(VCCONF, "---->nodes: %s\tip: %s eargm: %d\n", buffer, conf->db_ips[conf->ranges[i].db_ip],
                     conf->ranges[i].eargm_id);

        } else {
            verbosen(VCCONF, "---->nodes: %s\tip: %s\tbackup: %s eargm: %d\n", buffer,
                     conf->db_ips[conf->ranges[i].db_ip], conf->backup_ips[conf->ranges[i].sec_ip],
                     conf->ranges[i].eargm_id);
        }
        if (conf->ranges[i].num_tags > 0) {
            verbosen(VCCONF, "\t----tags: ");
            for (j = 0; j < conf->ranges[i].num_tags; j++) {
                verbosen(VCCONF, "tag%d:%s  ", j, conf->specific_tags[conf->ranges[i].specific_tags[j]]);
            }
            verbosen(VCCONF, "\n");
        }
    }
}

void print_cluster_conf(cluster_conf_t *conf)
{
    verbose(VCCONF, ">>>>> Paths <<<<<");
    verbose(VCCONF, "DB file pathname: %s\n%16s: %s\n%16s: %s\n%16s: %s", conf->DB_pathname, "EAR_TMP",
            conf->install.dir_temp, "EAR_ETC", conf->install.dir_conf, "Plug-ins path", conf->install.dir_plug);
    verbose(VCCONF, ">>>>> Default plug-ins <<<<<");
    verbose(VCCONF, "      Energy: %s\nPower models: %s", conf->install.obj_ener, conf->install.obj_power_model);

    if (strlen(conf->net_ext) > 1) {
        verbosen(
            VCCONF, "\nVerbose: %u\n--->Default_policy: %s\n--->Min_time_perf_acc: %u\n--->Network_extension: %s\n",
            conf->verbose, conf->power_policies[conf->default_policy].name, conf->min_time_perf_acc, conf->net_ext);
    } else {
        verbosen(VCCONF, "\nVerbose: %u\n--->Default_policy: %s (id %d)\n--->Min_time_perf_acc: %u\n", conf->verbose,
                 conf->power_policies[conf->default_policy].name, conf->default_policy, conf->min_time_perf_acc);
    }

    int i;
    verbosen(VCCONF, "\n>>>>> Policies configuration section <<<<<\n");
    for (i = 0; i < conf->num_policies; i++)
        print_policy_conf(&conf->power_policies[i]);

    verbose(VCCONF, "\n>>>>> Authorization section <<<<<");

    verbose(VCCONF, "Users");
    for (i = 0; i < conf->auth_users_count; i++)
        verbose(VCCONF, "--->user: %s", conf->auth_users[i]);

    verbose(VCCONF, "Groups");
    for (i = 0; i < conf->auth_groups_count; i++)
        verbose(VCCONF, "--->groups: %s", conf->auth_groups[i]);

    verbose(VCCONF, "Accounts");
    for (i = 0; i < conf->auth_acc_count; i++)
        verbose(VCCONF, "--->acc: %s", conf->auth_accounts[i]);

    verbose(VCCONF, "Admin users");
    for (i = 0; i < conf->admin_users_count; i++)
        verbose(VCCONF, "--->admin: %s", conf->admin_users[i]);

    verbosen(VCCONF, ">>>>> Specific node configurations section <<<<<");
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
        print_tags_conf(&conf->tags[i], i);

    verbosen(VCCONF, "\n>>>>> Computational nodes <<<<<\n");
    for (i = 0; i < conf->num_islands; i++)
        print_islands_conf(&conf->islands[i]);

    verbosen(VCCONF, "\n>>>>> Data Center Monitoring  section <<<<<<\n");
    for (i = 0; i < conf->num_edcmons_tags; i++) {
        print_edcmon_tags_conf(&conf->edcmon_tags[i], i);
    }
    verbose(VCCONF, " ");

    verbosen(VCCONF, "\n>>>>> Energy tags section <<<<<\n");
    for (i = 0; i < conf->num_etags; i++)
        print_energy_tag(&conf->e_tags[i]);

    verbosen(VCCONF, "\n>>>>> EAR library section <<<<<\n");
    print_earlib_conf(&conf->earlib);

    verbosen(VCCONF, "\n");
}
