/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef MANAGEMENT_GOVERNOR_H
#define MANAGEMENT_GOVERNOR_H

#define _GNU_SOURCE
#include <common/plugins.h>
#include <common/states.h>
#include <common/types.h>
#include <sched.h>

// Governors
//
// The purpose of this file is to split the governor from cpufreq API, because
// it created symbol conflicts when compiling.
//
// More information about governors:
// 	https://www.kernel.org/doc/Documentation/cpu-freq/governors.txt
//
// Note: governor last means the previous governor before governor change. Init
//       means the governor set before the API was initialized.
//
struct governor_s {
    uint conservative;
    uint performance;
    uint userspace;
    uint powersave;
    uint ondemand;
    uint unknown;
    uint last;
} Governor __attribute__((weak)) = {
    .conservative = 0,
    .performance  = 1,
    .userspace    = 2,
    .powersave    = 3,
    .ondemand     = 4,
    .unknown      = 5,
    .last         = 6,
};

struct goverstr_s {
    char *conservative;
    char *performance;
    char *userspace;
    char *powersave;
    char *ondemand;
    char *unknown;
} Goverstr __attribute__((weak)) = {
    .conservative = "conservative",
    .performance  = "performance",
    .userspace    = "userspace",
    .powersave    = "powersave",
    .ondemand     = "ondemand",
    .unknown      = "unknown",
};

// To compile well
#define mgt_governor_tostr(g, b) governor_tostr(g, b)
#define mgt_governor_toint(b, g) governor_toint(b, g)
#define mgt_governor_is(b, g)    governor_is(b, g)

/* Returns a governor name given a governor id. */
state_t governor_tostr(uint governor, char *buffer);
/* Returns a governor id given a governor name. */
state_t governor_toint(char *buffer, uint *governor);
/* Given a governor name and id, returns true if it is the same. */
int governor_is(char *buffer, uint governor);

#endif // MANAGEMENT_GOVERNOR_H
