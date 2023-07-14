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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/config.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/string_enhanced.h>
#include <common/types/configuration/cluster_conf_eargm.h>

state_t EARGM_token(char *token)
{
    if (strcasestr(token,"EARGM")!=NULL) return EAR_SUCCESS;
    if (strcasestr(token,"GlobalManager")!=NULL) return EAR_SUCCESS;
    return EAR_ERROR;
}

void parse_EARGM_specific(eargm_conf_t *conf, char *line)
{
    int idx = -1, i = 0, current_id;
    int local_idx = 0;
    char *token, *main_ptr, *scd_ptr, tmp_found;

    //setting the initial pointer to null is necessary for realloc
    if (conf->num_eargms < 1)
        conf->eargms = NULL;

    token = strtok_r(line, "=", &main_ptr);
    while (token != NULL)
    {
        strtoup(token);
        debug("Checking token %s", token);
        if (!strcmp(token, "EARGMID"))
        {
            token = strtok_r(NULL, " ", &main_ptr);
            int aux = atoi(token);
            for (i = 0; i < conf->num_eargms; i++)
                if (conf->eargms[i].id == aux) idx = i;

            if (idx < 0)
            {
                local_idx = conf->num_eargms;
                conf->eargms = realloc(conf->eargms, sizeof(eargm_def_t)*(conf->num_eargms+1));
                memset(&conf->eargms[local_idx], 0, sizeof(eargm_def_t)); //this could be replaced by a default init function
                conf->eargms[local_idx].id = aux;

            }
            else local_idx = idx;
        }
        else if (!strcmp(token, "ENERGY"))
        {
            token = strtok_r(NULL, " ", &main_ptr);
            conf->eargms[local_idx].energy = atol(token);
        }
        else if (!strcmp(token, "POWER"))
        {
            token = strtok_r(NULL, " ", &main_ptr);
            conf->eargms[local_idx].power = atol(token);
        }
        else if (!strcmp(token, "NODE"))
        {
            token = strtok_r(NULL, " ", &main_ptr);
            strclean(token, '\n');
            remove_chars(token, ' ');
            strcpy(conf->eargms[local_idx].node, token);
        }
        else if (!strcmp(token, "PORT"))
        {
            token = strtok_r(NULL, " ", &main_ptr);
            conf->eargms[local_idx].port = atoi(token);
        }
        else if (!strcmp(token, "META"))
        {
            token = strtok_r(NULL, " ", &main_ptr);
            token = strtok_r(token, ",", &scd_ptr);
            while (token != NULL)
            {
                current_id = atoi(token);
                tmp_found = 0;
                debug("checking if node %d is already in subs", current_id);
                for (i = 0; i < conf->eargms[local_idx].num_subs; i++) //search to avoid duplicates
                {
                    if (conf->eargms[local_idx].subs[i] == current_id) {
                        tmp_found = 1;
                        break;
                    }
                }
                if (tmp_found) continue; 
                debug("node %d is NOT already in subs", current_id);
                conf->eargms[local_idx].subs = realloc(conf->eargms[local_idx].subs, 
                        sizeof(int)*(conf->eargms[local_idx].num_subs + 1));
                conf->eargms[local_idx].subs[conf->eargms[local_idx].num_subs] = current_id;
                conf->eargms[local_idx].num_subs++;
                token = strtok_r(NULL, ",", &scd_ptr);
            }
        }
        token = strtok_r(NULL, "=", &main_ptr);
    }

    if (idx < 0)
        conf->num_eargms++;

}

state_t EARGM_parse_token(eargm_conf_t *conf, char *token)
{
    state_t found=EAR_ERROR;

    debug("EARGM_parse_token %s",token);
    //GLOBAL MANAGER
    if (!strcmp(token, "GLOBALMANAGERVERBOSE") || !strcmp(token, "EARGMVERBOSE"))
    {
        token = strtok(NULL, "=");
        conf->verbose = atoi(token);
        found=EAR_SUCCESS;
    }
    else if (!strcmp(token, "GLOBALMANAGERENERGYUSEAGGREGATED") || !strcmp(token, "EARGMENERGYUSEAGGREGATED"))
    {
        token = strtok(NULL, "=");
        conf->use_aggregation = atoi(token);
        found=EAR_SUCCESS;
    }
    else if (!strcmp(token, "GLOBALMANAGERENERGYPERIODT1") || !strcmp(token, "EARGMENERGYPERIODT1"))
    {
        token = strtok(NULL, "=");
        conf->t1 = atoi(token);
        found=EAR_SUCCESS;
    }
    else if (!strcmp(token, "GLOBALMANAGERENERGYPERIODT2") || !strcmp(token, "EARGMENERGYPERIODT2") )
    {
        token = strtok(NULL, "=");
        conf->t2 = atoi(token);
        found=EAR_SUCCESS;
    }
    else if (!strcmp(token, "GLOBALMANAGERENERGYUNITS") || !strcmp(token, "EARGMENERGYUNITS"))
    {
        token = strtok(NULL, "=");

        //
        strclean(token, '\n');
        remove_chars(token, ' ');
        strtoup(token);

        if (!strcmp(token,"-"))	conf->units=BASIC;
        else if (!strcmp(token,"K")) conf->units=KILO;
        else if (!strcmp(token,"M")) conf->units=MEGA;
        else conf->units=KILO;
        found=EAR_SUCCESS;
    }
    else if (!strcmp(token, "GLOBALMANAGERPOLICY") || !strcmp(token, "EARGMPOLICY"))
    {
        token = strtok(NULL, "=");

        // 
        strclean(token, '\n');
        remove_chars(token, ' ');
        strtoup(token);

        if (strcmp(token,"MAXENERGY") == 0) {
            conf->policy=MAXENERGY;
        } else {
            conf->policy=MAXPOWER;
        }
        found=EAR_SUCCESS;
    }
    else if (!strcmp(token, "GLOBALMANAGERENERGYLIMIT") || !strcmp(token, "EARGMENERGYLIMIT"))
    {
        token = strtok(NULL, "=");
        conf->energy = atol(token);
        found=EAR_SUCCESS;
    }
    else if (!strcmp(token, "GLOBALMANAGERPOWERLIMIT") || !strcmp(token, "EARGMPOWERLIMIT"))
    {
        token = strtok(NULL, "=");
        /* It mas be included in power */
        conf->power = atol(token);
        found=EAR_SUCCESS;
    }
    else if (!strcmp(token, "GLOBALMANAGERPOWERPERIOD") || !strcmp(token, "EARGMPOWERPERIOD"))
    {
        token = strtok(NULL, "=");
        conf->t1_power = atoi(token);
        found=EAR_SUCCESS;
    }
    else if (!strcmp(token, "GLOBALMANAGERPOWERCAPMODE") || !strcmp(token, "EARGMPOWERCAPMODE"))
    {
        token = strtok(NULL, "=");
        conf->powercap_mode = atoi(token);
        found=EAR_SUCCESS;
    }else if (!strcmp(token, "GLOBALMANAGERPOWERCAPSUSPENDLIMIT") || !strcmp(token, "EARGMPOWERCAPSUSPENDLIMIT"))
    {
        token = strtok(NULL, "=");
        conf->defcon_power_limit = atoi(token);
        found=EAR_SUCCESS;
    }else if (!strcmp(token, "GLOBALMANAGERPOWERCAPSUSPENDACTION") || !strcmp(token,"EARGMPOWERCAPSUSPENDACTION"))
    {
        token = strtok(NULL, "=");
        strclean(token, '\n');
        strcpy(conf->powercap_limit_action,token);
        found=EAR_SUCCESS;
    }else if (!strcmp(token, "GLOBALMANAGERPOWERCAPRESUMEACTION") || !strcmp(token, "EARGMPOWERCAPRESUMEACTION"))
    {
        token = strtok(NULL, "=");
        strclean(token, '\n');
        strcpy(conf->powercap_lower_action,token);
        found=EAR_SUCCESS;
    }else if (!strcmp(token, "GLOBALMANAGERPOWERCPRESUMELIMIT") || !strcmp(token,"EARGMPOWERCAPRESUMELIMIT"))
    {
        token = strtok(NULL, "=");
        conf->defcon_power_lower = atoi(token);
        found=EAR_SUCCESS;
    }else if (!strcmp(token, "GLOBALMANAGERENERGYACTION") || !strcmp(token, "EARGMENERGYACTION"))
    {
        token = strtok(NULL, "=");
        strclean(token, '\n');
        strcpy(conf->energycap_action,token);
        found=EAR_SUCCESS;
    }
    else if (!strcmp(token, "GLOBALMANAGERWARNINGSPERC") || !strcmp(token, "EARGMWARNINGSPERC"))
    {
        token = strtok(NULL, "=");
        token = strtok(token, ",");
        int perc=0;

        while (token != NULL)
        {
            conf->defcon_limits[perc++] = atoi(token);
            token = strtok(NULL, ",");
        }
        found=EAR_SUCCESS;
    }
    else if (!strcmp(token, "GLOBALMANAGERENERGYGRACEPERIODS") || !strcmp(token, "EARGMENERGYGRACEPERIODS"))
    {
        token = strtok(NULL, "=");
        conf->grace_periods = atoi(token);
        found=EAR_SUCCESS;
    }
    else if (!strcmp(token, "GLOBALMANAGERPORT") || !strcmp(token, "EARGMPORT"))
    {
        token = strtok(NULL, "=");
        conf->port = atoi(token);
        found=EAR_SUCCESS;
    }
    else if (!strcmp(token, "GLOBALMANAGERENERGYMODE") || !strcmp(token, "EARGMENERGYMODE"))
    {
        token = strtok(NULL, "=");
        conf->mode = atoi(token);
        found=EAR_SUCCESS;
    }
    else if (!strcmp(token, "GLOBALMANAGERMAIL") || !strcmp(token, "EARGMMAIL"))
    {
        token = strtok(NULL, "=");
        strclean(token, '\n');
        remove_chars(token, ' ');
        strcpy(conf->mail, token);
        found=EAR_SUCCESS;
    }
    else if (!strcmp(token, "GLOBALMANAGERHOST") || !strcmp(token, "EARGMHOST"))
    {
        token = strtok(NULL, "=");
        strclean(token, '\n');
        remove_chars(token, ' ');
        strcpy(conf->host, token);
        found=EAR_SUCCESS;
    }
    else if (!strcmp(token, "GLOBALMANAGERUSELOG") || !strcmp(token, "EARGMUSELOG"))
    {
        token = strtok(NULL, "=");
        conf->use_log = atoi(token);
        found=EAR_SUCCESS;
    }
    else if (!strcmp(token, "GLOBALMANAGERID") || !strcmp(token, "EARGMID"))
    {
        token[strlen(token)] = '=';
        parse_EARGM_specific(conf, token);
        found = EAR_SUCCESS;
    }
    else if (!strcmp(token, "GLOBALMANAGERREPORTPLUGINS") || !strcmp(token, "EARGMREPORTPLUGINS"))
    {
        token = strtok(NULL, "=");
        strclean(token, '\n');
        if (strlen(token) < sizeof(conf->plugins) - 1) {
            strncpy(conf->plugins, token, sizeof(conf->plugins) - 1);
            found = EAR_SUCCESS;
        } else {
            found = EAR_WARNING;
        }
    }
    return found;
}

void check_cluster_conf_eargm(eargm_conf_t *conf)
{
    if (conf->num_eargms == 0) {
        verbose(2,"Not having an EARGMId definition is deprecated. Using global values to create an EARGM def");
        conf->eargms = realloc(conf->eargms, sizeof(eargm_def_t)*(conf->num_eargms+1));
        memset(&conf->eargms[0], 0, sizeof(eargm_def_t)); //this could be replaced by a default init function
        conf->eargms[0].id = 0;
        conf->eargms[0].port = conf->port;
        conf->eargms[0].energy = conf->energy;
        conf->eargms[0].power = conf->power;
        strcpy(conf->eargms[0].node, conf->host);
        conf->num_eargms++;
    }
}

void copy_eargmd_conf(eargm_conf_t *dest,eargm_conf_t *src)
{
    memcpy(dest,src,sizeof(eargm_conf_t));
}

void set_default_eargm_conf(eargm_conf_t *eargmc)
{
    eargmc->verbose=1;
    eargmc->use_aggregation=1;
    eargmc->t1=DEFAULT_T1;
    eargmc->t2=DEFAULT_T2;
    eargmc->energy=DEFAULT_T2*DEFAULT_POWER;
    eargmc->units=1;
    eargmc->policy=MAXENERGY;
    eargmc->port=EARGM_PORT_NUMBER;
    eargmc->mode=0;
    eargmc->defcon_limits[0]=85;
    eargmc->defcon_limits[1]=90;
    eargmc->defcon_limits[2]=95;
    eargmc->grace_periods=GRACE_T1;
    strcpy(eargmc->mail,"nomail");
    eargmc->use_log=EARGMD_FILE_LOG;
    /* Values for POWERCAP */
    eargmc->power=EARGM_DEF_POWERCAP_LIMIT;
    eargmc->t1_power=EARGM_DEF_T1_POWER;
    eargmc->powercap_mode=EARGM_POWERCAP_DEF_MODE;  /* 1=auto by default, 0=monitoring_only */
    sprintf(eargmc->powercap_limit_action,EARGM_POWERCAP_DEF_ACTION);
    sprintf(eargmc->powercap_lower_action,EARGM_POWERCAP_DEF_ACTION);
    sprintf(eargmc->energycap_action,EARGM_ENERGYCAP_DEF_ACTION);
    eargmc->defcon_power_limit=EARGM_POWERCAP_DEF_ACTION_LIMIT;
    eargmc->defcon_power_lower=EARGM_POWERCAP_DEF_ACTION_LOWER;
    strcpy(eargmc->plugins, "");

}

void get_powercap_type(uint power, char *buff)
{
    switch(power) {
        case POWER_CAP_DISABLED:
            strcpy(buff, "(disabled)");
            break;
        case POWER_CAP_AUTO_CONFIG:
            strcpy(buff, "(auto_configure)");
            break;
        case POWER_CAP_UNLIMITED:
            strcpy(buff, "(unlimited, this is not a valid value)");
            break;
        default:
            strcpy(buff, "");
            break;
    }
}

void print_eargm_def(eargm_def_t *conf, char tabulate)
{
    int i;
    char buff[256];
    get_powercap_type(conf->power, buff);
    if (tabulate) {
        verbosen(VCCONF,"\t\t->");
    }
    verbosen(VCCONF,"EARGM %d node %s (port %u) ", conf->id, conf->node, conf->port);
    verbosen(VCCONF,"energy limit %ld power limit %ld %s", conf->energy, conf->power, buff);
    if (conf->num_subs > 0) {
        verbosen(VCCONF, "meta-eargm: yes, controlled eargms: ");
    }
    for (i = 0; i<conf->num_subs; i++) {
        verbosen(VCCONF, "%d, ", conf->subs[i]);
    }
    verbosen(VCCONF, "\n");
}

void print_eargm_conf(eargm_conf_t *conf)
{
    int i;
    verbosen(VCCONF,"--> EARGM configuration\n");
    verbosen(VCCONF,"\t eargm: verbosen %u \tuse_aggregation %u \tt1 %lu \tt2 %lu \tenergy limit: %ld \tport: %u \tmode: %u\tmail: %s\thost: %s\n",
            conf->verbose,conf->use_aggregation,conf->t1,conf->t2,conf->energy,conf->port, conf->mode, conf->mail, conf->host);
    verbosen(VCCONF,"\t eargm: defcon levels [%u,%u,%u] grace period %u\n",conf->defcon_limits[0],conf->defcon_limits[1],conf->defcon_limits[2],
            conf->grace_periods);
    verbosen(VCCONF,"\t policy %u (0=MaxEnergy,other=error) units=%u (-,K,M)\n",conf->policy,conf->units);
    verbosen(VCCONF,"\t use_log %u report plugins %s\n",conf->use_log, conf->plugins);
    verbosen(VCCONF,"\t cluster_power_limit %ld powercap_check_period %lu\n",conf->power,conf->t1_power);
    verbosen(VCCONF,"\t powercap_mode %lu (0=monitoring, 1=auto [def])\n",conf->powercap_mode);
    verbosen(VCCONF,"\t power limit for action %lu and for lower %lu\n",conf->defcon_power_limit, conf->defcon_power_lower);
    verbosen(VCCONF,"\t powercap_limit_action %s and powercap_lower_action %s\n",conf->powercap_limit_action, conf->powercap_lower_action);
    verbosen(VCCONF,"\t energycap_action %s\n",conf->energycap_action);
    verbosen(VCCONF,"\t EARGM definitions\n");
    for (i = 0; i < conf->num_eargms; i++)
        print_eargm_def(&conf->eargms[i], 1);
}

