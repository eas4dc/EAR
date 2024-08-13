/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
#include <common/system/poll.h>
#include <common/system/process.h>
#include <common/system/sockets.h>
#include <common/string_enhanced.h>

#define EDB_NTYPES                    7
#define EDB_MAX_CONNECTIONS           2048
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
