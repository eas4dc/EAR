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

#ifndef COMMON_HARDWARE_MRS_H
#define COMMON_HARDWARE_MRS_H

#include <common/types/generic.h>

ullong mrs_midr();

ullong mrs_cntfrq();

ullong mrs_dfr0();

ullong mrs_pmccntr();

ullong mrs_pmccfiltr();

#endif //COMMON_HARDWARE_MRS_H