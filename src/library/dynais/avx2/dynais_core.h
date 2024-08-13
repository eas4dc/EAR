/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef DYNAIS_AVX2_CORE_H
#define DYNAIS_AVX2_CORE_H

#include <common/types/generic.h>

void avx2_dynais_core_n(uint sample, uint size, uint level);
void avx2_dynais_core_0(uint sample, uint size, uint level);

#endif //DYNAIS_AVX2_CORE_H
