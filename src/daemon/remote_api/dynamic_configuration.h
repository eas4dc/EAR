/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

/**
*    \file dynamic_configurarion.h
*    \brief exports the API to control the dynamic reconfiguration of EARlib arguments (at this moment frequency and min_tim eth)
*
*/

#ifndef _DYN_CONF_H
#define _DYN_CONF_H

/** Creates a socket to accept remote commands for dynamic EAR configuration. It is dessigned to be executed in the context of a new thread
*/
void *eard_dynamic_configuration(void *no_args);

/** Returns the frequency defined dynamically
*/
ulong max_dyn_freq();

#endif
