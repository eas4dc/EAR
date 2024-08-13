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
#include <common/states.h>
#include <library/api/ear.h>
#include <library/api/ear_mpi.h>
#include <library/policies/policy.h>
#include <common/output/debug.h>

void before_init()
{
	debug("before_init");
}

void after_init()
{
	debug("after_init");
	ear_init();
}

#if MPI_OPTIMIZED
p2i last_buf, last_dest;
#endif

void before_mpi(mpi_call call_type, p2i buf, p2i dest)
{
	debug("before_mpi");
#if MPI_OPTIMIZED
        last_buf  = buf;
        last_dest = dest;
#endif
#if EAR_OFF
  return;
#endif
	policy_mpi_init(call_type);
	ear_mpi_call(call_type,buf,dest);
}

void after_mpi(mpi_call call_type)
{
	debug("after_mpi");
#if EAR_OFF
  return;
#endif
	policy_mpi_end(call_type);
}

void before_finalize()
{
	debug("before_finalize");
	ear_finalize(EAR_SUCCESS);
}

void after_finalize()
{
	debug("after_finalize");
}
