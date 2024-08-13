/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _APP_SEVER_API_H
#define _APP_SERVER_API_H
#include <common/config.h>
/** Creates a named pipe to provide dc energy metrics  at user level */
int create_app_connection();

/** Returns the energy in mJ and the time in ms  */
int ear_energy(ulong *energy_mj,ulong *time_ms);

/** Releases resources to connect with applications */
int dispose_app_connection();

void *eard_non_earl_api_service(void *noinfo);
#else
#endif
