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

#ifndef CONFIG_SCHED_H
#define CONFIG_SCHED_H


/**
 * Different variables per scheduling manager
 */
#if SCHED_SLURM
#include <slurm/slurm.h>
#define BATCH_STEP SLURM_BATCH_SCRIPT
#else
#define BATCH_STEP                   -2
#endif

/** @}  */
#endif //CONFIG_SCHED_H
