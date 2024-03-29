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

#ifndef COMMON_UTILS_OVERHEAD_H
#define COMMON_UTILS_OVERHEAD_H

#include <common/types.h>

#define ENABLE_OVERHEAD 0

// Suscribes a section of code. It receives a description of that code section,
// and returns a ID which will be used to circle the section by start-stop
// marks. Later, you can all report to print all the gathered data.
//
// Ex:
//  overhead_suscribe("get frequency", &id_freq);
//  overhead_start(id_freq);
//  ... (get frequency code)
//  overhead_stop(id_freq);
//  ... (things)
//  overhead_report(1);

void overhead_suscribe(const char *description, uint *id);

void overhead_start(uint id);

void overhead_stop(uint id);

void overhead_report(int print_header);

void overhead_print_header();

#endif //COMMON_UTILS_OVERHEAD_H
