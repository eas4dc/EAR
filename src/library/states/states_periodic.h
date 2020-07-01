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


#ifndef _STATES_PERIODIC_H_
#define _STATES_PERIODIC_H_

/** Executed at job end */
void states_periodic_end_job(int my_id, FILE *ear_fd, char *app_name);

/** Executed at job end */
void states_periodic_begin_job(int my_id, FILE *ear_fd, char *app_name);

/** It must be executed when EAR goes to periodic mode. states_periodic_new_iteration must be executed after that when going to periodic mode */
void states_periodic_begin_period(int my_id, FILE *ear_fd, unsigned long event, unsigned int size);

/** It must be executed when EAR is running in periodic mode. It is called at the end/begin of each period (called just once) */
void states_periodic_end_period(uint iterations);


/** Executed every N mpi calls */
void states_periodic_new_iteration(int my_id, uint period, uint iterations, uint level, ulong event, ulong mpi_calls_iter);
#else
#endif

