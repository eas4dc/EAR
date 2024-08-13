/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#include <common/states.h>
#include <common/types/types.h>
#include <common/output/verbose.h>

state_t report_init()
{
	verbose(0, "dummy report_init");
	return EAR_SUCCESS;
}

state_t report_applications(application_t *apps, uint count)
{
	verbose(0, "dummy report_applications");
	return EAR_SUCCESS;
}


state_t report_loops(loop_t *loops, uint count)
{
	verbose(0, "dummy report_loops");
	return EAR_SUCCESS;
}
