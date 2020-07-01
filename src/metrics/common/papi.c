/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*	
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*	
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING	
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/config.h>
#include <common/output/verbose.h>
#include <metrics/common/papi.h>

void  metrics_get_app_name(char *app_name)
{
	const PAPI_exe_info_t *prginfo = NULL;

	//PAPI_INIT_TEST(__FILE__);

	if ((prginfo = PAPI_get_executable_info()) == NULL)
	{
		error("executable info not available");
		strcpy(app_name,"NO NAME");
		return;
	}

	strcpy(app_name, prginfo->fullname);
}

const PAPI_hw_info_t *metrics_get_hw_info()
{
	//
	//PAPI_INIT_TEST(__FILE__);

	// General hardware info by PAPI
	return PAPI_get_hardware_info();
}

int metrics_get_node_size()
{
	const PAPI_hw_info_t *hwinfo = metrics_get_hw_info();
	return hwinfo->sockets * hwinfo->cores * hwinfo->threads;
}

/*
 *
 * PAPI control (not static, but internal)
 *
 */

int _papi_start_counters(int event_set)
{
	int result;

	if ((result = PAPI_start(event_set) != PAPI_OK)) {
		error("Error while starting accounting (%s)", PAPI_strerror(result));
		return 0;
	}

	return 1;
}

int _papi_stop_counters(int event_set, long long *event_values)
{
	int result;

	if ((result = PAPI_stop(event_set, event_values)) != PAPI_OK) {
		error("Error while stopping accounting (%s)", PAPI_strerror(result));
		return 0;
	}
	return 1;
}

int _papi_read_counters(int event_set, long long *event_values)
{
	int result;

	if ((result = PAPI_read(event_set, event_values)) != PAPI_OK) {
		error("Error while reading counters (%s)", PAPI_strerror(result));
		return 0;
	}
	return 1;
}

int _papi_event_add(int event_set, char *event_name)
{
	int result;

	if ((result = PAPI_add_named_event(event_set, event_name)) != PAPI_OK) {
		error("Error while adding %s event (%s)", event_name, PAPI_strerror(result));
		return 0;
	}
	return 1;
}

int _papi_multiplex_init()
{
	int papi_multi;

	if ((papi_multi = PAPI_multiplex_init()) != PAPI_OK){
		error("Error when multiplexing %s", PAPI_strerror(papi_multi));
		return 0;
	}
	return 1;
}

int _papi_component_init(char *component_name)
{
	const PAPI_component_info_t *info = NULL;
	int cid;

	if ((cid = PAPI_get_component_index(component_name)) < 0) {
		error("Error, %s component not found (%s)", component_name, PAPI_strerror(cid));
		return 0;
	}
	if ((info = PAPI_get_component_info(cid)) == NULL) {
		error("Error error while accessing to component %s info", component_name);
		return 0;
	}
	if (info->disabled) {
		error("Error, %s disabled (%s)", component_name, info->disabled_reason);
		return 0;
	}

	debug( "PAPI component %s has been initialized correctly", component_name);
	return 1;
}

int _papi_init()
{
	int papi_init;

	if (PAPI_is_initialized() != PAPI_NOT_INITED) {
		return 1;
	}

	if ((papi_init = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT) {
		error("Error intializing PAPI (%s)", PAPI_strerror(papi_init));
		return 0;
	}

	debug( "PAPI has been initialized correctly");
	return 1;
}
