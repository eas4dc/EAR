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


#ifndef _EAR_BASE_H_
#define _EAR_BASE_H

/** Executed at application start. Reads EAR configuration env. variables */
void states_begin_job(int my_id,  char *app_name);

/** Executed at NEW_LOOP or END_NEW_LOOP DynAIS event */
void states_begin_period(int my_id, ulong event, ulong size,ulong level);

/** Executed at NEW_ITERATION DynAIS event */
void states_new_iteration(int my_id, uint size, uint iterations, uint level, ulong event,ulong mpi_calls_iter);

/** Executed at END_LOOP or END_NEW_LOOP DynAIS event */
void states_end_period(uint iterations);

/** Executed at application end */
void states_end_job(int my_id, char *app_name);

/** Returns the current EAR state */
int states_my_state();

/** */
ulong select_near_freq(ulong avg);

#endif
