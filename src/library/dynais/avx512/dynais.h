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

#ifndef DYNAIS_AVX512_H
#define DYNAIS_AVX512_H

#include <library/dynais/dynais.h>

dynais_call_t avx512_dynais_init(ushort window, ushort levels);

int avx512_dynais(uint sample, uint *size, uint *level);

void avx512_dynais_dispose();

#endif //DYNAIS_AVX512_H