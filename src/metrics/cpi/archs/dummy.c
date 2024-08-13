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
    static double cpi_copy = 0.0;

	memset(ciD, 0, sizeof(cpi_t));
	ciD->instructions = overflow_zeros_u64(ci2->instructions, ci1->instructions);
	ciD->cycles       = overflow_zeros_u64(ci2->cycles, ci1->cycles);
	ciD->stalls       = overflow_zeros_u64(ci2->stalls, ci1->stalls);
    // If instructions is 0, we convert them to 1
    if (ciD->instructions == 0 || ciD->cycles == 0) {
        ciD->instructions = (ciD->instructions > 0) ? ciD->instructions : 1;
        ciD->cycles       = (ciD->cycles       > 0) ? ciD->cycles       : 1;
        // And if CPI is not NULL, we return the last valid CPI
        if (cpi != NULL) {
            *cpi = cpi_copy;
        }
    } else if (cpi != NULL) {
		*cpi = ((double) ciD->cycles) / ((double) ciD->instructions);
        // Saving valid CPI
        cpi_copy = *cpi;
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
	snprintf(buffer, length, "Instructions: %llu\nCycles: %llu (cpi: %0.2lf)\n", ci->instructions, ci->cycles, cpi);
	return EAR_SUCCESS;
}
