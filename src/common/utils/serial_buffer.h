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
//
//	1) Serial buffer initialization
//
//		wide_buffer_t w;
//		// Initializing 16KB of memory.
//		serial_alloc(&w, 16384);
//
//	2) Example of packing data
//
//		// Cleans the buffer and metadata
//		serial_clean(&w);
//		// Adding new elements
//		serial_add_elem(&w, (char *) elem1, sizeof(elem1_t)*elem1_length);
//		serial_add_elem(&w, (char *) elem2, sizeof(elem2_t)*elem2_length);
//
//	3) Example of sending data
//
//		char  *data = w.data;
//		size_t size = serial_size(&w, 0);
//		some_sending_function(fd, data, size)));
//
//	4) Example of receiving data
//
// 		wide_buffer_t w;
//		// Initializing 16KB of memory.
//		serial_alloc(&w, 16384);
//		// You can use this because the first parameter in the buffer
//		// structure is the allocated space.
//		some_receiving_function(call, (char **) &w);
//
//	5) Example of unpacking data
//
//		// Rewind is necessary to set the internal pointer to the first
//		// element again. The first time is not required.
//		serial_rewind(&w);
//		serial_copy_elem(&w, (char *) elem1, NULL);
//		serial_copy_elem(&w, (char *) elem2, NULL);
//

typedef struct serial_buffer_t {
	char *data;
	size_t size;
} serial_buffer_t;

typedef serial_buffer_t wide_buffer_t;

#define SIZE_1KB 1024
#define SIZE_2KB 2048
#define SIZE_4KB 4096
#define SIZE_8KB 8192

void serial_alloc(wide_buffer_t *b, size_t size);

void serial_free(wide_buffer_t *b);

/* Allocates more space if needed. */
void serial_resize(wide_buffer_t *b, size_t size);

/* Marks the data buffer as 0 bytes taken and rewinds the element pointer. */
void serial_clean(wide_buffer_t *b);

/* Resize + clean. */
void serial_reset(wide_buffer_t *b, size_t size);

/* Back to the first the element pointer. */
void serial_rewind(wide_buffer_t *b);

/* Adds a new element to the buffer. */
void serial_add_elem(wide_buffer_t *b, char *data, size_t size);

/* Copies the next element inside the buffer. */
char *serial_copy_elem(wide_buffer_t *b, char *data, size_t *size);

/* Returns the pointer to the data (avoid the metadata). */
char *serial_data(wide_buffer_t *b);

/* Returns the size of the whole buffer or just the data (if 1). */
size_t serial_size(wide_buffer_t *b);

#endif
