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

#ifndef COMMON_UTILS_STRESS_H
#define COMMON_UTILS_STRESS_H

#include <common/system/time.h>

void stress_alloc();

void stress_free();
// Stress the system bandwidth by memcpy during a period in miliseconds.
void stress_bandwidth(ullong ms);
// Stress the system through while-spinning during a period in miliseconds.
void stress_spin(ullong ms);

#endif