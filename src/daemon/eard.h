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

#ifndef _EARD_H_
#define _EARD_H_

static void eard_exit(uint restart);
static void eard_close_comm();
static int is_new_application();
static int is_new_service(int req,int pid);
static int application_timeout();
static void configure_new_values(settings_conf_t *dyn,resched_t *resched,cluster_conf_t *cluster,my_node_conf_t *node);
static void init_frequency_list();
static int is_valid_sec_tag(ulong tag);
static void set_global_eard_variables();
static int read_coefficients();
static void signal_catcher();
static void Usage(char *app);
static int eard_rapl(int must_read);
static int eard_uncore(int must_read);
static int eard_freq(int must_read);
static int eard_system(int must_read);
static void eard_restart();
static int check_ping();
static void  create_tmp(char *tmp_dir);
static void create_connector(char *ear_tmp,char *nodename,int i);
static void connect_service(int req,application_t *new_app);
static void eard_set_freq(unsigned long new_freq,unsigned long max_freq);

#else
#endif
