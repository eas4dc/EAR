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
#include <common/config.h>
#include <common/states.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>

state_t ear_cuda_init()
{
	verbose_master(2,"CUDA detected");
	return 0;
}
