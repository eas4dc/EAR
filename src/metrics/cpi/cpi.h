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

#ifndef EAR_CPI_H
#define EAR_CPI_H

#include <common/states.h>
#include <common/plugins.h>
#include <common/hardware/topology.h>

int init_basic_metrics();

void reset_basic_metrics();

void start_basic_metrics();

void stop_basic_metrics(llong *cycles, llong *instructions);

void read_basic_metrics(llong *cycles, llong *instructions);

void get_basic_metrics(llong *total_cycles, llong *instructions);

#endif //EAR_PRIVATE_CACHE_H