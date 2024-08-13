/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_PRIVATE_TYPES_H
#define EAR_PRIVATE_TYPES_H

#include <common/sizes.h>
#include <metrics/common/omsr.h>

//#include <metrics/energy/energy_cpu.h>

typedef void *edata_t;

typedef struct ehandler {
	char manufacturer[SZ_NAME_MEDIUM];
	char product[SZ_NAME_MEDIUM];
	void *context;
	int fds_rapl[MAX_PACKAGES];
} ehandler_t;

typedef long long rapl_data_t;
typedef edata_t node_data_t;

#endif //EAR_PRIVATE_TYPES_H

