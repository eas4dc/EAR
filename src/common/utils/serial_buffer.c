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

//#define SHOW_DEBUGS 1

#include <stdlib.h>
#include <string.h>
#include <common/output/debug.h>
#include <common/utils/string.h>
#include <common/utils/serial_buffer.h>

#define psize_alloc(b) ((size_t *) &b->data[0]);
#define psize_taken(b) ((size_t *) &b->data[sizeof(size_t)]);
#define pelem_next(b)  ((size_t *) &b->data[sizeof(size_t)*2]);
#define size_meta      sizeof(size_t)*3

//  0: size alloc
//  8: size taken
// 16: elem next (pointer)
// 24: data (first elem size + first elem)

static void serial_debug(wide_buffer_t *b, char *from)
{
	size_t *size_alloc;
	size_t *size_taken;
	size_t *elem_next;
	size_t size_data;
	int i, j;

	#if !SHOW_DEBUGS
	return;
	#endif

	// Taking pointers from first bytes
	size_alloc = psize_alloc(b);
	size_taken = psize_taken(b);
	elem_next  = pelem_next(b);
	size_data  = *size_taken - size_meta;
	
	debug("Wide buffer content: (%s)", from);
	debug("wide[00]: %llu (allocated)", *size_alloc);
	debug("wide[08]: %llu (taken)",     *size_taken);
	debug("wide[16]: %llu (elem)",      *elem_next);
	debug("wide[xx]: %llu (data)",       size_data);

	// Printing first content
	for (i = 0, j = 24; i < 8; ++i, j += 8) {
		//debug("wide[%d]: %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx", j,
		debug("wide[%d]: %03hhu %03hhu %03hhu %03hhu %03hhu %03hhu %03hhu %03hhu", j,
			b->data[j+7], b->data[j+6], b->data[j+5], b->data[j+4],
			b->data[j+3], b->data[j+2], b->data[j+1], b->data[j+0]);
	}

	unused(size_alloc);
	unused(size_data);
	unused(elem_next);
}

static void serial_init(wide_buffer_t *b, size_t size)
{
	size_t *size_alloc;
	size_t *size_taken;
	size_t *elem_next;

	if (b->data != NULL) {
		return;
	}
	// Size meta + size first data size + size data
	if ((size = size_meta + sizeof(size_t) + size) < 8192) {
		size = 8192;
	}
	b->data = calloc(1, size);
	// Taking pointers from first bytes
	size_alloc  = psize_alloc(b);
	size_taken  = psize_taken(b);
	elem_next   = pelem_next(b);
	// Default values
	*size_alloc = size;
	*size_taken = size_meta;
	*elem_next  = size_meta;

	serial_debug(b, "serial_init");
}

void serial_alloc(wide_buffer_t *b, size_t size)
{
	b->data = NULL;
	serial_init(b, size);
}

void serial_free(wide_buffer_t *b)
{
	free(b->data);
	b->data = NULL;
}

void serial_resize(wide_buffer_t *b, size_t size)
{
	size_t *size_alloc;

	// Init if required
	serial_init(b, size);
	// Resize does not counts current taken space
	size_alloc = psize_alloc(b);
	// Size meta + size data size + new data size taken
	if ((size = (size_meta + sizeof(size_t) + size)) > *size_alloc) {
		b->data = realloc(b->data, size);
		// Recovering size_alloc
		size_alloc = psize_alloc(b);
		// New size
		*size_alloc = size;
	}
	
	serial_debug(b, "serial_resize");
}

void serial_clean(wide_buffer_t *b)
{
	size_t *size_taken;
	size_t *elem_next;
	
	// Init if NULL
	serial_init(b, 0);
	// Resetting content
	size_taken = psize_taken(b);
	elem_next  = pelem_next(b);
	// Pointing to the first byte in data
	*size_taken = size_meta;
	*elem_next  = size_meta;	
	
	serial_debug(b, "serial_clean");
}

void serial_assign(wide_buffer_t *b, size_t size)
{
	size_t *size_taken = psize_taken(b);

	// Cleans the metadata
	serial_clean(b);
	// Resize with new value in case of requirement
	serial_resize(b, size);
	// The size taken is size of metadata + the incoming data size
	*size_taken = size_meta + size; 
	
	serial_debug(b, "serial_assign");
}

void serial_rewind(wide_buffer_t *b)
{
	size_t *elem;
	
	 elem = pelem_next(b);
	*elem = size_meta;	
	
	serial_debug(b, "serial_rewind");
}

void serial_add_elem(wide_buffer_t *b, char *data, size_t size)
{
	size_t *size_alloc;
	size_t *size_taken;

	// Error handling
	serial_resize(b, size);
	// Taking pointers
	size_alloc = psize_alloc(b);
	size_taken = psize_taken(b);
	//
	memcpy(&b->data[*size_taken], (char *) &size, sizeof(size_t));
	*size_taken += sizeof(size_t);
	debug("Adding a packet of %lu bytes", size);
	memcpy(&b->data[*size_taken], data, size);
	*size_taken += size;
	
	serial_debug(b, "serial_add_elem");
	// Removing unused
	unused(size_alloc);
}

char *serial_copy_elem(wide_buffer_t *b, char *data, size_t *size)
{
	size_t *size_alloc;
	size_t *size_taken;
	size_t *size_elem;
	size_t *elem_next;
	char   *p;

	//
	size_alloc = psize_alloc(b);
	size_taken = psize_taken(b);
	elem_next  = pelem_next(b);
	// It is the end, try to rewind
	if (*elem_next == *size_taken) {
		return NULL;
	}
	// Copying the content data
	size_elem = (size_t *) &b->data[*elem_next];
	if (data == NULL) {
		p = calloc(1, *size_elem);
	} else {
		p = data;
	}
	// Elem points directly to the data
	*elem_next += sizeof(size_t);
	// Copying
	p = memcpy(p, &b->data[*elem_next], *size_elem);
	debug("Copying a packet of %lu bytes", *size_elem);
	// Elem points to next element
	*elem_next += *size_elem;
	
	serial_debug(b, "serial_copy_elem");
	// Removing unused
	unused(size_alloc);

	return p;
}

char *serial_data(wide_buffer_t *b)
{
	return &b->data[size_meta];
}

size_t serial_size(wide_buffer_t *b, uint data)
{
	size_t *size_taken = psize_taken(b);
	if (data) {
		return *size_taken - size_meta;
	}
	return *size_taken;
}
