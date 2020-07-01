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

#ifndef EAR_SLURM_PLUGIN_ENV_H
#define EAR_SLURM_PLUGIN_ENV_H

#include <slurm_plugin/slurm_plugin.h>

/*
 * Reading function
 */
int plug_read_plugstack(spank_t sp, int ac, char **av, plug_serialization_t *sd);

int plug_print_application(spank_t sp, application_t *app);

int plug_read_application(spank_t sp, plug_serialization_t *sd);

int plug_read_hostlist(spank_t sp, plug_serialization_t *sd);

/*
 * Serialization functions
 */
int plug_print_variables(spank_t sp);

int plug_clean_components(spank_t sp);

int plug_deserialize_local(spank_t sp, plug_serialization_t *sd);

int plug_deserialize_local_alloc(spank_t sp, plug_serialization_t *sd);

int plug_serialize_remote(spank_t sp, plug_serialization_t *sd);

int plug_deserialize_remote(spank_t sp, plug_serialization_t *sd);

int plug_serialize_task(spank_t sp, plug_serialization_t *sd);

/*
 * Cleaning functions
 */ 
int plug_clean_remote(spank_t sp);

int plug_clean_task(spank_t sp);

#endif
