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

#ifndef EAR_EAR_SIG_EXT_H
#define EAR_EAR_SIG_EXT_H

#include <metrics/io/io.h>
#include <library/common/library_shared_data.h>
typedef struct sig_ext{
	io_data_t iod;
	mpi_information_t *mpi_stats;		
	mpi_calls_types_t *mpi_types;
}sig_ext_t;

#endif 
