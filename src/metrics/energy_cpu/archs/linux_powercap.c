/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

// #define SHOW_DEBUGS 1
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <common/config.h>
#include <common/states.h>

#include <common/output/verbose.h>
#include <common/output/debug.h>
#include <common/hardware/hardware_info.h>
#include <metrics/common/apis.h>

/* 
* https://www.kernel.org/doc/html/next/power/powercap/powercap.html
*/

#define linux_powercap_folder "/sys/devices/virtual/powercap/intel-rapl/intel-rapl:%d"
/* Core means package in this case */
#define linux_powercap_core_folder   "/sys/devices/virtual/powercap/intel-rapl/intel-rapl:%d/"
#define linux_powercap_pck_metric   "/sys/devices/virtual/powercap/intel-rapl/intel-rapl:%d/%s"
#define linux_powercap_uncore_folder "/sys/devices/virtual/powercap/intel-rapl/intel-rapl:%d/intel-rapl:%d:0"
#define linux_powercap_metric        "/sys/devices/virtual/powercap/intel-rapl/intel-rapl:%d/intel-rapl:%d:%d/%s"
#define linux_powercap_energy_file   "energy_uj"
#define linux_powercap_domain_name   "name"
#define linux_powercap_pck_name      "package-%d"
#define linux_powercap_dram_name     "dram"
// Based on linux doc. Each of these power zones contains two subzones,
//      intel-rapl:j:0 and
//      intel-rapl:j:1 (j = 0, 1)
// representing the "core" and the "uncore" parts of the given CPU package,
// respectively.
//      #define core_id   0
//      #define uncore_id 1
#define core_id   1
#define uncore_id 0

static pthread_mutex_t linux_powercap_lock = PTHREAD_MUTEX_INITIALIZER;
static uint *linux_powercap_socket;
static int *linux_powercap_core_fds;
static int *linux_powercap_uncore_fds;
static topology_t my_topo;
static uint linux_powercap_num_sockets = 0;

state_t linux_powercap_load(topology_t * tp_in)
{
	char aux_folder_name[1024];
	char pck_name[128];
	char devname[128];
	state_t s;
	int j;

	debug("linux_powercap_load");
	/* If it is not initialized, I do it, else, I get the ids */
	pthread_mutex_lock(&linux_powercap_lock);

	if (state_fail
	    (s =
	     topology_select(tp_in, &my_topo, TPSelect.socket, TPGroup.merge,
			     0))) {
		debug("topology select fails");
		pthread_mutex_unlock(&linux_powercap_lock);
		return s;
	}
	linux_powercap_socket = calloc(my_topo.cpu_count, sizeof(uint));
	if (linux_powercap_socket == NULL) {
		debug("Error allocating memory");
		pthread_mutex_unlock(&linux_powercap_lock);
		return EAR_ERROR;
	}
	linux_powercap_core_fds = calloc(my_topo.cpu_count, sizeof(int));
	if (linux_powercap_core_fds == NULL) {
		debug("Error allocating memory");
		pthread_mutex_unlock(&linux_powercap_lock);
		return EAR_ERROR;
	}
	linux_powercap_uncore_fds = calloc(my_topo.cpu_count, sizeof(int));
	if (linux_powercap_uncore_fds == NULL) {
		debug("Error allocating memory");
		pthread_mutex_unlock(&linux_powercap_lock);
		return EAR_ERROR;
	}

	debug("linux_powercap: Using %d sockets", my_topo.cpu_count);

	for (j = 0; j < my_topo.cpu_count; j++) {
		sprintf(aux_folder_name, linux_powercap_folder, j);
		debug("testing driver folder %s", aux_folder_name);
		/* Check socket folder */
		if (ear_file_is_directory(aux_folder_name)) {
			debug("Socket %d exists", j);
			linux_powercap_socket[j] = 1;

			/* Core */
			sprintf(aux_folder_name, linux_powercap_core_folder, j);
			debug("Testing %s", aux_folder_name);
			if (ear_file_is_directory(aux_folder_name)) {
				debug("core folder detected %s",
				      aux_folder_name);
				sprintf(pck_name, linux_powercap_pck_name, j);
				sprintf(aux_folder_name,
					linux_powercap_pck_metric, j,
					linux_powercap_domain_name);

				/* This code is just to check the name is the expected one */
				debug("Reding %s to check device",
				      aux_folder_name);
				int fd = open(aux_folder_name, O_RDONLY);
				if (fd >= 0) {
					memset(devname, 0, sizeof(devname));
					read(fd, devname, sizeof(devname));
					debug("device name %s", devname);
					if (strcmp(devname, pck_name) == 0) {
						debug
						    ("Folder for pack %d (%s) found",
						     j, pck_name);
						close(fd);
					}
				} else {
					debug("error reading device name %s",
					      aux_folder_name);
					return EAR_ERROR;
				}

				/* Here we open the metric file */
				sprintf(aux_folder_name,
					linux_powercap_pck_metric, j,
					linux_powercap_energy_file);
				debug("Testing metric %s", aux_folder_name);
				if ((linux_powercap_core_fds[j] =
				     open(aux_folder_name, O_RDONLY)) >= 0) {
					linux_powercap_num_sockets++;
					debug("Core file readable (%s) fd = %d",
					      aux_folder_name,
					      linux_powercap_core_fds[j]);
				} else {
					debug("Core file not readable (%s)",
					      aux_folder_name);
					return EAR_ERROR;
				}
			} else {
				debug("core folder not detected %s",
				      aux_folder_name);
				return EAR_ERROR;
			}

			/* Uncore */
			sprintf(aux_folder_name, linux_powercap_uncore_folder,
				j, j);
			debug("Testing %s", aux_folder_name);
			if (ear_file_is_directory(aux_folder_name)) {
				debug("uncore folder detected %s",
				      aux_folder_name);
				/* This code is just to check the name is the expected one */
				sprintf(aux_folder_name, linux_powercap_metric,
					j, j, uncore_id,
					linux_powercap_domain_name);

				debug("Reding %s to check device",
				      aux_folder_name);
				int fd = open(aux_folder_name, O_RDONLY);
				if (fd >= 0) {
					memset(devname, 0, sizeof(devname));
					read(fd, devname, sizeof(devname));
					debug("uncore device name %s", devname);
					if (strcmp
					    (devname,
					     linux_powercap_dram_name) == 0) {
						debug
						    ("Folder for dram %d (%s) found",
						     j,
						     linux_powercap_dram_name);
						close(fd);
					}
				} else {
					debug("error reading device name %s",
					      aux_folder_name);
				}
				/* We open the metric file now */
				sprintf(aux_folder_name, linux_powercap_metric,
					j, j, uncore_id,
					linux_powercap_energy_file);
				debug("Testing metric %s", aux_folder_name);
				if ((linux_powercap_uncore_fds[j] =
				     open(aux_folder_name, O_RDONLY)) >= 0) {
					linux_powercap_num_sockets++;
					debug
					    ("Uncore file readable (%s) fd = %d",
					     aux_folder_name,
					     linux_powercap_uncore_fds[j]);
				} else {
					debug("Uncore file not readable (%s)",
					      aux_folder_name);
				}
			} else {
				debug("uncore folder not detected %s",
				      aux_folder_name);
			}
		} else {
			debug("Socket %d does not exists", j);
		}
	}
	pthread_mutex_unlock(&linux_powercap_lock);
	if (linux_powercap_num_sockets)
		return EAR_SUCCESS;
	return EAR_ERROR;
}

state_t linux_powercap_init(ctx_t * c)
{
	return EAR_SUCCESS;
}

state_t linux_powercap_dispose(ctx_t * c)
{
	for (uint s = 0; s < my_topo.cpu_count; s++) {
		if (linux_powercap_socket[s]) {
			if (linux_powercap_core_fds[s] > 0)
				close(linux_powercap_core_fds[s]);
			if (linux_powercap_uncore_fds[s] > 0)
				close(linux_powercap_uncore_fds[s]);
		}
	}
	return EAR_SUCCESS;
}

state_t linux_powercap_count_devices(ctx_t * c, uint * devs_count_in)
{
	*devs_count_in = my_topo.cpu_count;
	return EAR_SUCCESS;
}

/* Data is stored DRAM0 DRAM1 PCK0 PCK1 */
/* Data is reported in nano J */
state_t linux_powercap_read(ctx_t * c, ullong * values)
{
	char energy_uj[64];
	for (uint s = 0; s < my_topo.cpu_count; s++) {
		if (linux_powercap_socket[s]) {
			/* Reading core energy in socket s */
			if (linux_powercap_core_fds[s] > 0) {
				lseek(linux_powercap_core_fds[s], 0, SEEK_SET);
				memset(energy_uj, 0, sizeof(energy_uj));
				read(linux_powercap_core_fds[s], energy_uj,
				     sizeof(energy_uj));
				values[my_topo.cpu_count + s] =
				    (ullong) atoll(energy_uj) * 1000;
				debug("CORE energy[%d]  %llu nJ", s,
				      values[my_topo.cpu_count + s]);
			}
			/* Reading uncore energy in socket s */
			if (linux_powercap_uncore_fds[s] > 0) {
				lseek(linux_powercap_uncore_fds[s], 0,
				      SEEK_SET);
				memset(energy_uj, 0, sizeof(energy_uj));
				read(linux_powercap_uncore_fds[s], energy_uj,
				     sizeof(energy_uj));
				values[s] = (ullong) atoll(energy_uj) * 1000;
				debug("UNCORE enrgy[%d] %llu nJ", s, values[s]);
			}
		}
	}
	return EAR_SUCCESS;
}
