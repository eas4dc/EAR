/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <common/environment_common.h>

int openmpi_num_nodes()
{
	char *cn = ear_getenv("OMPI_MCA_orte_num_nodes");
	if (cn != NULL)
		return atoi(cn);
	else
		return -1;
}
