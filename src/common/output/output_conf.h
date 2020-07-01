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


#ifndef _OUTCONF_H
#define _OUTCONF_H

#define SHOW_WARNINGS  1
#define SHOW_ERRORS     1
#define SHOW_LOGS       1

/* EARD VERBOSE constants */
/* Used in eard_rapi.h */
#define VCONNECT 0
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
