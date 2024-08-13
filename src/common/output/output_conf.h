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

#define SHOW_WARNINGS 1
#define SHOW_ERRORS   1

#define VCONNECT         2 // eard_rapi.c
#define VCONF            0 // eard.c
#define VEARD            0 // eard.c
#define VEARD_PC         1
#define VEARD_INIT       0
#define VEARD_NMGR       1
#define VEARD_NMGR_DEBUG 1
#define VEARD_LAPI       2
#define VNODEPMON        1 // Node power monitoring
#define VNODEPMON_BASIC  0 // Minimum Node power monitoring 
#define VJOBPMON         1 // Job accounting in power monitoring
#define VJOBPMON_BASIC   0 // Basic job accounting in power monitoring. Only new & end msg
#define VPMON_DEBUG      3 // Power monitoring debug level. Set to 0 when debugging, 4-5 for disabling.
#define VTASKMON         2 // Task monitoring
#define VRAPI            2 // dynamic_configuration
#define VAPI             2 // eard_api.c
#define VCHCK            2 // checkpoint
#define VGM              0 // GM
#define VGM_PC           0 // GM POWERCAP
#define VCCONF_DEF       0 // cluster_conf
#define VPRIV            0 // cluster_conf
#define VMYSQL           0 // mysql
#define VDBH             0 // db_helper.c
#define VTYPE            2
#define VCOMM            4 // src/common/messaging

#endif
