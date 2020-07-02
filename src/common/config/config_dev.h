/**************************************************************
*   Energy Aware Runtime (EAR)
*   This program is part of the Energy Aware Runtime (EAR).
*
*   EAR provides a dynamic, transparent and ligth-weigth solution for
*   Energy management.
*
*       It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*   BSC Contact     mailto:ear-support@bsc.es
*   Lenovo contact  mailto:hpchelp@lenovo.com
*
*   EAR is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Lesser General Public
*   License as published by the Free Software Foundation; either
*   version 2.1 of the License, or (at your option) any later version.
*
*   EAR is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Lesser General Public License for more details.
*
*   You should have received a copy of the GNU Lesser General Public
*   License along with EAR; if not, write to the Free Software
*   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*   The GNU LEsser General Public License is contained in the file COPYING
*/

#ifndef EAR_CONFIG_DEV_H
#define EAR_CONFIG_DEV_H

/** PONERLO A 1 EN TOS LAOS Y QUIAR COMENT */
/* When set to 1, master processes synchronize at application start to identify
 * the nombre of masters connected and expected. EARL is set to off in case
 * expected!=connected. */
#define EAR_LIB_SYNC						1
/* Specifies if RAPL msut be read with MSR registers (1) or with PAPI (0) */
#define USE_MSR_RAPL						1
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
#define USE_NEW_PROP                        1
#define NUM_PROPS                           3
// #define EAR_TRACER_MPI 1

// Maximum number of tries when doing non-blocking communications
#define MAX_SOCKET_COMM_TRIES 	10000000

#define EAR_CPUPOWER	1
#define EARL_RESEARCH 1
#define ONLY_MASTER 1
#define USE_GPUS 0

#endif //EAR_CONFIG_DEV_H
