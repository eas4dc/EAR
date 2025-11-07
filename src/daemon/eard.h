/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EARD_H_
#define _EARD_H_

#include <daemon/shared_configuration.h>

void eard_exit(uint restart);
static void configure_new_values(cluster_conf_t *cluster, my_node_conf_t *node);
static void init_frequency_list();
static void set_global_eard_variables();
static int read_coefficients();
static void signal_catcher();
static void Usage(char *app);
static void eard_restart();
static void create_tmp(char *tmp_dir);

#else
#endif
