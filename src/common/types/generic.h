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

#ifndef _EAR_TYPES_GENERIC
#define _EAR_TYPES_GENERIC

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

typedef unsigned char		uchar;
typedef unsigned long long	ull;
typedef unsigned long long	ullong;
typedef   signed long long	llong;
typedef unsigned long		ulong;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef long double			ldouble;

// Not generic
typedef uint8_t			job_type;
typedef ulong			job_id;

#define GENERIC_NAME 		256
#define SHORT_GENERIC_NAME 		16
#define	UID_NAME			8
#define POLICY_NAME 		32
#define ENERGY_TAG_SIZE		32
#define MAX_PATH_SIZE		256
#define NODE_SIZE			256
#define NAME_SIZE			128
#define ID_SIZE				64
#define USER			64

#define ear_min(X, Y) (((X) < (Y)) ? (X) : (Y))
#define ear_max(X, Y) (((X) > (Y)) ? (X) : (Y))

#endif
