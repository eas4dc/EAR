/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EARGM_CONF_H
#define _EARGM_CONF_H

#include <common/config.h>
#include <common/states.h>
#include <common/types/generic.h>

#define MAXENERGY    0
#define MAXPOWER     1
#define BASIC        0
#define KILO         1
#define MEGA         2

#define DECON_LIMITS 3

typedef struct eargm_def {
    int id;
    uint port;
    long energy;
    long power;
    char node[GENERIC_NAME];
    int num_subs;
    int *subs; // eargms under that meta_eargm
} eargm_def_t;

typedef struct eargm_conf {
    uint verbose;         /* default 1 */
    uint use_aggregation; /* Use aggregated metrics.Default 1 */
    ulong t1;             /* default 60 seconds */
    ulong t2;             /* default 600 seconds */
    long energy;          /* mandatory */
    /* PowerCap */
    long power;
    ulong t1_power;
    ulong powercap_mode;                      /* 1=auto by default, 0=monitoring_only */
    ulong defcon_power_limit;                 /* Percentages from the maximum to execute the action (0..100)*/
    ulong defcon_power_lower;                 /* Percentages from the maximum to execute the restore (0..100)*/
    char powercap_limit_action[GENERIC_NAME]; /* Script file for powercap actions */
    char powercap_lower_action[GENERIC_NAME]; /* Script file for powercap actions */
    /****/
    uint units;  /* 0=J, 1=KJ=default, 2=MJ, or Watts when using Power */
    uint policy; /* 0=MaxEnergy (default), 1=MaxPower ( not yet implemented) */
    uint port;   /* mandatory */
    uint mode;   /* refers to energy budget mode */
    uint defcon_limits[3];
    uint grace_periods;
    char mail[GENERIC_NAME];
    char host[GENERIC_NAME];
    char energycap_action[GENERIC_NAME]; /* This action is execute in any WARNING level */
    char plugins[SZ_PATH_INCOMPLETE];
    uint use_log;

    /* Specific EARGM definitions */
    int num_eargms;
    eargm_def_t *eargms;
} eargm_conf_t;

state_t EARGM_token(char *token);
state_t EARGM_parse_token(eargm_conf_t *conf, char *token);
void check_cluster_conf_eargm(eargm_conf_t *conf);
void copy_eargmd_conf(eargm_conf_t *dest, eargm_conf_t *src);
void set_default_eargm_conf(eargm_conf_t *eargmc);
void print_eargm_def(eargm_def_t *conf, char tabulate);
void print_eargm_conf(eargm_conf_t *conf);

#endif
