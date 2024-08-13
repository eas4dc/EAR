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

state_t policy_gpu_load(settings_conf_t *app_settings, polsym_t *psyms);
#endif

