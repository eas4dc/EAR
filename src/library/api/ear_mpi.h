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

#ifndef LIBRARY_EAR_MPI_H
#define LIBRARY_EAR_MPI_H

#include <library/api/mpi.h>

void before_init();

void after_init();

void before_mpi(mpi_call call_type, p2i buf, p2i dest);

void after_mpi(mpi_call call_type);

void before_finalize();

void after_finalize();

#endif //LIBRARY_EAR_MPI_H
