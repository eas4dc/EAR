/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_GPUPROF_H
#define METRICS_GPUPROF_H

#include <common/types.h>
#include <common/states.h>
#include <common/system/time.h>
#include <metrics/common/apis.h>

// This is a class to set a list of GPU events and add it to a GPU to compute
// its metrics. You have a test example in gpuprof.c. Its important to take
// into account that not all internal APIs allow different events on different
// GPUs.

typedef struct gpuprof_s {
	timestamp_t  time;
    double       time_s;
    double       samples_count;
    double      *values; //Metrics in this device
    uint         values_count;
    char        *hash; //To control event changes
} gpuprof_t;

typedef struct gpuprof_evs_s {
    char name[64];
    char units[16];
    uint id;
} gpuprof_evs_t;

// API building scheme
#define GPUPROF_F_LOAD(name)             void name (gpuprof_ops_t *ops)
#define GPUPROF_F_INIT(name)             state_t name ()
#define GPUPROF_F_DISPOSE(name)          void name ()
#define GPUPROF_F_GET_INFO(name)         void name (apinfo_t *info)
#define GPUPROF_F_EVENTS_GET(name)       void name (const gpuprof_evs_t **evs, uint *evs_count)
#define GPUPROF_F_EVENTS_SET(name)       void name (int dev, char *evs)
#define GPUPROF_F_EVENTS_UNSET(name)     void name (int dev)
#define GPUPROF_F_READ(name)             state_t name (gpuprof_t *data)
#define GPUPROF_F_READ_RAW(name)         state_t name (gpuprof_t *data)
#define GPUPROF_F_DATA_DIFF(name)        void name (gpuprof_t *data2, gpuprof_t *data1, gpuprof_t *dataD)
#define GPUPROF_F_DATA_ALLOC(name)       void name (gpuprof_t **data)
#define GPUPROF_F_DATA_COPY(name)        void name (gpuprof_t *dataD, gpuprof_t *dataS)

#define GPUPROF_DEFINES(name) \
GPUPROF_F_LOAD             (gpuprof_ ##name ##_load); \
GPUPROF_F_INIT             (gpuprof_ ##name ##_init); \
GPUPROF_F_DISPOSE          (gpuprof_ ##name ##_dispose); \
GPUPROF_F_GET_INFO         (gpuprof_ ##name ##_get_info); \
GPUPROF_F_EVENTS_GET       (gpuprof_ ##name ##_events_get); \
GPUPROF_F_EVENTS_SET       (gpuprof_ ##name ##_events_set); \
GPUPROF_F_EVENTS_UNSET     (gpuprof_ ##name ##_events_unset); \
GPUPROF_F_READ             (gpuprof_ ##name ##_read); \
GPUPROF_F_READ_RAW         (gpuprof_ ##name ##_read_raw); \
GPUPROF_F_DATA_DIFF        (gpuprof_ ##name ##_data_diff); \
GPUPROF_F_DATA_ALLOC       (gpuprof_ ##name ##_data_alloc); \
GPUPROF_F_DATA_COPY        (gpuprof_ ##name ##_data_copy);

typedef struct gpuprof_ops_s {
    GPUPROF_F_INIT((*init));
    GPUPROF_F_DISPOSE((*dispose));
    GPUPROF_F_GET_INFO((*get_info));
    GPUPROF_F_EVENTS_GET((*events_get));
    GPUPROF_F_EVENTS_SET((*events_set));
    GPUPROF_F_EVENTS_UNSET((*events_unset));
    GPUPROF_F_READ((*read));
    GPUPROF_F_READ_RAW((*read_raw));
    GPUPROF_F_DATA_DIFF((*data_diff));
    GPUPROF_F_DATA_ALLOC((*data_alloc));
    GPUPROF_F_DATA_COPY((*data_copy));
} gpuprof_ops_t;

void gpuprof_load(int force_api);

state_t gpuprof_init();

void gpuprof_dispose();

void gpuprof_get_info(apinfo_t *info);

void gpuprof_events_get(const gpuprof_evs_t **evs, uint *evs_count);

// Events is a list of strings in id format. Device can be all_devs.
void gpuprof_events_set(int dev, char *evs);

void gpuprof_events_unset(int dev);

// You need a list of events to read something.
state_t gpuprof_read(gpuprof_t *data);

state_t gpuprof_read_raw(gpuprof_t *data);

state_t gpuprof_read_diff(gpuprof_t *data2, gpuprof_t *data1, gpuprof_t *data_diff);

state_t gpuprof_read_copy(gpuprof_t *data2, gpuprof_t *data1, gpuprof_t *data_diff);

// Data
void gpuprof_data_diff(gpuprof_t *data2, gpuprof_t *data1, gpuprof_t *dataD);

void gpuprof_data_alloc(gpuprof_t **data);

void gpuprof_data_copy(gpuprof_t *dataD, gpuprof_t *dataS);

/** This is a comparison function you can use for searching and sorting arrays of gpuprof_evs_t.
	* In fact, it compares two gpuprof events and says who is greater.
	* \return -1 The first argument is "less" than the second.
	* \return 0 Both arguments are equal.
	* \return 1 The first argument is "greater". */
int gpuprof_compare_events(const void *gpuprof_ev_ptr1, const void *gpuprof_ev_ptr2);

#endif
