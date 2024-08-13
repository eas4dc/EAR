/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef DB_IO_COMMON_H
#define DB_IO_COMMON_H
#include <common/config.h>

//
#define EAR_TYPE_APPLICATION    1
#define EAR_TYPE_LOOP           2

#define PERIODIC_AGGREGATION_ARGS   4
#define EAR_EVENTS_ARGS             6
#define POWER_SIGNATURE_ARGS        9
#define APPLICATION_ARGS            6
#define LOOP_ARGS                   9
#define JOB_ARGS                    17



#if USE_GPUS 

//Signatures
#define FULL_SIGNATURE_ARGS              29
#define SIMPLE_SIGNATURE_ARGS            16

//Periodic_metrics
#define FULL_PERIODIC_METRIC_ARGS        11
#define SIMPLE_PERIODIC_METRIC_ARGS       6

#else //no USE_GPU
      
//Signatures
#define FULL_SIGNATURE_ARGS              27
#define SIMPLE_SIGNATURE_ARGS            14

//Periodic_metrics
#define FULL_PERIODIC_METRIC_ARGS        10
#define SIMPLE_PERIODIC_METRIC_ARGS       6

#endif


#define FULL_AVG_SIGNATURE_ARGS          25
#define SIMPLE_AVG_SIGNATURE_ARGS        15

#if USE_GPUS
#define GPU_SIGNATURE_ARGS 5
#endif

#endif
