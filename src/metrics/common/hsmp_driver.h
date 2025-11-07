/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_COMMON_HSMP_DRIVER_H
#define METRICS_COMMON_HSMP_DRIVER_H

#include <metrics/common/hsmp.h>

state_t hsmp_driver_open(topology_t *tp, mode_t mode);

state_t hsmp_driver_close();

state_t hsmp_driver_send(int socket, uint function, uint *args, uint *reps);

#endif
