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

#ifndef DYNAIS_AVX2_H
#define DYNAIS_AVX2_H

#include <library/dynais/dynais.h>

dynais_call_t avx2_dynais_init(uint window, uint levels);

int avx2_dynais(uint sample, uint *size, uint *level);

void avx2_dynais_dispose();

#endif //DYNAIS_AVX2_H