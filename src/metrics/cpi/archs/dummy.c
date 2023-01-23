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

#include <common/output/debug.h>
#include <common/math_operations.h>
#include <metrics/common/perf.h>
#include <metrics/cpi/archs/dummy.h>

state_t cpi_dummy_load(topology_t *tp, cpi_ops_t *ops)
{
	replace_ops(ops->init,       cpi_dummy_init);
	replace_ops(ops->dispose,    cpi_dummy_dispose);
	replace_ops(ops->read,       cpi_dummy_read);
	replace_ops(ops->data_diff,  cpi_dummy_data_diff);
	replace_ops(ops->data_copy,  cpi_dummy_data_copy);
	replace_ops(ops->data_print, cpi_dummy_data_print);
	replace_ops(ops->data_tostr, cpi_dummy_data_tostr);
	return EAR_SUCCESS;
}

state_t cpi_dummy_init(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t cpi_dummy_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t cpi_dummy_read(ctx_t *c, cpi_t *ci)
{
	memset(ci, 0, sizeof(cpi_t));
	return EAR_SUCCESS;
}

// Helpers
state_t cpi_dummy_data_diff(cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpi)
{

	*cpi = 0.0;
	memset(ciD, 0, sizeof(cpi_t));
	//
	ciD->instructions = overflow_magic_u64(ci2->instructions, ci1->instructions, MAXBITS64);
	ciD->cycles = overflow_magic_u64(ci2->cycles, ci1->cycles, MAXBITS64);
	ciD->stalls = overflow_magic_u64(ci2->stalls, ci1->stalls, MAXBITS64);
	// Computing cycles per instructions
    ciD->instructions = ear_max(ciD->instructions, 1);
	if (ciD->cycles > 0) {
		*cpi = ((double) ciD->cycles) / ((double) ciD->instructions);
	}

	return EAR_SUCCESS;
}

state_t cpi_dummy_data_copy(cpi_t *dst, cpi_t *src)
{
	memcpy(dst, src, sizeof(cpi_t));
	return EAR_SUCCESS;
}

state_t cpi_dummy_data_print(cpi_t *ci, double cpi, int fd)
{
	char buffer[SZ_BUFFER];
	if (state_ok(cpi_dummy_data_tostr(ci, cpi, buffer, SZ_BUFFER))) {
		dprintf(fd, "%s", buffer);
	}
	return EAR_SUCCESS;
}

state_t cpi_dummy_data_tostr(cpi_t *ci, double cpi, char *buffer, size_t length)
{
	snprintf(buffer, length,
		"Instructions: %llu\nCycles: %llu\nStalls: %llu\nCPI: %0.2lf\n",
		ci->instructions, ci->cycles, ci->stalls, cpi);
	return EAR_SUCCESS;
}
