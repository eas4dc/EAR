/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EAR_TYPES_APPLICATION
#define _EAR_TYPES_APPLICATION

#include <common/config.h>
#include <common/types/job.h>
#include <common/types/power_signature.h>
#include <common/types/signature.h>
#include <common/utils/serial_buffer.h>
#include <stdint.h>
#include <stdio.h>

// WARNING! This type is serialized through functions application_serialize and
// application_deserialize. If you want to add new types, make sure to update
// these functions too.
typedef struct application {
    job_t job;
    uint8_t is_mpi;
    uint8_t is_learning;
    char node_id[GENERIC_NAME];
    power_signature_t power_sig; // power_sig are power metrics related to the whole job, not only the mpi part
    signature_t signature;       // signature refers to the mpi part, it includes power metrics and performance metrics
} application_t;

/** Resets the memory region pointed by \p app. */
void init_application(application_t *app);

/** Replicates the application in *source to *destiny. */
void copy_application(application_t *destiny, application_t *source);

/** Cleaned remake of the classic print 'fd' function. */
void application_print_channel(FILE *file, application_t *app);

/** Uses signature to fill the basic (power) signature
 */
void fill_basic_sig(application_t *app);

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
int append_application_text_file(char *path, application_t *app, char is_extended, int add_header, int single_column);

/** Outputs an application to the fd given in CSV format. Returns EAR_SUCCESS */
int print_application_fd(int fd, application_t *app, int new_line, char is_extended, int single_column);

int create_app_header(char *header, char *path, uint num_gpus, char is_extended, int single_column);

/** PENDING */
int scan_application_fd(FILE *fd, application_t *app, char is_extended);

/*
 *
 * I don't know what is this
 *
 */

#define create_ID(id, sid) (id * 100 + sid)
#define EID(app)           (app->job.local_id)

void read_application_fd_binary(int fd, application_t *app);

void print_application_fd_binary(int fd, application_t *app);

/*
 *
 * We have to take a look these print functions and clean
 *
 */

/** Prints the contents of app to the STDOUT. */
int print_application(application_t *app);

/** Prints a summary of the application to STDOUT. */
void report_application_data(application_t *app);

/** \todo */
void verbose_application_data(uint vl, application_t *app);

/** Prints a summary of the application (only mpi part,power signature is not reported) to STDOUT */
void report_mpi_application_data(int vl, application_t *app);

void application_serialize(serial_buffer_t *b, application_t *app);

void application_deserialize(serial_buffer_t *b, application_t *app);

state_t application_create_header_str(char *header_dst, size_t header_dst_size, char * header_prefix, uint num_gpus, char is_extended, int single_column);

#endif
