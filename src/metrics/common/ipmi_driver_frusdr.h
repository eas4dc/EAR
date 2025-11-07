/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_COMMON_IPMI_DRIVER_FRUSDR_H
#define METRICS_COMMON_IPMI_DRIVER_FRUSDR_H

#include <metrics/common/ipmi.h>

void ipmi_driver_sdrs_discover(ipmi_dev_t *devs, uint devs_count);

void ipmi_driver_frus_discover(ipmi_dev_t *devs, uint devs_count);

#endif // METRICS_COMMON_IPMI_DRIVER_FRUSDR_H