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

#include <daemon/shared_configuration.h>

void eard_exit(uint restart);
static void configure_new_values(cluster_conf_t *cluster,my_node_conf_t *node);
static void init_frequency_list();
static void set_global_eard_variables();
static int read_coefficients();
static void signal_catcher();
static void Usage(char *app);
static void eard_restart();
static void  create_tmp(char *tmp_dir);

#else
#endif
