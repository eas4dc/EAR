/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_GPU_RSMI_H
#define METRICS_GPU_RSMI_H

#include <metrics/gpu/gpu.h>

GPU_DEFINES(rsmi);

state_t gpu_rsmi_pool(void *c);

#endif
