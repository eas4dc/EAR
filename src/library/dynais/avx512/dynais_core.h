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

#ifndef EAR_DYNAIS_CORE_H
#define EAR_DYNAIS_CORE_H

#include <common/types/generic.h>

void dynais_core_n(ushort sample, ushort size, ushort level);
void dynais_core_0(ushort sample, ushort size, ushort level);

#endif //EAR_DYNAIS_CORE_H