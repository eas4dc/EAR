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

#ifndef COMMON_UTILS_SERIAL_BUFFER_H
#define COMMON_UTILS_SERIAL_BUFFER_H

// This is a buffer to hold data in packages or elements. It has intern metadata
// to control the allocated data, the current used data and also a pointer to the
// current element. The current data element is the one to be returned when using
// functions like serial_copy_elem(). 

typedef struct wide_buffer_s {
	char *data;
} wide_buffer_t;

void serial_alloc(wide_buffer_t *b, size_t size);

void serial_free(wide_buffer_t *b);

/* Allocates more space if needed. */
void serial_resize(wide_buffer_t *b, size_t size);

/* Rewinds the element pointer and marks the data buffer as 0 bytes taken. */
void serial_clean(wide_buffer_t *b);

/* Rewinds the data pointer and assigns space for an incoming data. */
void serial_assign(wide_buffer_t *b, size_t size);

/* Resets the current data element pointer. */
void serial_rewind(wide_buffer_t *b);

/* Adds a new element to the buffer. */
void serial_add_elem(wide_buffer_t *b, char *data, size_t size);

/* Copies the next element inside the buffer. */
char *serial_copy_elem(wide_buffer_t *b, char *data, size_t *size);

/* Returns the pointer to the data (avoid the metadata). */
char *serial_data(wide_buffer_t *b);

/* Returns the size of the whole buffer or just the data (if 1). */
size_t serial_size(wide_buffer_t *b, uint data);

#endif
