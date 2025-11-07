/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_HARDWARE_MRS_H
#define COMMON_HARDWARE_MRS_H

#include <common/types/generic.h>

ullong mrs_midr();

ullong mrs_cntfrq();

ullong mrs_dfr0();

ullong mrs_pmccntr();

ullong mrs_pmccfiltr();

#endif // COMMON_HARDWARE_MRS_H