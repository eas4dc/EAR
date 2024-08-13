/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#ifndef _EARL_CPUPRIO_SUPPORT_H
#define _EARL_CPUPRIO_SUPPORT_H


/** Encapsulates priority setup. */
void policy_setup_priority();

/** This function resets priority configuration for CPUs used by the job
 * in the case priority system is enabled by the user. */
void policy_reset_priority();


#endif
