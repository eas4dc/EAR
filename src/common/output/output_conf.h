/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_OUTCONF_H
#define EAR_OUTCONF_H

#define SHOW_WARNINGS        1
#define SHOW_ERRORS          1

#define VCONNECT_DEF         2 // eard_rapi.c
#define VCONF_DEF            0 // eard.c
#define VEARD_DEF            0 // eard.c
#define VEARD_PC_DEF         1
#define VEARD_INIT_DEF       0
#define VEARD_NMGR_DEF       1
#define VEARD_NMGR_DEBUG_DEF 1
#define VEARD_LAPI_DEF       2
#define VNODEPMON_DEF        1 // Node power monitoring
#define VNODEPMON_BASIC_DEF  0 // Minimum Node power monitoring
#define VJOBPMON_DEF         1 // Job accounting in power monitoring
#define VJOBPMON_BASIC_DEF   0 // Basic job accounting in power monitoring. Only new & end msg
#define VPMON_DEBUG_DEF      3 // Power monitoring debug level. Set to 0 when debugging, 4-5 for disabling.
#define VTASKMON_DEF         2 // Task monitoring
#define VRAPI_DEF            2 // dynamic_configuration
#define VAPI_DEF             2 // eard_api.c
#define VCHCK_DEF            2 // checkpoint
#define VGM_DEF              0 // GM
#define VGM_PC_DEF           0 // GM POWERCAP
#define VCCONF_DEF           0 // cluster_conf
#define VPRIV_DEF            0 // cluster_conf
#define VMYSQL_DEF           0 // mysql
#define VDBH_DEF             0 // db_helper.c
#define VTYPE_DEF            2
#define VCOMM_DEF            4 // src/common/messaging

int VCONNECT __attribute__((weak))         = VCONNECT_DEF;
int VCONF __attribute__((weak))            = VCONF_DEF;
int VEARD __attribute__((weak))            = VEARD_DEF;
int VEARD_PC __attribute__((weak))         = VEARD_PC_DEF;
int VEARD_INIT __attribute__((weak))       = VEARD_INIT_DEF;
int VEARD_NMGR __attribute__((weak))       = VEARD_NMGR_DEF;
int VEARD_NMGR_DEBUG __attribute__((weak)) = VEARD_NMGR_DEBUG_DEF;
int VEARD_LAPI __attribute__((weak))       = VEARD_LAPI_DEF;
int VNODEPMON __attribute__((weak))        = VNODEPMON_DEF;
int VNODEPMON_BASIC __attribute__((weak))  = VNODEPMON_BASIC_DEF;
int VJOBPMON __attribute__((weak))         = VJOBPMON_DEF;
int VJOBPMON_BASIC __attribute__((weak))   = VJOBPMON_BASIC_DEF;
int VPMON_DEBUG __attribute__((weak))      = VPMON_DEBUG_DEF;
int VTASKMON __attribute__((weak))         = VTASKMON_DEF;
int VRAPI __attribute__((weak))            = VRAPI_DEF;
int VAPI __attribute__((weak))             = VAPI_DEF;
int VCHCK __attribute__((weak))            = VCHCK_DEF;
int VGM __attribute__((weak))              = VGM_DEF;
int VGM_PC __attribute__((weak))           = VGM_PC_DEF;
int VCCONF __attribute__((weak))           = VCCONF_DEF;
int VPRIV __attribute__((weak))            = VPRIV_DEF;
int VMYSQL __attribute__((weak))           = VMYSQL_DEF;
int VDBH __attribute__((weak))             = VDBH_DEF;
int VTYPE __attribute__((weak))            = VTYPE_DEF;
int VCOMM __attribute__((weak))            = VCOMM_DEF;

#endif
