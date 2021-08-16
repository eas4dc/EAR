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

#ifndef _EAR_TYPES_APPLICATION
#define _EAR_TYPES_APPLICATION

#include <stdio.h>
#include <stdint.h>
#include <common/config.h>
#include <common/types/job.h>
#include <common/types/signature.h>
#include <common/types/power_signature.h>

typedef struct application
{
	job_t job;	
	uint8_t is_mpi;
	uint8_t is_learning;
	char node_id[GENERIC_NAME];		
	power_signature_t power_sig; // power_sig are power metrics related to the whole job, not only the mpi part
	signature_t	signature; 		 // signature refers to the mpi part, it includes power metrics and performance metrics
} application_t;


/** Resets the data. */
void init_application(application_t *app);

/** Replicates the application in *source to *destiny */
void copy_application(application_t *destiny, application_t *source);

/** Cleaned remake of the classic print 'fd' function */
void application_print_channel(FILE *file, application_t *app);

/*
 *
 * Obsolete?
 *
 */

/** Reads a file of applications saved in CSV format. A block of memory is
    allocated for this read applications, and is returned by the argument 'apps'.
    The returned integer is the number of applications read. If the integer is
    negative, one of the following errors ocurred: EAR_ALLOC_ERROR,
    EAR_READ_ERROR or EAR_OPEN_ERROR. */
int read_application_text_file(char *path, application_t **apps, char is_extended);

/** Appends in a file an application in CSV format. The returned integer is one
*   of the following states: EAR_SUCCESS or EAR_ERROR. */
int append_application_text_file(char *path, application_t *app, char is_extended);

/** PENDING */
int scan_application_fd(FILE *fd, application_t *app, char is_extended);

/*
 *
 * I don't know what is this
 *
 */

#define create_ID(id,sid)	(id*100+sid)

void read_application_fd_binary(int fd,application_t *app);

void print_application_fd_binary(int fd,application_t *app);

void mark_as_eard_connected(int jid,int sid,int pid);
uint is_already_connected(int jid,int sid,int pid);
void mark_as_eard_disconnected(int jid,int sid,int pid);

/*
 *
 * We have to take a look these print functions and clean
 *
 */

/** Prints the contents of app to the STDOUT*/
int print_application(application_t *app);

/** Prints a summary of the application to STDOUT */
void report_application_data(application_t *app);
void verbose_application_data(uint vl,application_t *app);

/** Prints a summary of the application (only mpi part,power signature is not reported) to STDOUT */
void report_mpi_application_data(application_t *app);

#endif
