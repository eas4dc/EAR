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

#ifndef EAR_CONFIG_DEV_H
#define EAR_CONFIG_DEV_H

/** PONERLO A 1 EN TOS LAOS Y QUIAR COMENT */
/* When set to 1, master processes synchronize at application start to identify
 * the nombre of masters connected and expected. EARL is set to off in case
 * expected!=connected. */
#define EAR_LIB_SYNC						1
/** EARD threads selection, do not modify except for debug purposes **/
/* When set to 1 , creates a thread in EARD to support application queries apart
 * from EARL, do not set to 0 except for debug purposes */
#define APP_API_THREAD						1
/* When set to 1 , creates a thread in EARD for powermonitoring, do not set to 0
 * except for debug purposes */
#define POWERMON_THREAD						1
/* When set to 1 , creates a thread in EARD for external commands, do not set to
 * 0 except for debug purposes */
#define EXTERNAL_COMMANDS_THREAD 				1
/* Just for ear.conf reading. */
#define EARDBD_TYPES						7
/** Specifies if the new version of the commands propagation is used and the number
 * of jumps per node. */
#define NUM_PROPS                           3
// #define EAR_TRACER_MPI 1

// Maximum number of tries when doing non-blocking communications
#define MAX_SOCKET_COMM_TRIES 	10000000

#define EAR_CPUPOWER	1
#define EARL_RESEARCH 1
#define ONLY_MASTER 1
//#define USE_GPUS_LIB 1
//
#define SHARE_INFO_PER_PROCESS 1
#define SHARE_INFO_PER_NODE 0

//#define POWERCAP 0
//#define POWERCAP_EXT 0
//#define DISTRIBUTE_SH_SIG 1
#define DYN_PAR 1
#endif //EAR_CONFIG_DEV_H
