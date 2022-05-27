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

#ifndef EAR_OUTCONF_H
#define EAR_OUTCONF_H

#define SHOW_WARNINGS 1
#define SHOW_ERRORS   1

#define VCONNECT  2 //eard_rapi.c
#define VCONF     0 //eard.c
#define VEARD     0 //eard.c
#define VEARD_PC  1
#define VEARD_INIT 0
#define VEARD_NMGR 1
#define VEARD_LAPI 2
#define VNODEPMON 1 			// Node power monitoring
#define VNODEPMON_BASIC 0   // Minimum Node power monitoring 
#define VJOBPMON  1 			// Job accounting in power monitoring
#define VJOBPMON_BASIC 0 	// Basic job accounting in power monitoring. Only new & end msg
#define VRAPI     2 //dynamic_configuration
#define VAPI      2 //eard_api.c
#define VCHCK     2 //checkpoint
#define VGM       0 //GM
#define VGM_PC    0 //GM POWERCAP
#define VCCONF    0 //cluster_conf
#define VPRIV     0 //cluster_conf
#define VMYSQL    0 //mysql
#define VDBH      0 //db_helper.c
#define VTYPE     2

#endif
