/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

#ifndef METRICS_GPUPROF_DCGMI_H
#define METRICS_GPUPROF_DCGMI_H

#include <metrics/gpu/gpuprof.h>

// DCGMI limits all GPUs to monitor the same events. Because under the hood we
// are using POPEN which opens an additional process. If we wanted to add
// different events in different GPUs, we would have to open multiple POPEN
// processes.

GPUPROF_DEFINES(dcgmi);

#endif // METRICS_GPUPROF_DCGMI_H
