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
#include <common/types/types.h>
#include <common/system/time.h>
#include <common/system/process.h>
#include <common/system/sockets.h>
#include <common/string_enhanced.h>

#define EDB_NTYPES                    7
#define EDB_MAX_CONNECTIONS           FD_SETSIZE - 48
#define EDB_OFFLINE                   0 // To test EARDBD offline
// These are the type of the events passed by sockets.
#define EDB_TYPE_ENERGY_REP           1
#define EDB_TYPE_APP_MPI              2
#define EDB_TYPE_APP_SEQ              9
#define EDB_TYPE_APP_LEARN            10
#define EDB_TYPE_LOOP                 3
#define EDB_TYPE_EVENT                4
#define EDB_TYPE_ENERGY_AGGR          5
#define EDB_TYPE_SYNC_QUESTION        6
#define EDB_TYPE_SYNC_ANSWER          7
#define EDB_TYPE_STATUS               8
// Sync options (is used to accumulate different types in one value).
#define EDB_SYNC_TYPE_AGGRS           0x040 // Synchronize aggregations
#define EDB_SYNC_ALL                  0x100 // Synchronize all types
// The reason to insert data in DB. 
#define EDB_INSERT_BY_SYNC            0 // Insert by sync request
#define EDB_INSERT_BY_TIME            1 // Insert by time completed
#define EDB_INSERT_BY_FULL            2 // Insert by allocation arrays full
// These are the allocation percentages per type.
#define ALLOC_PERCENT_APPS_MPI        30 // 30
#define ALLOC_PERCENT_APPS_SEQUENTIAL 30 // 60
#define ALLOC_PERCENT_APPS_LEARNING   5  // 65
#define ALLOC_PERCENT_LOOPS           21 // 86
#define ALLOC_PERCENT_EVENTS          3  // 89
#define ALLOC_PERCENT_ENERGY_REPS     7  // 96
#define ALLOC_PERCENT_ENERGY_AGGRS    4  // 100
// Type indexes
#define index_appsm                   0
#define index_appsn                   1
#define index_appsl                   2
#define index_loops                   3
#define index_evens                   4
#define index_enrgy                   5
#define index_aggrs                   6

typedef struct sync_question_s {
	uint sync_option;
	uint veteran;
} sync_question_t;

typedef struct sync_answer_s {
	uint veteran;
	int answer;
} sync_answer_t;

typedef struct eardbd_status_s {
	state_t  insert_states[EDB_NTYPES];
	uint     samples_recv[EDB_NTYPES];
	uint     sockets_online;
} eardbd_status_t;

#endif //EAR_EARDBD_H
