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
#include <metrics/common/perf.h>

/* 
 * Information collected for SPR with this configuration
 * CPU(s):                  160
  On-line CPU(s) list:   0-159
Vendor ID:               GenuineIntel
  Model name:            Intel(R) Xeon(R) Platinum 8460Y+
    CPU family:          6
    Model:               143
* perf stat -vvvvvvvvvv -e power/energy-ram/ sleep 5
* sys_perf_event_open: pid -1  cpu 0  group_fd -1  flags 0 = 3
------------------------------------------------------------
perf_event_attr:
  type                             184
  size                             128
  config                           0x3
  sample_type                      IDENTIFIER
  read_format                      TOTAL_TIME_ENABLED|TOTAL_TIME_RUNNING
  disabled                         1
  inherit                          1
------------------------------------------------------------
sys_perf_event_open: pid -1  cpu 40  group_fd -1  flags 0 = 4

* perf stat -vvvvvvvvvv -e power/energy-pkg/ sleep 5
* ------------------------------------------------------------
perf_event_attr:
  type                             184
  size                             128
  config                           0x2
  sample_type                      IDENTIFIER
  read_format                      TOTAL_TIME_ENABLED|TOTAL_TIME_RUNNING
  disabled                         1
  inherit                          1
------------------------------------------------------------

* perf stat -vvvvvvvvvv -e power/energy-psys/ sleep 5
* ------------------------------------------------------------
perf_event_attr:
  type                             184
  size                             128
  config                           0x5
  sample_type                      IDENTIFIER
  read_format                      TOTAL_TIME_ENABLED|TOTAL_TIME_RUNNING
  disabled                         1
  inherit                          1
------------------------------------------------------------

Extra information can be gathered from sys files

ls /sys/bus/event_source/devices/power/
cpumask  events  format  perf_event_mux_interval_ms  power  subsystem  type  uevent
ls /sys/bus/event_source/devices/power/events/
energy-pkg  energy-pkg.scale  energy-pkg.unit  energy-psys  energy-psys.scale  energy-psys.unit  energy-ram  energy-ram.scale  energy-ram.unit

cat /sys/bus/event_source/devices/power/events/energy-pkg
event=0x02
cat /sys/bus/event_source/devices/power/events/energy-ram
event=0x03
cat /sys/bus/event_source/devices/power/events/energy-ram.unit
Joules
cat /sys/bus/event_source/devices/power/events/energy-ram.scale 
2.3283064365386962890625e-10
*/

/* This list must be extended for other CPU models */
#define POWER_ENERGY_PCK		0x2
#define POWER_ENERGY_RAM    0x3
#define POWER_ENERGY_SYS    0x5

#define NO_OPTIONS 0

#define EVENTS_PATH			"/sys/bus/event_source/devices/power"

static pthread_mutex_t perf_rapl_lock = PTHREAD_MUTEX_INITIALIZER;
static uint *perf_rapl_socket;
static state_t *perf_rapl_core_st;
static state_t *perf_rapl_uncore_st;
static topology_t my_topo;
static uint perf_rapl_num_sockets = 0;	// Used also to check whether the API is loaded
static perf_t *perf_rapl_core_perf, *perf_rapl_uncore_perf;
static int perf_type;
static double pkg_scale, ram_scale;

static state_t perf_rapl_read_conf();

state_t perf_rapl_load(topology_t * tp_in)
{
	state_t s;
	int j;

	debug("perf_rapl_load");
	if (tp_in->vendor != VENDOR_INTEL
	    || tp_in->model != MODEL_SAPPHIRE_RAPIDS) {
		debug("Warning, Vendor %u Model %u not tested , not loading",
		      tp_in->vendor, tp_in->model);
		return EAR_ERROR;
	}

	if (perf_rapl_num_sockets) {
		return EAR_ERROR;
	}

	if (state_fail(perf_rapl_read_conf())) {
		debug("perf_rapl: Configuration failed");
		pthread_mutex_unlock(&perf_rapl_lock);
		return EAR_ERROR;
	}

	/* If it is not initialized, I do it, else, I get the ids */

	pthread_mutex_lock(&perf_rapl_lock);

	if (state_fail
	    (s =
	     topology_select(tp_in, &my_topo, TPSelect.socket, TPGroup.merge,
			     0))) {
		debug("topology select fails");
		pthread_mutex_unlock(&perf_rapl_lock);
		return s;
	}

	/* To detect socket is enabled */
	perf_rapl_socket = calloc(my_topo.cpu_count, sizeof(uint));
	if (perf_rapl_socket == NULL) {
		pthread_mutex_unlock(&perf_rapl_lock);
		return EAR_ERROR;
	}

	/* Status of  perf access */
	perf_rapl_core_st = calloc(my_topo.cpu_count, sizeof(state_t));
	if (perf_rapl_core_st == NULL) {
		pthread_mutex_unlock(&perf_rapl_lock);
		return EAR_ERROR;
	}

	perf_rapl_uncore_st = calloc(my_topo.cpu_count, sizeof(state_t));
	if (perf_rapl_uncore_st == NULL) {
		pthread_mutex_unlock(&perf_rapl_lock);
		return EAR_ERROR;
	}

	/* Perf data */
	perf_rapl_core_perf = calloc(my_topo.cpu_count, sizeof(perf_t));
	if (perf_rapl_core_perf == NULL) {
		pthread_mutex_unlock(&perf_rapl_lock);
		return EAR_ERROR;
	}

	perf_rapl_uncore_perf = calloc(my_topo.cpu_count, sizeof(perf_t));
	if (perf_rapl_uncore_perf == NULL) {
		pthread_mutex_unlock(&perf_rapl_lock);
		return EAR_ERROR;
	}

	debug("perf_rapl: Using %d sockets. All memory allocated",
	      my_topo.cpu_count);

	for (j = 0; j < my_topo.cpu_count; j++) {
		if (state_fail
		    (perf_rapl_core_st[j] =
		     perf_open_cpu(&perf_rapl_core_perf[j], NULL, -1, perf_type,
				   (ulong) POWER_ENERGY_PCK, NO_OPTIONS,
				   my_topo.cpus[j].id))) {
			debug("perf_rapl: event %x open for cpu %d failed",
			      POWER_ENERGY_PCK, my_topo.cpus[j].id);
		}
		if (state_fail
		    (perf_rapl_uncore_st[j] =
		     perf_open_cpu(&perf_rapl_uncore_perf[j], NULL, -1,
				   perf_type, (ulong) POWER_ENERGY_RAM,
				   NO_OPTIONS, my_topo.cpus[j].id))) {
			debug("perf_rapl: event %x open for cpu %d failed",
			      POWER_ENERGY_RAM, my_topo.cpus[j].id);
		}
		perf_rapl_num_sockets =
		    perf_rapl_num_sockets + (state_ok(perf_rapl_core_st[j])
					     ||
					     state_ok(perf_rapl_uncore_st[j]));
		perf_rapl_socket[j] = state_ok(perf_rapl_core_st[j])
		    || state_ok(perf_rapl_uncore_st[j]);
	}

	pthread_mutex_unlock(&perf_rapl_lock);

#if SHOW_DEBUGS
	debug("perf_rapl: %u sockets with events detected",
	      perf_rapl_num_sockets);
	if (perf_rapl_num_sockets) {
		for (j = 0; j < my_topo.cpu_count; j++) {
			debug("perf_rapl: socket[%u] pck %s ram %s", j,
			      (state_ok(perf_rapl_core_st[j]) ? "enabled" :
			       "disabled"),
			      (state_ok(perf_rapl_uncore_st[j]) ? "enabled" :
			       "disabled"));
		}
	}
#endif

	if (perf_rapl_num_sockets) {
		return EAR_SUCCESS;
	} else {
		return EAR_ERROR;
	}
}

state_t perf_rapl_init(ctx_t * c)
{
	if (perf_rapl_num_sockets)
		return EAR_SUCCESS;
	return EAR_ERROR;
}

state_t perf_rapl_dispose(ctx_t * c)
{
	if (!perf_rapl_num_sockets)
		return EAR_SUCCESS;

	for (uint s = 0; s < my_topo.cpu_count; s++) {
		if (perf_rapl_socket[s]) {
			if (state_ok(perf_rapl_core_st[s]))
				perf_close(&perf_rapl_core_perf[s]);
			if (state_ok(perf_rapl_uncore_st[s]))
				perf_close(&perf_rapl_uncore_perf[s]);
		}
	}

	/* In case it comes from ear_finalize
	 * TODO: This call yields to undefined behaviour. */
	pthread_mutex_unlock(&perf_rapl_lock);
	return EAR_SUCCESS;
}

state_t perf_rapl_count_devices(ctx_t * c, uint * devs_count_in)
{
	if (perf_rapl_num_sockets) {
		*devs_count_in = my_topo.cpu_count;
		return EAR_SUCCESS;
	}
	return EAR_ERROR;
}

/* Data is stored DRAM0 DRAM1 PCK0 PCK1 */
/* Data is reported in nano J */
state_t perf_rapl_read(ctx_t * c, ullong * values)
{
	if (perf_rapl_num_sockets) {
		llong local_energy = 0;
		pthread_mutex_lock(&perf_rapl_lock);
		for (uint s = 0; s < my_topo.cpu_count; s++) {
			if (perf_rapl_socket[s]) {
				/* Reading core energy in socket s */
				if (state_ok(perf_rapl_core_st[s])) {
					local_energy = 0;
					if (state_fail
					    (perf_read
					     (&perf_rapl_core_perf[s],
					      &local_energy))) {
						debug
						    ("perf_rapl: socket[%u] read pck error",
						     s);
					}
					debug
					    ("perf_rapl: socket[%u] read core %lld",
					     s, local_energy);
					values[my_topo.cpu_count + s] =
					    (ullong) local_energy *pkg_scale *
					    1000000000;
					debug("CORE energy[%d]  %llu nJ", s,
					      values[my_topo.cpu_count + s]);
				}
				/* Reading uncore energy in socket s */
				if (state_ok(perf_rapl_uncore_st[s])) {
					local_energy = 0;
					if (state_fail
					    (perf_read
					     (&perf_rapl_uncore_perf[s],
					      &local_energy))) {
						debug
						    ("perf_rapl: socket[%u] read ram error",
						     s);
					}
					debug
					    ("perf_rapl: socket[%u] read uncore %lld",
					     s, local_energy);
					values[s] =
					    (ullong) local_energy *pkg_scale *
					    1000000000;
					debug("UNCORE enrgy[%d] %llu nJ", s,
					      values[s]);
				}
			}
		}
		pthread_mutex_unlock(&perf_rapl_lock);
		return EAR_SUCCESS;
	}

	return EAR_ERROR;
}

// Thread-safety sured by the caller of this function
static state_t perf_rapl_read_conf()
{
	char cfile[MAX_PATH_SIZE];
	FILE *fconfig;

	/* Event type */
	snprintf(cfile, sizeof(cfile), "%s/type", EVENTS_PATH);
	debug("perf_rapl: detecting perf type in %s", cfile);
	fconfig = fopen(cfile, "r");
	if (fconfig == NULL) {
		debug("perf_rapl: type file %s cannot be read", cfile);
		return EAR_ERROR;
	}

	pthread_mutex_lock(&perf_rapl_lock);

	fscanf(fconfig, "%d", &perf_type);
	fclose(fconfig);
	debug("perf_rapl: type for rapl events detected %d", perf_type);

	/* PCK Scale */
	snprintf(cfile, sizeof(cfile), "%s/events/energy-pkg.scale",
		 EVENTS_PATH);
	debug("perf_rapl: detecting pkg scale %s", cfile);
	fconfig = fopen(cfile, "r");
	if (fconfig == NULL) {
		debug("perf_rapl: pck scale file cannot be opened");
		pthread_mutex_unlock(&perf_rapl_lock);
		return EAR_ERROR;
	}
	fscanf(fconfig, "%lf", &pkg_scale);
	debug("perf_rapl: scale for pkg %E", pkg_scale);
	fclose(fconfig);

	/* RAM Units */
	snprintf(cfile, sizeof(cfile), "%s/events/energy-ram.scale",
		 EVENTS_PATH);
	debug("perf_rapl: detecting ram scale %s", cfile);
	fconfig = fopen(cfile, "r");
	if (fconfig == NULL) {
		debug("perf_rapl: ram scale file cannot be opened");
		pthread_mutex_unlock(&perf_rapl_lock);
		return EAR_ERROR;
	}
	fscanf(fconfig, "%lf", &ram_scale);
	debug("perf_rapl: scale for ram %E", ram_scale);
	fclose(fconfig);
	pthread_mutex_unlock(&perf_rapl_lock);
	return EAR_SUCCESS;
}
