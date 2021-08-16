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

#ifndef _IO_H
#define _IO_H
#include <common/states.h>
#include <common/types.h>
#include <common/plugins.h>

#define IO_PATH "/proc/%d/io"

typedef struct io_data{
	ullong rchar;
	ullong wchar;
	ullong syscr;
	ullong syscw;
	ullong read_bytes;
	ullong write_bytes;
	ullong cancelled;
}io_data_t;

/** Initializes io info for a given pid */
state_t io_init(ctx_t *c,pid_t pid);
/** Reads io data associated with the pid used in io_init and stores it in iodata. iodata must be a valib pointer */
state_t io_read(ctx_t *c, io_data_t *iodata);
/** Allocates memory for a io_data_t type */
state_t io_alloc(io_data_t **iodata);
/** Releases a previously allocated type with io_alloc */
state_t io_free(io_data_t *iodata);
/** Prints iodata in stdout */
state_t io_print(io_data_t *iodata);
/** Generates a string in msg with the content of iodata */
state_t io_tostr(io_data_t *iodata,char *msg,size_t len);
/** Computes the difference between two iodata readings */
state_t io_diff(io_data_t *diff,io_data_t *iodata_init,io_data_t *iodata_end);
/** Copies from src to dst */
state_t io_copy(io_data_t *dst,io_data_t *src);
/** Releases the context previously opened with io_init */
state_t io_dispose(ctx_t *c);


#endif
