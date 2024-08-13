/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


// gcc -Isrc -o types_tests type_tests.c
// gcc -I.. -DWF_SUPPORT=1 -o type_sizes type_sizes.c
// gcc -I.. -DWF_SUPPORT=0 -o type_sizes type_sizes.c
#include <stdio.h>
#include <stdlib.h>
#include <common/types/job.h>
#include <common/types/application.h>
#include <common/types/loop.h>



void main(int argc,char *argv[])
{
	printf("Loop size %u\n", sizeof(loop_t));
	printf("Application size %u\n", sizeof(application_t));
	printf("Job size %u\n", sizeof(job_t));
}
