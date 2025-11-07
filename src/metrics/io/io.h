/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _IO_H
#define _IO_H
#include <common/plugins.h>
#include <common/states.h>
#include <common/types.h>

#define IO_PATH "/proc/%d/io"

typedef struct io_data {
    ullong rchar;
    ullong wchar;
    ullong syscr;
    ullong syscw;
    ullong read_bytes;
    ullong write_bytes;
    ullong cancelled;
} io_data_t;

/** Initializes io info for a given pid */
state_t io_init(ctx_t *c, pid_t pid);
/** Reads io data associated with the pid used in io_init and stores it in iodata. iodata must be a valib pointer */
state_t io_read(ctx_t *c, io_data_t *iodata);
/** Allocates memory for a io_data_t type */
state_t io_alloc(io_data_t **iodata);
/** Releases a previously allocated type with io_alloc */
state_t io_free(io_data_t *iodata);
/** Prints iodata in stdout */
state_t io_print(io_data_t *iodata);
/** Generates a string in msg with the content of iodata */
state_t io_tostr(io_data_t *iodata, char *msg, size_t len);
/** Computes the difference between two iodata readings */
state_t io_diff(io_data_t *diff, io_data_t *iodata_init, io_data_t *iodata_end);
/** Copies from src to dst */
state_t io_copy(io_data_t *dst, io_data_t *src);
/** Releases the context previously opened with io_init */
state_t io_dispose(ctx_t *c);
/** Accumulate data from \p src to \p dst. The
 * context \p c is for design purposes, but not used. */
state_t io_accum(ctx_t *c, io_data_t *dst, io_data_t *src);
#endif
