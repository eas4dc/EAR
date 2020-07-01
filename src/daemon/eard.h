/**************************************************************
 * *   Energy Aware Runtime (EAR)
 * *   This program is part of the Energy Aware Runtime (EAR).
 * *
 * *   EAR provides a dynamic, transparent and ligth-weigth solution for
 * *   Energy management.
 * *
 * *       It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
 * *
 * *       Copyright (C) 2017  
 * *   BSC Contact     mailto:ear-support@bsc.es
 * *   Lenovo contact  mailto:hpchelp@lenovo.com
 * *
 * *   EAR is free software; you can redistribute it and/or
 * *   modify it under the terms of the GNU Lesser General Public
 * *   License as published by the Free Software Foundation; either
 * *   version 2.1 of the License, or (at your option) any later version.
 * *   
 * *   EAR is distributed in the hope that it will be useful,
 * *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 * *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * *   Lesser General Public License for more details.
 * *   
 * *   You should have received a copy of the GNU Lesser General Public
 * *   License along with EAR; if not, write to the Free Software
 * *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * *   The GNU LEsser General Public License is contained in the file COPYING  
 * */

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
