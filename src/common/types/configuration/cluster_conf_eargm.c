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

state_t EARGM_parse_token(eargm_conf_t *conf,char *token)
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
		else if (!strcmp(token, "GLOBALMANAGERUSEAGGREGATED") || !strcmp(token, "EARGMUSEAGGREGATED"))
		{
			token = strtok(NULL, "=");
			conf->use_aggregation = atoi(token);
			found=EAR_SUCCESS;
		}
		else if (!strcmp(token, "GLOBALMANAGERPERIODT1") || !strcmp(token, "EARGMPERIODT1"))
		{
			token = strtok(NULL, "=");
			conf->t1 = atoi(token);
			found=EAR_SUCCESS;
		}
		else if (!strcmp(token, "GLOBALMANAGERPERIODT2") || !strcmp(token, "EARGMPERIODT2") )
		{
			token = strtok(NULL, "=");
			conf->t2 = atoi(token);
			found=EAR_SUCCESS;
		}
		else if (!strcmp(token, "GLOBALMANAGERUNITS") || !strcmp(token, "EARGMUNITS"))
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
			conf->energy = atoi(token);
			found=EAR_SUCCESS;
		}
		#if POWERCAP
		else if (!strcmp(token, "GLOBALMANAGERPOWERLIMIT") || !strcmp(token, "EARGMPOWERLIMIT"))
		{
			token = strtok(NULL, "=");
			/* It mas be included in power */
			conf->power = atoi(token);
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
		#endif
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
		else if (!strcmp(token, "GLOBALMANAGERGRACEPERIODS") || !strcmp(token, "EARGMGRACEPERIODS"))
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
		else if (!strcmp(token, "GLOBALMANAGERMODE") || !strcmp(token, "EARGMMODE"))
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
		return found;
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
  eargmc->policy=MAXPOWER;
  eargmc->port=EARGM_PORT_NUMBER;
  eargmc->mode=0;
  eargmc->defcon_limits[0]=85;
  eargmc->defcon_limits[1]=90;
  eargmc->defcon_limits[2]=95;
  eargmc->grace_periods=GRACE_T1;
  strcpy(eargmc->mail,"nomail");
  eargmc->use_log=EARGMD_FILE_LOG;
  /* Values for POWERCAP */
  #if POWERCAP
  eargmc->power=EARGM_DEF_POWERCAP_LIMIT;
  eargmc->t1_power=EARGM_DEF_T1_POWER;
  eargmc->powercap_mode=EARGM_POWERCAP_DEF_MODE;  /* 1=auto by default, 0=monitoring_only */
  sprintf(eargmc->powercap_limit_action,EARGM_POWERCAP_DEF_ACTION);
  sprintf(eargmc->powercap_lower_action,EARGM_POWERCAP_DEF_ACTION);
  sprintf(eargmc->energycap_action,EARGM_ENERGYCAP_DEF_ACTION);
  eargmc->defcon_power_limit=EARGM_POWERCAP_DEF_ACTION_LIMIT;
  eargmc->defcon_power_lower=EARGM_POWERCAP_DEF_ACTION_LOWER;
  #endif

}
void print_eargm_conf(eargm_conf_t *conf)
{
  verbosen(VCCONF,"--> EARGM configuration\n");
  verbosen(VCCONF,"\t eargm: verbosen %u \tuse_aggregation %u \tt1 %lu \tt2 %lu \tenergy limit: %lu \tport: %u \tmode: %u\tmail: %s\thost: %s\n",
      conf->verbose,conf->use_aggregation,conf->t1,conf->t2,conf->energy,conf->port, conf->mode, conf->mail, conf->host);
  verbosen(VCCONF,"\t eargm: defcon levels [%u,%u,%u] grace period %u\n",conf->defcon_limits[0],conf->defcon_limits[1],conf->defcon_limits[2],
  conf->grace_periods);
  verbosen(VCCONF,"\t policy %u (0=MaxEnergy,other=error) units=%u (-,K,M)\n",conf->policy,conf->units);
  verbosen(VCCONF,"\t use_log %u\n",conf->use_log);
  #if POWERCAP
  verbosen(VCCONF,"\t cluster_power_limit %lu powercap_check_period %lu\n",conf->power,conf->t1_power);
  verbosen(VCCONF,"\t powercap_mode %lu (0=monitoring, 1=auto [def])\n",conf->powercap_mode);
  verbosen(VCCONF,"\t power limit for action %lu and for lower %lu\n",conf->defcon_power_limit, conf->defcon_power_lower);
  verbosen(VCCONF,"\t powercap_limit_action %s and powercap_lower_action %s\n",conf->powercap_limit_action, conf->powercap_lower_action);
  verbosen(VCCONF,"\t energycap_action %s\n",conf->energycap_action);
  #endif
}

