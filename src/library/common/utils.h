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

/**
 *    \file shared_configuration.h
 *    \brief This file defines the API to create/attach/dettach/release the shared memory area between the EARD and the EARLib. It is used to communicate the EARLib updates 
 *	remotelly requested
 */

#ifndef _LIB_UTILS_H
#define _LIB_UTILS_H

#include <common/states.h>

state_t utils_create_plugin_path(char *result_path, char *install_path, char *custom_path, int authorized_user);
#endif
