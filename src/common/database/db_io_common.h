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

#ifndef DB_IO_COMMON_H
#define DB_IO_COMMON_H
#include <common/config.h>

//
#define EAR_TYPE_APPLICATION    1
#define EAR_TYPE_LOOP           2

//number of arguments inserted into periodic_metrics
#define PERIODIC_AGGREGATION_ARGS   4
#define EAR_EVENTS_ARGS             6
#define POWER_SIGNATURE_ARGS        9
#define APPLICATION_ARGS            5
#define LOOP_ARGS                   8
#define JOB_ARGS                    16

#define PERIODIC_METRIC_ARGS        10

#if !DB_SIMPLE

#if USE_GPUS //DB_FULL and GPUs
#define SIGNATURE_ARGS              26
#else //DB_FULL and no GPUs
#define SIGNATURE_ARGS              24
#endif
#define AVG_SIGNATURE_ARGS          25

#else //if DB_SIMPLE

#if USE_GPUS //DB_SIMPLE and GPUs
#define SIGNATURE_ARGS              16
#else //DB_SIMPLE and no GPUs
#define SIGNATURE_ARGS              14
#endif
#define AVG_SIGNATURE_ARGS          15
#endif

#if USE_GPUS
#define GPU_SIGNATURE_ARGS 5
#endif

#endif
