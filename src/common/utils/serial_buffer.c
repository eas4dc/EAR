/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <stdlib.h>
#include <string.h>
#include <common/output/debug.h>
#include <common/utils/string.h>
#include <common/utils/serial_buffer.h>

#define psize_alloc(b)     (&b->size) // P* to the total size allocated
#define psize_taken(b)     ((size_t *) &b->data[0]) // P* to the size taken from size alloc
#define pelem_unvisited(b) ((size_t *) &b->data[sizeof(size_t)]) // P* to the first unvisited element
#define ppole_position     (sizeof(size_t)*2) // The first two sizes, taken+next

//  0: size taken
//  8: first unvisited element (pointer)
// 16: data (first elem size + first elem)

static void serial_debug(serial_buffer_t *b, char *from)
{
	#if SHOW_DEBUGS
	size_t *size_alloc;
	size_t *size_taken;
	size_t *elem_next;
	int i, j;

	// Taking pointers from first bytes
	size_alloc = psize_alloc(b);
	size_taken = psize_taken(b);
	elem_next  = pelem_unvisited(b);

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

static void serial_init(serial_buffer_t *b, size_t size)
{
	size_t *size_alloc;
	size_t *size_taken;
	size_t *elem_next;

	// Size meta + size first data size + size data
	size        = size_calc(SIZE_1KB, size);
	b->data     = calloc(1, size);
	// Taking pointers from first bytes
	size_alloc  = psize_alloc(b);
	size_taken  = psize_taken(b);
	elem_next   = pelem_unvisited(b);
	// Default values
	*size_alloc = size;
	*size_taken = ppole_position;
	*elem_next  = ppole_position;

	serial_debug(b, "serial_init");
}

void serial_alloc(serial_buffer_t *b, size_t size)
{
	b->data = NULL;
	b->size = 0;
	serial_init(b, size);
}

void serial_free(serial_buffer_t *b)
{
	free(b->data);
	b->data = NULL;
	b->size = 0;
}

void serial_resize(serial_buffer_t *b, size_t size)
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

static void serial_expand(serial_buffer_t *b, size_t size_new_elem)
{
	size_t *size_alloc = psize_alloc(b);
	size_t *size_taken = psize_taken(b);

	if ((*size_taken + size_new_elem + sizeof(size_t)) > *size_alloc) {
		serial_resize(b, *size_taken + size_new_elem + sizeof(size_t));
	}
}

void serial_clean(serial_buffer_t *b)
{
	size_t *size_taken;
	size_t *elem_next;
	
	// Resetting content
	size_taken = psize_taken(b);
	elem_next  = pelem_unvisited(b);
	// Pointing to the first byte in data
	*size_taken = ppole_position;
	*elem_next  = ppole_position;
	
	serial_debug(b, "serial_clean");
}

void serial_reset(serial_buffer_t *b, size_t size)
{
	// Resize with new value in case of requirement
	serial_resize(b, size);
	// Cleans the metadata
	serial_clean(b);

	serial_debug(b, "serial_reset");
}

void serial_rewind(serial_buffer_t *b)
{
	size_t *elem;
	
	 elem = pelem_unvisited(b);
	*elem = ppole_position;
	
	serial_debug(b, "serial_rewind");
}

void serial_add_elem(serial_buffer_t *b, char *data, size_t size)
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

// _point gets a pointer to, _copy copies the bytes
static char *serial_point_next_elem(serial_buffer_t *b, size_t *size)
{
    size_t *size_taken;
    size_t *size_elem;
    size_t *elem_next;
    char   *p;

    size_taken = psize_taken(b);
    elem_next  = pelem_unvisited(b);
    // It is the end, try to rewind
    if (*elem_next == *size_taken) {
        return NULL;
    }
    // Copying the content data
    size_elem = (size_t *) &b->data[*elem_next];

    if (size != NULL) {
        *size = *size_elem;
    }
    // Elem points directly to the data
    *elem_next += sizeof(size_t);
    p = &b->data[*elem_next];
    *elem_next += *size_elem;
    return p;
}

char *serial_copy_elem(serial_buffer_t *b, char *data, size_t *size)
{
	size_t *size_taken;
	size_t *size_elem;
	size_t *elem_next;
	char   *p;

	//
	size_taken = psize_taken(b);
	elem_next  = pelem_unvisited(b);
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

char *serial_data(serial_buffer_t *b)
{
	return b->data;
}

size_t serial_size(serial_buffer_t *b)
{
	size_t *size_taken = psize_taken(b);
	return *size_taken;
}

void serial_dictionary_push(serial_buffer_t *b, char *tag, char *param, size_t param_size)
{
    char ext[4];
    memset(ext, 0,  4);
    serial_add_elem(b, tag, strlen(tag)+1);
    serial_add_elem(b, ext, 4); // Extra space for future complementary data
    serial_add_elem(b, param, param_size);
}

int serial_dictionary_pop(serial_buffer_t *b, char *tag, char *param, size_t expected_size)
{
    char *param_next;
    char *tag_next;
    size_t size = 0;

    // String manipulation
    serial_rewind(b);
    // Tag iterations
    while ((tag_next = serial_point_next_elem(b, NULL)) != NULL) {
        if (strcmp(tag_next, tag) == 0) {
            //printf("%s\n", tag_next);
            // Removing the TAG to avoid revisiting the same parameter. We are
            // doing this because it's possible to have a tag N times, and we
            // can't use next_elem variable to prevent revisiting it because a
            // small change in unpacking call order would escape other previous
            // parameters of being found.
            sprintf(tag_next, "%s", "?");

            serial_point_next_elem(b, NULL); //Extra space
            param_next = serial_point_next_elem(b, &size);
            memset(param, 0, expected_size);

            if (expected_size >= size) {
                memcpy(param, param_next, size);
            } else {
                memcpy(param, param_next, expected_size);
            }
            return 1;
        }
        serial_point_next_elem(b, NULL);
        serial_point_next_elem(b, NULL);
    }
    return 0;
}

void serial_dictionary_print(serial_buffer_t *b)
{
    char param[1024];
    char tag[1024];
    size_t size;

    while (serial_copy_elem(b, tag, &size) != NULL) {
        serial_copy_elem(b, param, &size); // Extra space
        serial_copy_elem(b, param, &size);
        printf("%s: %lu\n", tag, size);
    }
}
