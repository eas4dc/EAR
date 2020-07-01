/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING
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
