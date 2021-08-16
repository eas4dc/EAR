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
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#ifndef _EAR_TYPES_LOOP
#define _EAR_TYPES_LOOP

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

/** Sets a loop to null values */
int set_null_loop(loop_t *loop);
/** Returns true if the loop data is not, return -1 in case of error */
int is_null(loop_t *loop);

/** Initalizes the loop given by parameter. */
int loop_init(loop_t *loop, job_t *job,ulong event, ulong size, ulong level);

/* Cleans information that could generate DB failures */
void clean_db_loop(loop_t *loop,double limit);

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
/* Same as previous one but without job information */
int append_loop_text_file_no_job(char *path, loop_t *loop);
/* Adds the ts to the output */
int append_loop_text_file_no_job_with_ts(char *path, loop_t *loop,ullong currtime);

/** Given a loop_t and a file descriptor, outputs the contents of said loop to the fd.*/
void print_loop_fd(int fd, loop_t *loop);


#endif
