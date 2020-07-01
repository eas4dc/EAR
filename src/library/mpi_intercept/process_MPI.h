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

#ifndef PROCESS_MPI_H
#define PROCESS_MPI_H

#include <library/mpi_intercept/MPI_types.h>

/** called before the MPI_init. Connects with ear_daemon */
void before_init();

/** called after the MPI_init */
void after_init();

/** called before all the mpi calls except mpi_init and mpi_finalize. It calls dynais and, based on 
*    dynais reported status, calls the main EAR functionallity */
void before_mpi(int call, p2i buf, p2i dest);

/** called after all the mpi calls excel mpi_init and mpi_finalize. */
void after_mpi();

/** called before mpi_finalize, it calls EAR functions to summarize metrics */
void before_finalize();

/** called after mpi_finalize */
void after_finalize();

#endif //PROCESS_MPI_H
