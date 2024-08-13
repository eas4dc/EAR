/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

#ifndef _CRAY_PM_H_
#define _CRAY_PM_H_
#include <common/states.h>

state_t cray_pm_init();
state_t cray_pm_get(char *metric, ulong * value);
state_t cray_pm_dispose();
#endif
