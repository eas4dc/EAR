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

/**
 * @file policy_conf.h
 * @brief This file includes types, constants,and headers to specify the number of supported policies and the policies id's
 *
 * When adding a new policy, we must provide a set of functions to identify policies by name (targeted to users) and by policy id, for internal use.
 * Moreover, basic functions to copy, print etc are provided
 */


#ifndef _POLICY_CONF_H
#define _POLICY_CONF_H

#include <common/config.h>
#include <common/types/generic.h>

#define TOTAL_POLICIES          3
#define MIN_ENERGY_TO_SOLUTION  0
#define MIN_TIME_TO_SOLUTION    1
#define MONITORING_ONLY         2
#define MAX_POLICY_SETTINGS     5
#define POLICY_NAME_SIZE			64


typedef struct policy_conf
{
    uint policy; 
    char name[POLICY_NAME_SIZE];
    char tag[POLICY_NAME_SIZE];
    double settings[MAX_POLICY_SETTINGS];
    //double th; 
    //uint num_settings;
   	float def_freq;
    uint p_state;
    uint is_available; //default at 0, not available
} policy_conf_t;


#include <common/types/configuration/cluster_conf.h>

/** copy dest=src */
void copy_policy_conf(policy_conf_t *dest,policy_conf_t *src);

/** prints in the stdout policy configuration */
void print_policy_conf(policy_conf_t *p);

/** Sets all pointers to NULL and all values to their default*/
void init_policy_conf(policy_conf_t *p);

state_t POLICY_token(unsigned int *num_policiesp, policy_conf_t **power_policiesl,char *line);

#endif
