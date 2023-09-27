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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef COMMON_CONFIG_SCHED_H
#define COMMON_CONFIG_SCHED_H

// Config install first because by now is defining the SCHED_* macros.
#include <common/config/config_install.h>
// Including this because NULL_STEPID
#include <common/config/config_env.h>

#if SCHED_SLURM
#include <slurm/slurm.h>
#endif

#if SCHED_SLURM
#define BATCH_STEP              ((int) SLURM_BATCH_SCRIPT)
#define INTERACT_STEP           ((int) SLURM_INTERACTIVE_STEP)
#endif

#if SCHED_PBS
#define BATCH_STEP              0
#endif

#if SCHED_OAR
#define BATCH_STEP              0
#endif

#if !SCHED_SLURM && !SCHED_PBS && !SCHED_OAR
#define BATCH_STEP              NULL_STEPID
#endif

#endif //COMMON_CONFIG_SCHED_H
