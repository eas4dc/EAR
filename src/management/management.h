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

#ifndef TEST_H
#define TEST_H

#include <metrics/metrics.h>
#include <management/cpufreq/cpufreq.h>
#include <management/cpufreq/priority.h>
#include <management/imcfreq/imcfreq.h>
#include <management/cpupow/cpupow.h>
#include <management/gpu/gpu.h>

typedef struct manages_apis_s {
    apinfo_t cpu; // cpufreq
    apinfo_t pri; // cpuprio
    apinfo_t imc; // imcfreq
    apinfo_t pow; // cpupow
    apinfo_t gpu; // gpu
} manages_apis_t;

void management_init(topology_t *tp, int eard, manages_apis_t **m);

void management_data_print();

#endif //TEST_H
