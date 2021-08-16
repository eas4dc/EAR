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

#ifndef _EARL_VERBOSE_H
#define _EARL_VERBOSE_H

#include <common/output/verbose.h>
#include <common/output/error.h>
#include <common/output/debug.h>

#define SHOW_ERRORS_LIB 1
#define verbose_master(...) if (masters_info.my_master_rank >= 0) verbose(__VA_ARGS__)
#define verbosen_master(...) if (masters_info.my_master_rank >= 0) verbosen(__VA_ARGS__)
#if SHOW_ERRORS_LIB
#define error_lib(...) error(__VA_ARGS__)
#else
#define error_lib(...)
#endif

#endif
