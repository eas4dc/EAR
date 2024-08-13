/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef DYNAIS_AVX512_H
#define DYNAIS_AVX512_H

#include <library/dynais/dynais.h>

dynais_call_t avx512_dynais_init(ushort window, ushort levels);

int avx512_dynais(uint sample, uint *size, uint *level);

void avx512_dynais_dispose();

#endif //DYNAIS_AVX512_H