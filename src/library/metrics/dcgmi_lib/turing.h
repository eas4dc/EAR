/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

#ifndef _DCGMI_LIB_TURING_H_
#define _DCGMI_LIB_TURING_H_

#include <common/states.h>

#include <library/metrics/dcgmi_lib/common.h>

state_t dcgmi_lib_turing_init(uint api, uint all_events, dcgmi_lib_t *dcgmi_data,
                              int (*event_info)[DCGMI_LIB_SUPPORTED_EVENTS][2]);

#endif // _DCGMI_LIB_TURING_H_
