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

#include <common/config.h>

#ifndef _EAR_TYPES_GM_WARNING
#define _EAR_TYPES_GM_WARNING

#include <common/types/generic.h>
#define NODE_SIZE 256

typedef struct gm_warning 
{
    ulong level;
    ulong new_p_state;
    double energy_percent;
    double inc_th;
    ulong energy_t1;
    ulong energy_t2;
    ulong energy_limit;
    ulong energy_p1;
    ulong energy_p2;
    char policy[64];
} gm_warning_t;


// Function declarations

/** Replicates the periodic_metric in *source to *destiny */
void copy_gm_warning(gm_warning_t *destiny, gm_warning_t *source);

/** Initializes all values of the periodic_metric to 0 , sets the nodename */
void init_gm_warning(gm_warning_t *pm);


#endif
