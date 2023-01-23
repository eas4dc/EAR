/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

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
