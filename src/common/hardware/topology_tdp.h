/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_HARDWARE_TOPOLOGY_TDP_H
#define COMMON_HARDWARE_TOPOLOGY_TDP_H

#include <common/hardware/topology.h>

// TDP base list
#define TDP_HASWELL   VENDOR_INTEL, MODEL_HASWELL_X, 100
#define TDP_BROADWELL VENDOR_INTEL, MODEL_BROADWELL_X, 100
#define TDP_SKYLAKE   VENDOR_INTEL, MODEL_SKYLAKE_X, 150
#define TDP_ICELAKE   VENDOR_INTEL, MODEL_ICELAKE_X, 175
#define TDP_ZEN       VENDOR_AMD, FAMILY_ZEN, 150
#define TDP_ZEN3      VENDOR_AMD, FAMILY_ZEN3, 200

// TDP specific list
#define TDP_BROADWELL_E5_2686_V4   VENDOR_INTEL, MODEL_BROADWELL_X, "E5-2686 v4", 145
#define TDP_SKYLAKE_PLATINUM_8175M VENDOR_INTEL, MODEL_SKYLAKE_X, "Platinum 8175M", 125

#endif
