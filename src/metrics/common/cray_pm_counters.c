/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

//#define SHOW_DEBUGS 0

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <common/sizes.h>
#include <common/states.h>
#include <common/output/debug.h>
#include <metrics/common/file.h>

#define CRAY_PM_BASE "/sys/cray/pm_counters/"

// Metrics
#define VERSION		"version"
#define ENERGY    "energy"

// Metrics
#define ENERGY_UNITS 		1
static uint cray_pm_initialized = 0;
static int fd_version = -1;
static int fd_energy = -1;
static int cray_version = -1;

// This is a prototype, we should detect what is available based on version

state_t cray_pm_init()
{
	state_t st = EAR_ERROR;
	char cray_version_str[64];
	char pm_metric[MAX_PATH_SIZE];
	snprintf(pm_metric, sizeof(pm_metric), "%s/%s", CRAY_PM_BASE, VERSION);
	debug("CRAY PM: Testing %s", pm_metric);

	// Version file exists ?
	if (filemagic_can_read(pm_metric, &fd_version)) {
		debug("CRAY PM: %s available", pm_metric);
		if (filemagic_word_read(fd_version, cray_version_str, 1)) {
			cray_version = atoi(cray_version_str);
			debug("CRAY PM: %d version detected", cray_version);
		}
		snprintf(pm_metric, sizeof(pm_metric), "%s/%s", CRAY_PM_BASE,
			 ENERGY);
		debug("CRAY PM: Testing %s", pm_metric);

		// Energy file exists ?
		if (filemagic_can_read(pm_metric, &fd_energy)) {
			cray_pm_initialized = 1;
			st = EAR_SUCCESS;
		} else {
			close(fd_version);
		}
	}
	return st;
}

static state_t static_read_energy(ulong * value)
{
	char energy_str[128];
	state_t st = EAR_ERROR;
	if (filemagic_word_read(fd_energy, energy_str, 1)) {
		*value = strtoul(energy_str, NULL, 0);
		st = EAR_SUCCESS;
	}
	return st;
}

state_t cray_pm_get(char *metric, ulong * value)
{
	state_t st = EAR_ERROR;
	if (cray_pm_initialized) {
		if (strcmp(metric, "energy") == 0)
			st = static_read_energy(value);
	}
	return st;
}

state_t cray_pm_dispose()
{
	if (cray_pm_initialized) {
		close(fd_version);
		close(fd_energy);
	}
	return EAR_SUCCESS;
}
