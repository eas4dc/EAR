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

#ifndef _EAR_TYPES_LOOP
#define _EAR_TYPES_LOOP

#define GENERIC_NAME 256
#include <common/types/generic.h>
#include <common/types/signature.h>
#include <common/types/job.h>

typedef struct loop_id
{
    ulong event;
    ulong size;
    ulong level;
} loop_id_t;

typedef struct loop
{
    loop_id_t id;
		ulong jid,step_id;
    char node_id[GENERIC_NAME];
    ulong total_iterations;
    signature_t signature;
} loop_t;


// Function declarations

// MANAGEMENT

/** Create a new loop_id_t based on dynais information. */
int create_loop_id(loop_id_t *id,ulong event, ulong size, ulong level);

/** Initalizes the loop given by parameter. */
int loop_init(loop_t *loop, job_t *job,ulong event, ulong size, ulong level);

/** Given a new loop detected by dynais returns either a new loop_id (if it is 
*   really a new loop never detected) or the pointer to the already existing 
*   information. If it is a new loop, memory is allocated for this loop. 
*   Memory for N signatures is reserved. */
int new_loop(loop_t *l,loop_id_t *lid);

/** Add values for a new computed signature. */
void add_loop_signature(loop_t *loop,  signature_t *sig);

/** Loop is finished, number of iterations is included. */
void end_loop(loop_t *loop, ulong iterations);

/** Copies the source loop into the destiny one */
void copy_loop(loop_t *destiny, loop_t *source);

// REPORTING
/** Appends in a file a loop in CSV format. The returned integer is one
*   of the following states: EAR_SUCCESS or EAR_ERROR. */
int append_loop_text_file(char *path, loop_t *loop,job_t *job);

/** Given a loop_t and a file descriptor, outputs the contents of said loop to the fd.*/
void print_loop_fd(int fd, loop_t *loop);


#endif
