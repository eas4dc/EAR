/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef UP_METRICS_H
#define UP_METRICS_H

#include <common/system/plugin_manager.h>
#include <metrics/metrics.h>

typedef struct mets_s {
    metrics_info_t mi;
    metrics_read_t mr;
} mets_t;

#endif // UP_METRICS_H