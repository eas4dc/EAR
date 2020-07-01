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



#ifndef _EAR_API_H_
#define _EAR_API_H_

#include <library/mpi_intercept/MPI_types.h>
#include <library/mpi_intercept/MPI_calls_coded.h>

/** Initializes all the elements of the library as well as connecting to the daemon. */
void ear_init();
/** Given the information corresponding a MPI call, creates a DynAIS event
*   and processes it as well as creating the trace. If the library is in
*   a learning phase it does nothing. */
void ear_mpi_call(mpi_call call_type, p2i buf, p2i dest);
/** Finalizes the processes, closing and registering metrics and traces, as well as
*   closing the connection to the daemon and releasing the memory from DynAIS. */
void ear_finalize();


/***** API for manual application modification *********/
void ear_new_iteration(unsigned long loop_id);
void ear_end_loop(unsigned long loop_id);
unsigned long ear_new_loop();

#else
#endif



