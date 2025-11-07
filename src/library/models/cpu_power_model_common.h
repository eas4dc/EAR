/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _CPU_POWER_MODE_COMMON_H
#define _CPU_POWER_MODE_COMMON_H

/** This type defines the stragey for assigning the power consumption of an application.
 * Two possible values available:
 * - node: The power is assinged based on node workload, i.e., the application get all node power consumption if it is
 * using it exclusively.
 * - job: The job (application) get the power assigned based on the resources it is consuming, i.e., its affinity mask.
 *   \todo: This documentation can be improved if you think it is not clear. The name of type can be changed as well :)
 */
typedef enum node_sharing_strategy_t {
    node,
    job
} node_sharing_strategy_t;
#endif
