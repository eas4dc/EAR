/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EARL_GPU_SUPPORT_H_
#define _EARL_GPU_SUPPORT_H_

#include <common/includes.h>
#include <library/policies/policy_ctx.h>

/** Loads a policy GPU plug-in called gpu_<app_settings->policy_name>.so and stores its symbols in psyms.
 * If app_settings->user_type is authorized, the entire policy plug-in path can be forced to be loaded.
 * Prevents any non-master process to load the GPU policy plug-in. */
state_t policy_gpu_load(settings_conf_t *app_settings, polsym_t *psyms);

/** Returns whether the optimization is enabled, so a GPU policy plug-in read its value.  */
uint policy_gpu_opt_enabled();

/** Unlocks the GPU optimization, letting other processes to acquire it. */
void policy_gpu_unlock_opt();
#endif // _EARL_GPU_SUPPORT_H_
