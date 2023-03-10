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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

//#define SHOW_DEBUGS 1

#include <stdlib.h>
#include <string.h>
#include <common/output/debug.h>
#include <common/utils/string.h>
#include <common/utils/serial_buffer.h>

#define psize_alloc(b) (&b->size)
#define psize_taken(b) ((size_t *) &b->data[0])
#define pelem_next(b)  ((size_t *) &b->data[sizeof(size_t)])
#define size_meta      (sizeof(size_t)*2)

//  0: size taken
//  8: elem next (pointer)
// 16: data (first elem size + first elem)

static void serial_debug(wide_buffer_t *b, char *from)
{
	#if SHOW_DEBUGS
	size_t *size_alloc;
	size_t *size_taken;
	size_t *elem_next;
	int i, j;


	// Taking pointers from first bytes
	size_alloc = psize_alloc(b);
	size_taken = psize_taken(b);
	elem_next  = pelem_next(b);

	debug("Wide buffer content: (%s)", from);
	debug("alloc: %lu", *size_alloc);
	debug("taken: %lu", *size_taken);
	debug("next : %lu",  *elem_next);

	// Printing first content
	for (i = 0, j = 16; i < 8; ++i, j += 8) {
		//debug("wide[%d]: %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx", j,
		debug("data[%d]: %03hhu %03hhu %03hhu %03hhu %03hhu %03hhu %03hhu %03hhu", j,
			b->data[j+7], b->data[j+6], b->data[j+5], b->data[j+4],
			b->data[j+3], b->data[j+2], b->data[j+1], b->data[j+0]);
	}

	unused(size_alloc);
	unused(elem_next);
	#endif
}

static size_t size_calc(size_t size_pow, size_t size)
{
	if (size_pow >= size) {
		return size_pow;
	}
	return size_calc(size_pow*2, size);
}

static void serial_init(wide_buffer_t *b, size_t size)
{
	size_t *size_alloc;
	size_t *size_taken;
	size_t *elem_next;

	// Size meta + size first data size + size data
	size    = size_calc(SIZE_1KB, size);
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
	b->size = 0;
	serial_init(b, size);
}

void serial_free(wide_buffer_t *b)
{
	free(b->data);
	b->data = NULL;
	b->size = 0;
}

void serial_resize(wide_buffer_t *b, size_t size)
{
	size_t *size_alloc;

	// Resize does not counts current taken space
	size_alloc = psize_alloc(b);
	// Resize is for allocated big chunks, not for small expansions.
	if (size > *size_alloc) {
		size = size_calc(SIZE_1KB, size);
		b->data = realloc(b->data, size);
		*size_alloc = size;
	}
	
	serial_debug(b, "serial_resize");
}

static void serial_expand(wide_buffer_t *b, size_t size_new_elem)
{
	size_t *size_alloc = psize_alloc(b);
	size_t *size_taken = psize_taken(b);

	if ((*size_taken + size_new_elem + sizeof(size_t)) > *size_alloc) {
		serial_resize(b, *size_taken + size_new_elem + sizeof(size_t));
	}
}

void serial_clean(wide_buffer_t *b)
{
	size_t *size_taken;
	size_t *elem_next;
	
	// Resetting content
	size_taken = psize_taken(b);
	elem_next  = pelem_next(b);
	// Pointing to the first byte in data
	*size_taken = size_meta;
	*elem_next  = size_meta;	
	
	serial_debug(b, "serial_clean");
}

void serial_reset(wide_buffer_t *b, size_t size)
{

	// Resize with new value in case of requirement
	serial_resize(b, size);
	// Cleans the metadata
	serial_clean(b);

	serial_debug(b, "serial_reset");
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
	size_t *size_taken;

	// Expand allocated space if needed
	serial_expand(b, size);
	// Taking pointers
	size_taken = psize_taken(b);
	//
	memcpy(&b->data[*size_taken], (char *) &size, sizeof(size_t));
	*size_taken += sizeof(size_t);
	debug("Adding a packet of %lu bytes", size);
	memcpy(&b->data[*size_taken], data, size);
	*size_taken += size;
	
	serial_debug(b, "serial_add_elem");
}

char *serial_copy_elem(wide_buffer_t *b, char *data, size_t *size)
{
	size_t *size_taken;
	size_t *size_elem;
	size_t *elem_next;
	char   *p;

	//
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
	if (size != NULL) {
		*size = *size_elem;
	}
	// Elem points directly to the data
	*elem_next += sizeof(size_t);
	// Copying
	p = memcpy(p, &b->data[*elem_next], *size_elem);
	debug("Copying a packet of %lu bytes", *size_elem);
	// Elem points to next element
	*elem_next += *size_elem;
	
	serial_debug(b, "serial_copy_elem");

	return p;
}

char *serial_data(wide_buffer_t *b)
{
	return b->data;
}

size_t serial_size(wide_buffer_t *b)
{
	size_t *size_taken = psize_taken(b);
	return *size_taken;
}
