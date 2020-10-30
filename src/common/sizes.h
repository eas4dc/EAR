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

#ifndef EAR_SIZES_H
#define EAR_SIZES_H

#include <linux/limits.h>

#define SZ_NAME_SHORT		128
#define SZ_NAME_MEDIUM		NAME_MAX + 1
#define SZ_NAME_LARGE		512
#define SZ_BUFF_BIG			PATH_MAX
#define SZ_BUFF_EXTRA		PATH_MAX * 2
#define SZ_PATH_KERNEL		64
#define SZ_PATH				PATH_MAX
#define SZ_PATH_MEDIUM		2048
#define SZ_PATH_SHORT		1024
#define SZ_PATH_INCOMPLETE	SZ_PATH - (SZ_PATH_SHORT * 2)

#endif //EAR_SIZES_H
