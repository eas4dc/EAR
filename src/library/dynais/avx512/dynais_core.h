/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef DYNAIS_AVX512_CORE_H
#define DYNAIS_AVX512_CORE_H

#include <common/types/generic.h>

void avx512_dynais_core_n(ushort sample, ushort size, ushort level);
void avx512_dynais_core_0(ushort sample, ushort size, ushort level);

#endif // EAR_DYNAIS_CORE_H
