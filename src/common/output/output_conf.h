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

#ifndef _OUTCONF_H
#define _OUTCONF_H

#define SHOW_WARNINGS  1
#define SHOW_ERRORS     1
#define SHOW_LOGS       1

/* EARD VERBOSE constants */
/* Used in eard_rapi.h */
#define VCONNECT 2
#define VMSG    3
/* Used in eard.c */
#define VCONF   1
#define VEARD   2
/* Used in power monitoring */
#define VNODEPMON   1
#define VJOBPMON    1
/* Used in dynamic_configuration */
#define VRAPI 2
/* Used in eard_api.c */
#define VAPI    2
/* Used in checkpoint */
#define VCHCK 2


/* USed in GM */
#define VGM 0


/* Used to print the cluster_conf */
#define VCCONF 0
#define VPRIV 0

/* Used in base mysql library calls. */
#define VMYSQL 0
/* Used in db_helper.c */
#define VDBH 0


#define VMETRICS 3
#define VTYPE   2
#define VJOBINFO 1

#define DYN_VERBOSE 2
#define LRZ_VERBOSE_LEVEL 0

#endif
