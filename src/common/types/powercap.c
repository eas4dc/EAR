/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <string.h>

#include <common/types/powercap.h>

pc_device_types_map_t device_map[] = {
	{"DRAM_0", DRAM0},
    {"DRAM_1", DRAM1},
    {"CPU_0", CPU0},
    {"CPU_1", CPU1},
    {"GPU_", GPUSTART},
	{"PCK_0", CPU0},
    {"PCK_1", CPU1},
	{"DRAM0", DRAM0},
    {"DRAM1", DRAM1},
    {"CPU0", CPU0},
    {"CPU1", CPU1},
    {"PCK0", CPU0},
    {"PCK1", CPU1},
    {"GPU", GPUSTART}
};

int device_map_size = sizeof(device_map) / sizeof(device_map[0]);

powercap_device_types get_device_enum(const char *device_name) {
    for (int i = 0; i < device_map_size; i++) {
		int len = strlen(device_map[i].name);
		if (strncmp(device_name, device_map[i].name, len) == 0) {
			if (i == GPUSTART) {
				// GPU case, add an offset to the index
                if (device_name[len] >= '0' && device_name[len] <= '9')
					return device_map[i].type + (device_name[len] - 48);
				else return -1;
            } else return device_map[i].type;
	}
	}
    return -1;
}