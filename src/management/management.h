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

#ifndef MANAGEMENT_H
#define MANAGEMENT_H

#include <management/gpu/gpu.h>
#include <management/cpufreq/cpufreq.h>
#include <management/imcfreq/imcfreq.h>

typedef struct manages_s {
    ctx_t c;
    uint  api;
    uint  ok;
    uint  granularity;
    uint  devs_count;
    void *avail_list;
    uint  avail_count;
    void *current_list;
    void *set_list;
} manages_t;

typedef struct manages_apis_s {
	manages_t cpufreq;
	manages_t imcfreq;
	manages_t gpu;
} manages_apis_t;

void manages_apis_init(topology_t *tp, uint eard, manages_apis_t **m_in);

void manages_apis_dispose();

void manages_apis_print();

void manages_apis_print_available();

#endif //MANAGEMENT_H
