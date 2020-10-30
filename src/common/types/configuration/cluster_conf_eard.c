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
#include <common/string_enhanced.h>
#include <common/types/configuration/cluster_conf_eard.h>

state_t EARD_token(char *token)
{
	if (strcasestr(token,"EARD")!=NULL) return EAR_SUCCESS;
	if (strcasestr(token,"NodeDaemon")!=NULL) return EAR_SUCCESS;
	return EAR_ERROR;
}

state_t EARD_parse_token(eard_conf_t *conf,char *token)
{
		state_t found=EAR_ERROR;

		debug("EARD_parse_token %s",token);
    if (!strcmp(token, "NODEDAEMONVERBOSE"))
    {
      token = strtok(NULL, "=");
      conf->verbose = atoi(token);
			found = EAR_SUCCESS;
    }
    else if (!strcmp(token, "NODEDAEMONPOWERMONFREQ"))
    {
      token = strtok(NULL, "=");
      conf->period_powermon = atoi(token);
			found = EAR_SUCCESS;
    }
    else if (!strcmp(token, "NODEDAEMONMAXPSTATE"))
    {
      token = strtok(NULL, "=");
      conf->max_pstate = atoi(token);
			found = EAR_SUCCESS;
    }
    else if (!strcmp(token, "NODEDAEMONTURBO"))
    {
      token = strtok(NULL, "=");
      conf->turbo = atoi(token);
			found = EAR_SUCCESS;
    }
    else if (!strcmp(token, "NODEDAEMONPORT"))
    {
      token = strtok(NULL, "=");
      conf->port = atoi(token);
			found = EAR_SUCCESS;
    }
    else if (!strcmp(token, "NODEUSEDB"))
    {
      token = strtok(NULL, "=");
      conf->use_mysql = atoi(token);
			found = EAR_SUCCESS;
    }
    else if (!strcmp(token, "NODEUSEEARDBD"))
    {
      token = strtok(NULL, "=");
      conf->use_eardbd = atoi(token);
			found = EAR_SUCCESS;
    }
    else if (!strcmp(token, "NODEDAEMONFORCEFREQUENCIES"))
    {
      token = strtok(NULL, "=");
      conf->force_frequencies = atoi(token);
			found = EAR_SUCCESS;
    }
    else if (!strcmp(token, "NODEUSELOG"))
    {
      token = strtok(NULL, "=");
      conf->use_log = atoi(token);
			found = EAR_SUCCESS;
    }
	

		return found;
}



void copy_eard_conf(eard_conf_t *dest,eard_conf_t *src)
{
  memcpy(dest,src,sizeof(eard_conf_t));

}

void set_default_eard_conf(eard_conf_t *eardc)
{
  eardc->verbose=1;           /* default 1 */
    eardc->period_powermon=POWERMON_FREQ;  /* default 30000000 (30secs) */
    eardc->max_pstate=1;       /* default 1 */
    eardc->turbo=USE_TURBO;             /* Fixed to 0 by the moment */
    eardc->port=DAEMON_PORT_NUMBER;              /* mandatory */
    eardc->use_mysql=1;         /* Must EARD report to DB */
    eardc->use_eardbd=1;        /* Must EARD report to DB using EARDBD */
  eardc->force_frequencies=1; /* EARD will force frequencies */
  eardc->use_log=EARD_FILE_LOG;


}
void print_eard_conf(eard_conf_t *conf)
{
  verbosen(VCCONF,"\t eard: verbosen %u period %lu max_pstate %lu \n",conf->verbose,conf->period_powermon,conf->max_pstate);
  verbosen(VCCONF,"\t eard: turbo %u port %u use_db %u use_eardbd %u \n",conf->turbo,conf->port,conf->use_mysql,conf->use_eardbd);
  verbosen(VCCONF,"\t eard: force_frequencies %u\n",conf->force_frequencies);
  verbosen(VCCONF,"\t eard: use_log %u\n",conf->use_log);

}

