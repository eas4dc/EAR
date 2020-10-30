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

#ifndef EAR_EAR_METRICS_H
#define EAR_EAR_METRICS_H

#include <common/types/application.h>

/** Returns the current time in usecs */
long long metrics_time();

/** Returns the difference between times in usecs */
long long metrics_usecs_diff(long long end, long long init);

/** Initializes local metrics as well as daemon's metrics */
int metrics_init();

/** Stops metrics collection and computes the accumulated data*/
void metrics_dispose(signature_t *metrics, ulong procs);

/** Restarts the current metrics and recomputes the signature */
void metrics_compute_signature_begin();

/** */
int metrics_compute_signature_finish(signature_t *metrics, uint iterations, ulong min_time_us, ulong procs);

/** Estimates whether the current time running the loops is enough to compute the signature */
int time_ready_signature(ulong min_time_us);

/** Copute the number of vector instructions since signature reports FP ops, metrics is valid signature*/
unsigned long long metrics_vec_inst(signature_t *metrics);
#endif //EAR_EAR_METRICS_H
