/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_COMMON_HSMP_ESMI_H
#define METRICS_COMMON_HSMP_ESMI_H

#include <metrics/common/hsmp.h>

// This class is part of hsmp.h and is not intended to be used alone. This class
// is not lock protected. It is recommended to use the functions found in hsmp.h.
state_t hsmp_esmi_open(topology_t *tp, mode_t mode);

state_t hsmp_esmi_close();

state_t hsmp_esmi_send(int socket, uint function, uint *args, uint *reps);

#endif
