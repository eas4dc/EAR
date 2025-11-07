/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_CONFIG_DEV_H
#define COMMON_CONFIG_DEV_H

#include <common/config/config_install.h>

/**
 * \file config_dev.h
 * This file defines default features that EAR code provides  */

/* To be removed and set fixed in the code */
#define EARL_RESEARCH 1
/* When set to 1 , creates a thread in EARD to support application queries apart
 *  *  * from EARL, do not set to 0 except for debug purposes */
#define APP_API_THREAD 1
/* When set to 1, creates a thread in EARD for powermonitoring, do not set to 0
 *  *  * except for debug purposes */
#define POWERMON_THREAD 1
/* When set to 1 , creates a thread in EARD for external commands, do not set to
 *  *  * 0 except for debug purposes */
#define EXTERNAL_COMMANDS_THREAD 1
#define USE_LEARNING_APPS        1
/* Disable for testing overhead.
 *  *  * Enables accounting of MPI statistics all call time. */
#define MPI_STATS_ENABLED 1
/* For EAR validation */
// #define FAKE_ERROR_USE_DUMMY 1
// #define FAKE_EARD_NOT_CONNECTED_ENERGY_PLUGIN 1
// #define FAKE_ENERGY_PLUGIN_ERROR_PATH 1

#endif // COMMON_CONFIG_DEV_H
