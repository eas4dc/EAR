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

#ifndef EAR_EARDBD_H
#define EAR_EARDBD_H

#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <common/config.h>
#include <common/system/process.h>
#include <common/system/sockets.h>
#include <common/types/types.h>
#include <common/string_enhanced.h>

#define CONTENT_TYPE_PER	1
#define CONTENT_TYPE_APM	2
#define CONTENT_TYPE_APN	9
#define CONTENT_TYPE_APL	10
#define CONTENT_TYPE_LOO	3
#define CONTENT_TYPE_EVE	4
#define CONTENT_TYPE_AGG	5
#define CONTENT_TYPE_QST	6
#define CONTENT_TYPE_ANS	7
#define CONTENT_TYPE_PIN	8
#define SYNC_ENRGY 			0x001
#define SYNC_APPSM 			0x002
#define SYNC_APPSN			0x004
#define SYNC_APPSL 			0x008
#define SYNC_LOOPS			0x010
#define SYNC_EVNTS			0x020
#define SYNC_AGGRS			0x040
#define SYNC_ALL			0x100
#define RES_SYNC			0
#define RES_TIME			1
#define RES_OVER			2
#define RES_FAIL			4
#define i_appsm 			0
#define i_appsn				1
#define i_appsl				2
#define i_loops				3
#define i_evnts				4
#define i_enrgy 			5
#define i_aggrs				6
#define MAX_TYPES			7
#define MAX_CONNECTIONS		FD_SETSIZE - 48
#define OFFLINE				0

#define sync_option(option, type) \
	((option & type) > 0)

#define sync_option_m(option, type1, type2) \
	((sync_option(option, type1)) || (sync_option(option, type2)))

typedef struct sync_qst {
	uint sync_option;
	uint veteran;
} sync_qst_t;

typedef struct sync_ans {
	uint veteran;
	int answer;
} sync_ans_t;

#endif //EAR_EARDBD_H
