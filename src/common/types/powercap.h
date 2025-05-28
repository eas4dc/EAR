/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EAR_TYPES_POWERCAP
#define _EAR_TYPES_POWERCAP

#define PC_STATUS_OK        0
#define PC_STATUS_GREEDY    1
#define PC_STATUS_RELEASE   2
#define PC_STATUS_ASK_DEF   3
#define PC_STATUS_IDLE      4
#define PC_STATUS_STOP      5
#define PC_STATUS_START     6
#define PC_STATUS_RUN       7

typedef enum {
	DRAM0 = 0,
	DRAM1,
	CPU0,
	CPU1,
	GPUSTART,
	NUM_DEV_TYPES
} powercap_device_types;

typedef struct pc_device_types_map {
    const char *name;
    powercap_device_types type;
} pc_device_types_map_t;

extern pc_device_types_map_t device_map[];

powercap_device_types get_device_enum(const char *device_name);

#endif
